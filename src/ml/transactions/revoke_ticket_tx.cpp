/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "revoke_ticket_tx.h"

#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <script/structured_data/structured_data.h>
#include <ml/transactions/buy_ticket_tx.h>
#include <ml/transactions/ml_tx_helpers.h>
#include <ml/transactions/ml_tx_size.h>
#include <ml/transactions/ml_tx_type.h>
#include <policy/feerate.h>

const unsigned int rvt_current_version = 0;

bool rvt_script(CScript& script, unsigned int version)
{
    if (version > rvt_current_version)
        return false;

    script = sds_create(SDC_PoUW) << MLTX_RevokeTicket << version;

    return true;
}

bool rvt_script_valid(const CScript& script, std::string& reason)
{
    return rvt_script_valid(sds_script_items(script), reason);
}

bool rvt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason)
{
    unsigned int version;
    return rvt_parse_script(items, version, reason);
}

bool rvt_parse_script(const CScript& script,
                      unsigned int& version, std::string& reason)
{
    return rvt_parse_script(sds_script_items(script), version, reason);
}

bool rvt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, std::string& reason)
{
    if (items.size() < 2) {
        reason = "invalid-script-size";
        return false;
    }

    if (!sds_valid(items, reason))
        return false;

    auto cls = sds_class(items);
    if (cls != SDC_PoUW) {
        reason = "not-pouw-class";
        return false;
    }

    int mltx_int = CScriptNum(items[2], false).getint();
    if (!mltx_valid(mltx_int) || static_cast<MLTxType>(mltx_int) != MLTX_RevokeTicket) {
        reason = "not-revoketicket-tx";
        return false;
    }

    int version_int = CScriptNum(items[3], false).getint();
    if (version_int < 0 || version_int > static_cast<int>(rvt_current_version)) {
        reason = "invalid-revoketicket-version";
        return false;
    }

    return true;
}

bool rvt_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin,
                  CTxOut& refund_txout, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, std::string& reason)
{
    // sizes
    if (tx.vin.size() <= mltx_ticket_txin_index || tx.vin.size() > mltx_ticket_txin_index + 1) {
        reason = "invalid-input-count";
        return false;
    }

    if (tx.vout.size() <= mltx_refund_txout_index) {
        reason = "invalid-output-count";
        return false;
    }

    // inputs
    for (auto txin: tx.vin)
        if (txin.prevout.IsNull()) {
            reason = "null-input";
            return false;
        }
    ticket_txin = tx.vin[mltx_ticket_txin_index];

    // refund output
    refund_txout = tx.vout[mltx_refund_txout_index];
    CTxDestination refund_destination;
    bool refund_destination_ok = ExtractDestination(refund_txout.scriptPubKey, refund_destination) && IsValidDestination(refund_destination);
    bool refund_value_ok = refund_txout.nValue != 0 && MoneyRange(refund_txout.nValue);
    if (!refund_destination_ok || !refund_value_ok) {
        reason = "invalid-refund";
        return false;
    }

    // change output (optional)
    CTxDestination change_destination;
    if (tx.vout.size() > mltx_change_txout_index) {
        change_txout = tx.vout[mltx_change_txout_index];
        bool change_destination_ok = ExtractDestination(change_txout.scriptPubKey, change_destination) && IsValidDestination(change_destination);
        bool change_value_ok = change_txout.nValue != 0 && MoneyRange(change_txout.nValue);
        if (change_destination_ok != change_value_ok) {
            reason = "invalid-change";
            return false;
        }
        else if (!change_destination_ok && !change_value_ok)
            change_txout = CTxOut();
    }

    // structured script
    script = sds_from_tx(tx, reason);
    if (script.size() == 0)
        return false;
    items = sds_script_items(script);
    if (!rvt_parse_script(items, version, reason))
        return false;

    return true;
}

bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxOut& refund_txout, const CTxOut& change_txout,
            const unsigned int version)
{
    // refund value
    if (refund_txout.nValue == 0 || !MoneyRange(refund_txout.nValue))
        return false;

    // ticket
    if (ticket_txin.prevout.IsNull() ||
            ticket_txin.prevout.n != mltx_refund_txout_index)
        return false;

    // refund
    CTxDestination refund_destination;
    bool refund_destination_ok = mltx_is_payment_txout(refund_txout) && ExtractDestination(refund_txout.scriptPubKey, refund_destination) && IsValidDestination(refund_destination);
    bool refund_value_ok = refund_txout.nValue != 0 && MoneyRange(refund_txout.nValue);
    if (!refund_destination_ok || !refund_value_ok)
        return false;

    // change (optional)
    CTxDestination change_destination;
    bool change_destination_ok = mltx_is_payment_txout(change_txout) && ExtractDestination(change_txout.scriptPubKey, change_destination) && IsValidDestination(change_destination);
    bool change_value_ok = change_txout.nValue != 0 && MoneyRange(change_txout.nValue);
    if (change_destination_ok != change_value_ok)
        return false;   // inconsistent settings
    bool has_change = change_destination_ok && change_value_ok;

    // script
    CScript script;
    if (!rvt_script(script, version))
        return false;

    // structured data script transaction outputs
    auto script_txouts = sds_tx_outputs(script);
    if (script_txouts.size() != 1)
        return false;

    // tx construction
    // (assumes mltx_ticket_txin_index=0, mltx_refund_txout_index=1, mltx_change_txout_index=2, sds_first_output_index=0)
    tx.vin.clear();
    tx.vin.push_back(ticket_txin);
    tx.vout.clear();
    tx.vout.push_back(script_txouts[0]);
    tx.vout.push_back(refund_txout);
    if (has_change)
        tx.vout.push_back(change_txout);

    return true;
}

bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& refund_address, const CAmount& refund,
            const CTxDestination& change_address, const CAmount& change,
            const unsigned int version)
{
    return rvt_tx(tx,
                  ticket_txin,
                  CTxOut(refund, GetScriptForDestination(refund_address)),
                  CTxOut(change, GetScriptForDestination(change_address)),
                  version);
}

bool rvt_tx_valid(const CTransaction& tx, std::string& reason)
{
    CTxIn ticket_txin;
    CTxOut refund_txout;
    CTxOut change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;

    if (!rvt_parse_tx(tx, ticket_txin, refund_txout, change_txout, script, items, version, reason))
        return false;

    if (version > rvt_current_version) {
        reason = "invalid-rvt-version";
        return false;
    }

    return true;
}

