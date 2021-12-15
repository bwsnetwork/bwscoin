/* * Copyright (c) 2009-2016 The Bitcoin Core developers
 * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_WALLET_RPCWALLET_H
#define BWSCOIN_WALLET_RPCWALLET_H

#include <string>
#include "support/allocators/secure.h"

class CRPCTable;
class CWallet;
class JSONRPCRequest;
class UniValue;

void RegisterWalletRPCCommands(CRPCTable &t);

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
CWallet *GetWalletForJSONRPCRequest(const JSONRPCRequest& request);

std::string HelpRequiringPassphrase(const CWallet * const);
void EnsureWalletIsUnlocked(const CWallet * const);
bool EnsureWalletIsAvailable(const CWallet * const, bool avoidException);

SecureString ValidatedPasswordFromOptionalValue(CWallet* pwallet, const UniValue& value);

UniValue rescanblockchain(const JSONRPCRequest& request);

#endif //BWSCOIN_WALLET_RPCWALLET_H
