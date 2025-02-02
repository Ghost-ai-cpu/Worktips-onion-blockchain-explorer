#define CROW_ENABLE_SSL


#include "src/page.h"

#include "ext/crow/crow.h"
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/CurrentBlockchainStatus.h"

#include <fstream>
#include <regex>

using boost::filesystem::path;
using wtipseg::remove_bad_chars;

using namespace std;

namespace mywtips
{
struct jsonresponse: crow::response
{
    jsonresponse(const nlohmann::json& _body)
            : crow::response {_body.dump()}
    {
        add_header("Access-Control-Allow-Origin", "*");
        add_header("Access-Control-Allow-Headers", "Content-Type");
        add_header("Content-Type", "application/json");
    }
};
}

int
main(int ac, const char* av[])
{
    // get command line options
    wtipseg::CmdLineOptions opts {ac, av};

    auto help_opt                      = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    auto port_opt                      = opts.get_option<string>("port");
    auto bindaddr_opt                  = opts.get_option<string>("bindaddr");
    auto bc_path_opt                   = opts.get_option<string>("bc-path");
    auto daemon_url_opt                = opts.get_option<string>("daemon-url");
    auto ssl_crt_file_opt              = opts.get_option<string>("ssl-crt-file");
    auto ssl_key_file_opt              = opts.get_option<string>("ssl-key-file");
    auto no_blocks_on_index_opt        = opts.get_option<string>("no-blocks-on-index");
    auto testnet_url                   = opts.get_option<string>("testnet-url");
    auto stagenet_url                  = opts.get_option<string>("stagenet-url");
    auto mainnet_url                   = opts.get_option<string>("mainnet-url");
    auto mempool_info_timeout_opt      = opts.get_option<string>("mempool-info-timeout");
    auto mempool_refresh_time_opt      = opts.get_option<string>("mempool-refresh-time");
    auto log_level                     = opts.get_option<string>("log-level");
    auto testnet_opt                   = opts.get_option<bool>("testnet");
    auto stagenet_opt                  = opts.get_option<bool>("stagenet");
    auto enable_key_image_checker_opt  = opts.get_option<bool>("enable-key-image-checker");
    auto enable_output_key_checker_opt = opts.get_option<bool>("enable-output-key-checker");
    auto enable_autorefresh_option_opt = opts.get_option<bool>("enable-autorefresh-option");
    auto enable_pusher_opt             = opts.get_option<bool>("enable-pusher");
    auto enable_js_opt                 = opts.get_option<bool>("enable-js");
    auto enable_mixin_details_opt      = opts.get_option<bool>("enable-mixin-details");
    auto enable_json_api_opt           = opts.get_option<bool>("enable-json-api");
    auto enable_as_hex_opt             = opts.get_option<bool>("enable-as-hex");
    auto enable_tx_cache_opt           = opts.get_option<bool>("enable-tx-cache");
    auto enable_block_cache_opt        = opts.get_option<bool>("enable-block-cache");
    auto show_cache_times_opt          = opts.get_option<bool>("show-cache-times");
    auto enable_emission_monitor_opt   = opts.get_option<bool>("enable-emission-monitor");



    bool testnet                      {*testnet_opt};
    bool stagenet                     {*stagenet_opt};

    if (testnet && stagenet)
    {
        cerr << "testnet and stagenet cannot be specified at the same time!" << endl;
        return EXIT_FAILURE;
    }

    const cryptonote::network_type nettype = testnet ?
        cryptonote::network_type::TESTNET : stagenet ?
        cryptonote::network_type::STAGENET : cryptonote::network_type::MAINNET;

    bool enable_pusher                {*enable_pusher_opt};
    bool enable_js                    {*enable_js_opt};
    bool enable_key_image_checker     {*enable_key_image_checker_opt};
    bool enable_autorefresh_option    {*enable_autorefresh_option_opt};
    bool enable_output_key_checker    {*enable_output_key_checker_opt};
    bool enable_mixin_details         {*enable_mixin_details_opt};
    bool enable_json_api              {*enable_json_api_opt};
    bool enable_as_hex                {*enable_as_hex_opt};
    bool enable_tx_cache              {*enable_tx_cache_opt};
    bool enable_block_cache           {*enable_block_cache_opt};
    bool enable_emission_monitor      {*enable_emission_monitor_opt};
    bool show_cache_times             {*show_cache_times_opt};


    // set  worktips log output level
    mlog_configure("", true);
    if (log_level)
        mlog_set_log(log_level->c_str());

    //cast port number in string to uint
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

    string bindaddr = *bindaddr_opt;

    // cast no_blocks_on_index_opt to uint
    uint64_t no_blocks_on_index = boost::lexical_cast<uint64_t>(*no_blocks_on_index_opt);

    bool use_ssl {false};

    string ssl_crt_file;
    string ssl_key_file;

    // check if ssl enabled and files exist

    if (ssl_crt_file_opt and ssl_key_file_opt)
    {
        if (!boost::filesystem::exists(boost::filesystem::path(*ssl_crt_file_opt)))
        {
            cerr << "ssl_crt_file path: " << *ssl_crt_file_opt
                 << "does not exist!" << endl;

            return EXIT_FAILURE;
        }

        if (!boost::filesystem::exists(boost::filesystem::path(*ssl_key_file_opt)))
        {
            cerr << "ssl_key_file path: " << *ssl_key_file_opt
                 << "does not exist!" << endl;

            return EXIT_FAILURE;
        }

        ssl_crt_file = *ssl_crt_file_opt;
        ssl_key_file = *ssl_key_file_opt;

        use_ssl = true;
    }



    // get blockchain path
    path blockchain_path;

    if (!wtipseg::get_blockchain_path(bc_path_opt, blockchain_path, nettype)) {
      cerr << "Error getting blockchain path." << endl;
      return EXIT_FAILURE;
    }

    cout << blockchain_path << endl;


    // create instance of our MicroCore
    // and make pointer to the Blockchain
    wtipseg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    // initialize mcore and core_storage
    if (!wtipseg::init_blockchain(blockchain_path.string(),
                               mcore, core_storage, nettype))
    {
        cerr << "Error accessing blockchain." << endl;
        return EXIT_FAILURE;
    }

    string daemon_url {*daemon_url_opt};

    if (testnet && daemon_url == "http:://127.0.0.1:31022")
        daemon_url = "http:://127.0.0.1:33022";
    if (stagenet && daemon_url == "http:://127.0.0.1:31022")
        daemon_url = "http:://127.0.0.1:32022";

    uint64_t mempool_info_timeout {5000};

    try
    {
        mempool_info_timeout = boost::lexical_cast<uint64_t>(*mempool_info_timeout_opt);
    }
    catch (boost::bad_lexical_cast &e)
    {
        cout << "Cant cast " << (*mempool_info_timeout_opt) <<" into numbers. Using default values."
             << endl;
    }

    uint64_t mempool_refresh_time {10};


    if (enable_emission_monitor == true)
    {
        // This starts new thread, which aim is
        // to calculate, store and monitor
        // current total Worktips emission amount.

        // This thread stores the current emission
        // which it has caluclated in
        // <blockchain_path>/emission_amount.txt file,
        // e.g., ~/.worktips/lmdb/emission_amount.txt.
        // So instead of calcualting the emission
        // from scrach whenever the explorer is started,
        // the thread is initalized with the values
        // found in emission_amount.txt file.

        wtipseg::CurrentBlockchainStatus::blockchain_path
                = blockchain_path;
        wtipseg::CurrentBlockchainStatus::nettype
                = nettype;
        wtipseg::CurrentBlockchainStatus::daemon_url
                = daemon_url;
        wtipseg::CurrentBlockchainStatus::set_blockchain_variables(
                &mcore, core_storage);

        // launch the status monitoring thread so that it keeps track of blockchain
        // info, e.g., current height. Information from this thread is used
        // by tx searching threads that are launched for each user independently,
        // when they log back or create new account.
        wtipseg::CurrentBlockchainStatus::start_monitor_blockchain_thread();
    }


    wtipseg::MempoolStatus::blockchain_path
            = blockchain_path;
    wtipseg::MempoolStatus::nettype
            = nettype;
    wtipseg::MempoolStatus::daemon_url
            = daemon_url;
    wtipseg::MempoolStatus::set_blockchain_variables(
            &mcore, core_storage);

    wtipseg::MempoolStatus::network_info initial_info;
    strcpy(initial_info.block_size_limit_str, "0.0");
    strcpy(initial_info.block_size_median_str, "0.0");
    strcpy(initial_info.total_blockchain_size_str, "0.0");
    wtipseg::MempoolStatus::current_network_info = initial_info;

    try
    {
        mempool_refresh_time = boost::lexical_cast<uint64_t>(*mempool_refresh_time_opt);

    }
    catch (boost::bad_lexical_cast &e)
    {
        cout << "Cant cast " << (*mempool_refresh_time_opt)
             <<" into number. Using default value."
             << endl;
    }

    // launch the status monitoring thread so that it keeps track of blockchain
    // info, e.g., current height. Information from this thread is used
    // by tx searching threads that are launched for each user independently,
    // when they log back or create new account.
    wtipseg::MempoolStatus::mempool_refresh_time = mempool_refresh_time;
    wtipseg::MempoolStatus::start_mempool_status_thread();
 
    // create instance of page class which
    // contains logic for the website
    wtipseg::page wtipsblocks(&mcore,
                          core_storage,
                          daemon_url,
                          nettype,
                          enable_pusher,
                          enable_js,
                          enable_as_hex,
                          enable_key_image_checker,
                          enable_output_key_checker,
                          enable_autorefresh_option,
                          enable_mixin_details,
                          enable_tx_cache,
                          enable_block_cache,
                          show_cache_times,
                          no_blocks_on_index,
                          mempool_info_timeout,
                          *testnet_url,
                          *stagenet_url,
                          *mainnet_url);

    // crow instance
    crow::SimpleApp app;

    // get domian url based on the request
    auto get_domain = [&use_ssl](crow::request const& req) {
        return (use_ssl ? "https://" : "http://")
               + req.get_header_value("Host");
    };

    CROW_ROUTE(app, "/")
    ([&](const crow::request& req) {
        return crow::response(wtipsblocks.index2());
    });

    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return wtipsblocks.index2(page_no);
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](const crow::request& req, size_t block_height) {
        return crow::response(wtipsblocks.show_block(block_height));
    });

    CROW_ROUTE(app, "/block/<string>")
    ([&](const crow::request& req, string block_hash) {
        return crow::response(wtipsblocks.show_block(remove_bad_chars(block_hash)));
    });

    CROW_ROUTE(app, "/tx/<string>")
    ([&](const crow::request& req, string tx_hash) {
        return crow::response(wtipsblocks.show_tx(remove_bad_chars(tx_hash)));
    });

    if (enable_as_hex)
    {
        CROW_ROUTE(app, "/txhex/<string>")
        ([&](string tx_hash) {
            return crow::response(wtipsblocks.show_tx_hex(remove_bad_chars(tx_hash)));
        });

        CROW_ROUTE(app, "/ringmembershex/<string>")
        ([&](string tx_hash) {
            return crow::response(wtipsblocks.show_ringmembers_hex(remove_bad_chars(tx_hash)));
        });

        CROW_ROUTE(app, "/blockhex/<uint>")
        ([&](size_t block_height) {
            return crow::response(wtipsblocks.show_block_hex(block_height, false));
        });

        CROW_ROUTE(app, "/blockhexcomplete/<uint>")
        ([&](size_t block_height) {
            return crow::response(wtipsblocks.show_block_hex(block_height, true));
        });

//        CROW_ROUTE(app, "/ringmemberstxhex/<string>")
//        ([&](string tx_hash) {
//            return crow::response(xmrblocks.show_ringmemberstx_hex(remove_bad_chars(tx_hash)));
//        });

        CROW_ROUTE(app, "/ringmemberstxhex/<string>")
        ([&](string tx_hash) {
            return mywtips::jsonresponse {wtipsblocks.show_ringmemberstx_jsonhex(remove_bad_chars(tx_hash))};
        });

    }

    CROW_ROUTE(app, "/tx/<string>/<uint>")
    ([&](string tx_hash, uint16_t with_ring_signatures)
     {
        return wtipsblocks.show_tx(remove_bad_chars(tx_hash), with_ring_signatures);
    });

    CROW_ROUTE(app, "/service_node/<string>")
    ([&](const crow::request& req, std::string service_node_pubkey) {
        return crow::response(wtipsblocks.show_service_node(remove_bad_chars(service_node_pubkey)));
    });

    CROW_ROUTE(app, "/service_nodes")
    ([&](const crow::request& req) {
        return wtipsblocks.render_service_nodes_html(true /*add_header_and_footer*/);
    });

    CROW_ROUTE(app, "/quorums")
    ([&](const crow::request& req) {
        return wtipsblocks.render_quorum_states_html(true /*add_header_and_footer*/);
    });

    CROW_ROUTE(app, "/checkpoint_quorum/<uint>")
    ([&](const crow::request& req, uint64_t height) {
        return wtipsblocks.render_single_quorum_html(service_nodes::quorum_type::checkpointing, height);
    });

    CROW_ROUTE(app, "/obligations_quorum/<uint>")
    ([&](const crow::request& req, uint64_t height) {
        return wtipsblocks.render_single_quorum_html(service_nodes::quorum_type::obligations, height);
    });

    CROW_ROUTE(app, "/checkpoints")
    ([&](const crow::request& req) {
        return wtipsblocks.render_checkpoints_html(true /*add_header_and_footer*/);
    });

    // TODO(worktips): This should be combined into the normal search mechanism, we shouldn't have 2 search bars.
    CROW_ROUTE(app, "/search_service_node").methods("GET"_method)
    ([&](const crow::request& req) {
        return wtipsblocks.show_service_node(remove_bad_chars(string(req.url_params.get("value"))));
    });


    CROW_ROUTE(app, "/myoutputs").methods("POST"_method)
    ([&](const crow::request& req)
     {

        map<std::string, std::string> post_body
                = wtipseg::parse_crow_post_data(req.body);

        if (post_body.count("wtips_address") == 0
            || post_body.count("viewkey") == 0
            || post_body.count("tx_hash") == 0)
        {
            return string("wtips address, viewkey or tx hash not provided");
        }

        string tx_hash     = remove_bad_chars(post_body["tx_hash"]);
        string wtips_address = remove_bad_chars(post_body["wtips_address"]);
        string viewkey     = remove_bad_chars(post_body["viewkey"]);

        // this will be only not empty when checking raw tx data
        // using tx pusher
        string raw_tx_data = remove_bad_chars(post_body["raw_tx_data"]);

        string domain      =  get_domain(req);

        return wtipsblocks.show_my_outputs(tx_hash, wtips_address,
                                         viewkey, raw_tx_data,
                                         domain);
    });

    CROW_ROUTE(app, "/myoutputs/<string>/<string>/<string>")
    ([&](const crow::request& req, string tx_hash,
        string wtips_address, string viewkey)
     {

        string domain = get_domain(req);

        return wtipsblocks.show_my_outputs(remove_bad_chars(tx_hash),
                                         remove_bad_chars(wtips_address),
                                         remove_bad_chars(viewkey),
                                         string {},
                                         domain);
    });

    CROW_ROUTE(app, "/prove").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = wtipseg::parse_crow_post_data(req.body);

            if (post_body.count("wtipsaddress") == 0
                || post_body.count("txprvkey") == 0
                || post_body.count("txhash") == 0)
            {
                return string("wtips address, tx private key or "
                                      "tx hash not provided");
            }

            string tx_hash     = remove_bad_chars(post_body["txhash"]);
            string tx_prv_key  = remove_bad_chars(post_body["txprvkey"]);
            string wtips_address = remove_bad_chars(post_body["wtipsaddress"]);

            // this will be only not empty when checking raw tx data
            // using tx pusher
            string raw_tx_data = remove_bad_chars(post_body["raw_tx_data"]);

            string domain      = get_domain(req);

            return wtipsblocks.show_prove(tx_hash,
                                        wtips_address,
                                        tx_prv_key,
                                        raw_tx_data,
                                        domain);
    });


    CROW_ROUTE(app, "/prove/<string>/<string>/<string>")
    ([&](const crow::request& req, string tx_hash,
         string wtips_address, string tx_prv_key) {

        string domain = get_domain(req);

        return wtipsblocks.show_prove(remove_bad_chars(tx_hash),
                                    remove_bad_chars(wtips_address),
                                    remove_bad_chars(tx_prv_key),
                                    string {},
                                    domain);
    });

    if (enable_pusher)
    {
        CROW_ROUTE(app, "/rawtx")
        ([&](const crow::request& req) {
            return wtipsblocks.show_rawtx();
        });

        CROW_ROUTE(app, "/checkandpush").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = wtipseg::parse_crow_post_data(req.body);

            if (post_body.count("rawtxdata") == 0 || post_body.count("action") == 0)
            {
                return string("Raw tx data or action not provided");
            }

            string raw_tx_data = remove_bad_chars(post_body["rawtxdata"]);
            string action      = remove_bad_chars(post_body["action"]);

            if (action == "check")
                return wtipsblocks.show_checkrawtx(raw_tx_data, action);
            else if (action == "push")
                return wtipsblocks.show_pushrawtx(raw_tx_data, action);
            return string("Provided action is neither check nor push");

        });
    }

    if (enable_key_image_checker)
    {
        CROW_ROUTE(app, "/rawkeyimgs")
        ([&](const crow::request& req) {
            return wtipsblocks.show_rawkeyimgs();
        });

        CROW_ROUTE(app, "/checkrawkeyimgs").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = wtipseg::parse_crow_post_data(req.body);

            if (post_body.count("rawkeyimgsdata") == 0)
            {
                return string("Raw key images data not given");
            }

            if (post_body.count("viewkey") == 0)
            {
                return string("Viewkey not provided. Cant decrypt key image file without it");
            }

            string raw_data = remove_bad_chars(post_body["rawkeyimgsdata"]);
            string viewkey  = remove_bad_chars(post_body["viewkey"]);

            return wtipsblocks.show_checkrawkeyimgs(raw_data, viewkey);
        });
    }


    if (enable_output_key_checker)
    {
        CROW_ROUTE(app, "/rawoutputkeys")
        ([&](const crow::request& req) {
            return wtipsblocks.show_rawoutputkeys();
        });

        CROW_ROUTE(app, "/checkrawoutputkeys").methods("POST"_method)
        ([&](const crow::request& req) {

            map<std::string, std::string> post_body
                    = wtipseg::parse_crow_post_data(req.body);

            if (post_body.count("rawoutputkeysdata") == 0)
            {
                return string("Raw output keys data not given");
            }

            if (post_body.count("viewkey") == 0)
            {
                return string("Viewkey not provided. Cant decrypt "
                                      "key image file without it");
            }

            string raw_data = remove_bad_chars(post_body["rawoutputkeysdata"]);
            string viewkey  = remove_bad_chars(post_body["viewkey"]);

            return wtipsblocks.show_checkcheckrawoutput(raw_data, viewkey);
        });
    }


    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return wtipsblocks.search(remove_bad_chars(string(req.url_params.get("value"))));
    });

    CROW_ROUTE(app, "/mempool")
    ([&](const crow::request& req) {
        return wtipsblocks.mempool(true);
    });

    // alias to  "/mempool"
    CROW_ROUTE(app, "/txpool")
    ([&](const crow::request& req) {
        return wtipsblocks.mempool(true);
    });

