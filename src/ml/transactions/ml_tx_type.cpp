/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "ml_tx_type.h"

#include <primitives/transaction.h>
#include <script/structured_data/structured_data.h>

bool mltx_valid(const MLTxType& type)
{
    return type < MLTX_COUNT;
}

bool mltx_valid(const int type)
{
    return type >= 0 && type < static_cast<int>(MLTX_COUNT);
}

bool mltx_valid(const unsigned int type)
{
    return type < static_cast<int>(MLTX_COUNT);
}

bool mltx_is_ml(const MLTxType& type)
{
    return type < MLTX_Regular;
}

bool mltx_is_ml(const int type)
{
    return type >= 0 && type < static_cast<int>(MLTX_Regular);
}

bool mltx_is_ml(const unsigned int type)
{
    return type < static_cast<int>(MLTX_Regular);
}

bool mltx_is_regular(const MLTxType& type)
{
    return type == MLTX_Regular;
}

bool mltx_is_regular(const int type)
{
    return type == static_cast<int>(MLTX_Regular);
}

bool mltx_is_regular(const unsigned int type)
{
    return type == static_cast<int>(MLTX_Regular);
}

MLTxType mltx_type(const CTransaction& tx)
{
    std::string reason;
    CScript script;
    if (!sds_from_tx(tx, script, reason))
        return MLTX_Regular;

    const auto& items = sds_script_items(script);
    if (items.size() < 3)
        return MLTX_Regular;

    if (sds_class(items) != SDC_PoUW)
        return MLTX_Regular;

    int mltx_type_int = CScriptNum(items[2], false).getint();
    if (!mltx_valid(mltx_type_int))
        return MLTX_Regular;

    return static_cast<MLTxType>(mltx_type_int);
}

const std::string mltx_name(const MLTxType& type)
{
    switch (type) {
    case MLTX_BuyTicket: return "buy_ticket"; break;
    case MLTX_RevokeTicket: return "revoke_ticket"; break;
    case MLTX_PayForTask: return "pay_for_task"; break;
    case MLTX_JoinTask: return "join_task"; break;
    case MLTX_Regular: return "regular"; break;
    default: return "invalid"; break;
    }
}
