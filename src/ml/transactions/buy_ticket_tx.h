/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// A Buy Ticket (ByT) transaction is sent by the
// actor to prove its intent and stake the required
// funds. The actors can be clients, miners, supervisors,
// etc. basically each type of participant in the
// machine learning

#ifndef BWSCOIN_BUY_TICKET_TX_H
#define BWSCOIN_BUY_TICKET_TX_H

#include <ml/transactions/actor_type.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <json/nlohmann/json.hpp>

class CCoinsViewCache;
class CChainParams;
class Coin;
class CFeeRate;
class CValidationState;

extern const unsigned int byt_current_version;   // should be monotonic

// The follwing functions are useful for processing ByT transactions.
// They are intended to be very specific and lighter than the related
// BuyTicketTx class. However, for complete encapsulation of features
// and data, use the class.

// Script

// ticket script
// (use the output script only if function returns true)
bool byt_script(CScript& script,
                const ActorType& actor, const CTxDestination& reward_address,
                const nlohmann::json& payload, const unsigned int version = byt_current_version);
bool byt_script(CScript& script,
                const ActorType& actor, const CTxDestination& reward_address,
                const std::string& payload, const unsigned int version = byt_current_version);
bool byt_script(CScript& script,
                const ActorType& actor, const CTxDestination& reward_address,
                const std::vector<unsigned char>& payload, const unsigned int version = byt_current_version);

// validate the script
// (validation is based on parsing)
bool byt_script_valid(const CScript& script, std::string& reason);
bool byt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason);

// parse and validate the data
// (use the output values only if the function returns true)
bool byt_parse_script(const CScript& script,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      nlohmann::json& payload, std::string& reason);
bool byt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      nlohmann::json& payload, std::string& reason);

// Payload

// the maximum size of the payload script
uint32_t byt_max_payload_size();

// validate the payload
bool byt_payload_valid(const nlohmann::json& payload);

// payload serialization
// (use the output tx only if the function returns true)
bool byt_payload_string(const nlohmann::json& payload, std::string& str, const int indent = -1);
bool byt_payload_json(const std::string& str, nlohmann::json& payload);

// Transaction

// parse and validate the transaction
// (use the output values only if the function returns true)
bool byt_parse_tx(const CTransaction& tx,
                  CTxOut& stake_txout, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                  nlohmann::json& payload, std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxOut& stake_txout, const CTxOut& change_txout,
            const ActorType& actor, const CTxDestination& reward_address,
            nlohmann::json& payload, const unsigned int version = byt_current_version);
bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxDestination& stake_address, const CAmount& stake,
            const CTxDestination& change_address, const CAmount& change,
            const ActorType& actor, const CTxDestination& reward_address,
            nlohmann::json& payload, const unsigned int version = byt_current_version);

// validate the transaction
bool byt_tx_valid(const CTransaction& tx, std::string& reason);

// Inputs and outputs

// verify if certain elements can belong to ByT transactions
bool byt_is_stake_output(const Coin& coin, const uint32_t txout_index);

// non-contextual input and output tests
bool byt_check_inputs_nc(const CTransaction& tx, CValidationState &state);
bool byt_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state);
bool byt_check_outputs_nc(const CTransaction& tx, CValidationState &state);
bool byt_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state);

// contextual input and output tests
bool byt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, CValidationState &state);

// Other

// estimate the fee for the transaction (assumes that
// the change output is also present)
CAmount byt_fee(const unsigned int txin_count, const CFeeRate& fee_rate);


// Wrapper class for Buy Ticket transactions

class BuyTicketTx
{
public:
    static BuyTicketTx from_script(const CScript& script);
    static BuyTicketTx from_tx(const CTransaction& tx);

    static const std::string name();

public:
    BuyTicketTx();

    unsigned int version() const { return _version; }
    void set_version(const unsigned int version);

    ActorType actor() const { return _actor; }
    void set_actor(const ActorType& actor);

    const CTxDestination reward_address() const { return _reward_address; }
    void set_reward_address(const CTxDestination& address);

    const nlohmann::json payload() const { return _payload; }
    void set_payload(const nlohmann::json& payload);
    void set_payload(const std::string& payload);

    const std::vector<CTxIn> funding_txins() const { return _tx.vin; }
    void set_funding_txins(const std::vector<CTxIn>& txins);

    const CTxOut stake_txout() const { return _stake_txout; }
    void set_stake_txout(const CTxOut& txout);
    const CTxDestination stake_address() const { return _stake_address; }
    void set_stake_address(const CTxDestination& address);
    CAmount stake_amount() const { return _stake_amount; }
    void set_stake_amount(const CAmount& amount);

    const CTxOut change_txout() const { return _change_txout; }
    void set_change_txout(const CTxOut& txout);
    const CTxDestination change_address() const { return _change_address; }
    void set_change_address(const CTxDestination& address);
    CAmount change_amount() const { return _change_amount; }
    void set_change_amount(const CAmount& amount);

    bool valid();

    const CScript structured_data_script();

    const std::vector<CTxIn> tx_inputs() const { return funding_txins(); }
    const std::vector<CTxOut> tx_outputs();

    const CTransaction tx();

private:
    bool regenerate_if_needed();

private:
    unsigned int _version;
    ActorType _actor;
    CTxDestination _reward_address;
    nlohmann::json _payload;

    CTxDestination _stake_address;
    CAmount _stake_amount;
    CTxOut _stake_txout;

    CTxDestination _change_address;
    CAmount _change_amount;
    CTxOut _change_txout;

    bool _dirty;

    CScript _script;
    CMutableTransaction _tx;
};

#endif // BWSCOIN_BUY_TICKET_TX_H
