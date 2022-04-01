//
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 Project PAI Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//


#include <core_io.h>

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>
#include <stake/staketx.h>
#include <ml/transactions/ml_tx_helpers.h>
#include <ml/transactions/ml_tx_type.h>
#include <ml/transactions/buy_ticket_tx.h>
#include <ml/transactions/pay_for_task_tx.h>
#include <ml/transactions/revoke_ticket_tx.h>

UniValue ValueFromAmount(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
}

std::string FormatScript(const CScript& script)
{
    std::string ret;
    CScript::const_iterator it = script.begin();
    opcodetype op;
    while (it != script.end()) {
        CScript::const_iterator it2 = it;
        std::vector<unsigned char> vch;
        if (script.GetOp2(it, op, &vch)) {
            if (op == OP_0) {
                ret += "0 ";
                continue;
            } else if ((op >= OP_1 && op <= OP_16) || op == OP_1NEGATE) {
                ret += strprintf("%i ", op - OP_1NEGATE - 1);
                continue;
            } else if (op >= OP_NOP && op <= OP_NOP10) {
                std::string str(GetOpName(op));
                if (str.substr(0, 3) == std::string("OP_")) {
                    ret += str.substr(3, std::string::npos) + " ";
                    continue;
                }
            }
            if (vch.size() > 0) {
                ret += strprintf("0x%x 0x%x ", HexStr(it2, it - vch.size()), HexStr(it - vch.size(), it));
            } else {
                ret += strprintf("0x%x ", HexStr(it2, it));
            }
            continue;
        }
        ret += strprintf("0x%x ", HexStr(it2, script.end()));
        break;
    }
    return ret.substr(0, ret.size() - 1);
}

const std::map<unsigned char, std::string> mapSigHashTypes = {
    {static_cast<unsigned char>(SIGHASH_ALL), std::string("ALL")},
    {static_cast<unsigned char>(SIGHASH_ALL|SIGHASH_ANYONECANPAY), std::string("ALL|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_NONE), std::string("NONE")},
    {static_cast<unsigned char>(SIGHASH_NONE|SIGHASH_ANYONECANPAY), std::string("NONE|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_SINGLE), std::string("SINGLE")},
    {static_cast<unsigned char>(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY), std::string("SINGLE|ANYONECANPAY")},
};

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 * @param[in] fAttemptSighashDecode    Whether to attempt to decode sighash types on data within the script that matches the format
 *                                     of a signature. Only pass true for scripts you believe could contain signatures. For example,
 *                                     pass false, or omit the this argument (defaults to false), for scriptPubKeys.
 */
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode, const bool fAttemptNullDataOnlyDecode)
{
    if (!fAttemptNullDataOnlyDecode && script[0] == OP_RETURN && script[1] != OP_STRUCT)
        return std::string(GetOpName(OP_RETURN)) + " " + HexStr(script.begin() + 1, script.end());

    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        if (!script.GetOp(pc, opcode, vch)) {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4) {
            if (vch.size() <= static_cast<std::vector<unsigned char>::size_type>(4)) {
                str += strprintf("%d", CScriptNum(vch, false).getint());
            } else {
                // the IsUnspendable check makes sure not to try to decode OP_RETURN data that may match the format of a signature
                if (fAttemptSighashDecode && !script.IsUnspendable()) {
                    std::string strSigHashDecode;
                    // goal: only attempt to decode a defined sighash type from data that looks like a signature within a scriptSig.
                    // this won't decode correctly formatted public keys in Pubkey or Multisig scripts due to
                    // the restrictions on the pubkey formats (see IsCompressedOrUncompressedPubKey) being incongruous with the
                    // checks in CheckSignatureEncoding.
                    if (CheckSignatureEncoding(vch, SCRIPT_VERIFY_STRICTENC, nullptr)) {
                        const unsigned char chSigHashType = vch.back();
                        if (mapSigHashTypes.count(chSigHashType)) {
                            strSigHashDecode = "[" + mapSigHashTypes.find(chSigHashType)->second + "]";
                            vch.pop_back(); // remove the sighash type byte. it will be replaced by the decode.
                        }
                    }
                    str += HexStr(vch) + strSigHashDecode;
                } else {
                    str += HexStr(vch);
                }
            }
        } else {
            str += GetOpName(opcode);
        }
    }
    return str;
}

std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serializeFlags);
    ssTx << tx;
    return HexStr(ssTx.begin(), ssTx.end());
}

