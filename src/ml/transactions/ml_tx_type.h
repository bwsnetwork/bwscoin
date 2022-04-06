/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// Machine learning workflow spans several special transaction types. These are
// defined in the structured data script.

#ifndef BWSCOIN_ML_TX_TYPE_H
#define BWSCOIN_ML_TX_TYPE_H

#include <string>

class CTransaction;

enum MLTxType : unsigned int
{
    MLTX_BuyTicket = 0,
    MLTX_RevokeTicket,
    MLTX_PayForTask,
    MLTX_JoinTask,

    MLTX_Regular,   // this should always immediately precede the count
    MLTX_COUNT
};

bool mltx_valid(const MLTxType& type);
bool mltx_valid(const int type);
bool mltx_valid(const unsigned int type);

bool mltx_is_ml(const MLTxType& type);
bool mltx_is_ml(const int type);
bool mltx_is_ml(const unsigned int type);

bool mltx_is_regular(const MLTxType& type);
bool mltx_is_regular(const int type);
bool mltx_is_regular(const unsigned int type);

MLTxType mltx_type(const CTransaction& tx);

const std::string mltx_name(const MLTxType& type);

#endif // BWSCOIN_ML_TX_TYPE_H
