//
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 Project PAI Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//


#include "policy/policy.h"
#include "txmempool.h"
#include "validation.h"
#include "keystore.h"
#include "script/sign.h"
#include "util.h"
#include "stake/extendedvotebits.h"

#include "test/test_bwscoin.h"

#include <boost/test/unit_test.hpp>
#include <boost/range/iterator_range.hpp>
#include <list>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool testPool;

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(txParent);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);

    // Just the parent:
    testPool.addUnchecked(txParent.GetHash(), entry.FromTx(txParent));
    poolSize = testPool.size();
    testPool.removeRecursive(txParent);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 1);
    
    // Parent, children, grandchildren:
    testPool.addUnchecked(txParent.GetHash(), entry.FromTx(txParent));
    for (int i = 0; i < 3; i++)
    {
        testPool.addUnchecked(txChild[i].GetHash(), entry.FromTx(txChild[i]));
        testPool.addUnchecked(txGrandChild[i].GetHash(), entry.FromTx(txGrandChild[i]));
    }
    // Remove Child[0], GrandChild[0] should be removed:
    poolSize = testPool.size();
    testPool.removeRecursive(txChild[0]);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 2);
    // ... make sure grandchild and child are gone:
    poolSize = testPool.size();
    testPool.removeRecursive(txGrandChild[0]);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    poolSize = testPool.size();
    testPool.removeRecursive(txChild[0]);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    // Remove parent, all children/grandchildren should go:
    poolSize = testPool.size();
    testPool.removeRecursive(txParent);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0);

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        testPool.addUnchecked(txChild[i].GetHash(), entry.FromTx(txChild[i]));
        testPool.addUnchecked(txGrandChild[i].GetHash(), entry.FromTx(txGrandChild[i]));
    }
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    poolSize = testPool.size();
    testPool.removeRecursive(txParent);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
}

template<typename name>
void CheckSort(CTxMemPool &pool, std::vector<std::string> &sortedOrder)
{
    BOOST_CHECK_EQUAL(pool.size(), sortedOrder.size());
    typename CTxMemPool::indexed_transaction_set::index<name>::type::iterator it = pool.mapTx.get<name>().begin();
    int count=0;
    for (; it != pool.mapTx.get<name>().end(); ++it, ++count) {
        BOOST_CHECK_EQUAL(it->GetTx().GetHash().ToString(), sortedOrder[count]);
    }
}

