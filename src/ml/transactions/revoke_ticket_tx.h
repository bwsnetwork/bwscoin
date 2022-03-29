/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// A Revoke Ticket (RvT) transaction is sent by the ticket
// buyer in order to indicate that the respective
// ticket shoud not be used for funding ML operations.

#ifndef BWSCOIN_REVOKE_TICKET_TX_H
#define BWSCOIN_REVOKE_TICKET_TX_H

#include <uint256.h>
#include <validation.h>
#include <script/script.h>
#include <script/standard.h>
#include <primitives/transaction.h>

extern const unsigned int rvt_current_version;   // should be monotonic

// The follwing functions are useful for processing RvT transactions.
// They are intended to be very specific and lighter than the related
// RevokeTicketTx class. However, for complete encapsulation of features
// and data, use the class.

// structured script
// (use the output script only if function returns true)
bool rvt_script(CScript& script, unsigned int version = rvt_current_version);

// validate the script
// (validation is based on parsing)
bool rvt_script_valid(const CScript& script, std::string& reason);
bool rvt_script_valid(const std::vector<std::vector<unsigned char>> items, std::string& reason);

// parse and validate the data
// (validation is based on parsing)
bool rvt_parse_script(const CScript& script,
                      unsigned int& version, std::string& reason);
bool rvt_parse_script(const std::vector<std::vector<unsigned char>> items,
                      unsigned int& version, std::string& reason);

// parse and validate the transaction
// (use the output values only if the function returns true)
bool rvt_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin,
                  CTxOut& refund_txout, CTxOut& change_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxOut& refund_txout, const CTxOut& change_txout,
            const unsigned int version = rvt_current_version);
bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& refund_address, const CAmount& refund,
            const CTxDestination& change_address, const CAmount& change,
            const unsigned int version = rvt_current_version);

// validate the transaction
bool rvt_tx_valid(const CTransaction& tx, std::string& reason);

// non-contextual input and output tests
bool rvt_check_inputs_nc(const CTransaction& tx, CValidationState &state);
bool rvt_check_outputs_nc(const CTransaction& tx, CValidationState &state);

// contextual input tests
bool rvt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, const CChainParams& chain_params, const int spend_height, CValidationState &state);
bool rvt_check_outputs(const CTransaction& tx, CValidationState &state);

// Wrapper class for Revoke Ticket transactions

class RevokeTicketTx
{
public:
    static RevokeTicketTx from_script(const CScript& script);
    static RevokeTicketTx from_tx(const CTransaction& tx);

    static const std::string name();

public:
    RevokeTicketTx();

    unsigned int version() const { return _version; }
    void set_version(const unsigned int version);

    const CTxIn ticket_txin() const { return _ticket_txin; }
    void set_ticket_txin(const CTxIn& txin);

    const CTxOut refund_txout() const { return _refund_txout; }
    void set_refund_txout(const CTxOut& txout);
    const CTxDestination refund_address() const { return _refund_address; }
    void set_refund_address(const CTxDestination& address);
    CAmount refund_amount() const { return _refund_amount; }
    void set_refund_amount(const CAmount amount);

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

    CTxIn _ticket_txin;

    CTxDestination _refund_address;
    CAmount _refund_amount;
    CTxOut _refund_txout;

    CTxDestination _change_address;
    CAmount _change_amount;
    CTxOut _change_txout;

    bool _dirty;

    CScript _script;
    CMutableTransaction _tx;
};

#endif // BWSCOIN_REVOKE_TICKET_TX_H
