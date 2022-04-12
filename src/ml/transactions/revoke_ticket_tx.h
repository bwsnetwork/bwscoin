/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// A Revoke Ticket (RvT) transaction is sent by the ticket
// buyer in order to indicate that the respective
// ticket shoud not be used for funding ML operations.

#ifndef BWSCOIN_REVOKE_TICKET_TX_H
#define BWSCOIN_REVOKE_TICKET_TX_H

#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

class CCoinsViewCache;
class CChainParams;
class Coin;
class CFeeRate;
class CValidationState;

extern const unsigned int rvt_current_version;   // should be monotonic

// The follwing functions are useful for processing RvT transactions.
// They are intended to be very specific and lighter than the related
// RevokeTicketTx class. However, for complete encapsulation of features
// and data, use the class.

// Script

// structured script
// (use the output script only if function returns true)
bool rvt_script(CScript& script, const unsigned int version = rvt_current_version);

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

// Transaction

// parse and validate the transaction
// (use the output values only if the function returns true)
bool rvt_parse_tx(const CTransaction& tx,
                  CTxIn& ticket_txin,
                  CTxOut& refund_txout,
                  CScript& script,
                  std::vector<std::vector<unsigned char>> items,
                  unsigned int& version, std::string& reason);

// generate the transaction
// (use the output tx only if the function returns true)
bool rvt_tx(CMutableTransaction& tx,
            const CTransaction& ticket,
            const CFeeRate& fee_rate,
            const unsigned int version = rvt_current_version);
bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxOut& refund_txout,
            const unsigned int version = rvt_current_version);
bool rvt_tx(CMutableTransaction& tx,
            const CTxIn& ticket_txin,
            const CTxDestination& refund_address, const CAmount& refund,
            const unsigned int version = rvt_current_version);

// validate the transaction
bool rvt_tx_valid(const CTransaction& tx, std::string& reason);

// Inputs and outputs

// generate the required refund output
// (parses ticket internally)
// (use the output only if the function returns true)
bool rvt_refund_output(const CTransaction& ticket, const CFeeRate& fee_rate, CTxOut& txout);

// verify if certain elements can belong to RvT transactions
bool rvt_is_refund_output(const Coin& coin, const uint32_t txout_index);

// non-contextual input and output tests
bool rvt_check_inputs_nc(const CTransaction& tx, CValidationState &state);
bool rvt_check_inputs_nc(const std::vector<CTxIn>& txins, CValidationState &state);
bool rvt_check_outputs_nc(const CTransaction& tx, CValidationState &state);
bool rvt_check_outputs_nc(const std::vector<CTxOut>& txouts, CValidationState &state);

// contextual input tests
bool rvt_check_inputs(const CTransaction& tx, const CCoinsViewCache& inputs, const CChainParams& chain_params, const int spend_height, CValidationState &state);
bool rvt_check_outputs(const CTransaction& tx, const CTransaction& ticket, CValidationState &state);

// Other

// estimate the fee for the transaction
CAmount rvt_fee(const CFeeRate& fee_rate);

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

    bool _dirty;

    CScript _script;
    CMutableTransaction _tx;
};

#endif // BWSCOIN_REVOKE_TICKET_TX_H
