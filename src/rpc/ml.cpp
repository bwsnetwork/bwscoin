/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "rpc/server.h"
#include "util.h"
#include "utilstrencodings.h"
#include "policy/feerate.h"
#include "policy/rbf.h"
#include "primitives/transaction.h"
#include "ml/transactions/buy_ticket_tx.h"
#include "ml/transactions/pay_for_task_tx.h"
#include "core_io.h"
#include "key_io.h"
#include <univalue.h>

UniValue createbuytickettransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "createbuytickettransaction [{\"txid\":\"id\",\"vout\":n},...] {\"version\":version,\"actor\":actor,\"reward_address\":adress,\"stake_address\":address,\"stake_amount\":amount,\"change_address\":address,\"change_amount\":amount} ( locktime )  ( replaceable ) ( expiry )\n"
            "\nCreate a ticket purchase transaction spending the given inputs to stake funds for the intended operations.\n"
            "First output is a structured data containing the actor type and address where the reward should be sent.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"inputs\"                           (array, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"id\",               (string, required) The transaction id\n"
            "         \"vout\":n,                    (numeric, required) The output number\n"
            "         \"sequence\":n                 (numeric, optional) The sequence number\n"
            "       } \n"
            "       ,...\n"
            "     ]\n"
            "2. \"ticket_data\"                      (object, required) A json object with ticket details\n"
            "     {\n"
            "       \"version\": n,                  (numeric, optional) The version of the ticket\n"
            "       \"actor\": \"type\",             (string, required) The type of actor (client, miner, ...)\n"
            "       \"reward_address\": \"address\"  (string, required) The address where the reward must be paid\n"
            "       \"stake_address\": \"address\",  (string, required) The address where the staked funds are sent\n"
            "       \"stake_amount\": n,             (numeric, required) The amount of " + CURRENCY_UNIT + " to stake\n"
            "       \"change_address\": \"address\"  (string, optional) The address where the change for this transaction is sent\n"
            "       \"change_amount\": n             (numeric, optional) The amount of change. Must be present if the change address is\n"
            "     }\n"
            "3. locktime                             (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
            "4. replaceable                          (boolean, optional, default=false) Marks this transaction as BIP125 replaceable.\n"
            "                                           Allows this transaction to be replaced by a transaction with higher fees. If provided, it is an error if explicit sequence numbers are incompatible.\n"
            "5. expiry                               (numeric, optional, default=0) Expiration height. 0 value means no expiry."

            "\nResult:\n"
            "\"transaction\"                         (string) hex string of the transaction\n"

            "\nExamples:\n"
            + HelpExampleCli("createbuytickettransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"version\\\":1,\\\"actor\\\":\\\"client\\\",\\\"reward_address\\\":\\\"address\\\",\\\"stake_address\\\":\\\"address\\\",\\\"stake_amount\\\":100,\\\"change_address\\\":\\\"address\\\",\\\"change_amount\\\":50}\"")
            + HelpExampleCli("createbuytickettransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"actor\\\":\\\"client\\\",\\\"reward_address\\\":\\\"address\\\",\\\"stake_address\\\":\\\"address\\\",\\\"stake_amount\\\":100}\"")
            + HelpExampleRpc("createbuytickettransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"version\\\":1,\\\"actor\\\":\\\"client\\\",\\\"reward_address\\\":\\\"address\\\",\\\"stake_address\\\":\\\"address\\\",\\\"stake_amount\\\":100,\\\"change_address\\\":\\\"address\\\",\\\"change_amount\\\":50}\"")
            + HelpExampleRpc("createbuytickettransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"actor\\\":\\\"client\\\",\\\"reward_address\\\":\\\"address\\\",\\\"stake_address\\\":\\\"address\\\",\\\"stake_amount\\\":100}\"")
        );

    // structure validation

    RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VOBJ, UniValue::VNUM}, true);
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_array();
    UniValue ticket_data = request.params[1].get_obj();

    // optional values

    uint32_t lock_time = 0;
    if (!request.params[2].isNull()) {
        int64_t lock_time_int = request.params[2].get_int64();
        if (lock_time_int < 0 || lock_time_int > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        lock_time = static_cast<uint32_t>(lock_time_int);
    }

    bool rbf_opt_in = request.params[3].isTrue();

    uint32_t expiry = 0;
    if (!request.params[4].isNull()) {
        int64_t expiry_int = request.params[4].get_int64();
        if (expiry_int < 0 || expiry_int > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, expiry out of range");
        expiry = static_cast<uint32_t>(expiry_int);
    }

    // inputs

    std::vector<CTxIn> txins;
    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int output = vout_v.get_int();
        if (output < 0)
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t sequence;
        if (rbf_opt_in)
            sequence = MAX_BIP125_RBF_SEQUENCE;
        else if (lock_time)
            sequence = std::numeric_limits<uint32_t>::max() - 1;
        else
            sequence = std::numeric_limits<uint32_t>::max();

        // set the sequence number if passed in the parameters object
        const UniValue& sequence_obj = find_value(o, "sequence");
        if (sequence_obj.isNum()) {
            int64_t seq_nr_64 = sequence_obj.get_int64();
            if (seq_nr_64 < 0 || seq_nr_64 > std::numeric_limits<uint32_t>::max())
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            sequence = static_cast<uint32_t>(seq_nr_64);
        }

        CTxIn in(COutPoint(txid, static_cast<uint32_t>(output)), CScript(), sequence);

        txins.push_back(in);
    }

    // ticket data

    unsigned int version = byt_current_version;
    ActorType actor = AT_COUNT;
    CTxDestination reward_address = CNoDestination(), stake_address = CNoDestination(), change_address = CNoDestination();
    CAmount stake_amount = -1, change_amount = -1;

    std::vector<std::string> ticket_data_keys = ticket_data.getKeys();
    for (const auto& key : ticket_data_keys) {
        if (key == "version") {
            int64_t version_int = ticket_data["version"].get_int64();
            if (version_int < 0 || version_int > std::numeric_limits<unsigned int>::max() || version_int > byt_current_version)
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, ticket version is out of range");
            version = static_cast<unsigned int>(version_int);
        } else if (key == "actor") {
            actor = at_from_string(ticket_data["actor"].get_str());
            if (!at_valid(actor))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, actor is not valid");
        } else if (key == "reward_address") {
            const auto& reward_address_string = ticket_data["reward_address"].get_str();
            reward_address = DecodeDestination(reward_address_string);
            if (!IsValidDestination(reward_address))
                throw JSONRPCError(RPCErrorCode::INVALID_ADDRESS_OR_KEY, std::string("Invalid BWS Coin address: ") + reward_address_string);
        } else if (key == "stake_address") {
            const auto& stake_address_string = ticket_data["stake_address"].get_str();
            stake_address = DecodeDestination(stake_address_string);
            if (!IsValidDestination(stake_address))
                throw JSONRPCError(RPCErrorCode::INVALID_ADDRESS_OR_KEY, std::string("Invalid BWS Coin address: ") + stake_address_string);
        } else if (key == "stake_amount") {
            int64_t stake_amount_int = ticket_data["stake_amount"].get_int64();
            if (stake_amount_int == 0 || !MoneyRange(stake_amount_int))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, staked amount is out of range");
            stake_amount = static_cast<CAmount>(stake_amount_int);
        } else if (key == "change_address") {
            const auto& change_address_string = ticket_data["change_address"].get_str();
            change_address = DecodeDestination(change_address_string);
            if (!IsValidDestination(change_address))
                throw JSONRPCError(RPCErrorCode::INVALID_ADDRESS_OR_KEY, std::string("Invalid BWS Coin address: ") + change_address_string);
        } else if (key == "change_amount") {
            int64_t change_amount_int = ticket_data["change_amount"].get_int64();
            if (change_amount_int == 0 || !MoneyRange(change_amount_int))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, change amount is out of range");
            change_amount = static_cast<CAmount>(change_amount_int);
        }
    }

    // transaction

    CMutableTransaction mtx;
    if (!byt_tx(mtx, txins, stake_address, stake_amount, change_address, change_amount, actor, reward_address, version))
        throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Could not create the transaction");

    mtx.nLockTime = lock_time;
    mtx.nExpiry = expiry;

    return EncodeHexTx(mtx);
}

