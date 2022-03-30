/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "actor_type.h"

#include <ml/transactions/buy_ticket_tx.h>
#include <ml/transactions/ml_tx_type.h>
#include <primitives/transaction.h>
#include <script/structured_data/structured_data.h>

#include <algorithm>
#include <cctype>

bool at_valid(const ActorType& actor)
{
    return actor < AT_COUNT;
}

bool at_valid(const int actor)
{
    return actor >= 0 && actor < static_cast<int>(AT_COUNT);
}

bool at_valid(const unsigned int actor)
{
    return actor < static_cast<int>(AT_COUNT);
}

ActorType at_actor(const CTransaction& tx)
{
    std::string reason;
    CScript script;
    if (!sds_from_tx(tx, script, reason))
        return AT_COUNT;

    const auto& items = sds_script_items(script);
    if (items.size() < 3)
        return AT_COUNT;

    if (sds_class(items) != SDC_PoUW)
        return AT_COUNT;

    int mltx_type_int = CScriptNum(items[2], false).getint();
    if (!mltx_valid(mltx_type_int))
        return AT_COUNT;

    if (static_cast<MLTxType>(mltx_type_int) != MLTX_BuyTicket)
        return AT_COUNT;

    int version_int = CScriptNum(items[3], false).getint();
    if (version_int < 0 || version_int > static_cast<int>(byt_current_version))
        return AT_COUNT;

    int actor_int = CScriptNum(items[4], false).getint();
    if (!at_valid(actor_int))
        return AT_COUNT;

    return static_cast<ActorType>(actor_int);
}

ActorType at_from_string(const std::string& str)
{
    std::string lstr;
    std::transform(str.begin(), str.end(), std::back_inserter(lstr),
                   [](unsigned char c) { return std::tolower(c); });

    if (lstr == "client")
        return AT_Client;
    else if (lstr == "miner")
        return AT_Miner;

    return AT_COUNT;
}

std::string at_to_string(const ActorType& at)
{
    switch (at) {
    case AT_Client: return "client"; break;
    case AT_Miner: return "miner"; break;
    default: return ""; break;
    }
}
