/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_WALLET_TICKETBUYER_TICKETBUYER_H
#define BWSCOIN_WALLET_TICKETBUYER_TICKETBUYER_H

#include "ticketbuyerconfig.h"
#include "validationinterface.h"

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

class CWallet;

// The Automatic Ticket Buyer (TB)
// This is responsible with monitoring the blockchain advance
// and automatically generate and publish any ticket purchase transaction
// on behalf of the user.

class CTicketBuyer : public CValidationInterface {
public:
    CTicketBuyer() = delete;
    CTicketBuyer(CWallet* wallet);
    virtual ~CTicketBuyer();

    virtual void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;

    CTicketBuyerConfig& GetConfig() { return config; }

    void start();
    void stop();    // does not stop immediately, but only after the current iteration
    bool isStarted() const;

private:
    void mainLoop();

    CTicketBuyerConfig config;

    CWallet* pwallet;

    std::atomic<bool> configured;

    std::atomic<bool> shouldRun;

    std::thread thread;

    std::condition_variable cv;
    std::mutex mtx;
};

#endif // BWSCOIN_WALLET_TICKETBUYER_TICKETBUYER_H