UniValue createpayfortasktransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "createpayfortasktransaction {\"ticket\":{\"txid\":\"id\",\"vout\":n},\"extra_funding\":[{\"txid\":\"id\",\"vout\":n},...]} {\"version\":version,\"task\":task,\"stake_amount\":amount,\"change_address\":address,\"change_amount\":amount} ( locktime )  ( replaceable ) ( expiry )\n"
            "\nCreate a task submission transaction spending the given ticket and optional extra inputs to stake funds for the miners.\n"
            "The structured data script can be spread on multiple data outputs, the first being the first output of the transaction, and the following parts after the stake and optional change outputs.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"inputs\"                           (object, required) A json object with ticket details\n"
            "     {\n"
            "       \"ticket\"                       (object, required) A json object with funding ticket details\n"
            "         {\n"
            "           \"txid\":\"id\",             (string, required) The transaction id\n"
            "           \"vout\":n,                  (numeric, required) The output number\n"
            "           \"sequence\":n               (numeric, optional) The sequence number\n"
            "         } \n"
            "         \"extra_funding\"              (array, optional) A json array with extra funding objects\n"
            "         [\n"
            "           {\n"
            "             \"txid\":\"id\",           (string, required) The transaction id\n"
            "             \"vout\":n,                (numeric, required) The output number\n"
            "             \"sequence\":n             (numeric, optional) The sequence number\n"
            "           } \n"
            "           ,...\n"
            "         ]\n"
            "     }\n"
            "2. \"task_data\"                        (object, required) A json object with the task details\n"
            "     {\n"
            "       \"version\": n,                  (numeric, optional) The version of the task\n"
            "       \"task\": \"task\",              (string, required) The string representation of the task details\n"
            "       \"stake_amount\": n,             (numeric, required) The amount of " + CURRENCY_UNIT + " to stake for this task\n"
            "       \"change_address\": \"address\"  (string, optional) The address where the change for this transaction is sent\n"
            "       \"change_amount\": n             (numeric, optional) The amount of change. Must be present if the change address is\n"
            "     }\n"
            "3. locktime                             (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
            "4. replaceable                          (boolean, optional, default=false) Marks this transaction as BIP125 replaceable.\n"
            "                                           Allows this transaction to be replaced by a transaction with higher fees. If provided, it is an error if explicit sequence numbers are incompatible.\n"
            "5. expiry                               (numeric, optional, default=0) Expiration height. 0 value means no expiry."

            "\nResult:\n"
            "\"transaction\"                         (string) hex string of the transaction\n"

            "\nExamples:\n"
            + HelpExampleCli("createpayfortasktransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"version\\\":1,\\\"task\\\":\\\"task\\\",\\\"stake_amount\\\":100,\\\"change_address\\\":\\\"address\\\",\\\"change_amount\\\":50}\"")
            + HelpExampleCli("createpayfortasktransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"task\\\":\\\"task\\\",\\\"stake_amount\\\":100}\"")
            + HelpExampleRpc("createpayfortasktransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"version\\\":1,\\\"task\\\":\\\"task\\\",\\\"stake_amount\\\":100,\\\"change_address\\\":\\\"address\\\",\\\"change_amount\\\":50}\"")
            + HelpExampleRpc("createpayfortasktransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"task\\\":\\\"task\\\",\\\"stake_amount\\\":100}\"")
        );

    // structure validation

    RPCTypeCheck(request.params, {UniValue::VOBJ, UniValue::VOBJ, UniValue::VNUM}, true);
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_obj();
    UniValue task_data = request.params[1].get_obj();

    // optional values

    uint32_t lock_time = 0;
    if (!request.params[2].isNull()) {
        int64_t lock_time_int = request.params[2].get_int64();
        if (lock_time_int < 0 || lock_time_int > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        lock_time = static_cast<uint32_t>(lock_time_int);
    }

    bool rbf_opt_in = request.params[3].isTrue();

    uint32_t expiry = 0;
    if (!request.params[4].isNull()) {
        int64_t expiry_int = request.params[4].get_int64();
        if (expiry_int < 0 || expiry_int > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, expiry out of range");
        expiry = static_cast<uint32_t>(expiry_int);
    }

    // inputs

    UniValue ins{UniValue::VARR};

    const UniValue& ticket_v = find_value(inputs, "ticket");
    if (!ticket_v.isObject())
        throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, missing ticket key");
    ins.push_back(ticket_v);

    const UniValue& extra_funding_v = find_value(inputs, "extra_funding");
    if (extra_funding_v.isArray()) {
        for (unsigned int idx = 0; idx < extra_funding_v.size(); idx++) {
            const UniValue& input = extra_funding_v[idx];
            if (input.isObject())
                ins.push_back(input);
        }
    }

    CTxIn ticket;
    std::vector<CTxIn> extra_funding;

    for (unsigned int idx = 0; idx < ins.size(); idx++) {
        const UniValue& input = ins[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int output = vout_v.get_int();
        if (output < 0)
            throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t sequence;
        if (rbf_opt_in)
            sequence = MAX_BIP125_RBF_SEQUENCE;
        else if (lock_time)
            sequence = std::numeric_limits<uint32_t>::max() - 1;
        else
            sequence = std::numeric_limits<uint32_t>::max();

        // set the sequence number if passed in the parameters object
        const UniValue& sequence_obj = find_value(o, "sequence");
        if (sequence_obj.isNum()) {
            int64_t seq_nr_64 = sequence_obj.get_int64();
            if (seq_nr_64 < 0 || seq_nr_64 > std::numeric_limits<uint32_t>::max())
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            sequence = static_cast<uint32_t>(seq_nr_64);
        }

        CTxIn in(COutPoint(txid, static_cast<uint32_t>(output)), CScript(), sequence);

        if (idx == 0)
            ticket = in;
        else
           extra_funding.push_back(in);
    }

    // task data

    unsigned int version = pft_current_version;
    nlohmann::json task;
    CAmount stake_amount = -1, change_amount = -1;
    CTxDestination change_address = CNoDestination();

    std::vector<std::string> task_data_keys = task_data.getKeys();
    for (const auto& key : task_data_keys) {
        if (key == "version") {
            int64_t version_int = task_data["version"].get_int64();
            if (version_int < 0 || version_int > std::numeric_limits<unsigned int>::max() || version_int > pft_current_version)
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, task version is out of range");
            version = static_cast<unsigned int>(version_int);
        } else if (key == "task") {
            const auto& task_string = task_data["task"].get_str();
            if (!pft_task_json(task_string, task))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, task is not valid");
        } else if (key == "stake_amount") {
            int64_t stake_amount_int = task_data["stake_amount"].get_int64();
            if (stake_amount_int == 0 || !MoneyRange(stake_amount_int))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, staked amount is out of range");
            stake_amount = static_cast<CAmount>(stake_amount_int);
        } else if (key == "change_address") {
            const auto& change_address_string = task_data["change_address"].get_str();
            change_address = DecodeDestination(change_address_string);
            if (!IsValidDestination(change_address))
                throw JSONRPCError(RPCErrorCode::INVALID_ADDRESS_OR_KEY, std::string("Invalid BWS Coin address: ") + change_address_string);
        } else if (key == "change_amount") {
            int64_t change_amount_int = task_data["change_amount"].get_int64();
            if (change_amount_int == 0 || !MoneyRange(change_amount_int))
                throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Invalid parameter, change amount is out of range");
            change_amount = static_cast<CAmount>(change_amount_int);
        }
    }

    // transaction

    CMutableTransaction mtx;
    if (!pft_tx(mtx, ticket, extra_funding, stake_amount, change_address, change_amount, task, version))
        throw JSONRPCError(RPCErrorCode::INVALID_PARAMETER, "Could not create the transaction");

    mtx.nLockTime = lock_time;
    mtx.nExpiry = expiry;

    return EncodeHexTx(mtx);
}

static const CRPCCommand commands[] =
{ //  category              name                           actor (function)              argNames
  //  --------------------- ----------------------------   ----------------------------  ----------
    { "ml",                 "createbuytickettransaction",  &createbuytickettransaction,  {"inputs","ticket_data","locktime","replaceable","expiry"} },
    { "ml",                 "createpayfortasktransaction", &createpayfortasktransaction, {"inputs","task_data","locktime","replaceable","expiry"} }
};

void RegisterMLRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