void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex)
{
    txnouttype type;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
        out.pushKV("type", GetTxnOutputType(type));
        return;
    }

    out.pushKV("reqSigs", nRequired);
    out.pushKV("type", GetTxnOutputType(type));

    UniValue a(UniValue::VARR);
    for (const CTxDestination& addr : addresses) {
        a.push_back(EncodeDestination(addr));
    }
    out.pushKV("addresses", a);
}

void StakingToUniv(const CTransaction& tx, UniValue& entry, bool fIncludeContrib) 
{
    std::vector<TicketContribData> contributions;
    CAmount totalContribution, totalVoteFeeLimit, totalRevocationFeeLimit;
    const auto& result = ParseTicketContribs(tx, contributions, totalContribution, totalVoteFeeLimit, totalRevocationFeeLimit);
    if (result) {
        entry.pushKV("ticket_price", ValueFromAmount(totalContribution));
        entry.pushKV("fee_limit", ValueFromAmount(totalVoteFeeLimit + totalRevocationFeeLimit));
        if (fIncludeContrib) {
            auto contribs = UniValue(UniValue::VARR);
            for (const auto& contribData : contributions) {
                auto contrib = UniValue(UniValue::VOBJ);
                contrib.pushKV("rewardAddr", EncodeDestination(contribData.rewardAddr));
                contrib.pushKV("contributedAmount", ValueFromAmount(contribData.contributedAmount));
                contribs.push_back(contrib);
            }
            entry.pushKV("contributions", contribs);
        }
    }
}

void StakeInfoToUniv(const CTransaction& tx, UniValue& entry, const std::map<uint256,std::shared_ptr<const CTransaction>>* const prevHashToTxMap)
{
    const auto txClass = ParseTxClass(tx);
    entry.pushKV("type", TxClassToString(txClass));
    if (txClass == ETxClass::TX_Vote) {
        UniValue voting(UniValue::VOBJ);
        const auto ticketHash = tx.vin[voteStakeInputIndex].prevout.hash;
        voting.pushKV("ticket", ticketHash.GetHex());
        VoteData voteData;
        ParseVote(tx, voteData);
        voting.pushKV("version", voteData.nVersion);
        voting.pushKV("vote", voteData.voteBits.isRttAccepted() ? "valid" : "invalid");
        voting.pushKV("blockhash", voteData.blockHash.GetHex());
        voting.pushKV("blockheight", (int64_t)voteData.blockHeight);
        entry.pushKV("voting", voting);
    } else if (txClass == ETxClass::TX_BuyTicket) {
        UniValue staking(UniValue::VOBJ);
        StakingToUniv(tx, staking);
        entry.pushKV("staking", staking);
    } else if (txClass == ETxClass::TX_RevokeTicket) {
        UniValue staking(UniValue::VOBJ);
        const auto ticketHash = tx.vin[revocationStakeInputIndex].prevout.hash;
        staking.pushKV("ticket", ticketHash.GetHex());
        if (prevHashToTxMap != nullptr) {
            const auto& it = prevHashToTxMap->find(ticketHash);
            if (it != std::end(*prevHashToTxMap)) {
                const auto& ticketTx = *it->second;
                StakingToUniv(ticketTx, staking);
            }
        }
        entry.pushKV("staking", staking);
    }
}

