/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "structured_data.h"

#include <script/standard.h>

const uint32_t sds_first_output_index = 0;
const uint32_t sds_max_script_size = 4096;

std::vector<std::vector<unsigned char>> sds_script_items(const CScript& script)
{
    txnouttype type;
    std::vector<std::vector<unsigned char>> items;
    if (!Solver(script, type, items) || type != TX_STRUCT_DATA)
        return std::vector<std::vector<unsigned char>>();

    return items;
}

StructuredDataVersion sds_version(const CScript& script)
{
    return sds_version(sds_script_items(script));
}

StructuredDataVersion sds_version(const std::vector<std::vector<unsigned char>>& script_items)
{
    if (script_items.size() < 1)
        return sdv_invalid;

    return static_cast<StructuredDataVersion>(CScriptNum(script_items[0], false).getint());
}

StructuredDataClass sds_class(const CScript& script)
{
    return sds_class(sds_script_items(script));
}

StructuredDataClass sds_class(const std::vector<std::vector<unsigned char>>& script_items)
{
    if (script_items.size() < 2)
        return SDC_COUNT;

    int data_class_int = CScriptNum(script_items[1], false).getint();
    if (data_class_int < 0 || data_class_int >= static_cast<long long>(SDC_COUNT))
        return SDC_COUNT;

    return static_cast<StructuredDataClass>(data_class_int);
}

CScript sds_payload(const CScript& script)
{
    return sds_payload(sds_script_items(script));
}

CScript sds_payload(const std::vector<std::vector<unsigned char>>& script_items)
{
    if (script_items.size() < 3)
        return CScript();

    CScript payload;
    for (auto i = script_items.begin() + 2; i != script_items.end(); i++)
        payload << *i;

    return payload;
}

bool sds_valid(const CScript& script, std::string& reason)
{
    return sds_valid(sds_script_items(script), reason);
}

bool sds_valid(const std::vector<std::vector<unsigned char>>& script_items, std::string& reason)
{
    if (script_items.size() < 2) {
        reason = "invalid-script-size";
        return false;
    }

    auto version = static_cast<StructuredDataVersion>(CScriptNum(script_items[0], false).getint());
    if (!sdv_valid(version)) {
        reason = "invalid-script-version";
        return false;
    }

    int data_class_int = CScriptNum(script_items[1], false).getint();
    if (!sdc_valid(data_class_int)) {
        reason = "invalid-script-class";
        return false;
    }

    return true;
}

bool sds_is_structured_data_txout(const CTxOut& txout)
{
    std::string reason;
    return sds_valid(txout.scriptPubKey, reason);
}

bool sds_is_first_output(const CTxOut& txout)
{
    return txout.nValue == 0 &&
            txout.scriptPubKey.size() > 1 &&
            txout.scriptPubKey[0] == OP_RETURN &&
            txout.scriptPubKey[1] == OP_STRUCT;
}

bool sds_is_subsequent_output(const CTxOut& txout)
{
    return txout.nValue == 0 &&
            txout.scriptPubKey.size() > 0 &&
            txout.scriptPubKey[0] == OP_RETURN &&
            (txout.scriptPubKey.size() == 1 || txout.scriptPubKey[1] != OP_STRUCT);
}

CScript sds_create(const StructuredDataClass cls, const StructuredDataVersion version)
{
    return CScript() << OP_RETURN << OP_STRUCT << version << cls;
}

bool sds_from_tx(const CTransaction& tx, CScript& script, std::string& reason)
{
    return sds_from_txouts(tx.vout, script, reason);
}

bool sds_from_txouts(const std::vector<CTxOut>& txouts, CScript& script, std::string& reason)
{
    // validation

    if (txouts.size() < sds_first_output_index + 1) {
        reason = "invalid-input-count";
        return false;
    }

    const auto& s = txouts[sds_first_output_index].scriptPubKey;

    if (s.size() < 2 || s[0] != OP_RETURN || s[1] != OP_STRUCT) {
        reason = "invalid-script-header";
        return false;
    }

    // script concatenation

    script.clear();
    script += txouts[sds_first_output_index].scriptPubKey;

    for (size_t i = sds_first_output_index + 1; i < txouts.size(); ++i)
    {
        const CScript& second_script = txouts[i].scriptPubKey;
        if (second_script.size() > 1 && second_script[0] == OP_RETURN)
            script.insert(script.end(), second_script.begin() + 1, second_script.end());
    }

    // validation

    if (!sds_valid(script, reason))
        return false;

    return true;
}

std::vector<CTxOut> sds_tx_outputs(const CScript& script)
{
    std::vector<CTxOut> outputs;

    unsigned int script_size = script.size();
    unsigned int processed = 0;
    while (processed < script_size)
    {
        unsigned int reserved_bytes = (processed == 0) ? 4 : 3;  // OP_RETURN + (OP_STRUCT) + push
        unsigned int chunk_size = std::min(nMaxStructDatacarrierBytes - reserved_bytes, script_size - processed);

        CScript scriptPubKey;
        if (processed > 0) scriptPubKey << OP_RETURN;
        scriptPubKey += CScript(script.begin() + processed, script.begin() + processed + chunk_size);

        CTxOut tx_out(0, scriptPubKey);
        outputs.push_back(tx_out);

        processed += chunk_size;
    }

    return outputs;
}


static StructuredData invalid_structured_data = StructuredData(SDC_COUNT);

StructuredData StructuredData::parse_tx(const CTransaction& tx)
{
    std::string reason;
    CScript script;
    sds_from_tx(tx, script, reason);

    return StructuredData::from_script(script);
}

StructuredData StructuredData::from_script(const CScript& script)
{
    if (script.size() < 2 || script[0] != OP_RETURN || script[1] != OP_STRUCT)
        return invalid_structured_data;

    auto items = ::sds_script_items(script);
    if (items.size() < 2)   // must contain at least version + data class
        return invalid_structured_data;

    int data_class_int = CScriptNum(items[1], false).getint();
    if (data_class_int < 0 || data_class_int >= static_cast<long long>(SDC_COUNT))
        return invalid_structured_data;

    StructuredData sd;
    sd._script = script;
    sd._data_class = static_cast<StructuredDataClass>(data_class_int);
    sd._version = static_cast<StructuredDataVersion>(CScriptNum(items[0], false).getint());

    return sd;
}

StructuredData::StructuredData(const StructuredDataClass cls, const StructuredDataVersion version) : _version(version), _data_class(cls)
{
    _script << OP_RETURN << OP_STRUCT << version << cls;
}

bool StructuredData::valid() const
{
    return sdc_valid(_data_class);
}

const std::vector<std::vector<unsigned char>> StructuredData::script_items()
{
    return ::sds_script_items(_script);
}

std::vector<CTxOut> StructuredData::tx_outputs() const
{
    return sds_tx_outputs(_script);
}
