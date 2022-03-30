/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "ml/transactions/ml_tx_helpers.h"

#include <script/standard.h>
#include <script/structured_data/structured_data.h>

const uint32_t mltx_stake_txout_index = sds_first_output_index + 1;
const uint32_t mltx_refund_txout_index = sds_first_output_index + 1;
const uint32_t mltx_change_txout_index = mltx_stake_txout_index + 1;
const uint32_t mltx_ticket_txin_index = 0;

bool mltx_is_payment_txout(const CTxOut& txout)
{
    if (txout.scriptPubKey.size() <= 0)
        return false;

    if (mltx_is_data_txout(txout))
        return false;

    return !txout.scriptPubKey.IsUnspendable();
}

bool mltx_is_legal_stake_txout(const CTxOut& txout)
{
    if (!mltx_is_payment_txout(txout))
        return false;

    txnouttype type;
    std::vector<std::vector<unsigned char> > solutions;
    if (!Solver(txout.scriptPubKey, type, solutions))
        return false;
    return type == TX_PUBKEYHASH || type == TX_SCRIPTHASH;
}

bool mltx_is_data_txout(const CTxOut& txout)
{
    return txout.scriptPubKey.size() > 0 && txout.scriptPubKey[0] == OP_RETURN;
}

bool mltx_is_structured_data_txout(const CTxOut& txout)
{
    std::string reason;
    return sds_valid(txout.scriptPubKey, reason);
}
