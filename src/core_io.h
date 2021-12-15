/* * Copyright (c) 2009-2016 The Bitcoin Core developers
 * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_CORE_IO_H
#define BWSCOIN_CORE_IO_H

#include "amount.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

class CBlock;
class CScript;
class CTransaction;
struct CMutableTransaction;
class uint256;
class UniValue;

// core_read.cpp
CScript ParseScript(const std::string& s);
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode = false);
bool DecodeHexTx(CMutableTransaction& tx, const std::string& strHexTx, bool fTryNoWitness = false);
bool DecodeHexBlk(CBlock&, const std::string& strHexBlk);
uint256 ParseHashUV(const UniValue& v, const std::string& strName);
uint256 ParseHashStr(const std::string&, const std::string& strName);
std::vector<unsigned char> ParseHexUV(const UniValue& v, const std::string& strName);

// core_write.cpp
UniValue ValueFromAmount(const CAmount& amount);
std::string FormatScript(const CScript& script);
std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_stake, bool include_hex = true, int serialize_flags = 0
, const std::map<uint256,std::shared_ptr<const CTransaction>>* const prevHashToTxMap = nullptr);
void StakingToUniv(const CTransaction& tx, UniValue& entry, bool fIncludeContrib = true);
void StakeInfoToUniv(const CTransaction& tx, UniValue& entry
, const std::map<uint256,std::shared_ptr<const CTransaction>>* const prevHashToTxMap = nullptr);

#endif // BWSCOIN_CORE_IO_H
