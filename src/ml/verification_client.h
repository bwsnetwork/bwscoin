/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#ifndef BWSCOIN_VERIFICATION_CLIENT_H
#define BWSCOIN_VERIFICATION_CLIENT_H

class CBlockHeader;

/**
 * Simple class to verify the block against the ML verification server.
 */

class VerificationClient
{
public:
    static bool Verify(const CBlockHeader& block);
};

#endif // BWSCOIN_VERIFICATION_CLIENT_H
