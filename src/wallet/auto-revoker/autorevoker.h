/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_WALLET_AUTOVREVOKER_AUTOVREVOKER_H
#define BWSCOIN_WALLET_AUTOVREVOKER_AUTOVREVOKER_H

#include "autorevokerconfig.h"
#include "validationinterface.h"

#include <atomic>

class CWallet;

// The Automatic Revoker (AR)
// This is responsible with monitoring the blockchain advance
// and automatically generate and publish any revocation transaction
// that is needed for a missed or expired ticket.

class CAutoRevoker : public CValidationInterface {
public:
    CAutoRevoker() = delete;
    CAutoRevoker(CWallet* wallet);
    virtual ~CAutoRevoker();

    virtual void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;

    CAutoRevokerConfig& GetConfig() { return config; }

    void start();
    void stop();
    bool isStarted() const;

private:
    CAutoRevokerConfig config;

    CWallet* pwallet;

    std::atomic<bool> configured;
};

#endif // BWSCOIN_WALLET_AUTOVREVOKER_AUTOVREVOKER_H
