//
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 Project PAI Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//


#include "test_bwscoin.h"

#include "chainparams.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"
#include "crypto/sha256.h"
#include "fs.h"
#include "key.h"
#include "validation.h"
#include "miner.h"
#include "net_processing.h"
#include "pubkey.h"
#include "random.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "streams.h"
#include "rpc/server.h"
#include "rpc/register.h"
#include "script/sigcache.h"

#include <memory>

uint256 insecure_rand_seed = GetRandHash();
FastRandomContext insecure_rand_ctx(insecure_rand_seed);

extern bool fPrintToConsole;
extern void noui_connect();

BasicTestingSetup::BasicTestingSetup(const std::string& chainName)
{
        SHA256AutoDetect();
        RandomInit();
        ECC_Start();
        SetupEnvironment();
        SetupNetworking();
        InitSignatureCache();
        InitScriptExecutionCache();
        fPrintToDebugLog = false; // don't want to write to debug.log file
        fCheckBlockIndex = true;
        SelectParams(chainName);
        noui_connect();
}

BasicTestingSetup::~BasicTestingSetup()
{
        ECC_Stop();
}

TestingSetup::TestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    const CChainParams& chainparams = Params();
        // Ideally we'd move all the RPC tests to the functional testing framework
        // instead of unit tests, but for now we need these here.

        RegisterAllCoreRPCCommands(tableRPC);
        ClearDatadirCache();
        pathTemp = fs::temp_directory_path() / strprintf("test_bwscoin_%lu_%i", (unsigned long)GetTime(), (int)(InsecureRandRange(100000)));
        fs::create_directories(pathTemp);
        gArgs.ForceSetArg("-datadir", pathTemp.string());

        // Note that because we don't bother running a scheduler thread here,
        // callbacks via CValidationInterface are unreliable, but that's OK,
        // our unit tests aren't testing multiple parts of the code at once.
        GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);

        mempool.setSanityCheck(1.0);
        pblocktree = new CBlockTreeDB(1 << 20, true);
        pcoinsdbview = new CCoinsViewDB(1 << 23, true);
        pcoinsTip = new CCoinsViewCache(pcoinsdbview);
        if (!LoadGenesisBlock(chainparams)) {
            throw std::runtime_error("LoadGenesisBlock failed.");
        }
        {
            CValidationState state;
            if (!ActivateBestChain(state, chainparams)) {
                throw std::runtime_error("ActivateBestChain failed.");
            }
        }
        nScriptCheckThreads = 3;
        for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
        g_connman = std::unique_ptr<CConnman>(new CConnman(0x1337, 0x1337)); // Deterministic randomness for tests.
        connman = g_connman.get();
        peerLogic.reset(new PeerLogicValidation(connman));
}

TestingSetup::~TestingSetup()
{
        threadGroup.interrupt_all();
        threadGroup.join_all();
        GetMainSignals().FlushBackgroundCallbacks();
        GetMainSignals().UnregisterBackgroundSignalScheduler();
        g_connman.reset();
        peerLogic.reset();
        UnloadBlockIndex();
        delete pcoinsTip;
        delete pcoinsdbview;
        delete pblocktree;
        fs::remove_all(pathTemp);
}

TestChain100Setup::TestChain100Setup(scriptPubKeyType pkType) : TestingSetup(CBaseChainParams::REGTEST)
{
    CScript scriptPubKey = CScript();
    coinbaseKey.MakeNewKey(true);

    switch (pkType)
    {
    case scriptPubKeyType::NoKey:
        // do nothing use the empty script
        break;

    case scriptPubKeyType::P2PK:
        scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
        break;

    case scriptPubKeyType::P2PKH:
        scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(coinbaseKey.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
        break;

    default:
        assert(!"Unknown scriptPubKeyType");
        break;
    }

    // Generate a 100-block chain:
    for (int i = 0; i < COINBASE_MATURITY; i++)
    {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        coinbaseTxns.push_back(*b.vtx[0]);
    }
}

//
// Create a new block with just given transactions, coinbase paying to
// scriptPubKey, and try to add it to the current chain.
//
CBlock
TestChain100Setup::CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    CBlock& block = pblocktemplate->block;

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    for (const CMutableTransaction& tx : txns)
        block.vtx.push_back(MakeTransactionRef(tx));
    // IncrementExtraNonce creates a valid coinbase and merkleRoot
    unsigned int extraNonce = 0;
    IncrementExtraNonce(&block, chainActive.Tip(), extraNonce);

    while (!CheckProofOfWork(block, chainparams.GetConsensus())) ++block.nNonce;

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    ProcessNewBlock(chainparams, shared_pblock, true, nullptr);

    CBlock result = block;
    return result;
}

