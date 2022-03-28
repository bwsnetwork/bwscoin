/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "buy_ticket_tx.h"

#include <coins.h>
#include <consensus/validation.h>
#include <ml/transactions/ml_tx_helpers.h>
#include <ml/transactions/ml_tx_size.h>
#include <ml/transactions/ml_tx_type.h>
#include <script/structured_data/structured_data.h>
#include <policy/feerate.h>

const unsigned int byt_current_version = 0;

bool byt_script(CScript& script,
                const ActorType& actor, const CTxDestination& reward_address, unsigned int version)
{
    if (version > byt_current_version)
        return false;

    if (!at_valid(actor))
        return false;

    if (reward_address.which() == 0)
        return false;

    int address_type = reward_address.which();
    uint160 address;
    if (address_type == 1)
        address = boost::get<const CKeyID>(reward_address);
    else if (address_type == 2)
        address = boost::get<const CScriptID>(reward_address);

    script = sds_create(SDC_PoUW) << MLTX_BuyTicket << version << actor << ToByteVector(address) << address_type;

    return true;
}

bool byt_script_valid(const CScript& script, std::string& reason)
{
    return byt_script_valid(sds_script_items(script), reason);
}

bool byt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason)
{
    unsigned int version;
    ActorType actor;
    CTxDestination reward_address;
    return byt_parse_script(items, version, actor, reward_address, reason);
}

bool byt_parse_script(const CScript& script,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      std::string& reason)
{
    return byt_parse_script(sds_script_items(script), version, actor, reward_address, reason);
}