void MlTxToUniv(BuyTicketTx& btx, UniValue& entry)
{
    if (!btx.valid()) {
        entry.pushKV("status", "INVALID!");
        return;
    }

    entry.pushKV("version", static_cast<uint64_t>(btx.version()));
    entry.pushKV("actor", at_to_string(btx.actor()));
    entry.pushKV("reward address", EncodeDestination(btx.reward_address()));

    UniValue stake{UniValue::VOBJ};
    stake.pushKV("address", EncodeDestination(btx.stake_address()));
    stake.pushKV("amount", btx.stake_amount());
    entry.pushKV("stake", stake);

    if (!btx.change_txout().IsNull()) {
        UniValue change{UniValue::VOBJ};
        change.pushKV("address", EncodeDestination(btx.change_address()));
        change.pushKV("amount", btx.change_amount());
        entry.pushKV("change", change);
    }
}

void MlTxToUniv(PayForTaskTx& ptx, UniValue& entry)
{
    if (!ptx.valid()) {
        entry.pushKV("status", "INVALID!");
        return;
    }

    entry.pushKV("version", static_cast<uint64_t>(ptx.version()));
    std::string task_str;
    if (pft_task_string(ptx.task(), task_str))
        entry.pushKV("task", task_str);
    else
        entry.pushKV("task", "invalid");

    UniValue ticket{UniValue::VOBJ};
    const auto& ticket_out = ptx.ticket_txin().prevout;
    ticket.pushKV("tx", ticket_out.hash.GetHex());
    ticket.pushKV("n", static_cast<uint64_t>(ticket_out.n));
    entry.pushKV("ticket", ticket);

    UniValue stake{UniValue::VOBJ};
    stake.pushKV("amount", ptx.stake_amount());
    entry.pushKV("stake", stake);

    if (!ptx.change_txout().IsNull()) {
        UniValue change{UniValue::VOBJ};
        change.pushKV("address", EncodeDestination(ptx.change_address()));
        change.pushKV("amount", ptx.change_amount());
        entry.pushKV("change", change);
    }
}

void MlTxToUniv(RevokeTicketTx& rtx, UniValue& entry)
{
    if (!rtx.valid()) {
        entry.pushKV("status", "INVALID!");
        return;
    }

    entry.pushKV("version", static_cast<uint64_t>(rtx.version()));

    UniValue ticket{UniValue::VOBJ};
    const auto& ticket_out = rtx.ticket_txin().prevout;
    ticket.pushKV("tx", ticket_out.hash.GetHex());
    ticket.pushKV("n", static_cast<uint64_t>(ticket_out.n));
    entry.pushKV("ticket", ticket);

    UniValue refund{UniValue::VOBJ};
    refund.pushKV("address", EncodeDestination(rtx.refund_address()));
    refund.pushKV("amount", rtx.refund_amount());
    entry.pushKV("refund", refund);
}

void MlTxToUniv(const CTransaction& tx, UniValue& entry)
{
    // quick checks
    if (tx.vout.size() < 1 || tx.vout[0].scriptPubKey[0] != OP_RETURN || tx.vout[0].scriptPubKey[1] != OP_STRUCT)
        return;

    UniValue ml(UniValue::VOBJ);

    switch (mltx_type(tx)) {
    case MLTX_BuyTicket: {
        BuyTicketTx btx = BuyTicketTx::from_tx(tx);
        MlTxToUniv(btx, ml);
        entry.pushKV("type", BuyTicketTx::name());
        entry.pushKV("ml", ml);
    } break;
    case MLTX_RevokeTicket: {
        RevokeTicketTx rtx = RevokeTicketTx::from_tx(tx);
        MlTxToUniv(rtx, ml);
        entry.pushKV("type", RevokeTicketTx::name());
        entry.pushKV("ml", ml);
    } break;
    case MLTX_PayForTask: {
        PayForTaskTx ptx = PayForTaskTx::from_tx(tx);
        MlTxToUniv(ptx, ml);
        entry.pushKV("type", PayForTaskTx::name());
        entry.pushKV("ml", ml);
    } break;
    case MLTX_Regular: {
        entry.pushKV("type", mltx_name(MLTX_Regular));
    } break;
    default:
        break;
    }
}

