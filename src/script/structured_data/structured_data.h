/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// Structured data scripts (SDSs) can span multiple outputs.
// Unlike simple data outputs, first output of an SDS must
// always be at index sds_first_output_index and starts with
// OP_RETURN + OP_STRUCT + version + data class.
// The other outputs of an SDS can be at any index, but
// they will be concatenated in the order of their indexes
// to produce the actual SDS. These secondary SDS outputs
// start only with OP_RETURN.

#ifndef BWSCOIN_STRUCTURED_DATA_H
#define BWSCOIN_STRUCTURED_DATA_H

#include <primitives/transaction.h>
#include <script/script.h>
#include <script/structured_data/structured_data_class.h>
#include <script/structured_data/structured_data_version.h>

// the index of the transaction's output where the script starts
extern const uint32_t sds_first_output_index;

// The following functions help manipulating the SDSs.
// They provide smaller footprint implementations of features
// supported by the StructuredData class, for point
// operations. However, for frequent accesses to script
// elements, like version or class, it is recommended to use
// the StructuredData class instead.
// Please note that they don't do implicit validation of
// SDS structure.

// the script items do NOT cantain the OP_RETURN + OP_STRUCT pair!
std::vector<std::vector<unsigned char>> sds_script_items(const CScript& script);

StructuredDataVersion sds_version(const CScript& script);
StructuredDataVersion sds_version(const std::vector<std::vector<unsigned char>>& script_items);

StructuredDataClass sds_class(const CScript& script);
StructuredDataClass sds_class(const std::vector<std::vector<unsigned char>>& script_items);

CScript sds_payload(const CScript& script);
CScript sds_payload(const std::vector<std::vector<unsigned char>>& script_items);

bool sds_valid(const CScript& script, std::string& reason);
bool sds_valid(const std::vector<std::vector<unsigned char>>& script_items, std::string& reason);

bool sds_is_structured_data_txout(const CTxOut& txout);
bool sds_is_first_output(const CTxOut& txout);
bool sds_is_subsequent_output(const CTxOut& txout);

CScript sds_create(const StructuredDataClass cls, const StructuredDataVersion version = sdv_current_version);

bool sds_from_tx(const CTransaction& tx, CScript& script, std::string& reason);
bool sds_from_txouts(const std::vector<CTxOut>& txouts, CScript& script, std::string& reason);

std::vector<CTxOut> sds_tx_outputs(const CScript& script);

// StructuredData is a helper class to manipulate the SDSs.
// It wraps the actual script and uses the above helper
// functions internally.

class StructuredData
{
public:
    static StructuredData parse_tx(const CTransaction& tx);

    static StructuredData from_script(const CScript& script);

public:
    StructuredData() { }
    StructuredData(const StructuredDataClass cls, const StructuredDataVersion version = sdv_current_version);

    bool valid() const;

    StructuredDataClass data_class() const { return _data_class; }

    const CScript& script() const { return _script; }
    const std::vector<std::vector<unsigned char>> script_items();   // without OP_RETURN + OP_STRUCT

    std::vector<CTxOut> tx_outputs() const;

private:
    template<typename T>
    friend StructuredData& operator<<(StructuredData& d, T v);

private:
    StructuredDataVersion _version;
    StructuredDataClass _data_class;

    CScript _script;
};

template<typename T>
StructuredData& operator<<(StructuredData& d, T v)
{
    d._script << v;
    return d;
}

#endif // BWSCOIN_STRUCTURED_DATA_H