TestChain100Setup::~TestChain100Setup()
{
}


Generator::Generator(const std::string& chainName)
: TestingSetup(chainName)
, coinbaseKey{ [](){
    auto key = CKey();
    key.MakeNewKey(true/*compressed as in TestChain100Setup*/);
    return key;}()
  }
, stakeKey{coinbaseKey}, rewardKey{coinbaseKey}, changeKey{coinbaseKey}
, coinbaseAddr{coinbaseKey.GetPubKey().GetID()},stakeAddr{coinbaseAddr}, rewardAddr{coinbaseAddr}, changeAddr{coinbaseAddr}
, coinbaseScript{GetScriptForDestination(coinbaseAddr)}, stakeScript{coinbaseScript}, rewardScript{coinbaseScript}, changeScript{coinbaseScript}
{
    tipName = "genesis"; // genesis block created in TestingSetup
    // coinbaseKey.MakeNewKey(true);
    // coinbaseAddr = coinbaseKey.GetPubKey().GetID();
    // coinbaseScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(coinbaseAddr) << OP_EQUALVERIFY << OP_CHECKSIG;
    // use same key, address and script
    // stakeKey = rewardKey = changeKey = coinbaseKey;
    // stakeAddr = rewardAddr = changeAddr = p2shOpTrueAddr;
    // stakeScript = rewardScript = changeScript = coinbaseScript = p2shOpTrueScript;
}

Generator::~Generator()
{
    ;
}

void Generator::SignTx(CMutableTransaction& tx, unsigned int nIn, const CScript& script, const CKey& key) const
{
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(script, tx, nIn, SIGHASH_ALL, 0, SIGVERSION_BASE);
    key.Sign(hash, vchSig);
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    tx.vin[nIn].scriptSig << vchSig << ToByteVector(key.GetPubKey());
}