//TODO move these in a common place, now a verbatim copy from transaction_tests.cpp
// contribution = stake + change + fee
CMutableTransaction CreateDummyBuyTicket(const CAmount& contribution, const CAmount& stake, const CAmount& change = ::dustRelayFee.GetFee(GetEstimatedSizeOfBuyTicketTx(false, true)) + 10)
{
    CMutableTransaction mtx;

    // create a dummy input to fund the transaction
    mtx.vin.push_back(CTxIn());

    // create a structured OP_RETURN output containing tx declaration
    BuyTicketData buyTicketData = { 1 };    // version
    CScript declScript = GetScriptForBuyTicketDecl(buyTicketData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // create an output that pays dummy ticket stake
    auto stakeKey = CKey();
    stakeKey.MakeNewKey(false);
    auto stakeAddr = stakeKey.GetPubKey().GetID();
    CScript stakeScript = GetScriptForDestination(stakeAddr);
    CAmount dummyStakeAmount = stake;
    mtx.vout.push_back(CTxOut(dummyStakeAmount, stakeScript));

    // create an OP_RETURN push containing a dummy address to send rewards to, and the amount contributed to stake
    auto rewardKey = CKey();
    rewardKey.MakeNewKey(false);
    auto rewardAddr = rewardKey.GetPubKey().GetID();
    const auto& ticketContribData = TicketContribData{ 1, rewardAddr, contribution, 0, TicketContribData::DefaultFeeLimit };
    CScript contributorInfoScript = GetScriptForTicketContrib(ticketContribData);
    mtx.vout.push_back(CTxOut(0, contributorInfoScript));

    // create an output which pays back dummy change
    auto changeKey = CKey();
    changeKey.MakeNewKey(false);
    auto changeAddr = changeKey.GetPubKey().GetID();
    CScript changeScript = GetScriptForDestination(changeAddr);
    mtx.vout.push_back(CTxOut(change, changeScript));

    return mtx;
}

CMutableTransaction CreateDummyVote(const uint256& blockHashToVoteOn)
{
    CMutableTransaction mtx;

    // create a reward generation input
    mtx.vin.push_back(CTxIn(COutPoint()));

    // create an input from a dummy BuyTicket stake
    uint256 dummyBuyTicketTxHash = uint256();
    mtx.vin.push_back(CTxIn(COutPoint(dummyBuyTicketTxHash, ticketStakeOutputIndex)));

    // create a structured OP_RETURN output containing tx declaration and dummy voting data
    uint32_t dummyBlockHeight = 55;
    VoteBits dummyVoteBits = VoteBits::rttAccepted;
    ExtendedVoteBits dummyExtendedVoteBits;
    VoteData voteData = { 1, blockHashToVoteOn, dummyBlockHeight, dummyVoteBits, defaultVoterStakeVersion, dummyExtendedVoteBits };
    CScript declScript = GetScriptForVoteDecl(voteData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // Create an output which pays back a dummy reward
    auto changeKey = CKey();
    changeKey.MakeNewKey(false);
    auto rewardAddr = changeKey.GetPubKey().GetID();
    CScript rewardScript = GetScriptForDestination(rewardAddr);
    CAmount dummyReward = 60;
    mtx.vout.push_back(CTxOut(dummyReward, rewardScript));

    return mtx;
}

CMutableTransaction CreateDummyRevokeTicket()
{
    CMutableTransaction mtx;

    // create an input from a dummy BuyTicket stake
    uint256 dummyBuyTicketTxHash = uint256();
    mtx.vin.push_back(CTxIn(COutPoint(dummyBuyTicketTxHash, ticketStakeOutputIndex)));

    // create a structured OP_RETURN output containing tx declaration
    RevokeTicketData revokeTicketData = { 1 };
    CScript declScript = GetScriptForRevokeTicketDecl(revokeTicketData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // Create an output which pays back a dummy refund
    auto changeKey = CKey();
    changeKey.MakeNewKey(false);
    auto rewardAddr = changeKey.GetPubKey().GetID();
    CScript rewardScript = GetScriptForDestination(rewardAddr);
    CAmount dummyRefund = 60;
    mtx.vout.push_back(CTxOut(dummyRefund, rewardScript));

    return mtx;
}

BOOST_AUTO_TEST_CASE(MempoolIndexingWithStakeTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx1.GetHash(), entry.Fee(10000LL).FromTx(tx1));

    CMutableTransaction txBuyTicket1 = CreateDummyBuyTicket(10LL, 10LL);    // small contrib, big fee
    pool.addUnchecked(txBuyTicket1.GetHash(), entry.Fee(10000LL).FromTx(txBuyTicket1));

    CMutableTransaction txBuyTicket2 = CreateDummyBuyTicket(10000LL, 10000LL); // big contrib, small fee
    pool.addUnchecked(txBuyTicket2.GetHash(), entry.Fee(10LL).FromTx(txBuyTicket2));

    const auto& blockHashToVoteOn1 = uint256S(std::string("0xabcdef"));
    // first vote on block1
    CMutableTransaction txVote1 = CreateDummyVote(blockHashToVoteOn1);
    pool.addUnchecked(txVote1.GetHash(), entry.Fee(30000LL).FromTx(txVote1));

    // second vote on block1
    CMutableTransaction txVote2 = CreateDummyVote(blockHashToVoteOn1);
    pool.addUnchecked(txVote2.GetHash(), entry.Fee(20000LL).FromTx(txVote2));

    const auto& blockHashToVoteOn2 = uint256S(std::string("0xfedcba"));
    // third vote on block2
    CMutableTransaction txVote3 = CreateDummyVote(blockHashToVoteOn2);
    pool.addUnchecked(txVote3.GetHash(), entry.Fee(10000LL).FromTx(txVote3));

    CMutableTransaction txRevokeTicket = CreateDummyRevokeTicket();
    pool.addUnchecked(txRevokeTicket.GetHash(), entry.Fee(10000LL).FromTx(txRevokeTicket));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx2.vout[0].nValue = 2 * COIN;
    pool.addUnchecked(tx2.GetHash(), entry.Fee(20000LL).FromTx(tx2));

    BOOST_CHECK_EQUAL(pool.size(), 8);

    auto& voted_hash_index = pool.mapTx.get<voted_block_hash>();

    auto votesForBlockHash = voted_hash_index.equal_range(blockHashToVoteOn1);
    for (const auto& tx :boost::make_iterator_range(votesForBlockHash)) {
        BOOST_CHECK_EQUAL(tx.GetTxClass(), ETxClass::TX_Vote);
    }

    BOOST_CHECK_EQUAL(2 ,voted_hash_index.count(blockHashToVoteOn1));
    BOOST_CHECK_EQUAL(1 ,voted_hash_index.count(blockHashToVoteOn2));

    auto& tx_class_index = pool.mapTx.get<tx_class>();
    BOOST_CHECK_EQUAL(3 ,tx_class_index.count(TX_Vote));
    BOOST_CHECK_EQUAL(2 ,tx_class_index.count(TX_BuyTicket));
    BOOST_CHECK_EQUAL(1 ,tx_class_index.count(TX_RevokeTicket));
    BOOST_CHECK_EQUAL(2 ,tx_class_index.count(TX_Regular));

    std::vector<std::string> sortedOrder;
    sortedOrder.resize(8);
    // regular sorted by ancestor score
    sortedOrder[0] = tx2.GetHash().ToString(); // 20000
    sortedOrder[1] = tx1.GetHash().ToString(); // 15000
    // purchases sorted by contribution, not fee
    sortedOrder[2] = txBuyTicket2.GetHash().ToString();    // 10
    sortedOrder[3] = txBuyTicket1.GetHash().ToString();    // 10000
    sortedOrder[4] = txVote1.GetHash().ToString();        // 30000
    sortedOrder[5] = txVote2.GetHash().ToString();        // 20000
    sortedOrder[6] = txVote3.GetHash().ToString();        // 10000
    sortedOrder[7] = txRevokeTicket.GetHash().ToString(); // 10000
    CheckSort<tx_class>(pool, sortedOrder);
}

BOOST_AUTO_TEST_CASE(MempoolIndexingTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;

    /* 3rd highest fee */
    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx1.GetHash(), entry.Fee(10000LL).FromTx(tx1));

    /* highest fee */
    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx2.vout[0].nValue = 2 * COIN;
    pool.addUnchecked(tx2.GetHash(), entry.Fee(20000LL).FromTx(tx2));

    /* lowest fee */
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx3.vout[0].nValue = 5 * COIN;
    pool.addUnchecked(tx3.GetHash(), entry.Fee(0LL).FromTx(tx3));

    /* 2nd highest fee */
    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vout.resize(1);
    tx4.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx4.vout[0].nValue = 6 * COIN;
    pool.addUnchecked(tx4.GetHash(), entry.Fee(15000LL).FromTx(tx4));

    /* equal fee rate to tx1, but newer */
    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vout.resize(1);
    tx5.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx5.vout[0].nValue = 11 * COIN;
    entry.nTime = 1;
    pool.addUnchecked(tx5.GetHash(), entry.Fee(10000LL).FromTx(tx5));
    BOOST_CHECK_EQUAL(pool.size(), 5);

    std::vector<std::string> sortedOrder;
    sortedOrder.resize(5);
    sortedOrder[0] = tx3.GetHash().ToString(); // 0
    sortedOrder[1] = tx5.GetHash().ToString(); // 10000
    sortedOrder[2] = tx1.GetHash().ToString(); // 10000
    sortedOrder[3] = tx4.GetHash().ToString(); // 15000
    sortedOrder[4] = tx2.GetHash().ToString(); // 20000
    CheckSort<descendant_score>(pool, sortedOrder);

    /* low fee but with high fee child */
    /* tx6 -> tx7 -> tx8, tx9 -> tx10 */
    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vout.resize(1);
    tx6.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx6.vout[0].nValue = 20 * COIN;
    pool.addUnchecked(tx6.GetHash(), entry.Fee(0LL).FromTx(tx6));
    BOOST_CHECK_EQUAL(pool.size(), 6);
    // Check that at this point, tx6 is sorted low
    sortedOrder.insert(sortedOrder.begin(), tx6.GetHash().ToString());
    CheckSort<descendant_score>(pool, sortedOrder);

    CTxMemPool::setEntries setAncestors;
    setAncestors.insert(pool.mapTx.find(tx6.GetHash()));
    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(1);
    tx7.vin[0].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_11;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx7.vout[1].nValue = 1 * COIN;

    CTxMemPool::setEntries setAncestorsCalculated;
    std::string dummy;
    BOOST_CHECK_EQUAL(pool.CalculateMemPoolAncestors(entry.Fee(2000000LL).FromTx(tx7), setAncestorsCalculated, 100, 1000000, 1000, 1000000, dummy), true);
    BOOST_CHECK(setAncestorsCalculated == setAncestors);

    pool.addUnchecked(tx7.GetHash(), entry.FromTx(tx7), setAncestors);
    BOOST_CHECK_EQUAL(pool.size(), 7);

    // Now tx6 should be sorted higher (high fee child): tx7, tx6, tx2, ...
    sortedOrder.erase(sortedOrder.begin());
    sortedOrder.push_back(tx6.GetHash().ToString());
    sortedOrder.push_back(tx7.GetHash().ToString());
    CheckSort<descendant_score>(pool, sortedOrder);

    /* low fee child of tx7 */
    CMutableTransaction tx8 = CMutableTransaction();
    tx8.vin.resize(1);
    tx8.vin[0].prevout = COutPoint(tx7.GetHash(), 0);
    tx8.vin[0].scriptSig = CScript() << OP_11;
    tx8.vout.resize(1);
    tx8.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx8.vout[0].nValue = 10 * COIN;
    setAncestors.insert(pool.mapTx.find(tx7.GetHash()));
    pool.addUnchecked(tx8.GetHash(), entry.Fee(0LL).Time(2).FromTx(tx8), setAncestors);

    // Now tx8 should be sorted low, but tx6/tx both high
    sortedOrder.insert(sortedOrder.begin(), tx8.GetHash().ToString());
    CheckSort<descendant_score>(pool, sortedOrder);

    /* low fee child of tx7 */
    CMutableTransaction tx9 = CMutableTransaction();
    tx9.vin.resize(1);
    tx9.vin[0].prevout = COutPoint(tx7.GetHash(), 1);
    tx9.vin[0].scriptSig = CScript() << OP_11;
    tx9.vout.resize(1);
    tx9.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx9.vout[0].nValue = 1 * COIN;
    pool.addUnchecked(tx9.GetHash(), entry.Fee(0LL).Time(3).FromTx(tx9), setAncestors);

    // tx9 should be sorted low
    BOOST_CHECK_EQUAL(pool.size(), 9);
    sortedOrder.insert(sortedOrder.begin(), tx9.GetHash().ToString());
    CheckSort<descendant_score>(pool, sortedOrder);

    std::vector<std::string> snapshotOrder = sortedOrder;

    setAncestors.insert(pool.mapTx.find(tx8.GetHash()));
    setAncestors.insert(pool.mapTx.find(tx9.GetHash()));
    /* tx10 depends on tx8 and tx9 and has a high fee*/
    CMutableTransaction tx10 = CMutableTransaction();
    tx10.vin.resize(2);
    tx10.vin[0].prevout = COutPoint(tx8.GetHash(), 0);
    tx10.vin[0].scriptSig = CScript() << OP_11;
    tx10.vin[1].prevout = COutPoint(tx9.GetHash(), 0);
    tx10.vin[1].scriptSig = CScript() << OP_11;
    tx10.vout.resize(1);
    tx10.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx10.vout[0].nValue = 10 * COIN;

    setAncestorsCalculated.clear();
    BOOST_CHECK_EQUAL(pool.CalculateMemPoolAncestors(entry.Fee(200000LL).Time(4).FromTx(tx10), setAncestorsCalculated, 100, 1000000, 1000, 1000000, dummy), true);
    BOOST_CHECK(setAncestorsCalculated == setAncestors);

    pool.addUnchecked(tx10.GetHash(), entry.FromTx(tx10), setAncestors);

    /**
     *  tx8 and tx9 should both now be sorted higher
     *  Final order after tx10 is added:
     *
     *  tx3 = 0 (1)
     *  tx5 = 10000 (1)
     *  tx1 = 10000 (1)
     *  tx4 = 15000 (1)
     *  tx2 = 20000 (1)
     *  tx9 = 200k (2 txs)
     *  tx8 = 200k (2 txs)
     *  tx10 = 200k (1 tx)
     *  tx6 = 2.2M (5 txs)
     *  tx7 = 2.2M (4 txs)
     */
    sortedOrder.erase(sortedOrder.begin(), sortedOrder.begin()+2); // take out tx9, tx8 from the beginning
    sortedOrder.insert(sortedOrder.begin()+5, tx9.GetHash().ToString());
    sortedOrder.insert(sortedOrder.begin()+6, tx8.GetHash().ToString());
    sortedOrder.insert(sortedOrder.begin()+7, tx10.GetHash().ToString()); // tx10 is just before tx6
    CheckSort<descendant_score>(pool, sortedOrder);

    // there should be 10 transactions in the mempool
    BOOST_CHECK_EQUAL(pool.size(), 10);

    // Now try removing tx10 and verify the sort order returns to normal
    pool.removeRecursive(pool.mapTx.find(tx10.GetHash())->GetTx());
    CheckSort<descendant_score>(pool, snapshotOrder);

    pool.removeRecursive(pool.mapTx.find(tx9.GetHash())->GetTx());
    pool.removeRecursive(pool.mapTx.find(tx8.GetHash())->GetTx());
    /* Now check the sort on the mining score index.
     * Final order should be:
     *
     * tx7 (2M)
     * tx2 (20k)
     * tx4 (15000)
     * tx1/tx5 (10000)
     * tx3/6 (0)
     * (Ties resolved by hash)
     */
    sortedOrder.clear();
    sortedOrder.push_back(tx7.GetHash().ToString());
    sortedOrder.push_back(tx2.GetHash().ToString());
    sortedOrder.push_back(tx4.GetHash().ToString());
    if (tx1.GetHash() < tx5.GetHash()) {
        sortedOrder.push_back(tx5.GetHash().ToString());
        sortedOrder.push_back(tx1.GetHash().ToString());
    } else {
        sortedOrder.push_back(tx1.GetHash().ToString());
        sortedOrder.push_back(tx5.GetHash().ToString());
    }
    if (tx3.GetHash() < tx6.GetHash()) {
        sortedOrder.push_back(tx6.GetHash().ToString());
        sortedOrder.push_back(tx3.GetHash().ToString());
    } else {
        sortedOrder.push_back(tx3.GetHash().ToString());
        sortedOrder.push_back(tx6.GetHash().ToString());
    }
    CheckSort<mining_score>(pool, sortedOrder);
}

BOOST_AUTO_TEST_CASE(MempoolAncestorIndexingTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;

    /* 3rd highest fee */
    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx1.GetHash(), entry.Fee(10000LL).FromTx(tx1));

    /* highest fee */
    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx2.vout[0].nValue = 2 * COIN;
    pool.addUnchecked(tx2.GetHash(), entry.Fee(20000LL).FromTx(tx2));
    uint64_t tx2Size = GetVirtualTransactionSize(tx2);

    /* lowest fee */
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx3.vout[0].nValue = 5 * COIN;
    pool.addUnchecked(tx3.GetHash(), entry.Fee(0LL).FromTx(tx3));

    /* 2nd highest fee */
    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vout.resize(1);
    tx4.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx4.vout[0].nValue = 6 * COIN;
    pool.addUnchecked(tx4.GetHash(), entry.Fee(15000LL).FromTx(tx4));

    /* equal fee rate to tx1, but newer */
    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vout.resize(1);
    tx5.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx5.vout[0].nValue = 11 * COIN;
    pool.addUnchecked(tx5.GetHash(), entry.Fee(10000LL).FromTx(tx5));
    BOOST_CHECK_EQUAL(pool.size(), 5);

    std::vector<std::string> sortedOrder;
    sortedOrder.resize(5);
    sortedOrder[0] = tx2.GetHash().ToString(); // 20000
    sortedOrder[1] = tx4.GetHash().ToString(); // 15000
    // tx1 and tx5 are both 10000
    // Ties are broken by hash, not timestamp, so determine which
    // hash comes first.
    if (tx1.GetHash() < tx5.GetHash()) {
        sortedOrder[2] = tx1.GetHash().ToString();
        sortedOrder[3] = tx5.GetHash().ToString();
    } else {
        sortedOrder[2] = tx5.GetHash().ToString();
        sortedOrder[3] = tx1.GetHash().ToString();
    }
    sortedOrder[4] = tx3.GetHash().ToString(); // 0

    CheckSort<ancestor_score>(pool, sortedOrder);

    /* low fee parent with high fee child */
    /* tx6 (0) -> tx7 (high) */
    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vout.resize(1);
    tx6.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx6.vout[0].nValue = 20 * COIN;
    uint64_t tx6Size = GetVirtualTransactionSize(tx6);

    pool.addUnchecked(tx6.GetHash(), entry.Fee(0LL).FromTx(tx6));
    BOOST_CHECK_EQUAL(pool.size(), 6);
    // Ties are broken by hash
    if (tx3.GetHash() < tx6.GetHash())
        sortedOrder.push_back(tx6.GetHash().ToString());
    else
        sortedOrder.insert(sortedOrder.end()-1,tx6.GetHash().ToString());

    CheckSort<ancestor_score>(pool, sortedOrder);

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(1);
    tx7.vin[0].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_11;
    tx7.vout.resize(1);
    tx7.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    uint64_t tx7Size = GetVirtualTransactionSize(tx7);

    /* set the fee to just below tx2's feerate when including ancestor */
    CAmount fee = (20000/tx2Size)*(tx7Size + tx6Size) - 1;

    pool.addUnchecked(tx7.GetHash(), entry.Fee(fee).FromTx(tx7));
    BOOST_CHECK_EQUAL(pool.size(), 7);
    sortedOrder.insert(sortedOrder.begin()+1, tx7.GetHash().ToString());
    CheckSort<ancestor_score>(pool, sortedOrder);

    /* after tx6 is mined, tx7 should move up in the sort */
    std::vector<CTransactionRef> vtx;
    vtx.push_back(MakeTransactionRef(tx6));
    pool.removeForBlock(vtx, 1);

    sortedOrder.erase(sortedOrder.begin()+1);
    // Ties are broken by hash
    if (tx3.GetHash() < tx6.GetHash())
        sortedOrder.pop_back();
    else
        sortedOrder.erase(sortedOrder.end()-2);
    sortedOrder.insert(sortedOrder.begin(), tx7.GetHash().ToString());
    CheckSort<ancestor_score>(pool, sortedOrder);
}


