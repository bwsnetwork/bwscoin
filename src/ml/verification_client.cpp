/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "verification_client.h"

#include "primitives/block.h"
#include "httpclient.h"
#include "streams.h"
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

bool VerificationClient::Verify(const CBlockHeader& block)
{
    std::string verificationServerAddress = gArgs.GetArg("-verificationserver", "localhost:50011");

    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;
    std::string blockHeaderHex = HexStr(ssBlock.begin(), ssBlock.end());

    UniValue body(UniValue::VOBJ);
    body.pushKV("msg_history_id", std::string(block.powMsgHistoryId));
    body.pushKV("msg_id", std::string(block.powMsgId));
    body.pushKV("nonce", static_cast<uint64_t>(block.nNonce));
    body.pushKV("block_header", blockHeaderHex);

    HttpClient client(verificationServerAddress);
    auto response = client.post("/verify", body);

    return response.status == HttpResponse::Ok;
}