CMutableTransaction Generator::CreateTicketPurchaseTx(const SpendableOut& spend, const CAmount& ticketPrice, const CAmount& fee)
{
    CMutableTransaction mtx;
    // create input to fund the transaction
    const auto& txin = CTxIn(spend.prevOut.hash, spend.prevOut.n);
    mtx.vin.push_back(txin);

    // create a structured OP_RETURN output containing tx declaration
    BuyTicketData buyTicketData = { 1 };    // version
    CScript declScript = GetScriptForBuyTicketDecl(buyTicketData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // create an output that pays ticket stake
    mtx.vout.push_back(CTxOut(ticketPrice, stakeScript));

    // create an OP_RETURN push containing a dummy address to send rewards to, and the amount contributed to stake
    const auto& ticketContribData = TicketContribData{ 1, rewardAddr, ticketPrice + fee, 0, TicketContribData::DefaultFeeLimit };
    CScript contributorInfoScript = GetScriptForTicketContrib(ticketContribData);
    mtx.vout.push_back(CTxOut(0, contributorInfoScript));

    // create an output which pays back change
    CAmount change = spend.amount - ticketPrice - fee;
    assert(change >= 0);
    mtx.vout.push_back(CTxOut(change, changeScript));

    SignTx(mtx, 0, coinbaseScript, coinbaseKey);

    boughtTicketHashToPrice[mtx.GetHash()] = ticketPrice;

    return mtx;
}

CMutableTransaction Generator::CreateVoteTx(const uint256& voteBlockHash, int voteBlockHeight, const uint256& ticketTxHash, VoteBits voteBits) const
{
    CMutableTransaction mtx;

    const auto& voterSubsidy = GetVoterSubsidy(voteBlockHeight+1/*spend height*/,Params().GetConsensus());
    const auto& ticketPrice  = boughtTicketHashToPrice.at(ticketTxHash);
    const auto& contributedAmount = ticketPrice + 2 /*fee*/;
    const auto& reward = CalculateGrossRemuneration( contributedAmount, ticketPrice, voterSubsidy, contributedAmount);
    // create a reward generation input
    mtx.vin.push_back(CTxIn(COutPoint(), Params().GetConsensus().stakeBaseSigScript));

    mtx.vin.push_back(CTxIn(COutPoint(ticketTxHash, ticketStakeOutputIndex)));

    // create a structured OP_RETURN output containing tx declaration and voting data
    int voteVersion = 1;
    ExtendedVoteBits extendedVoteBits;
    VoteData voteData = { voteVersion, voteBlockHash, static_cast<uint32_t>(voteBlockHeight), voteBits, defaultVoterStakeVersion, extendedVoteBits };
    CScript declScript = GetScriptForVoteDecl(voteData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // Create an output which pays back a reward
    mtx.vout.push_back(CTxOut(reward, rewardScript));

    SignTx(mtx, voteStakeInputIndex, stakeScript, coinbaseKey);

    return mtx;
}

CMutableTransaction Generator::CreateRevocationTx(const uint256& ticketTxHash) const
{
    CMutableTransaction mtx;

    // create an input from the purchased ticket stake
    mtx.vin.push_back(CTxIn(COutPoint(ticketTxHash, ticketStakeOutputIndex)));

    // create a structured OP_RETURN output containing tx declaration
    RevokeTicketData revokeTicketData = { 1 };
    CScript declScript = GetScriptForRevokeTicketDecl(revokeTicketData);
    mtx.vout.push_back(CTxOut(0, declScript));

    // Create an output which pays back ticket price as a refund
    const auto& ticketPrice  = boughtTicketHashToPrice.at(ticketTxHash);
    mtx.vout.push_back(CTxOut(ticketPrice, rewardScript));

    SignTx(mtx, revocationStakeInputIndex, stakeScript, coinbaseKey);

    return mtx;
}

CMutableTransaction Generator::CreateSpendTx(const SpendableOut& spend, const CAmount& fee) const
{
    CMutableTransaction mtx;
    // create input to fund the transaction
    const auto& txin = CTxIn(spend.prevOut.hash, spend.prevOut.n);
    mtx.vin.push_back(txin);

    // create an output that spends all
    mtx.vout.push_back(CTxOut(spend.amount - fee, rewardScript));

    SignTx(mtx, spend.prevOut.n, coinbaseScript, coinbaseKey);

    return mtx;
}

CMutableTransaction Generator::CreateSplitSpendTx(const SpendableOut& spend, const std::vector<CAmount>& payments, const CAmount& fee) const
{
    CMutableTransaction mtx;
    // create input to fund the transaction
    const auto& txin = CTxIn(spend.prevOut.hash, spend.prevOut.n);
    mtx.vin.push_back(txin);

    // create an output that spends all
    auto sum = CAmount(0);
    for (const auto& payment: payments){
        mtx.vout.push_back(CTxOut(payment, rewardScript));
        sum += payment;
    }

    mtx.vout.push_back(CTxOut(spend.amount - sum - fee, changeScript));

    SignTx(mtx, spend.prevOut.n, coinbaseScript, coinbaseKey);

    return mtx;
}

void Generator::SaveCoinbaseOut(const CBlock& b)
{
    SaveSpendableOuts(b, 0UL /*coinbase tx*/, {0UL});
}

void Generator::SaveSpendableOuts(const CBlock& b, uint32_t indexBlock, const std::vector<uint32_t>& indicesTxOut)
{
    const auto& tx = b.vtx[indexBlock];
    auto outs = std::list<SpendableOut>{};
    for (const auto& indexTxOut : indicesTxOut) {
        outs.push_back(MakeSpendableOut(*tx, indexTxOut));
    }
    spendableOuts.push_back(outs);
}

void Generator::SaveAllSpendableOuts(const CBlock& b)
{
    auto outs = std::list<SpendableOut>{};
    for (const auto& tx : b.vtx ) {
        if (tx->IsCoinBase()) {
            outs.push_back(MakeSpendableOut(*tx, 0UL));
        }
        else {
            // we assume that these outputs are not spent inside the same block
            const auto& txClass = ParseTxClass(*tx);
            switch(txClass){
            case TX_Regular:
                for (size_t i = 0; i < tx->vout.size(); ++i) {
                    outs.push_back(MakeSpendableOut(*tx, i));
                }
                break;
            case TX_BuyTicket:
                outs.push_back(MakeSpendableOut(*tx, ticketChangeOutputIndex));
                break;
            case TX_RevokeTicket:
                outs.push_back(MakeSpendableOut(*tx, revocationRefundOutputIndex));
                break;
            }
        }
    }
    spendableOuts.push_back(outs);
}

Generator::SpendableOut Generator::MakeSpendableOut(const CTransaction& tx, uint32_t indexOut) const
{
    return Generator::SpendableOut{COutPoint{tx.GetHash(), indexOut}, Tip()->nHeight, tx.vout[indexOut].nValue};
}

std::list<Generator::SpendableOut> Generator::OldestCoinOuts()
{
    if (spendableOuts.size() == 0)
        return std::list<Generator::SpendableOut>{};
    const auto oldest = spendableOuts.front();
    spendableOuts.pop_front();
    return oldest;
}

const CBlockIndex* Generator::Tip() const
{
    const auto& tip = chainActive.Tip();
    return tip;
}

const Consensus::Params& Generator::ConsensusParams() const
{
    return Params().GetConsensus();
}

CAmount Generator::NextRequiredStakeDifficulty() const
{
    CBlock dummyBlock;
    const auto& ticketPrice = CalculateNextRequiredStakeDifficulty(chainActive.Tip(), Params().GetConsensus());
    return ticketPrice;
}

void Generator::ReplaceVoteBits(CTransactionRef& tx, VoteBits voteBits) const
{
    // Regenerate vote tx using the same hash/height, but change the voteBits
    CMutableTransaction voteTx = *tx;
    assert(ParseTxClass(voteTx)==TX_Vote);
    const auto& ticketHash = voteTx.vin[voteStakeInputIndex].prevout.hash;
    voteTx = CreateVoteTx(Tip()->GetBlockHash(), Tip()->nHeight, ticketHash, voteBits);
    tx = MakeTransactionRef(voteTx);
}

void Generator::ReplaceStakeBaseSigScript(CTransactionRef& tx, const CScript& sigScript) const
{
    CMutableTransaction voteTx = *tx;
    assert(ParseTxClass(voteTx)==TX_Vote);
    voteTx.vin[voteSubsidyInputIndex].scriptSig = sigScript;
    tx = MakeTransactionRef(voteTx);
}

CScript Generator::RepeatOpCode(opcodetype opCode, uint16_t numRepeats) const
{
    auto result = CScript();
    for(auto i = 0u ; i < numRepeats; ++i)
    {
        result << opCode;
    }
    return result;
}

void Generator::SetTotalFeeLimit(CMutableTransaction& mtx, const CAmount& feeLimit, bool vote) const
{
    // replace all the contributor fee limits with zero,
    // except the first one, which is replaced with the total fee limit

    assert(ParseTxClass(mtx) == TX_BuyTicket);

    for (unsigned i = ticketContribOutputIndex; i < mtx.vout.size(); i+=2) {
        TicketContribData contrib;
        assert(ParseTicketContrib(mtx, i, contrib));
        if (vote)
            contrib.setVoteFeeLimit(i == ticketContribOutputIndex ? feeLimit : 0);
        else
            contrib.setRevocationFeeLimit(i == ticketContribOutputIndex ? feeLimit : 0);
        mtx.vout[i].scriptPubKey = GetScriptForTicketContrib(contrib);
    }
}

CBlock Generator::NextBlock(const std::string& blockName
                    , const SpendableOut* spend
                    , const std::list<SpendableOut>& ticketSpends
                    , const MungerType& munger)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();

    TestMemPoolEntryHelper entry;
    const auto& nextHeight = Tip()->nHeight + 1;

    mempool.clear();

    if (nextHeight > COINBASE_MATURITY) {
        // Generate votes once the stake validation height has been
        // reached.
        if (nextHeight >= params.nStakeValidationHeight){
            // Generate and add the vote transactions for the winning tickets
            const auto& winners = chainActive.Tip()->pstakeNode->Winners();
            const auto& voteBlockHash = Tip()->GetBlockHash();
            const auto& voteBlockHeight = Tip()->nHeight;
            for (const auto& ticket: winners) {
                const auto& voteTx = CreateVoteTx(voteBlockHash, voteBlockHeight, ticket);
                mempool.addUnchecked(voteTx.GetHash(), entry.Fee(0LL).SpendsCoinbase(false).FromTx(voteTx));
            }
        }

        // if (nextHeight >= params.nStakeEnabledHeight)
        {
            const auto& ticketPrice = NextRequiredStakeDifficulty();
            const auto& ticketFee = CAmount(2);
            for (const auto& it : ticketSpends){
                const auto& purchaseTx = CreateTicketPurchaseTx(it, ticketPrice, ticketFee);
                mempool.addUnchecked(purchaseTx.GetHash(), entry.Fee(0LL).SpendsCoinbase(true).FromTx(purchaseTx));
            }
        }

        // Generate and add revocations for any missed tickets.
        const auto& misses = chainActive.Tip()->pstakeNode->MissedTickets();
        for (const auto& missedHash: misses) {
            const auto& revocationTx = CreateRevocationTx(missedHash);
            mempool.addUnchecked(revocationTx.GetHash(), entry.Fee(0LL).SpendsCoinbase(false).FromTx(revocationTx));
        }
    }

    if (spend != nullptr)
    {
        const auto& fee = CAmount(2000);
        const auto& spendTx = CreateSpendTx(*spend, fee);
        mempool.addUnchecked(spendTx.GetHash(), entry.Fee(fee).SpendsCoinbase(false).FromTx(spendTx));
    }

    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(coinbaseScript);
    CBlock& block = pblocktemplate->block;

    if (munger){
        mempool.clear();
        // calling the munger which can change the block
        munger(block);

        // Re-create coinbase transaction to be able to generate a new commitment
        CMutableTransaction coinbaseTx;
        coinbaseTx.vin.resize(1);
        coinbaseTx.vin[0].prevout.SetNull();
        coinbaseTx.vout.resize(1);
        coinbaseTx.vout[0].scriptPubKey = coinbaseScript;
        coinbaseTx.vout[0].nValue = 0/*nFees*/ + GetMinerSubsidy(nextHeight, chainparams.GetConsensus());
        coinbaseTx.vin[0].scriptSig = CScript() << nextHeight << OP_0;
        block.vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
        GenerateCoinbaseCommitment(block, Tip(), chainparams.GetConsensus());
    }

    // IncrementExtraNonce creates a valid coinbase and merkleRoot
    unsigned int extraNonce = 0;
    IncrementExtraNonce(&block, chainActive.Tip(), extraNonce);

    while (!CheckProofOfWork(block, chainparams.GetConsensus())) ++block.nNonce;

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    submitblock_StateCatcher sc(shared_pblock->GetHash());
    RegisterValidationInterface(&sc);
    const auto bAccepted = ProcessNewBlock(chainparams, shared_pblock, true, nullptr);
    UnregisterValidationInterface(&sc);
    if (!bAccepted || sc.found)
    {
        lastValidationState = sc.state;
    }
    else {
        lastValidationState = CValidationState{};
    }

    CBlock result = block;
    return result;
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction &tx) {
    CTransaction txn(tx);
    return FromTx(txn);
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransaction &txn) {
    return CTxMemPoolEntry(MakeTransactionRef(txn), nFee, nTime, nHeight,
                           spendsCoinbase, spendsStake, sigOpCost, lp);
}

/**
 * @returns a real block (0000000000013b8ab2cd513b0261a14096412195a72a0c4827d229dcc7e0f7af)
 *      with 9 txs.
 */
CBlock getBlock13b8a()
{
    CBlock block;
    CDataStream stream(ParseHex("000000a090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c9966699404ae2294730c3c9f9bda53523ce50e9b95e558da2fdb261b4d4c86041b1ab1bf9320430000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000901000000010000000000000000000000000000000000000000000000000000000000000000ffffffff07044c86041b0146ffffffff0100f2052a01000000434104e18f7afbe4721580e81e8414fc8c24d7cfacf254bb5c7b949450c3e997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac00000000010000000196608ccbafa16abada902780da4dc35dafd7af05fa0da08cf833575f8cf9e836000000004a493046022100dab24889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644bf574493a07fc5edba06dbc07c311b947520c2d514bc5725dcb401ffffffff0100f2052a010000001976a914f15d1921f52e4007b146dfa60f369ed2fc393ce288ac000000000100000001fb766c1288458c2bafcfec81e48b24d98ec706de6b8af7c4e3c29419bfacb56d000000008c493046022100f268ba165ce0ad2e6d93f089cfcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc597fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c40f1f8b8d898235e571fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966affffffff0280969800000000001976a9146963907531db72d0ed1a0cfb471ccb63923446f388ac80d6e34c000000001976a914f0688ba1c0d1ce182c7af6741e02658c7d4dfcd388ac000000000100000002c40297f730dd7b5a99567eb8d27b78758f607507c52292d02d4031895b52f2ff010000008b483045022100f7edfd4b0aac404e5bab4fd3889e0c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67b56fe67512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffffca5065ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefb000000008a473044022068010362a13c7f9919fa832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14a35c003b78b72bd59738cd676f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffff01001ec4110200000043410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa8621865134a221bd01f28ec9999ee3e021e60766e9d1f3458c115fb28650605f11c9ac000000000100000001cdaf2f758e91c514655e2dc50633d1e4c84989f8aa90a0dbc883f0d23ed5c2fa010000008b48304502207ab51be6f12a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e5329eead9accd880d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff02404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac002d3101000000001976a9141befba0cdc1ad56529371864d9f6cb042faa06b588ac000000000100000001b4a47603e71b61bc3326efd90111bf02d2f549b067f4c4a8fa183b57a0f800cb010000008a4730440220177c37f9a505c3f1a1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987dad92acb0c64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6cb46cc1a4d3cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996ffffffff0280651406000000001976a9145505614859643ab7b547cd7f1f5e7e2a12322d3788ac00aa0271000000001976a914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac00000000010000000586c62cd602d219bb60edb14a3e204de0705176f9022fe49a538054fb14abb49e010000008c493046022100f2bc2aba2534becbdf062eb993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff03ad0e58ccdac3df9dc28a218bcf6f1997b0a93306faaa4b3a28ae83447b2179010000008b483045022100be12b2937179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702200971b51f853a53d644ebae9ec8f3512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff2acfcab629bbc8685792603762c921580030ba144af553d271716a95089e107b010000008b483045022100fa579a840ac258871365dd48cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cfbb1b659b83671618f45abc1326b9edcc77d552a4f2a805c0014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffdcdc6023bbc9944a658ddc588e61eacb737ddf0a3cd24f113b5a8634c517fcd2000000008b4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e55c571d65da7701ae2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffe15557cd5ce258f479dfd6dc6514edf6d7ed5b21fcfa4a038fd69f06b83ac76e010000008b483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558165f7c8c8aca2d86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff01404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000010000000166d7577163c932b4f9690ca6a80b6e4eb001f0a2fa9023df5595602aae96ed8d000000008a4730440220262b42546302dfb654a229cefc86432b89628ff259dc87edd1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b63f014104979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90d661a3d0a33161dab29934edeb36aa01976be3baf8affffffff02404b4c00000000001976a9144854e695a02af0aeacb823ccbc272134561e0a1688ac40420f00000000001976a914abee93376d6b37b5c2940655a6fcaf1c8e74237988ac0000000001000000014e3f8ef2e91349a9059cb4f01e54ab2597c1387161d3da89919f7ea6acdbb371010000008c49304602210081f3183471a5ca22307c0800226f3ef9c353069e0773ac76bb580654d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e240014104976c79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34058b800a098fc1740ce3012e8fc8a00c96af966ffffffff02c0e1e400000000001976a9144134e75a6fcb6042034aab5e18570cf1f844f54788ac404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000"), SER_NETWORK, PROTOCOL_VERSION);
    stream >> block;
    return block;
}
