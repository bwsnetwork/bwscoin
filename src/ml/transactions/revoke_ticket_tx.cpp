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
                  CTxOut& refund_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, std::string& reason)
{
    // non-contextual validations

    CValidationState state;
    if (!rvt_check_inputs_nc(tx, state) || !rvt_check_outputs_nc(tx, state)) {
        reason = state.GetRejectReason();
        return false;
    }

    // parsed values

    ticket_txin = tx.vin[mltx_ticket_txin_index];

    refund_txout = tx.vout[mltx_refund_txout_index];

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
            const CTxOut& refund_txout,
            const unsigned int version)
{
    // script

    CScript script;
    if (!rvt_script(script, version))
        return false;

    auto script_txouts = sds_tx_outputs(script);
    if (script_txouts.size() != 1)
        return false;

    // tx construction
    // (assumes mltx_ticket_txin_index=0, mltx_refund_txout_index=1, sds_first_output_index=0)

    tx.vin.clear();
    tx.vin.push_back(ticket_txin);
    tx.vout.clear();
    tx.vout.push_back(script_txouts[0]);
    tx.vout.push_back(refund_txout);

    // validations

    std::string reason;
    if (!rvt_tx_valid(tx, reason)) {
        tx.vin.clear();
        tx.vout.clear();
        return false;
    }

    return true;
}

bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& refund_address, const CAmount& refund,
            const unsigned int version)
{
    return rvt_tx(tx,
                  ticket_txin,
                  CTxOut(refund, GetScriptForDestination(refund_address)),
                  version);
}

bool rvt_tx_valid(const CTransaction& tx, std::string& reason)
{
    CTxIn ticket_txin;
    CTxOut refund_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;

    if (!rvt_parse_tx(tx, ticket_txin, refund_txout, script, items, version, reason))
        return false;

    return true;
}

bool rvt_check_inputs_nc(const CTransaction& tx, CValidationState &state)
{
    return rvt_check_inputs_nc(tx.vin, state);
}

bool rvt_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state)
{
    if (txins.size() != mltx_ticket_txin_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-revoketicket-input-count");

    const auto& ticket = txins[mltx_ticket_txin_index];
    if (ticket.prevout.n != mltx_stake_txout_index)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-reference");

    if (ticket.prevout.IsNull())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");

    return true;
}

bool rvt_check_outputs_nc(const CTransaction& tx, CValidationState &state)
{
    return rvt_check_outputs_nc(tx.vout, state);
}

bool rvt_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state)
{
    if (txouts.size() != mltx_refund_txout_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-revoketicket-output-count");

    std::string reason;
    if (!rvt_script_valid(txouts[sds_first_output_index].scriptPubKey, reason))
        return state.DoS(100, false, REJECT_INVALID, reason);

    const auto& refund_txout = txouts[mltx_refund_txout_index];
    if (refund_txout.nValue == 0 || !MoneyRange(refund_txout.nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-refund-amount");

    if (!mltx_is_payment_txout(refund_txout))
        return state.DoS(100, false, REJECT_INVALID, "bad-refund-address");

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
    if (!rvt_check_inputs_nc(tx, state))
        return false;

    if (!rvt_check_outputs_nc(tx, state))
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
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    std::string reason;

    if (!rvt_parse_tx(tx, ticket_txin, refund_txout, script, items, version, reason))
        return rtx;

    if (!mltx_is_payment_txout(refund_txout))
        return rtx;

    rtx.set_version(version);

    rtx.set_ticket_txin(ticket_txin);

    return rtx;
}

const std::string RevokeTicketTx::name()
{
    return mltx_name(MLTX_RevokeTicket);
}

RevokeTicketTx::RevokeTicketTx()
    : _version(rvt_current_version),
      _refund_address(CNoDestination()), _refund_amount(0),
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

    // script

    if (!rvt_script(_script, _version))
        return false;

    // transaction
    // (assumes mltx_ticket_txin_index=0, mltx_refund_txout_index=1, sds_first_output_index=0)

    _tx.vin.clear();
    _tx.vin.push_back(_ticket_txin);
    _tx.vout.clear();
    _tx.vout.push_back(CTxOut(0, _script));
    _tx.vout.push_back(_refund_txout);

    // validation

    CValidationState state;
    if (!rvt_check_inputs_nc(_tx, state) || !rvt_check_outputs_nc(_tx, state))
        return false;

    _dirty = false;

    return true;
}
