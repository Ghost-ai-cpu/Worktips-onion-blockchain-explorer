var config = {
    testnet: false, // this is adjusted page.h if needed. dont need to change manually
    stagenet: false, // this is adjusted page.h if needed. dont need to change manually
    coinUnitPlaces: 8,
    txMinConfirms: 10,         // corresponds to CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE in Worktips
    txCoinbaseMinConfirms: 30, // corresponds to CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW in Worktips
    coinSymbol: 'WTIP',
    openAliasPrefix: "wtip",
    coinName: 'Worktips',
    coinUriPrefix: 'worktips:',
    addressPrefix: 0x121bb2,
    integratedAddressPrefix: 0x161bb2,
    subAddressPrefix: 0x1a9bb2,
    addressPrefixTestnet: 0x1ddd32,
    integratedAddressPrefixTestnet: 0x221d32,
    subAddressPrefixTestnet: 158,
    addressPrefixStagenet: 0xdd32,
    integratedAddressPrefixStagenet: 0x4dd32,
    subAddressPrefixStagenet: 0x95d32,
    feePerKB: new JSBigInt('2000000000'),//20^10 - for testnet its not used, as fee is dynamic.
    dustThreshold: new JSBigInt('1000000000'),//10^10 used for choosing outputs/change - we decompose all the way down if the receiver wants now regardless of threshold
    txChargeRatio: 0.5,
    defaultMixin: 9, // minimum mixin for hardfork v5
    txChargeAddress: '',
    idleTimeout: 30,
    idleWarningDuration: 20,
    maxBlockNumber: 500000000,
    avgBlockTime: 120,
    debugMode: false
};