bool rvt_check_inputs_nc(const CTransaction& tx, CValidationState &state)
{
    if (tx.vin.size() < mltx_ticket_txin_index + 1 ||
            tx.vin.size() > mltx_ticket_txin_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-revoketicket-input-count");

    if (tx.vin[mltx_ticket_txin_index].prevout.n != mltx_stake_txout_index)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-reference");

    for (const auto& txin : tx.vin)
        if (txin.prevout.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");

    return true;
}

bool rvt_check_outputs_nc(const CTransaction& tx, CValidationState &state)
{
    if (tx.vout.size() < mltx_refund_txout_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-revoketicket-output-count");

    if (!sds_is_first_output(tx.vout[sds_first_output_index]))
        return state.DoS(100, false, REJECT_INVALID, "invalid-sds-first-output");

    if (tx.vout[mltx_refund_txout_index].nValue == 0 || !MoneyRange(tx.vout[mltx_refund_txout_index].nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-refund-amount");

    CTxDestination refund_destination;
    if (!ExtractDestination(tx.vout[mltx_refund_txout_index].scriptPubKey, refund_destination) ||
            !IsValidDestination(refund_destination))
        return state.DoS(100, false, REJECT_INVALID, "bad-refund-address");

    bool has_change = (tx.vout.size() >= mltx_change_txout_index + 1 &&
                       tx.vout[mltx_change_txout_index].nValue != 0 &&
            tx.vout[mltx_change_txout_index].scriptPubKey.size() > 0 &&
            tx.vout[mltx_change_txout_index].scriptPubKey[0] != OP_RETURN);

    if (has_change && !MoneyRange(tx.vout[mltx_change_txout_index].nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-change-amount");

    if (tx.vout.size() > (has_change ? mltx_change_txout_index + 1 : mltx_stake_txout_index + 1))
        return state.DoS(100, false, REJECT_INVALID, "bad-revoketicket-output-count-too-large");

    return true;
}

bool rvt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, const CChainParams& chain_params, const int spend_height, CValidationState &state)
{
    if (!rvt_check_inputs_nc(tx, state))
        return false;

    const auto& coin = inputs.AccessCoin(tx.vin[mltx_ticket_txin_index].prevout);

    if (coin.txType != MLTX_BuyTicket)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-input");

    if (!at_valid(coin.actor))
        return state.DoS(100, false, REJECT_INVALID, "bad-actor-for-revoketicket");

    const auto& consensus = chain_params.GetConsensus();
    if (spend_height - coin.nHeight < consensus.nMlTicketMaturity + consensus.nMlTicketExpiry)
        return state.DoS(100, false, REJECT_INVALID, "ticket-not-expired-yet");

    if (!mltx_is_legal_stake_txout(coin.out))
        return state.DoS(100, false, REJECT_INVALID, "illegal-stake-output");

    if (coin.IsSpent())
        return state.DoS(100, false, REJECT_INVALID, "ticket-stake-missingorspent");

    return true;
}

bool rvt_check_outputs(const CTransaction& tx, CValidationState &state)
{
    if (!rvt_check_outputs_nc(tx, state))
        return false;

    if (!rvt_check_inputs_nc(tx, state))
        return false;

    CTxDestination refund_destination;
    if (!ExtractDestination(tx.vout[mltx_refund_txout_index].scriptPubKey, refund_destination))
        return false;

    const auto ticket = GetMlTicket(tx.vin[mltx_ticket_txin_index].prevout.hash);
    if (ticket == nullptr)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-reference");

    CTxOut stake_txout, change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    ActorType actor;
    CTxDestination reward_address;
    std::string reason;
    if (!byt_parse_tx(*ticket, stake_txout, change_txout, script, items, version, actor, reward_address, reason))
        return state.DoS(100, false, REJECT_INVALID, reason);

    if (refund_destination.which() != reward_address.which())
        return state.DoS(100, false, REJECT_INVALID, "incorrect-refund-address-type");

    const auto& refund_addr = (refund_destination.which() == 1 ?
                                   boost::get<CKeyID>(refund_destination) :
                                   boost::get<CScriptID>(refund_destination));
    const auto& reward_addr = (reward_address.which() == 1 ?
                                   boost::get<CKeyID>(reward_address) :
                                   boost::get<CScriptID>(reward_address));
    if (refund_addr != reward_addr)
        return state.DoS(100, false, REJECT_INVALID, "incorrect-refund-address");

    if (tx.vout.size() > mltx_change_txout_index + 1) {
        CTxDestination change_destination;
        change_txout = tx.vout[mltx_change_txout_index];
        bool change_destination_ok = mltx_is_payment_txout(change_txout) && ExtractDestination(change_txout.scriptPubKey, change_destination) && IsValidDestination(change_destination);
        bool change_value_ok = change_txout.nValue != 0 && MoneyRange(change_txout.nValue);
        if (!change_destination_ok || !change_value_ok)
            return false;
    }
    return true;
}

RevokeTicketTx RevokeTicketTx::from_script(const CScript& script)
{
    RevokeTicketTx rtx;

    unsigned int version;
    std::string reason;
    if (!rvt_parse_script(script, version, reason))
        return rtx;

    rtx.set_version(version);

    return rtx;
}

RevokeTicketTx RevokeTicketTx::from_tx(const CTransaction& tx)
{
    RevokeTicketTx rtx;

    CTxIn ticket_txin;
    CTxOut refund_txout;
    CTxOut change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    std::string reason;

    if (!rvt_parse_tx(tx, ticket_txin, refund_txout, change_txout, script, items, version, reason))
        return rtx;

    if (!mltx_is_payment_txout(refund_txout))
        return rtx;

    rtx.set_version(version);

    rtx.set_ticket_txin(ticket_txin);

    rtx.set_change_txout(refund_txout);

    if (mltx_is_payment_txout(change_txout))
        rtx.set_change_txout(change_txout);

    return rtx;
}

const std::string RevokeTicketTx::name()
{
    return mltx_name(MLTX_RevokeTicket);
}

RevokeTicketTx::RevokeTicketTx()
    : _version(rvt_current_version),
      _refund_address(CNoDestination()), _refund_amount(0),
      _change_address(CNoDestination()), _change_amount(0),
      _dirty(true)
{
}

void RevokeTicketTx::set_version(const unsigned int version)
{
    _version = version;
    _dirty = true;
}

void RevokeTicketTx::set_ticket_txin(const CTxIn& txin)
{
    _ticket_txin = txin;
    _dirty = true;
}

void RevokeTicketTx::set_refund_txout(const CTxOut& txout)
{
    _refund_txout = txout;
    ExtractDestination(_refund_txout.scriptPubKey, _refund_address);
    _refund_amount = _refund_txout.nValue;
    _dirty = true;
}

void RevokeTicketTx::set_refund_address(const CTxDestination& address)
{
    _refund_txout.scriptPubKey = GetScriptForDestination(address);
    _refund_address = address;
    _dirty = true;
}

void RevokeTicketTx::set_refund_amount(const CAmount amount)
{
    _refund_txout.nValue = amount;
    _refund_amount = _refund_txout.nValue;
    _dirty = true;
}

void RevokeTicketTx::set_change_txout(const CTxOut& txout)
{
    _change_txout = txout;
    ExtractDestination(_change_txout.scriptPubKey, _change_address);
    _change_amount = _change_txout.nValue;
    _dirty = true;
}

void RevokeTicketTx::set_change_address(const CTxDestination& address)
{
    _change_txout.scriptPubKey = GetScriptForDestination(address);
    _change_address = address;
    _dirty = true;
}

void RevokeTicketTx::set_change_amount(const CAmount& amount)
{
    _change_txout.nValue = amount;
    _change_amount = _change_txout.nValue;
    _dirty = true;
}

bool RevokeTicketTx::valid()
{
    return regenerate_if_needed();
}

const CScript RevokeTicketTx::structured_data_script()
{
    if (!regenerate_if_needed())
        return CScript();

    return _script;
}

const std::vector<CTxIn> RevokeTicketTx::tx_inputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxIn>();

    return _tx.vin;
}

const std::vector<CTxOut> RevokeTicketTx::tx_outputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxOut>();

    return _tx.vout;
}

const CTransaction RevokeTicketTx::tx()
{
    if (!regenerate_if_needed())
        return CTransaction();

    return _tx;
}

bool RevokeTicketTx::regenerate_if_needed()
{
    if (!_dirty)
        return true;

    // inputs
    if (_ticket_txin.prevout.IsNull() ||
            _ticket_txin.prevout.n != mltx_stake_txout_index)
        return false;

    // script
    if (!rvt_script(_script, _version))
        return false;

    // outputs
    if (_refund_txout.IsNull() ||
            _refund_txout.nValue == 0 ||
            !MoneyRange(_refund_txout.nValue) ||
            !mltx_is_payment_txout(_refund_txout))
        return false;

    if (!_change_txout.IsNull() &&
            (_change_txout.nValue == 0 ||
             !MoneyRange(_change_txout.nValue) ||
             !mltx_is_payment_txout(_change_txout)))
        return false;

    // transaction
    // (assumes mltx_ticket_txin_index=0, mltx_refund_txout_index=1, mltx_change_txout_index=2, sds_first_output_index=0)
    _tx.vin.clear();
    _tx.vin.push_back(_ticket_txin);
    _tx.vout.clear();
    _tx.vout.push_back(CTxOut(0, _script));
    _tx.vout.push_back(_refund_txout);
    if (_change_txout.nValue != 0)
        _tx.vout.push_back(_change_txout);

    _dirty = false;

    return true;
}
