/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "pay_for_task_tx.h"

#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <ml/transactions/ml_tx_helpers.h>
#include <ml/transactions/ml_tx_size.h>
#include <ml/transactions/ml_tx_type.h>
#include <policy/feerate.h>

const unsigned int pft_current_version = 0;

bool pft_script(CScript& script,
                const nlohmann::json& task, const unsigned int version)
{
    if (version > pft_current_version)
        return false;

    if (!pft_task_valid(task))
        return false;

    std::vector<std::uint8_t> msg_pack = nlohmann::json::to_msgpack(task);
    script = sds_create(SDC_PoUW) << MLTX_PayForTask << version << msg_pack;

    return true;
}

bool pft_script(CScript& script,
                const std::string& task, const unsigned int version)
{
    auto j = nlohmann::json::parse(task);
    return pft_script(script, j, version);
}

bool pft_script_valid(const CScript& script, std::string& reason)
{
    return pft_script_valid(sds_script_items(script), reason);
}

bool pft_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason)
{
    unsigned int version;
    nlohmann::json task;
    return pft_parse_script(items, version, task, reason);
}

bool pft_parse_script(const CScript& script,
                      unsigned int& version, nlohmann::json& task,
                      std::string& reason)
{
    return pft_parse_script(sds_script_items(script), version, task, reason);
}

bool pft_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, nlohmann::json& task,
                      std::string& reason)
{
    if (items.size() < 5) {
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
    if (!mltx_valid(mltx_int) || static_cast<MLTxType>(mltx_int) != MLTX_PayForTask) {
        reason = "not-pft-tx";
        return false;
    }

    int version_int = CScriptNum(items[3], false).getint();
    if (version_int < 0 || version_int > static_cast<int>(pft_current_version)) {
        reason = "invalid-pft-version";
        return false;
    }

    try {
        task = nlohmann::json::from_msgpack(items[4]);
        version = static_cast<unsigned int>(version_int);
        return true;
    }  catch (...) {
    }

    reason = "invalid-task";
    return false;
}

bool pft_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin, std::vector<CTxIn>& extra_funding_txins,
                  CAmount& stake, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, nlohmann::json& task,
                  std::string& reason)
{
    // non-contextual validations

    CValidationState state;
    if (!pft_check_inputs_nc(tx, state) || !pft_check_outputs_nc(tx, state)) {
        reason = state.GetRejectReason();
        return false;
    }

    // parsed values

    ticket_txin = tx.vin[mltx_ticket_txin_index];

    // (assumes mltx_ticket_txin_index=0)
    extra_funding_txins.clear();
    for (size_t i = mltx_ticket_txin_index + 1; i < tx.vin.size(); ++i)
        extra_funding_txins.push_back(tx.vin[i]);

    stake = tx.vout[mltx_stake_txout_index].nValue;

    if (tx.vout.size() > mltx_change_txout_index && tx.vout[mltx_change_txout_index].nValue > 0)
        change_txout = tx.vout[mltx_change_txout_index];
    else
        change_txout = CTxOut();

    if (!sds_from_tx(tx, script, reason))
        return false;

    items = sds_script_items(script);
    if (!pft_parse_script(items, version, task, reason))
        return false;

    return true;
}

bool pft_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const std::vector<CTxIn> extra_funding_txins,
            const CAmount& stake, const CTxOut& change_txout,
            const nlohmann::json& task, const unsigned int version)
{
    // script

    CScript script;
    if (!pft_script(script, task, version))
        return false;

    auto script_txouts = sds_tx_outputs(script);
    if (script_txouts.size() == 0)
        return false;

    // tx construction
    // (assumes mltx_ticket_txin_index=0, mltx_stake_txout_index=1, mltx_change_txout_index=2, sds_first_output_index=0)

    tx.vin.clear();
    tx.vin.push_back(ticket_txin);
    tx.vin.insert(tx.vin.end(), extra_funding_txins.begin(), extra_funding_txins.end());
    tx.vout.clear();
    tx.vout.push_back(script_txouts[0]);
    tx.vout.push_back(CTxOut(stake, CScript()));
    if (!change_txout.IsNull())
        tx.vout.push_back(change_txout);
    for (uint32_t i = 1; i < script_txouts.size(); ++i)
        tx.vout.push_back(script_txouts[i]);

    // validations

    std::string reason;
    if (!pft_tx_valid(tx, reason)) {
        tx.vin.clear();
        tx.vout.clear();
        return false;
    }

    return true;
}

