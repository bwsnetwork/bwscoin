/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "join_task_tx.h"

#include <coins.h>
#include <validation.h>
#include <consensus/validation.h>
#include <script/structured_data/structured_data.h>
#include <ml/transactions/buy_ticket_tx.h>
#include <ml/transactions/ml_tx_helpers.h>
#include <ml/transactions/ml_tx_size.h>
#include <ml/transactions/ml_tx_type.h>
#include <policy/feerate.h>

const unsigned int jnt_current_version = 0;

bool jnt_script(CScript& script, const uint256& task_id, const unsigned int version)
{
    if (version > jnt_current_version)
        return false;

    if (task_id.IsNull())
        return false;

    script = sds_create(SDC_PoUW) << MLTX_JoinTask << version << ToByteVector(task_id);

    return true;
}

bool jnt_script_valid(const CScript& script, std::string& reason)
{
    return jnt_script_valid(sds_script_items(script), reason);
}

bool jnt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason)
{
    unsigned int version;
    uint256 task_id;
    return jnt_parse_script(items, version, task_id, reason);
}

bool jnt_parse_script(const CScript& script,
                      unsigned int& version, uint256& task_id, std::string& reason)
{
    return jnt_parse_script(sds_script_items(script), version, task_id, reason);
}

bool jnt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, uint256& task_id, std::string& reason)
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
    if (!mltx_valid(mltx_int) || static_cast<MLTxType>(mltx_int) != MLTX_JoinTask) {
        reason = "not-jointask-tx";
        return false;
    }

    int version_int = CScriptNum(items[3], false).getint();
    if (version_int < 0 || version_int > static_cast<int>(jnt_current_version)) {
        reason = "invalid-jointask-version";
        return false;
    }
    version = static_cast<unsigned int>(version_int);

    task_id = uint256(items[4]);
    if (task_id.IsNull()) {
        reason = "invalid-task-id";
        return false;
    }

    return true;
}

bool jnt_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin,
                  CTxOut& stake_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  uint256& task_id, unsigned int& version, std::string& reason)
{
    // non-contextual validations

    CValidationState state;
    if (!jnt_check_inputs_nc(tx, state) || !jnt_check_outputs_nc(tx, state)) {
        reason = state.GetRejectReason();
        return false;
    }

    // parsed values

    ticket_txin = tx.vin[mltx_ticket_txin_index];

    stake_txout = tx.vout[mltx_stake_txout_index];

    if (!sds_from_tx(tx, script, reason))
        return false;

    items = sds_script_items(script);
    if (!jnt_parse_script(items, version, task_id, reason))
        return false;

    return true;
}

bool jnt_tx(CMutableTransaction& tx,
            const CTransaction& ticket,
            const CTxDestination& stake_address,
            const CFeeRate& fee_rate,
            const uint256& task_id, const unsigned int version)
{
    CAmount stake;
    if (!jnt_stake_amount(ticket, fee_rate, stake))
        return false;

    return jnt_tx(tx,
                  CTxIn(ticket.GetHash(), mltx_stake_txout_index),
                  CTxOut(stake, GetScriptForDestination(stake_address)),
                  task_id, version);
}

bool jnt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxOut& stake_txout,
            const uint256& task_id, const unsigned int version)
{
    // script

    CScript script;
    if (!jnt_script(script, task_id, version))
        return false;

    auto script_txouts = sds_tx_outputs(script);
    if (script_txouts.size() != 1)
        return false;

    // tx construction
    // (assumes mltx_ticket_txin_index=0, mltx_stake_txout_index=1, sds_first_output_index=0)

    tx.vin.clear();
    tx.vin.push_back(ticket_txin);
    tx.vout.clear();
    tx.vout.push_back(script_txouts[0]);
    tx.vout.push_back(stake_txout);

    // validations

    std::string reason;
    if (!jnt_tx_valid(tx, reason)) {
        tx.vin.clear();
        tx.vout.clear();
        return false;
    }

    return true;
}

bool jnt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& stake_address, const CAmount& stake,
            const uint256& task_id, const unsigned int version)
{
    return jnt_tx(tx,
                  ticket_txin,
                  CTxOut(stake, GetScriptForDestination(stake_address)),
                  task_id, version);
}

bool jnt_stake_amount(const CTransaction& ticket, const CFeeRate& fee_rate, CAmount& stake)
{
    CTxOut stake_txout, change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    ActorType actor;
    CTxDestination reward_address;
    std::string reason;
    if (!byt_parse_tx(ticket, stake_txout, change_txout, script, items, version, actor, reward_address, reason))
        return false;

    stake = stake_txout.nValue - jnt_fee(fee_rate);

    return true;
}

CAmount jnt_fee(const CFeeRate& fee_rate)
{
    const auto& size = jnt_estimated_size(true);
    if (size <= 0)
        return 0;

    const auto& fee = fee_rate.GetFee(size);
    if (fee <= 0)
        return 0;

    return fee;
}

bool jnt_is_stake_output(const Coin& coin, const uint32_t txout_index)
{
    return coin.txType == MLTX_JoinTask && txout_index == mltx_stake_txout_index;
}

bool jnt_tx_valid(const CTransaction& tx, std::string& reason)
{
    CTxIn ticket_txin;
    CTxOut stake_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    uint256 task_id;

    if (!jnt_parse_tx(tx, ticket_txin, stake_txout, script, items, task_id, version, reason))
        return false;

    return true;
}

