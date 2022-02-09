/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_WALLET_AUTOREVOKER_AUTOREVOKERCFG_H
#define BWSCOIN_WALLET_AUTOREVOKER_AUTOREVOKERCFG_H

#include "support/allocators/secure.h"

// The Automatic Revoker (AR) configuration
class CAutoRevokerConfig {

public:
    // Enables the automatic revoking
    bool autoRevoke;

    // Wallet passphrase
    SecureString passphrase;

    CAutoRevokerConfig();

    void ParseCommandline();
};

#endif // BWSCOIN_WALLET_AUTOREVOKER_AUTOREVOKERCFG_H
