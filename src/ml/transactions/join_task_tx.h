/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// A Join Task (JnT) transaction is sent by the miner
// to indicate its participation in that task's training.

#ifndef BWSCOIN_JOIN_TASK_TX_H
#define BWSCOIN_JOIN_TASK_TX_H

#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

class CCoinsViewCache;
class CChainParams;
class Coin;
class CFeeRate;
class CValidationState;

extern const unsigned int jnt_current_version;   // should be monotonic

// The follwing functions are useful for processing JnT transactions.
// They are intended to be very specific and lighter than the related
// JoinTaskTx class. However, for complete encapsulation of features
// and data, use the class.

// structured script
// (use the output script only if function returns true)
bool jnt_script(CScript& script, const uint256& task_id, const unsigned int version = jnt_current_version);

// validate the script
// (validation is based on parsing)
bool jnt_script_valid(const CScript& script, std::string& reason);
bool jnt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason);

// parse and validate the data
// (validation is based on parsing)
bool jnt_parse_script(const CScript& script,
                      unsigned int& version, uint256& task_id, std::string& reason);
bool jnt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, uint256& task_id, std::string& reason);

// parse and validate the transaction
// (use the output values only if the function returns true)
bool jnt_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin,
                  CTxOut& stake_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  uint256& task_id, unsigned int& version, std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool jnt_tx(CMutableTransaction& tx,
            const CTransaction& ticket,
            const CTxDestination& stake_address,
            const CFeeRate& fee_rate,
            const uint256& task_id, const unsigned int version = jnt_current_version);
bool jnt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxOut& stake_txout,
            const uint256& task_id, const unsigned int version = jnt_current_version);
bool jnt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& stake_address, const CAmount& stake,
            const uint256& task_id, const unsigned int version = jnt_current_version);

// generate the required stake amount
// (parses ticket internally)
// (use the output only if the function returns true)
bool jnt_stake_amount(const CTransaction& ticket, const CFeeRate& fee_rate, CAmount& stake);

// estimate the fee for the transaction
CAmount jnt_fee(const CFeeRate& fee_rate);

// verify if certain elements can belong to JnT transactions
bool jnt_is_stake_output(const Coin& coin, const uint32_t txout_index);

// validate the transaction
bool jnt_tx_valid(const CTransaction& tx, std::string& reason);

// non-contextual input and output tests
bool jnt_check_inputs_nc(const CTransaction& tx, CValidationState &state);
bool jnt_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state);
bool jnt_check_outputs_nc(const CTransaction& tx, CValidationState &state);
bool jnt_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state);

// contextual input tests
bool jnt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, CValidationState &state);

//// Wrapper class for Join Task transactions

class JoinTaskTx
{
public:
    static JoinTaskTx from_script(const CScript& script);
    static JoinTaskTx from_tx(const CTransaction& tx);

    static const std::string name();

public:
    JoinTaskTx();

    unsigned int version() const { return _version; }
    void set_version(const unsigned int version);

    uint256 task_id() const { return _task_id; }
    void set_task_id(const uint256 task_id);

    const CTxIn ticket_txin() const { return _ticket_txin; }
    void set_ticket_txin(const CTxIn& txin);

    const CTxOut stake_txout() const { return _stake_txout; }
    void set_stake_txout(const CTxOut& txout);
    const CTxDestination stake_address() const { return _stake_address; }
    void set_stake_address(const CTxDestination& address);
    CAmount stake_amount() const { return _stake_amount; }
    void set_stake_amount(const CAmount amount);

    bool valid();

    const CScript structured_data_script();

    const std::vector<CTxIn> tx_inputs();
    const std::vector<CTxOut> tx_outputs();

    const CTransaction tx();

private:
    bool regenerate_if_needed();

private:
    unsigned int _version;
    uint256 _task_id;

    CTxIn _ticket_txin;

    CTxDestination _stake_address;
    CAmount _stake_amount;
    CTxOut _stake_txout;

    bool _dirty;

    CScript _script;
    CMutableTransaction _tx;
};

#endif // BWSCOIN_JOIN_TASK_TX_H