bool jnt_check_inputs_nc(const CTransaction& tx, CValidationState &state)
{
    return jnt_check_inputs_nc(tx.vin, state);
}

bool jnt_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state)
{
    if (txins.size() != mltx_ticket_txin_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-jointask-input-count");

    const auto& ticket = txins[mltx_ticket_txin_index];
    if (ticket.prevout.n != mltx_stake_txout_index)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-reference");

    if (ticket.prevout.IsNull())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");

    return true;
}

bool jnt_check_outputs_nc(const CTransaction& tx, CValidationState &state)
{
    return jnt_check_outputs_nc(tx.vout, state);
}

bool jnt_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state)
{
    if (txouts.size() != mltx_stake_txout_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-jointask-output-count");

    std::string reason;
    if (!jnt_script_valid(txouts[sds_first_output_index].scriptPubKey, reason))
        return state.DoS(100, false, REJECT_INVALID, reason);

    const auto& stake_txout = txouts[mltx_stake_txout_index];
    if (stake_txout.nValue == 0 || !MoneyRange(stake_txout.nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-amount");

    if (!mltx_is_legal_stake_txout(stake_txout))
        return state.DoS(100, false, REJECT_INVALID, "illegal-stake-output");

    return true;
}

bool jnt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, CValidationState &state)
{
    if (!jnt_check_inputs_nc(tx, state))
        return false;

    const auto& coin = inputs.AccessCoin(tx.vin[mltx_ticket_txin_index].prevout);

    if (coin.txType != MLTX_BuyTicket)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-input");

    if (coin.actor != AT_Miner)
        return state.DoS(100, false, REJECT_INVALID, "bad-actor-for-jointask");

    if (!mltx_is_legal_stake_txout(coin.out))
        return state.DoS(100, false, REJECT_INVALID, "illegal-stake-output");

    if (coin.IsSpent())
        return state.DoS(100, false, REJECT_INVALID, "ticket-stake-missingorspent");

    return true;
}

JoinTaskTx JoinTaskTx::from_script(const CScript& script)
{
    JoinTaskTx jtx;

    unsigned int version;
    uint256 task_id;
    std::string reason;
    if (!jnt_parse_script(script, version, task_id, reason))
        return jtx;

    jtx.set_version(version);
    jtx.set_task_id(task_id);

    return jtx;
}

JoinTaskTx JoinTaskTx::from_tx(const CTransaction& tx)
{
    JoinTaskTx jtx;

    CTxIn ticket_txin;
    CTxOut stake_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    uint256 task_id;
    std::string reason;

    if (!jnt_parse_tx(tx, ticket_txin, stake_txout, script, items, task_id, version, reason))
        return jtx;

    if (!mltx_is_legal_stake_txout(stake_txout))
        return jtx;

    jtx.set_version(version);
    jtx.set_task_id(task_id);

    jtx.set_ticket_txin(ticket_txin);

    jtx.set_stake_txout(stake_txout);

    return jtx;
}

const std::string JoinTaskTx::name()
{
    return mltx_name(MLTX_JoinTask);
}

JoinTaskTx::JoinTaskTx()
    : _version(jnt_current_version),
      _stake_address(CNoDestination()), _stake_amount(0),
      _dirty(true)
{
}

void JoinTaskTx::set_version(const unsigned int version)
{
    _version = version;
    _dirty = true;
}

void JoinTaskTx::set_task_id(const uint256 task_id)
{
    _task_id = task_id;
    _dirty = true;
}

void JoinTaskTx::set_ticket_txin(const CTxIn& txin)
{
    _ticket_txin = txin;
    _dirty = true;
}

void JoinTaskTx::set_stake_txout(const CTxOut& txout)
{
    _stake_txout = txout;
    ExtractDestination(_stake_txout.scriptPubKey, _stake_address);
    _stake_amount = _stake_txout.nValue;
    _dirty = true;
}

void JoinTaskTx::set_stake_address(const CTxDestination& address)
{
    _stake_txout.scriptPubKey = GetScriptForDestination(address);
    _stake_address = address;
    _dirty = true;
}

void JoinTaskTx::set_stake_amount(const CAmount amount)
{
    _stake_txout.nValue = amount;
    _stake_amount = _stake_txout.nValue;
    _dirty = true;
}

bool JoinTaskTx::valid()
{
    return regenerate_if_needed();
}

const CScript JoinTaskTx::structured_data_script()
{
    if (!regenerate_if_needed())
        return CScript();

    return _script;
}

const std::vector<CTxIn> JoinTaskTx::tx_inputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxIn>();

    return _tx.vin;
}

const std::vector<CTxOut> JoinTaskTx::tx_outputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxOut>();

    return _tx.vout;
}

const CTransaction JoinTaskTx::tx()
{
    if (!regenerate_if_needed())
        return CTransaction();

    return _tx;
}

bool JoinTaskTx::regenerate_if_needed()
{
    if (!_dirty)
        return true;

    // script

    if (!jnt_script(_script, _task_id, _version))
        return false;

    // transaction
    // (assumes mltx_ticket_txin_index=0, mltx_stake_txout_index=1, sds_first_output_index=0)

    _tx.vin.clear();
    _tx.vin.push_back(_ticket_txin);
    _tx.vout.clear();
    _tx.vout.push_back(CTxOut(0, _script));
    _tx.vout.push_back(_stake_txout);

    // validation

    CValidationState state;
    if (!jnt_check_inputs_nc(_tx, state) || !jnt_check_outputs_nc(_tx, state))
        return false;

    _dirty = false;

    return true;
}