void TxInToUniv(const CTxIn& txin, const bool coinbase, const std::map<uint256,std::shared_ptr<const CTransaction>>* const prevHashToTxMap, UniValue& entry)
{
    if (coinbase)
        entry.pushKV("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
    else {
        entry.pushKV("txid", txin.prevout.hash.GetHex());
        entry.pushKV("vout", static_cast<int64_t>(txin.prevout.n));
        UniValue o(UniValue::VOBJ);
        o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
        o.pushKV("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
        entry.pushKV("scriptSig", o);
        if (!txin.scriptWitness.IsNull()) {
            UniValue txinwitness(UniValue::VARR);
            for (const auto& item : txin.scriptWitness.stack) {
                txinwitness.push_back(HexStr(item.begin(), item.end()));
            }
            entry.pushKV("txinwitness", txinwitness);
        }
        if( prevHashToTxMap != nullptr) {
            const auto& it = prevHashToTxMap->find(txin.prevout.hash);
            if (it != std::end(*prevHashToTxMap) ){
                UniValue prevOut(UniValue::VARR);
                const auto& prevTx = it->second;
                for (const auto& txOut : prevTx->vout) {
                    txnouttype type;
                    std::vector<CTxDestination> addresses;
                    int nRequired;
                    if (ExtractDestinations(txOut.scriptPubKey, type, addresses, nRequired)) {
                        UniValue a(UniValue::VARR);
                        for (const CTxDestination& addr : addresses) {
                            a.push_back(EncodeDestination(addr));
                        }

                        UniValue vout(UniValue::VOBJ);
                        vout.pushKV("addresses", a);
                        vout.pushKV("value", ValueFromAmount(txOut.nValue));
                        prevOut.push_back(vout);
                    }
                }
                entry.pushKV("prevOut", prevOut);
            }
        }
    }
    entry.pushKV("sequence", static_cast<int64_t>(txin.nSequence));
}

void TxOutToUniv(const CTxOut& txout, const unsigned int idx, UniValue& entry, bool include_hex)
{
    assert(entry.isObject());

    entry.pushKV("value", ValueFromAmount(txout.nValue));
    entry.pushKV("n", static_cast<int64_t>(idx));

    UniValue o(UniValue::VOBJ);
    ScriptPubKeyToUniv(txout.scriptPubKey, o, include_hex);
    entry.pushKV("scriptPubKey", o);
}

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_stake, bool include_ml, bool include_hex, int serialize_flags
            , const std::map<uint256,std::shared_ptr<const CTransaction>>* const prevHashToTxMap)
{
    entry.pushKV("txid", tx.GetHash().GetHex());
    if (tx.IsCoinBase())
        entry.pushKV("type",  "coinbase");
    else {
        if (include_stake)
            StakeInfoToUniv(tx, entry, prevHashToTxMap);
        if (include_ml)
            MlTxToUniv(tx, entry);
    }

    entry.pushKV("hash", tx.GetWitnessHash().GetHex());
    entry.pushKV("version", tx.nVersion);
    entry.pushKV("size", (int)::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION));
    entry.pushKV("vsize", (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    entry.pushKV("locktime", (int64_t)tx.nLockTime);
    entry.pushKV("expiry", (int64_t)tx.nExpiry);

    UniValue vin(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        TxInToUniv(txin, tx.IsCoinBase(), prevHashToTxMap, in);
        vin.push_back(in);
    }
    entry.pushKV("vin", vin);

    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];

        UniValue out(UniValue::VOBJ);
        TxOutToUniv(txout, i, out);
        vout.push_back(out);
    }
    entry.pushKV("vout", vout);

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx, serialize_flags)); // the hex-encoded transaction. used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}