BOOST_AUTO_TEST_CASE(MempoolSizeLimitTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx1.GetHash(), entry.Fee(10000LL).FromTx(tx1));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_2;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_2 << OP_EQUAL;
    tx2.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx2.GetHash(), entry.Fee(5000LL).FromTx(tx2));

    pool.TrimToSize(pool.DynamicMemoryUsage()); // should do nothing
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // should remove the lower-feerate transaction
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));

    pool.addUnchecked(tx2.GetHash(), entry.FromTx(tx2));
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vin.resize(1);
    tx3.vin[0].prevout = COutPoint(tx2.GetHash(), 0);
    tx3.vin[0].scriptSig = CScript() << OP_2;
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_3 << OP_EQUAL;
    tx3.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(tx3.GetHash(), entry.Fee(20000LL).FromTx(tx3));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // tx3 should pay for tx2 (CPFP)
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));
    BOOST_CHECK(pool.exists(tx3.GetHash()));

    pool.TrimToSize(GetVirtualTransactionSize(tx1)); // mempool is limited to tx1's size in memory usage, so nothing fits
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));
    BOOST_CHECK(!pool.exists(tx3.GetHash()));

    CFeeRate maxFeeRateRemoved(25000, GetVirtualTransactionSize(tx3) + GetVirtualTransactionSize(tx2));
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + 1000);

    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vin.resize(2);
    tx4.vin[0].prevout.SetNull();
    tx4.vin[0].scriptSig = CScript() << OP_4;
    tx4.vin[1].prevout.SetNull();
    tx4.vin[1].scriptSig = CScript() << OP_4;
    tx4.vout.resize(2);
    tx4.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[0].nValue = 10 * COIN;
    tx4.vout[1].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vin.resize(2);
    tx5.vin[0].prevout = COutPoint(tx4.GetHash(), 0);
    tx5.vin[0].scriptSig = CScript() << OP_4;
    tx5.vin[1].prevout.SetNull();
    tx5.vin[1].scriptSig = CScript() << OP_5;
    tx5.vout.resize(2);
    tx5.vout[0].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[0].nValue = 10 * COIN;
    tx5.vout[1].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vin.resize(2);
    tx6.vin[0].prevout = COutPoint(tx4.GetHash(), 1);
    tx6.vin[0].scriptSig = CScript() << OP_4;
    tx6.vin[1].prevout.SetNull();
    tx6.vin[1].scriptSig = CScript() << OP_6;
    tx6.vout.resize(2);
    tx6.vout[0].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[0].nValue = 10 * COIN;
    tx6.vout[1].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(2);
    tx7.vin[0].prevout = COutPoint(tx5.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_5;
    tx7.vin[1].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[1].scriptSig = CScript() << OP_6;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[1].nValue = 10 * COIN;

    pool.addUnchecked(tx4.GetHash(), entry.Fee(7000LL).FromTx(tx4));
    pool.addUnchecked(tx5.GetHash(), entry.Fee(1000LL).FromTx(tx5));
    pool.addUnchecked(tx6.GetHash(), entry.Fee(1100LL).FromTx(tx6));
    pool.addUnchecked(tx7.GetHash(), entry.Fee(9000LL).FromTx(tx7));

    // we only require this remove, at max, 2 txn, because its not clear what we're really optimizing for aside from that
    pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(pool.exists(tx6.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    if (!pool.exists(tx5.GetHash()))
        pool.addUnchecked(tx5.GetHash(), entry.Fee(1000LL).FromTx(tx5));
    pool.addUnchecked(tx7.GetHash(), entry.Fee(9000LL).FromTx(tx7));

    pool.TrimToSize(pool.DynamicMemoryUsage() / 2); // should maximize mempool size by only removing 5/7
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx5.GetHash()));
    BOOST_CHECK(pool.exists(tx6.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    pool.addUnchecked(tx5.GetHash(), entry.Fee(1000LL).FromTx(tx5));
    pool.addUnchecked(tx7.GetHash(), entry.Fee(9000LL).FromTx(tx7));

    std::vector<CTransactionRef> vtx;
    SetMockTime(42);
    SetMockTime(42 + CTxMemPool::ROLLING_FEE_HALFLIFE);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + 1000);
    // ... we should keep the same min fee until we get a block
    pool.removeForBlock(vtx, 1);
    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + 1000)/2.0));
    // ... then feerate should drop 1/2 each halflife

    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2);
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 5 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + 1000)/4.0));
    // ... with a 1/2 halflife when mempool is < 1/2 its target size

    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 9 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + 1000)/8.0));
    // ... with a 1/4 halflife when mempool is < 1/4 its target size

    SetMockTime(42 + 7*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), 1000);
    // ... but feerate should never drop below 1000

    SetMockTime(42 + 8*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), 0);
    // ... unless it has gone all the way to 0 (after getting past 1000/2)

    SetMockTime(0);
}

