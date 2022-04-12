/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// A Pay for Task (PfT) transaction is sent by the
// client revealing the definition of the ML task
// to be executed, as well as the funds it allows
// to be rewarded to different actors in the process.

#ifndef BWSCOIN_PAY_FOR_TASK_TX_H
#define BWSCOIN_PAY_FOR_TASK_TX_H

#include <amount.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <script/structured_data/structured_data.h>
#include <json/nlohmann/json.hpp>

class CChainParams;
class CCoinsViewCache;
class Coin;
class CFeeRate;
class CValidationState;

extern const unsigned int pft_current_version;   // should be monotonic

// The follwing functions are useful for processing PfT transactions.
// They are intended to be very specific and lighter than the related
// PayForTaskTx class. However, for complete encapsulation of features
// and data, use the class.

// Script

// structured script
// (use the output script only if function returns true)
bool pft_script(CScript& script,
                const nlohmann::json& task, const unsigned int version = pft_current_version);
bool pft_script(CScript& script,
                const std::string& task, const unsigned int version = pft_current_version);

// validate the script
// (validation is based on parsing)
bool pft_script_valid(const CScript& script, std::string& reason);
bool pft_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason);

// parse and validate the data
// (validation is based on parsing)
bool pft_parse_script(const CScript& script,
                      unsigned int& version, nlohmann::json& task,
                      std::string& reason);
bool pft_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, nlohmann::json& task,
                      std::string& reason);

// Task

// the maximum size of the task script
uint32_t pft_max_task_size();

// validate the task
bool pft_task_valid(const nlohmann::json& task);

// task serialization
// (use the output tx only if the function returns true)
bool pft_task_string(const nlohmann::json& task, std::string& str, const int indent = -1);
bool pft_task_json(const std::string& str, nlohmann::json& task);

// Transaction

// parse and validate the transaction
// (use the output values only if the function returns true)
bool pft_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin, std::vector<CTxIn>& extra_funding_txins,
                  CAmount& stake, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, nlohmann::json& task,
                  std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool pft_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const std::vector<CTxIn> extra_funding_txins,
            const CAmount& stake, const CTxOut& change_txout,
            const nlohmann::json& task, const unsigned int version = pft_current_version);
bool pft_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const std::vector<CTxIn> extra_funding_txins,
            const CAmount& stake,
            const CTxDestination& change_address, const CAmount& change,
            const nlohmann::json& task, const unsigned int version = pft_current_version);

// validate the transaction
bool pft_tx_valid(const CTransaction& tx, std::string& reason);

// Inputs and outputs

// verify if certain elements can belong to PfT transactions
bool pft_is_stake_output(const Coin& coin, const uint32_t txout_index);

// non-contextual input and output tests
bool pft_check_inputs_nc(const CTransaction& tx, CValidationState &state);
bool pft_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state);
bool pft_check_outputs_nc(const CTransaction& tx, CValidationState &state);
bool pft_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state);

// contextual input tests
// (these also perform the non-contextual verifications)
bool pft_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, const CChainParams& chain_params, const int spend_height, CValidationState &state);

// Other

// calculate the fee for the transaction (assumes that
// the change output is also present)
CAmount pft_fee(const unsigned int extra_funding_count, const nlohmann::json& task, const CFeeRate& fee_rate);

// Wrapper class for Pay for Task transactions

class PayForTaskTx
{
public:
    static PayForTaskTx from_script(const CScript& script);
    static PayForTaskTx from_tx(const CTransaction& tx);

    static const std::string name();

public:
    PayForTaskTx();

    unsigned int version() const { return _version; }
    void set_version(const unsigned int version);

    const nlohmann::json task() const { return _task; }
    void set_task(const nlohmann::json& task);
    void set_task(const std::string& task);

    const CTxIn ticket_txin() const { return _ticket_txin; }
    void set_ticket_txin(const CTxIn& txin);

    const std::vector<CTxIn> extra_funding_txins() const { return _extra_funding_txins; }
    void set_extra_funding_txins(const std::vector<CTxIn> txins);

    CAmount stake_amount() const { return _stake_txout.nValue; }
    void set_stake_amount(const CAmount amount);
    const CTxOut stake_txout() const { return _stake_txout; }
    void set_stake_txout(const CTxOut& txout);

    const CTxOut change_txout() const { return _change_txout; }
    void set_change_txout(const CTxOut& txout);
    const CTxDestination change_address() const { return _change_address; }
    void set_change_address(const CTxDestination& address);
    CAmount change_amount() const { return _change_amount; }
    void set_change_amount(const CAmount& amount);

    bool valid();

    const CScript structured_data_script();

    const std::vector<CTxIn> tx_inputs();
    const std::vector<CTxOut> tx_outputs();

    const CTransaction tx();

private:
    bool regenerate_if_needed();

private:
    unsigned int _version;
    nlohmann::json _task;

    CTxIn _ticket_txin;
    std::vector<CTxIn> _extra_funding_txins;

    CTxOut _stake_txout;

    CTxDestination _change_address;
    CAmount _change_amount;
    CTxOut _change_txout;

    bool _dirty;

    CScript _script;
    CMutableTransaction _tx;
};

#endif // BWSCOIN_PAY_FOR_TASK_TX_H
