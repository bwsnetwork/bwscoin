/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "ml_tx_size.h"

#include <ml/transactions/buy_ticket_tx.h>
#include <ml/transactions/revoke_ticket_tx.h>
#include <ml/transactions/pay_for_task_tx.h>

size_t p2pkh_txin_estimated_size(bool compressed)
{
    // a P2PKH input has the following structure:
    // - previous outpoint hash     [32 bytes]
    // - previous outpoint index    [4 bytes]
    // - scriptsig size             [1 byte]
    // - push opcode                [1 byte]
    // - signature                  [71 or 72 bytes]
    // - push opcode                [1 byte]
    // - public key                 [33 bytes compressed, 65 bytes uncompressed]
    // - sequence                   [4 bytes]

    return 32 + 4 + 1 + 1 + 72 + 1 + (compressed ? 33 : 65) + 4;
}

size_t p2pkh_txout_estimated_size()
{
    // a P2PKH output has the following structure:
    // - value              [8 bytes]
    // - script size        [1 byte]
    // - OP_DUP             [1 byte]
    // - OP_HASH160         [1 byte]
    // - push opcode        [1 byte]
    // - public key hash    [20 bytes]
    // - OP_EQUALVERIFY     [1 byte]
    // - OP_CHECKSIG        [1 byte]

    return 8 + 1 + 1 + 1 + 1 + 20 + 1 + 1;
}

size_t byt_txout_estimated_size()
{
    CScript script;
    if (!byt_script(script, AT_Client, CKeyID()))
        return 0;

    CTxOut txout{0, script};
    return GetSerializeSize(txout, SER_NETWORK, PROTOCOL_VERSION);
}

size_t rvt_txout_estimated_size()
{
    CScript script;
    if (!rvt_script(script))
        return 0;

    CTxOut txout{0, script};
    return GetSerializeSize(txout, SER_NETWORK, PROTOCOL_VERSION);
}


std::vector<size_t> pft_txout_estimated_sizes(const nlohmann::json& task)
{
    CScript script;
    if (!pft_script(script, task))
        return std::vector<size_t>();

    std::vector<CTxOut> txouts = sds_tx_outputs(script);

    std::vector<size_t> sizes;
    for (const auto& txout: txouts)
        sizes.push_back(GetSerializeSize(txout, SER_NETWORK, PROTOCOL_VERSION));

    return sizes;
}

size_t byt_estimated_size(const unsigned long txin_count, const bool has_change, const bool include_expiry)
{
    // since the format of the buy ticket transaction is fixed,
    // its size can be estimated rather precisely.
    // A buy ticket transaction contains:
    // version + in count + (txin_count) regular inputs + out count (2|3) + buy ticket script output + stake address output + change output (optional) + locktime + expiry (optional)

    return 4
            + 1
            + txin_count * p2pkh_txin_estimated_size()
            + 1
            + byt_txout_estimated_size()
            + p2pkh_txout_estimated_size()
            + (has_change ? p2pkh_txout_estimated_size() : 0)
            + 4
            + (include_expiry ? 4 : 0);
}

size_t rvt_estimated_size(const bool include_expiry)
{
    // since the format of the revoke ticket transaction is fixed,
    // its size can be estimated rather precisely.
    // A revoke ticket transaction contains:
    // version + in count (1) + (1) regular input + out count (2) + revoke ticket script output + refund address output + locktime + expiry (optional)

    return 4
            + 1
            + p2pkh_txin_estimated_size()
            + 1
            + rvt_txout_estimated_size()
            + p2pkh_txout_estimated_size()
            + 4
            + (include_expiry ? 4 : 0);
}

size_t pft_estimated_size(const unsigned long extra_funding_count, const nlohmann::json& task, const bool has_change, const bool include_expiry)
{
    // since the format of the pay for task transaction is fixed,
    // its size can be estimated. However, given the variable size
    // of the tasks, the estimation might not be exact.
    // A buy ticket transaction contains:
    // version + in count (funding_count+1) + (1+funding_count) regular inputs + out count (2+) + pay for task first script output + stake address output + change output (optional) + subsequent pay for task script output + locktime + expiry (optional)

    const auto& sizes = pft_txout_estimated_sizes(task);
    if (sizes.size() < 1)
        return 0;

    return 4
            + 1
            + (1 + extra_funding_count) * p2pkh_txin_estimated_size()
            + 1
            + sizes[0]
            + p2pkh_txout_estimated_size()
            + (has_change ? p2pkh_txout_estimated_size() : 0)
            + std::accumulate(sizes.begin() + 1, sizes.end(), 0UL)
            + 4
            + (include_expiry ? 4 : 0);
}