BOOST_AUTO_TEST_CASE(MempoolPersistenceTest)
{
    auto signTicket = [](CMutableTransaction& mtx, CKey& key, const CAmount& contribution, const CAmount& change, const CScript& scriptPubKey) {
        CBasicKeyStore keyStore;
        keyStore.AddKey(key);

        CTransaction tx(mtx);

        SignatureData sigdata;
        BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keyStore, &tx, 0, contribution+change, SIGHASH_ALL), scriptPubKey, sigdata));
        UpdateTransaction(mtx, 0, sigdata);
    };

    auto checkExpiry = [](const CTxMemPool& pool, const uint32_t expectedExpiryForOldtx, const uint32_t expectedExpiryForNewtx) {
        auto& tx_class_index = pool.mapTx.get<tx_class>();
        auto tickets = tx_class_index.equal_range(ETxClass::TX_BuyTicket);
        for (auto tickettxiter = tickets.first; tickettxiter != tickets.second; ++tickettxiter) {
            if (tickettxiter->GetSharedTx()->nVersion == 1)
                BOOST_CHECK(tickettxiter->GetSharedTx()->nExpiry == expectedExpiryForOldtx);
            if (tickettxiter->GetSharedTx()->nVersion == 3)
                BOOST_CHECK(tickettxiter->GetSharedTx()->nExpiry == expectedExpiryForNewtx);
        }
    };

    const CAmount stake = 2*COIN;

    const CAmount change1 = 0;
    const CAmount fee1 = ::minRelayTxFee.GetFee(GetEstimatedSizeOfBuyTicketTx(false, false)) + 10;
    const CAmount contribution1 = stake + change1 + fee1;

    const CAmount change2 = 0;
    const CAmount fee2 = ::minRelayTxFee.GetFee(GetEstimatedSizeOfBuyTicketTx(false, true)) + 10;
    const CAmount contribution2 = stake + change2 + fee2;

    CKey key1;
    key1.MakeNewKey(true);
    CPubKey pubKey1 = key1.GetPubKey();
    const CScript& scriptPubKey1 = GetScriptForDestination(pubKey1.GetID());

    CKey key2;
    key2.MakeNewKey(true);
    CPubKey pubKey2 = key2.GetPubKey();
    const CScript& scriptPubKey2 = GetScriptForDestination(pubKey2.GetID());

    COutPoint out1(uint256S("1"), 0);
    COutPoint out2(uint256S("2"), 0);

    Coin coin1(CTxOut(contribution1, scriptPubKey1), 0, false, TX_Regular);
    Coin coin2(CTxOut(contribution2, scriptPubKey2), 0, false, TX_Regular);

    LOCK(cs_main);

    pcoinsTip->AddCoin(out1, std::move(coin1), true);
    BOOST_CHECK(pcoinsTip->HaveCoin(out1));

    pcoinsTip->AddCoin(out2, std::move(coin2), true);
    BOOST_CHECK(pcoinsTip->HaveCoin(out2));

    CMutableTransaction tx1 = CreateDummyBuyTicket(contribution1, stake, change1);
    tx1.vin.clear();
    tx1.vin.push_back(CTxIn(out1));
    tx1.nVersion = 1;
    tx1.nExpiry = 123;
    signTicket(tx1, key1, contribution1, change1, scriptPubKey1);
    CTransactionRef txr1 = MakeTransactionRef(tx1);

    CMutableTransaction tx2 = CreateDummyBuyTicket(contribution2, stake, change2);
    tx2.vin.clear();
    tx2.vin.push_back(CTxIn(out2));
    tx2.nExpiry = 456;
    signTicket(tx2, key2, contribution2, change2, scriptPubKey2);
    CTransactionRef txr2 = MakeTransactionRef(tx2);

    CValidationState state;
    BOOST_CHECK(AcceptToMemoryPool(mempool, state, txr1, nullptr, nullptr, false, 0));
    BOOST_CHECK(AcceptToMemoryPool(mempool, state, txr2, nullptr, nullptr, false, 0));

    checkExpiry(mempool, 123, 456);

    BOOST_CHECK(DumpMempool());

    mempool.removeRecursive(tx1);
    mempool.removeRecursive(tx2);

    BOOST_CHECK(mempool.size() == 0);

    BOOST_CHECK(LoadMempool());

    checkExpiry(mempool, 0, 456);
}

