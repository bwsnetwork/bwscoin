/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// Various functions and constants helping the implementation of
// machine learning transctions.

#ifndef BWSCOIN_ML_TX_HELPERS_H
#define BWSCOIN_ML_TX_HELPERS_H

#include <primitives/transaction.h>

extern const uint32_t mltx_stake_txout_index;
extern const uint32_t mltx_change_txout_index;
extern const uint32_t mltx_ticket_txin_index;

bool mltx_is_payment_txout(const CTxOut& txout);
bool mltx_is_data_txout(const CTxOut& txout);
bool mltx_is_structured_data_txout(const CTxOut& txout);

#endif // BWSCOIN_ML_TX_HELPERS_H
