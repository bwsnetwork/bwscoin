//
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 Project PAI Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//


#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "stake/staketx.h"
#include "crypto/common.h"
#include "chainparams.h"

uint256 CBlockHeader::GetHash() const
{
    if (this->nVersion & HARDFORK_VERSION_BIT)
        return SerializeHash<CBlockHashWriter>(*this);

    return SerializeHash<CHashWriter>(*this);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u,"
                    "nStakeDifficulty=%s, nVoteBits=%04x, nTicketPoolSize=%u, ticketLotteryState=%s, nFreshStake=%u, nStakeVersion=%u,"
                    "powMsgHistoryId=%s, powMsgId=%s, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        std::to_string(nStakeDifficulty), nVoteBits.getBits(), nTicketPoolSize, StakeStateToString(ticketLotteryState), nFreshStake, nStakeVersion,
        powMsgHistoryId, powMsgId,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}

void CBlockHeader::SetReadStakeDefaultBeforeFork()
{
    nStakeDifficulty = Params().GetConsensus().nMinimumStakeDiff;
    nVoteBits = VoteBits::rttAccepted;
    nTicketPoolSize = 0;
    ticketLotteryState.SetNull();
    nVoters = 0;
    nFreshStake = 0;
    nRevocations = 0;
    extraData.SetNull();
    nStakeVersion = 0;
    std::fill(powMsgHistoryId, powMsgHistoryId + MSG_ID_SIZE, 0);
    std::fill(powMsgId, powMsgId + MSG_ID_SIZE, 0);
}