BOOST_AUTO_TEST_CASE(MempoolMalleabilityTest)
{
    auto signTicket = [](CMutableTransaction& mtx, CKey& key, const CAmount& contribution, const CAmount& change, const CScript& scriptPubKey) {
        CBasicKeyStore keyStore;
        keyStore.AddKey(key);

        CTransaction tx(mtx);

        SignatureData sigdata;
        BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keyStore, &tx, 0, contribution+change, SIGHASH_ALL), scriptPubKey, sigdata));
        UpdateTransaction(mtx, 0, sigdata);
    };

    auto checkExpiry = [](const CTxMemPool& pool, const uint32_t expectedExpiryForOldtx, const uint32_t expectedExpiryForNewtx) {
        auto& tx_class_index = pool.mapTx.get<tx_class>();
        auto tickets = tx_class_index.equal_range(ETxClass::TX_BuyTicket);
        for (auto tickettxiter = tickets.first; tickettxiter != tickets.second; ++tickettxiter) {
            if (tickettxiter->GetSharedTx()->nVersion == 1)
                BOOST_CHECK(tickettxiter->GetSharedTx()->nExpiry == expectedExpiryForOldtx);
            if (tickettxiter->GetSharedTx()->nVersion == 3)
                BOOST_CHECK(tickettxiter->GetSharedTx()->nExpiry == expectedExpiryForNewtx);
        }
    };

    const CAmount stake = 2*COIN;
    const CAmount change = 0;
    const CAmount fee = ::minRelayTxFee.GetFee(GetEstimatedSizeOfBuyTicketTx(false, true)) + 10;
    const CAmount contribution = stake + change + fee;

    CKey key;
    key.MakeNewKey(true);
    CPubKey pubKey = key.GetPubKey();
    const CScript& scriptPubKey = GetScriptForDestination(pubKey.GetID());

    COutPoint out(uint256S("1"), 0);

    Coin coin1(CTxOut(contribution, scriptPubKey), 0, false, TX_Regular);

    LOCK(cs_main);

    pcoinsTip->AddCoin(out, std::move(coin1), true);
    BOOST_CHECK(pcoinsTip->HaveCoin(out));

    CMutableTransaction tx1 = CreateDummyBuyTicket(contribution, stake, change);
    tx1.vin.clear();
    tx1.vin.push_back(CTxIn(out));
    tx1.nVersion = 3;
    tx1.nLockTime = 0;
    tx1.nExpiry = 123;
    signTicket(tx1, key, contribution, change, scriptPubKey);
    CTransactionRef txr1 = MakeTransactionRef(tx1);

    CValidationState state;

    BOOST_CHECK(AcceptToMemoryPool(mempool, state, txr1, nullptr, nullptr, false, 0));

    checkExpiry(mempool, 0, 123);

    BOOST_CHECK(!AcceptToMemoryPool(mempool, state, txr1, nullptr, nullptr, false, 0) && state.GetRejectCode() == REJECT_DUPLICATE && state.GetRejectReason() == "txn-already-in-mempool");

    checkExpiry(mempool, 0, 123);

    CMutableTransaction tx2 = tx1;
    CTransactionRef txr2 = MakeTransactionRef(tx2);

    BOOST_CHECK(txr1->GetHash() == txr2->GetHash());

    BOOST_CHECK(!AcceptToMemoryPool(mempool, state, txr2, nullptr, nullptr, false, 0) && state.GetRejectCode() == REJECT_DUPLICATE && state.GetRejectReason() == "txn-already-in-mempool");

    checkExpiry(mempool, 0, 123);

    tx2.nExpiry = 456;

    signTicket(tx2, key, contribution, change, scriptPubKey);
    CTransactionRef txr21 = MakeTransactionRef(tx2);

    BOOST_CHECK(txr1->GetHash() != txr21->GetHash());

    BOOST_CHECK(!AcceptToMemoryPool(mempool, state, txr21, nullptr, nullptr, false, 0) && state.GetRejectCode() == REJECT_DUPLICATE && state.GetRejectReason() == "txn-mempool-conflict");

    checkExpiry(mempool, 0, 123);
}

BOOST_AUTO_TEST_SUITE_END()