//    CROW_ROUTE(app, "/altblocks")
//    ([&](const crow::request& req) {
//        return wtipsblocks.altblocks();
//    });

    CROW_ROUTE(app, "/robots.txt")
    ([&]() {
        string text = "User-agent: *\n"
                      "Disallow: ";
        return text;
    });

    if (enable_js)
    {
        cout << "Enable JavaScript checking of outputs and proving txs\n";

        CROW_ROUTE(app, "/js/jquery.min.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("jquery.min.js");
        });

        CROW_ROUTE(app, "/js/crc32.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("crc32.js");
        });

        CROW_ROUTE(app, "/js/biginteger.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("biginteger.js");
        });

        CROW_ROUTE(app, "/js/crypto.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("crypto.js");
        });

        CROW_ROUTE(app, "/js/config.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("config.js");
        });

        CROW_ROUTE(app, "/js/nacl-fast-cn.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("nacl-fast-cn.js");
        });

        CROW_ROUTE(app, "/js/base58.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("base58.js");
        });

        CROW_ROUTE(app, "/js/cn_util.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("cn_util.js");
        });

        CROW_ROUTE(app, "/js/sha3.js")
        ([&](const crow::request& req) {
            return wtipsblocks.get_js_file("sha3.js");
        });

        CROW_ROUTE(app, "/js/all_in_one.js")
        ([&]() {
            // /js/all_in_one.js file does not exist. it is generated on the fly
            // from the above real files.
            return wtipsblocks.get_js_file("all_in_one.js");
        });

    } // if (enable_js)

    if (enable_json_api)
    {

        cout << "Enable JSON API\n";

        CROW_ROUTE(app, "/api/transaction/<string>")
        ([&](string tx_hash) {

            mywtips::jsonresponse r{wtipsblocks.json_transaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/rawtransaction/<string>")
        ([&](string tx_hash) {

            mywtips::jsonresponse r{wtipsblocks.json_rawtransaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/detailedtransaction/<string>")
        ([&](string tx_hash) {

            mywtips::jsonresponse r{wtipsblocks.json_detailedtransaction(remove_bad_chars(tx_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/block/<string>")
        ([&](string block_no_or_hash) {

            mywtips::jsonresponse r{wtipsblocks.json_block(remove_bad_chars(block_no_or_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/rawblock/<string>")
        ([&](string block_no_or_hash) {

            mywtips::jsonresponse r{wtipsblocks.json_rawblock(remove_bad_chars(block_no_or_hash))};

            return r;
        });

        CROW_ROUTE(app, "/api/transactions").methods("GET"_method)
        ([&](const crow::request &req) {

            string page = regex_search(req.raw_url, regex {"page=\\d+"}) ?
                          req.url_params.get("page") : "0";

            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "25";

            mywtips::jsonresponse r{wtipsblocks.json_transactions(
                    remove_bad_chars(page), remove_bad_chars(limit))};

            return r;
        });

        CROW_ROUTE(app, "/api/mempool").methods("GET"_method)
        ([&](const crow::request &req) {

            string page = regex_search(req.raw_url, regex {"page=\\d+"}) ?
                          req.url_params.get("page") : "0";

            // default value for limit is some large number, so that
            // a call to api/mempool without any limit return all
            // mempool txs
            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "100000000";

            mywtips::jsonresponse r{wtipsblocks.json_mempool(
                    remove_bad_chars(page), remove_bad_chars(limit))};

            return r;
        });

        CROW_ROUTE(app, "/api/search/<string>")
        ([&](string search_value) {

            mywtips::jsonresponse r{wtipsblocks.json_search(remove_bad_chars(search_value))};

            return r;
        });

        CROW_ROUTE(app, "/api/networkinfo")
        ([&]() {

            mywtips::jsonresponse r{wtipsblocks.json_networkinfo()};

            return r;
        });

        CROW_ROUTE(app, "/api/emission")
        ([&]() {

            mywtips::jsonresponse r{wtipsblocks.json_emission()};
            return r;
        });

        CROW_ROUTE(app, "/api/circulating_supply") ([&]() {
            std::string result = std::to_string(wtipseg::CurrentBlockchainStatus::get_emission().coinbase / COIN);
            return std::move(result);
        });

        CROW_ROUTE(app, "/api/outputs").methods("GET"_method)
        ([&](const crow::request &req) {

            string tx_hash = regex_search(req.raw_url, regex {"txhash=\\w+"}) ?
                             req.url_params.get("txhash") : "";

            string address = regex_search(req.raw_url, regex {"address=\\w+"}) ?
                             req.url_params.get("address") : "";

            string viewkey = regex_search(req.raw_url, regex {"viewkey=\\w+"}) ?
                             req.url_params.get("viewkey") : "";

            bool tx_prove{false};

            try
            {
                tx_prove = regex_search(req.raw_url, regex {"txprove=[01]"}) ?
                           boost::lexical_cast<bool>(req.url_params.get("txprove")) :
                           false;
            }
            catch (const boost::bad_lexical_cast &e)
            {
                cerr << "Cant parse tx_prove as bool. Using default value" << endl;
            }

            mywtips::jsonresponse r{wtipsblocks.json_outputs(
                    remove_bad_chars(tx_hash),
                    remove_bad_chars(address),
                    remove_bad_chars(viewkey),
                    tx_prove)};

            return r;
        });

        CROW_ROUTE(app, "/api/outputsblocks").methods("GET"_method)
        ([&](const crow::request &req) {

            string limit = regex_search(req.raw_url, regex {"limit=\\d+"}) ?
                           req.url_params.get("limit") : "3";

            string address = regex_search(req.raw_url, regex {"address=\\w+"}) ?
                             req.url_params.get("address") : "";

            string viewkey = regex_search(req.raw_url, regex {"viewkey=\\w+"}) ?
                             req.url_params.get("viewkey") : "";

            bool in_mempool_aswell {false};

            try
            {
                in_mempool_aswell = regex_search(req.raw_url, regex {"mempool=[01]"}) ?
                           boost::lexical_cast<bool>(req.url_params.get("mempool")) :
                           false;
            }
            catch (const boost::bad_lexical_cast &e)
            {
                cerr << "Cant parse tx_prove as bool. Using default value" << endl;
            }

            mywtips::jsonresponse r{wtipsblocks.json_outputsblocks(
                    remove_bad_chars(limit),
                    remove_bad_chars(address),
                    remove_bad_chars(viewkey),
                    in_mempool_aswell)};

            return r;
        });

        CROW_ROUTE(app, "/api/version")
        ([&]() {

            mywtips::jsonresponse r{wtipsblocks.json_version()};

            return r;
        });

    } // if (enable_json_api)

    if (enable_autorefresh_option)
    {
        CROW_ROUTE(app, "/autorefresh")
        ([&]() {
            uint64_t page_no {0};
            bool refresh_page {true};
            return wtipsblocks.index2(page_no, refresh_page);
        });
    }

    // run the crow http server

    if (use_ssl)
    {
        cout << "Staring in ssl mode" << endl;
        app.bindaddr(bindaddr).port(app_port).ssl_file(ssl_crt_file, ssl_key_file)
                .multithreaded().run();
    }
    else
    {
        cout << "Staring in non-ssl mode" << endl;
        app.bindaddr(bindaddr).port(app_port).multithreaded().run();
    }


    if (enable_emission_monitor == true)
    {
        // finish Emission monitoring thread in a cotrolled manner.

        cout << "Waiting for emission monitoring thread to finish." << endl;

        wtipseg::CurrentBlockchainStatus::m_thread.interrupt();
        wtipseg::CurrentBlockchainStatus::m_thread.join();

        cout << "Emission monitoring thread finished." << endl;
    }

    // finish mempool thread

    cout << "Waiting for mempool monitoring thread to finish." << endl;

    wtipseg::MempoolStatus::m_thread.interrupt();
    wtipseg::MempoolStatus::m_thread.join();

    cout << "Mempool monitoring thread finished." << endl;

    cout << "The explorer is terminating." << endl;

    return EXIT_SUCCESS;
}
