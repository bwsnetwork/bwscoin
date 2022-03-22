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

class CFeeRate;
class CValidationState;

extern const unsigned int byt_current_version;   // should be monotonic

// The follwing functions are useful for processing ByT transactions.
// They are intended to be very specific and lighter than the related
// BuyTicketTx class. However, for complete encapsulation of features
// and data, use the class.

// ticket script
// (use the output script only if function returns true)
bool byt_script(CScript& script,
                const ActorType& actor, const CTxDestination& reward_address, unsigned int version = byt_current_version);

// validate the script
// (validation is based on parsing)
bool byt_script_valid(const CScript& script, std::string& reason);
bool byt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason);

// parse and validate the data
// (use the output values only if the function returns true)
bool byt_parse_script(const CScript& script,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      std::string& reason);
bool byt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                      std::string& reason);

// parse and validate the transaction
// (use the output values only if the function returns true)
bool byt_parse_tx(const CTransaction& tx,
                  CTxOut& stake_txout, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, ActorType& actor, CTxDestination& reward_address,
                  std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxOut& stake_txout, const CTxOut& change_txout,
            const ActorType& actor, const CTxDestination& reward_address, const unsigned int version = byt_current_version);
bool byt_tx(CMutableTransaction& tx,
            const std::vector<CTxIn> txins,
            const CTxDestination& stake_address, const CAmount& stake,
            const CTxDestination& change_address, const CAmount& change,
            const ActorType& actor, const CTxDestination& reward_address, const unsigned int version = byt_current_version);

// calculate the fee for the transaction (assumes that
// the change output is also present)
CAmount byt_fee(const unsigned int txin_count, const CFeeRate& fee_rate);

// check basic input sizes and correspondences
bool byt_basic_input_checks(const CTransaction& tx, CValidationState &state);

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