bool byt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      std::string& reason)
{
    if (items.size() != 7) {
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
    if (!mltx_valid(mltx_int) || static_cast<MLTxType>(mltx_int) != MLTX_BuyTicket) {
        reason = "not-byt-tx";
        return false;
    }

    int version_int = CScriptNum(items[3], false).getint();
    if (version_int < 0 || version_int > static_cast<int>(byt_current_version)) {
        reason = "invalid-byt-version";
        return false;
    }

    int actor_int = CScriptNum(items[4], false).getint();
    if (!at_valid(actor_int)) {
        reason = "invalid-actor-type";
        return false;
    }

    uint160 address(items[5]);
    if (address.IsNull()) {
        reason = "invalid-reward-address";
        return false;
    }

    int address_type = CScriptNum(items[6], false).getint();
    if (address_type != 1 && address_type != 2) {
        reason = "invalid-reward-address-type";
        return false;
    }

    version = static_cast<unsigned int>(version_int);
    actor = static_cast<ActorType>(actor_int);
    if (address_type == 1)
        reward_address = CKeyID(address);
    else
        reward_address = CScriptID(address);

    return true;
}

bool byt_parse_tx(const CTransaction& tx,
                  CTxOut& stake_txout, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                  std::string& reason)
{
    // sizes
    if (tx.vin.size() < 1) {
        reason = "invalid-input-count";
        return false;
    }

    if (tx.vout.size() <= mltx_stake_txout_index || tx.vout.size() > mltx_change_txout_index + 1) {
        reason = "invalid-output-count";
        return false;
    }

    // inputs
    for (auto& txin: tx.vin)
        if (txin.prevout.hash.IsNull()) {
            reason = "null-input";
            return false;
        }

    // stake output
    CTxDestination stake_destination;
    stake_txout = tx.vout[mltx_stake_txout_index];
    if (stake_txout.nValue == 0 ||
            !MoneyRange(stake_txout.nValue) ||
            stake_txout.scriptPubKey.size() <= 0 ||
            !ExtractDestination(stake_txout.scriptPubKey, stake_destination) ||
            !IsValidDestination(stake_destination)) {
        reason = "invalid-stake-output";
        return false;
    }

    // change output (optional)
    CTxDestination change_destination;
    if (tx.vout.size() > mltx_change_txout_index)
    {
        change_txout = tx.vout[mltx_change_txout_index];
        bool change_destination_ok = ExtractDestination(change_txout.scriptPubKey, change_destination) && IsValidDestination(change_destination);
        bool change_value_ok = change_txout.nValue != 0 && MoneyRange(change_txout.nValue);
        if (change_destination_ok != change_value_ok) {
            reason = "invalid-change-count";
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
    if (!byt_parse_script(items, version, actor, reward_address, reason))
        return false;

    return true;
}

bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxOut& stake_txout, const CTxOut& change_txout,
            const ActorType& actor, const CTxDestination& reward_address, const unsigned int version)
{
    // sizes
    if (txins.size() <= 0)
        return false;

    // script values
    if (version > byt_current_version ||
            !at_valid(actor) ||
            !IsValidDestination(reward_address))
        return false;

    // inputs
    for (auto& txin: txins)
        if (txin.prevout.IsNull())
            return false;

    // stake
    if (stake_txout.nValue == 0 ||
            !MoneyRange(stake_txout.nValue) ||
            !mltx_is_payment_txout(stake_txout))
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
    if (!byt_script(script, actor, reward_address, version))
        return false;

    // tx construction
    // (assumes mltx_stake_txout_index=1, mltx_change_txout_index=2)
    tx.vin.clear();
    tx.vin.insert(tx.vin.end(), txins.begin(), txins.end());
    tx.vout.clear();
    tx.vout.push_back(CTxOut(0, script));
    tx.vout.push_back(stake_txout);
    if (has_change)
        tx.vout.push_back(change_txout);

    return true;
}

bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxDestination& stake_address, const CAmount& stake,
            const CTxDestination& change_address, const CAmount& change,
            const ActorType& actor, const CTxDestination& reward_address, const unsigned int version)
{
    return byt_tx(tx,
                  txins,
                  CTxOut(stake, GetScriptForDestination(stake_address)),
                  CTxOut(change, GetScriptForDestination(change_address)),
                  actor, reward_address, version);
}

CAmount byt_fee(const unsigned int txin_count, const CFeeRate& fee_rate)
{
    const auto size = byt_estimated_size(txin_count, true, true);
    if (size <= 0)
        return 0;

    const auto fee = fee_rate.GetFee(size);
    if (fee <= 0)
        return 0;

    return fee;
}

bool byt_check_inputs_nc(const CTransaction& tx, CValidationState &state)
{
    if (tx.vin.size() < 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-input-count");

    for (const auto& txin : tx.vin)
        if (txin.prevout.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");

    return true;
}

bool byt_check_outputs_nc(const CTransaction& tx, CValidationState &state)
{
    if (tx.vout.size() < mltx_stake_txout_index + 1)
        return state.DoS(100, false, REJECT_INVALID, "bad-ticket-output-count");

    if (!sds_is_first_output(tx.vout[sds_first_output_index]))
        return state.DoS(100, false, REJECT_INVALID, "invalid-sds-first-output");

    if (tx.vout[mltx_stake_txout_index].nValue == 0 || !MoneyRange(tx.vout[mltx_stake_txout_index].nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-amount");

    if (tx.vout[mltx_stake_txout_index].scriptPubKey.size() <= 0 ||
            tx.vout[mltx_stake_txout_index].scriptPubKey[0] == OP_RETURN)
        return state.DoS(100, false, REJECT_INVALID, "bad-stake-address");

    if (!mltx_is_legal_stake_txout(tx.vout[mltx_stake_txout_index]))
        return state.DoS(100, false, REJECT_INVALID, "illegal-stake-output");

    bool has_change = (tx.vout.size() >= mltx_change_txout_index + 1 &&
                       tx.vout[mltx_change_txout_index].nValue != 0 &&
            tx.vout[mltx_change_txout_index].scriptPubKey.size() > 0 &&
            tx.vout[mltx_change_txout_index].scriptPubKey[0] != OP_RETURN);

    if (has_change && !MoneyRange(tx.vout[mltx_stake_txout_index].nValue))
        return state.DoS(100, false, REJECT_INVALID, "bad-change-amount");

    for (uint32_t i = (has_change ? mltx_change_txout_index + 1 : mltx_stake_txout_index + 1); i < tx.vout.size(); ++i)
        if (!sds_is_subsequent_output(tx.vout[i]))
            return state.DoS(100, false, REJECT_INVALID, "nonzero-sds-subsequent-output");

    return true;
}

bool byt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, CValidationState &state)
{
    if (!byt_check_inputs_nc(tx, state))
        return false;

    auto legal_input = [](const Coin& coin, const uint32_t index) -> bool {
        if (coin.IsCoinBase())
            return true;

        bool legal_coin_tx =
                coin.txType == MLTX_Regular ||
                (coin.txType == MLTX_BuyTicket && index == mltx_change_txout_index) ||
                (coin.txType == MLTX_PayForTask && index == mltx_change_txout_index);
        if (!legal_coin_tx)
            return false;

        return mltx_is_payment_txout(coin.out);
    };

    for (uint32_t i = 0; i < tx.vin.size(); ++i) {
        const auto& txin = tx.vin[i];
        const auto& coin = inputs.AccessCoin(txin.prevout);

        if (coin.IsSpent())
            return state.DoS(100, false, REJECT_INVALID, "bad-txin-missingorspent");

        if (!legal_input(coin, txin.prevout.n))
            return state.DoS(100, false, REJECT_INVALID, "illegal-txin");
    }

    return true;
}

BuyTicketTx BuyTicketTx::from_script(const CScript& script)
{
    BuyTicketTx btx;

    unsigned int version;
    ActorType actor;
    CTxDestination reward_address;
    std::string reason;
    if (!byt_parse_script(script, version, actor, reward_address, reason))
        return btx;

    btx.set_version(version);
    btx.set_actor(actor);
    btx.set_reward_address(reward_address);

    return btx;
}

BuyTicketTx BuyTicketTx::from_tx(const CTransaction& tx)
{
    BuyTicketTx btx;

    CTxOut stake_txout, change_txout;
    CScript script;
    std::vector<std::vector<unsigned char>> items;
    unsigned int version;
    ActorType actor;
    CTxDestination reward_address;
    std::string reason;

    if (!byt_parse_tx(tx, stake_txout, change_txout, script, items, version, actor, reward_address, reason))
        return btx;

    btx.set_version(version);
    btx.set_actor(actor);
    btx.set_reward_address(reward_address);

    btx.set_funding_txins(tx.vin);

    btx.set_stake_txout(stake_txout);
    if (mltx_is_payment_txout(change_txout))
        btx.set_change_txout(change_txout);

    return btx;
}

const std::string BuyTicketTx::name()
{
    return mltx_name(MLTX_BuyTicket);
}

BuyTicketTx::BuyTicketTx()
    : _version(byt_current_version), _actor(AT_Client), _reward_address(CNoDestination()),
      _stake_address(CNoDestination()), _stake_amount(0),
      _change_address(CNoDestination()), _change_amount(0),
      _dirty(true)
{
}

void BuyTicketTx::set_version(const unsigned int version)
{
    _version = version;
    _dirty = true;
}

void BuyTicketTx::set_actor(const ActorType& actor)
{
    _actor = actor;
    _dirty = true;
}

void BuyTicketTx::set_reward_address(const CTxDestination& address)
{
    _reward_address = address;
    _dirty = true;
}

void BuyTicketTx::set_funding_txins(const std::vector<CTxIn>& txins)
{
    _tx.vin.clear();
    _tx.vin.insert(_tx.vin.end(), txins.begin(), txins.end());
    _dirty = true;
}

void BuyTicketTx::set_stake_txout(const CTxOut& txout)
{
    _stake_txout = txout;
    ExtractDestination(_stake_txout.scriptPubKey, _stake_address);
    _stake_amount = _stake_txout.nValue;
    _dirty = true;
}

void BuyTicketTx::set_stake_address(const CTxDestination& address)
{
    _stake_txout.scriptPubKey = GetScriptForDestination(address);
    _stake_address = address;
    _dirty = true;
}

void BuyTicketTx::set_stake_amount(const CAmount& amount)
{
    _stake_txout.nValue = amount;
    _stake_amount = _stake_txout.nValue;
    _dirty = true;
}

void BuyTicketTx::set_change_txout(const CTxOut& txout)
{
    _change_txout = txout;
    ExtractDestination(_change_txout.scriptPubKey, _change_address);
    _change_amount = _change_txout.nValue;
    _dirty = true;
}

void BuyTicketTx::set_change_address(const CTxDestination& address)
{
    _change_txout.scriptPubKey = GetScriptForDestination(address);
    _change_address = address;
    _dirty = true;
}

void BuyTicketTx::set_change_amount(const CAmount& amount)
{
    _change_txout.nValue = amount;
    _change_amount = _change_txout.nValue;
    _dirty = true;
}

bool BuyTicketTx::valid()
{
    return regenerate_if_needed();
}

const CScript BuyTicketTx::structured_data_script()
{
    if (!regenerate_if_needed())
        return CScript();

    return _script;
}

const std::vector<CTxOut> BuyTicketTx::tx_outputs()
{
    if (!regenerate_if_needed())
        return std::vector<CTxOut>();

    return _tx.vout;
}

const CTransaction BuyTicketTx::tx()
{
    if (!regenerate_if_needed())
        return CTransaction();

    return _tx;
}

bool BuyTicketTx::regenerate_if_needed()
{
    if (!_dirty)
        return true;

    // inputs
    if (_tx.vin.size() <= 0)
        return false;

    for (auto& txin: _tx.vin)
        if (txin.prevout.IsNull())
            return false;

    // script
    if (!byt_script(_script, _actor, _reward_address, _version))
        return false;

    // outputs
    if (_stake_txout.nValue == 0 ||
            !MoneyRange(_stake_txout.nValue) ||
            !mltx_is_payment_txout(_stake_txout))
        return false;

    if (!_change_txout.IsNull() &&
            (_change_txout.nValue == 0 ||
             !MoneyRange(_change_txout.nValue) ||
             !mltx_is_payment_txout(_change_txout)))
        return false;

    // transaction
    // (assumes mltx_stake_txout_index=1, mltx_change_txout_index=2)
    _tx.vout.clear();
    _tx.vout.push_back(CTxOut(0, _script));
    _tx.vout.push_back(_stake_txout);
    if (_change_txout.nValue != 0)
        _tx.vout.push_back(_change_txout);

    _dirty = false;

    return true;
}