bool pft_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const std::vector<CTxIn> extra_funding_txins,
            const CAmount& stake,
            const CTxDestination& change_address, const CAmount& change,
            const nlohmann::json& task, const unsigned int version)
{
    return pft_tx(tx,
                  ticket_txin, extra_funding_txins,
                  stake,
                  CTxOut(change, GetScriptForDestination(change_address)),
                  task, version);
}

bool pft_is_stake_output(const Coin& coin, const uint32_t txout_index)
{
    return coin.txType == MLTX_PayForTask && txout_index == mltx_stake_txout_index;
}

bool pft_tx_valid(const CTransaction& tx, std::string& reason)
{
    CTxIn ticket_txin;
    std::vector<CTxIn> extra_funding_txins;
    CAmount stake;
    CTxOut change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    nlohmann::json task;

    if (!pft_parse_tx(tx, ticket_txin, extra_funding_txins, stake, change_txout, script, items, version, task, reason))
        return false;

    return true;
}

bool pft_task_valid(const nlohmann::json& task)
{
    // TODO: add all necessary validations here!
    return !task.empty();
}

bool pft_task_string(const nlohmann::json& task, std::string& str, const int indent)
{
    if (!pft_task_valid(task))
        return false;

    str = task.dump(indent);

    return true;
}

bool pft_task_json(const std::string& str, nlohmann::json& task)
{
    if (str.size() <= 0)
        return false;

    try {
        task = nlohmann::json::parse(str);
        return true;
    }  catch (...) {
    }

    return false;
}

CAmount pft_fee(const unsigned int extra_funding_count, const nlohmann::json& task, const CFeeRate& fee_rate)
{
    const auto size = pft_estimated_size(extra_funding_count, task, true, true);
    if (size <= 0)
        return 0;

    const auto fee = fee_rate.GetFee(size);
    if (fee <= 0)
        return 0;

    return fee;
}

bool pft_check_inputs_nc(const CTransaction& tx, CValidationState &state)
{
    return pft_check_inputs_nc(tx.vin, state);
}

bool pft_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state)
{
    if (txins.size() < mltx_ticket_txin_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-payfortask-input-count");

    if (txins[mltx_ticket_txin_index].prevout.n != mltx_stake_txout_index)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-reference");

    for (const auto& txin : txins)
        if (txin.prevout.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");

    return true;
}

bool pft_check_outputs_nc(const CTransaction& tx, CValidationState &state)
{
    return pft_check_outputs_nc(tx.vout, state);
}

bool pft_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state)
{
    if (txouts.size() < mltx_stake_txout_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-payfortask-output-count");

    if (!sds_is_first_output(txouts[sds_first_output_index]))
        return state.DoS(100, false, REJECT_INVALID, "invalid-sds-first-output");

    const auto& stake_txout = txouts[mltx_stake_txout_index];
    if (stake_txout.nValue == 0 || !MoneyRange(stake_txout.nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-amount");

    if (stake_txout.scriptPubKey.size() != 0)
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-address");

    bool has_change = false;
    if (txouts.size() >= mltx_change_txout_index + 1) {
        const auto& change_txout = txouts[mltx_change_txout_index];
        has_change = (change_txout.nValue > 0 &&
                change_txout.scriptPubKey.size() > 0 &&
                change_txout.scriptPubKey[0] != OP_RETURN);

        if (has_change) {
            if (!MoneyRange(change_txout.nValue))
                return state.DoS(100, false, REJECT_INVALID, "bad-change-amount");

            CTxDestination change_destination;
            if (!ExtractDestination(change_txout.scriptPubKey, change_destination) ||
                    !IsValidDestination(change_destination))
                return state.DoS(100, false, REJECT_INVALID, "bad-change-address");
        }
    }

    for (uint32_t i = (has_change ? mltx_change_txout_index + 1 : mltx_stake_txout_index + 1); i < txouts.size(); ++i)
        if (!sds_is_subsequent_output(txouts[i]))
            return state.DoS(100, false, REJECT_INVALID, "nonzero-sds-subsequent-output");

    std::string reason;
    CScript script;
    if (!sds_from_txouts(txouts, script, reason))
        return state.DoS(100, false, REJECT_INVALID, reason);

    if (!pft_script_valid(script, reason))
        return state.DoS(100, false, REJECT_INVALID, reason);

    return true;
}

bool pft_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, const CChainParams& chain_params, const int spend_height, CValidationState &state)
{
    if (!pft_check_inputs_nc(tx, state))
        return false;

    auto legal_input = [](const Coin& coin, const uint32_t index) -> bool {
        if (coin.IsCoinBase())
            return true;

        bool legal_coin_tx =
                coin.txType == MLTX_Regular ||
                (coin.txType == MLTX_BuyTicket && index == mltx_change_txout_index) ||
                (coin.txType == MLTX_RevokeTicket && index == mltx_refund_txout_index) ||
                (coin.txType == MLTX_PayForTask && index == mltx_change_txout_index);
        if (!legal_coin_tx)
            return false;

        CTxDestination destination;
        return ExtractDestination(coin.out.scriptPubKey, destination) || IsValidDestination(destination);
    };

    for (uint32_t i = 0; i < tx.vin.size(); ++i) {
        const auto& txin = tx.vin[i];
        const auto& coin = inputs.AccessCoin(txin.prevout);

        if (coin.IsSpent())
            return state.DoS(100, false, REJECT_INVALID, "bad-txin-missingorspent");

        if (i == mltx_ticket_txin_index) {
            if (coin.txType != MLTX_BuyTicket)
                return state.DoS(100, false, REJECT_INVALID, "bad-ticket-input");

            if (coin.actor != AT_Client)
                return state.DoS(100, false, REJECT_INVALID, "bad-actor-for-task-submission");

            if (spend_height - coin.nHeight < chain_params.GetConsensus().nMlTicketMaturity)
                return state.DoS(100, false, REJECT_INVALID, "immature-ticket");

            if (spend_height - coin.nHeight >= chain_params.GetConsensus().nMlTicketMaturity + chain_params.GetConsensus().nMlTicketExpiry)
                return state.DoS(100, false, REJECT_INVALID, "expired-ticket");

            if (!mltx_is_legal_stake_txout(coin.out))
                return state.DoS(100, false, REJECT_INVALID, "illegal-stake-output");
        } else {
            if (!legal_input(coin, txin.prevout.n))
                return state.DoS(100, false, REJECT_INVALID, "illegal-txin");
        }
    }

    return true;
}

PayForTaskTx PayForTaskTx::from_script(const CScript& script)
{
    PayForTaskTx ptx;

    unsigned int version;
    nlohmann::json task;
    std::string reason;
    if (!pft_parse_script(script, version, task, reason))
        return ptx;

    ptx.set_version(version);
    ptx.set_task(task);

    return ptx;
}

PayForTaskTx PayForTaskTx::from_tx(const CTransaction& tx)
{
    PayForTaskTx ptx;

    CTxIn ticket_txin;
    std::vector<CTxIn> extra_funding_txins;
    CAmount stake;
    CTxOut change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    nlohmann::json task;
    std::string reason;

    if (!pft_parse_tx(tx, ticket_txin, extra_funding_txins, stake, change_txout, script, items, version, task, reason))
        return ptx;

    ptx.set_version(version);
    ptx.set_task(task);

    ptx.set_ticket_txin(ticket_txin);
    ptx.set_extra_funding_txins(extra_funding_txins);

    ptx.set_stake_amount(stake);

    CTxDestination change_destination;
    if (change_txout.nValue > 0 &&
            MoneyRange(change_txout.nValue) &&
            ExtractDestination(change_txout.scriptPubKey, change_destination) &&
            IsValidDestination(change_destination))
        ptx.set_change_txout(change_txout);

    return ptx;
}

const std::string PayForTaskTx::name()
{
    return mltx_name(MLTX_PayForTask);
}

PayForTaskTx::PayForTaskTx()
    : _version(pft_current_version),
      _change_address(CNoDestination()), _change_amount(0),
      _dirty(true)
{
}

void PayForTaskTx::set_version(const unsigned int version)
{
    _version = version;
    _dirty = true;
}

void PayForTaskTx::set_task(const nlohmann::json& task)
{
    _task = task;
    _dirty = true;
}

void PayForTaskTx::set_task(const std::string& task)
{
    nlohmann::json j;
    if (!pft_task_json(task, j))
        set_task(nlohmann::json());
    set_task(j);
}

void PayForTaskTx::set_ticket_txin(const CTxIn& txin)
{
    _ticket_txin = txin;
    _dirty = true;
}

void PayForTaskTx::set_extra_funding_txins(const std::vector<CTxIn> txins)
{
    _extra_funding_txins.clear();
    _extra_funding_txins.insert(_extra_funding_txins.end(), txins.begin(), txins.end());
    _dirty = true;
}

void PayForTaskTx::set_stake_amount(const CAmount amount)
{
    _stake_txout.nValue = amount;
    _dirty = true;
}

void PayForTaskTx::set_stake_txout(const CTxOut& txout)
{
    _stake_txout = txout;
    _dirty = true;
}

void PayForTaskTx::set_change_txout(const CTxOut& txout)
{
    _change_txout = txout;
    ExtractDestination(_change_txout.scriptPubKey, _change_address);
    _change_amount = _change_txout.nValue;
    _dirty = true;
}

void PayForTaskTx::set_change_address(const CTxDestination& address)
{
    _change_txout.scriptPubKey = GetScriptForDestination(address);
    _change_address = address;
    _dirty = true;
}

void PayForTaskTx::set_change_amount(const CAmount& amount)
{
    _change_txout.nValue = amount;
    _change_amount = _change_txout.nValue;
    _dirty = true;
}


bool PayForTaskTx::valid()
{
    return regenerate_if_needed();
}

const CScript PayForTaskTx::structured_data_script()
{
    if (!regenerate_if_needed())
        return CScript();

    return _script;
}

const std::vector<CTxIn> PayForTaskTx::tx_inputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxIn>();

    return _tx.vin;
}

const std::vector<CTxOut> PayForTaskTx::tx_outputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxOut>();

    return _tx.vout;
}

const CTransaction PayForTaskTx::tx()
{
    if (!regenerate_if_needed())
        return CTransaction();

    return _tx;
}

bool PayForTaskTx::regenerate_if_needed()
{
    if (!_dirty)
        return true;

    // script

    if (!pft_script(_script, _task, _version))
        return false;

    auto script_txouts = sds_tx_outputs(_script);
    if (script_txouts.size() <= 0)
        return false;

    // transaction
    // (assumes mltx_ticket_txin_index=0, mltx_stake_txout_index=1, mltx_change_txout_index=2, sds_first_output_index=0)

    _tx.vin.clear();
    _tx.vin.push_back(_ticket_txin);
    _tx.vin.insert(_tx.vin.end(), _extra_funding_txins.begin(), _extra_funding_txins.end());
    _tx.vout.clear();
    _tx.vout.push_back(script_txouts[0]);
    _tx.vout.push_back(_stake_txout);
    if (_change_txout.nValue > 0)
        _tx.vout.push_back(_change_txout);
    for (size_t i = 1; i < script_txouts.size(); ++i)
        _tx.vout.push_back(script_txouts[i]);

    // validation

    CValidationState state;
    if (!pft_check_inputs_nc(_tx, state) || !pft_check_outputs_nc(_tx, state))
        return false;

    _dirty = false;

    return true;
}
