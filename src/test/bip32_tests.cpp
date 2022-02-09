//
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 Project PAI Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//


#include <boost/test/unit_test.hpp>

#include <key.h>
#include <key_io.h>
#include <uint256.h>
#include <util.h>
#include <utilstrencodings.h>
#include <test/test_bwscoin.h>

#include <string>
#include <vector>

struct TestDerivation {
    std::string pub;
    std::string prv;
    unsigned int nChild;
};

struct TestVector {
    std::string strHexMaster;
    std::vector<TestDerivation> vDerive;

    explicit TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

    TestVector& operator()(std::string pub, std::string prv, unsigned int nChild) {
        vDerive.push_back(TestDerivation());
        TestDerivation &der = vDerive.back();
        der.pub = pub;
        der.prv = prv;
        der.nChild = nChild;
        return *this;
    }
};

/**
 * BWSCOIN Note: if unit test updating is required
 * For updating the following test vectors:
 * - clone bip32utils from https://github.com/prusnak/bip32utils.git;
 * - in BIP32Key.py (lines 26-29), update the EX_MAIN_PRIVATE, EX_MAIN_PUBLIC, EX_TEST_PRIVATE and EX_TEST_PUBLIC with their corresponding base58Prefixes from chainparams.cpp;
 * - in BIP32Key.py (line 43), change the hmac seed to "BWS Coin seed";
 * - in BIP32Key.py (at the end of file) add the following lines:

    # BIP0032 Test vector 3
    entropy = '4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be'.decode('hex')
    m = BIP32Key.fromEntropy(entropy)
    print("Test vector 3:")
    print("Master (hex):", entropy.encode('hex'))
    print("* [Chain m]")
    m.dump()

    print("* [Chain m/0]")
    m = m.ChildKey(0+BIP32_HARDEN)
    m.dump()

 * - in bip0032-vectors.sh (at the end of file) add the following lines:
 # BIP0032 Test vector #3

echo Generating BIP0032 test vector 3:
echo 4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be | \
    ../bip32gen -v \
    -i entropy -f - -x -n 512 \
    -o privkey,wif,pubkey,addr,xprv,xpub -F - -X \
    m \
    m/0h

 * - run the bip0032-vectors.sh script with the following command line:

 ./bip0032-vectors.sh > bip0032-vectors.out

  * - copy the public and private keys from the bip0032-vectors.out file into the three vectors below (test1, test2 and test3).
  * - run make check
 */

TestVector test1 =
  TestVector("000102030405060708090a0b0c0d0e0f")
    ("bwsc5xjWp9NfsoyN27ALLLyN8FnuGq1wu6reJpQAeZfTA5qF9WExQrUDhpVY4yZDAJcv5KVUDFqf4kzxmU3ZrWsP5iV6PtUnXgPcVizmtSGaepm",
     "bwsp7bf4xBt5wzekUbHYQfxfR1iniN9FSC3Hob9MPcoBUg9woKuDSYxerBfsP9zNXTR8z2SBUN9utfzdpqSfGsLEgBdU5THpCGnG3o6YnEJTjZP",
     0x80000000)
    ("bwsc61699mpn4hbjiBk54X1wd7K6hCpeqBzk1vL6oSmiEKNDsoUFovwPc1cHNQno5DmLdgfrepW68nPZGnTeGZj2odf8hDJLr6uenv65q75PMdj",
     "bwsp7e1hHpLC8tH8AfsH8r1EusEz8jwxNHBPWh5HYVuSYugvXd8WqdRpkNncgeiUo99XWqzNNxKwPuwZ7kuyGGsaTLBBRFhx1DcMdoUwTLw1Xnf",
     1)
    ("bwsc62jq1jjLXkVegeefCcMR8ChtTksbG5KpkZjZDKXafpfKZpz5VVN1HjDGYLFtRsUPEVUrY9PqyP1wZj7iFBUK2Lw8VEeu64eeX1nXdsNwD2Q",
     "bwsp7ffP9nEkbwB398msGwLiQxdmuHztoAWUFLUjxNfJzQz2DeeLXBrSS6PbrWnmUWV7wCUQ8qbkAVSNAr9CWu9Dxirv54Lk6PdxV9Q8RDNQPJ3",
     0x80000002)
    ("bwsc65uFNr2VfkNzovDtkrk7mMApcqeUfqMsjVMRX76m7gWDBriX1jHYj2ASdxcsgGnNZKqJEPwqDZWCW7HeFtYJtHBdApDyUjM6Aw69d8dDNWJ",
     "bwsp7ipoWtXujw4PGQM6qBjR476i4NmnCvYXEG6cGAEVSGpuqgNn3RmysPLmxBz7oNjMFazjbozbgZrtpED7MrQrmYNaK3Y7M9ywR2fh1S9iExg",
     2)
    ("bwsc67GSjwfqEN5eHNDxwUzK4M3nQ23nPTt4KB8w8LyEhUijNkmphs2rShueFYnTPT9byDLoFwyKwK1pg1KRCKetsL6fDyPQzKcxZgnEKu6cY6f",
     "bwsp7kBzszBFJYm2jrMB1oycM6yfqZB5vZ4howt7sQ6y253S2aS5jZXHb55yZkBQAr7CQHQxvpdb8dhE1LBXDKBgHu6VZCzMWXusc5aVkZ2B6Ax",
     1000000000)
    ("bwsc68HWXe5oAZCVFvRCe1sZztjFhJk9CCN4heNAsqG9Zy5P6KZQya5MraA4Tvps1ohWiEdPrgA7MVWBWFDKdcjofDBxzRD5qZLdfqwQZ1mafMq",
     "bwsp7mD4fgbDEjssiQYQiLrsHef98qsSjHYiCR7MctPstZQ5k9Dg1GZnzwLPnAdxnMi5FZ4Jajs42uYdDUx8CuXHKGS5ywSC9v8t2MbSaECcXtz",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("bwsc5xjWp9NfsoyN1Vr6Nknc6HWT7W4ZSBaw6wd63GytZ17nfoEXzSrpQszP4BSN86aG7BzxGFFy9KFk5mMKe1UHiMoZz7DDoMBzMQGUKMN2yAs",
     "bwsp7bf4xBt5wzekTyyJT5muP3SLZ3BryGmabiNGnL7csbSVKcto29MFZFAiNPVTWaHpxCdXjdSEfBdzLRPGrJTBh4jtJnwBkHFBjLPS92RGSzS",
     0)
    ("bwsc5zwzvWFcQpBdYQYPdDfKbkdm2y3oHx12VaNsVYD5nHReUHHkBhAp9YHgA4SizW5aGW1wnwd3QRB6R8D5Vodp5nej27YfebE4FAWvqY5yy7o",
     "bwsp7dsZ4Ym2Uzs1ztfbhYectWZeUWB6q3BfzM84EbLp6skM86x1DPfFHuU1UHYTV7Q1iEAfZFHwvUqMjFVdx8BXha5LK66Dp2mAefngAsJ1kpY",
     0xFFFFFFFF)
    ("bwsc63chyLQ5xwgZSEpRXpPcR2iyM5Yaj79ZN3E9qNJtzXLQioHESvVVGmEvR2vL3LJP2kS8L816Wf817AXhxKWkqSye4otUnByHXv8xusKJy33",
     "bwsp7gYG7NuW38Mwtiwdc9NuhnerncftGCLCroyLaRSdK7f7NcwVUcyvR8RFjEL93JVjX66PduidnRiBCgCE79N5cvsMusVXeqwCttuv6AAtcoX",
     1)
    ("bwsc66BcZUS82ULpZJBMS9SPQbcessURdTaweQMB3zuQ1Ln2LNn7UduRHJ5b37WhfKFowVPrpkr6o1CzzbLaJidoxG1z7HuD5TWxDCYm8aA7woP",
     "bwsp7j7AhWwY6f2D1nJZWURghMYYKQbjAYmb9B6Mo438Kw6izCSNWLPrRfFvMLrj8f1zvF86uFHC2gDZndM4AXrzWBzEwDx63CUqDz8ecRrfYG7",
     0xFFFFFFFE)
    ("bwsc67NdXTKYSKmGPHoPzFJELQivzqn2uy9VcrE36VLGLiHT7MgkZjxDiTVE2KZ7QU7cMfoFMAM7qb1bAsQgG1zxTWxyYtd4aGvDL7rCH1pPSnD",
     "bwsp7kJBfVpxWWSeqmvc4aHXdAepSNuLT4L97cyDqYTzfJc9mBM1bSSerpfZLW4MbybvXeL4m9MwPu4H5jpmaWJTeEKmuKAySwKDGrtSMBmBVZo",
     2)
    ("bwsc69NNLtizugrXWfVRNhKqeBDKZZrkCS9cDTa249LxifbYyAPf8yN2PWvU9Vi9extFby5M8b2zc6guUaLA1ikCBMaJ6fB2BNQV3Pb7koaiTVL",
     "bwsp7nHvUwEQysXuy9cdT2K8vw9D16z3jXLFiEKCoCUh3FvFcz3vAfrTXt6oThXaYQdmn8RBDtmBw8cwVkf1iojcgKV7y3gwK6UD24AGww3XU3z",
     0);

TestVector test3 =
  TestVector("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be")
    ("bwsc5xjWp9NfsoyN1s3j73PjuzerY9Ktw1zPXbgRdZTSeYhv5KCd7rGfpPXLxUJT1Zvjes8Z3cdZANsNJLeRsAoFCbB9G6GKJRVerea5wxvpbSZ",
     "bwsp7bf4xBt5wzekUMAwBNP3CkajygTCU7B32NRcNcbAy92cj8rt9Ym6xkhgGfGXBh7nmVG7HrshAuy8M2SVzMg1aQz5fU7cseXyb8eQDugKzVd",
      0x80000000)
    ("bwsc61AqfMUNVRaEyH8JcVGTTKQBdNzsfANaCYSWGCts9boCfZTcntFGtsXWhQHgRVBQJvi2oCkYATQZfMDtG7SKxBTDbnppkBwc1deS3yzEbGY",
     "bwsp7e6PoPynZcFdRmFWgpFkk5L54v8BCFZDhKBh1G2bUC7uKP7spaji3Ehr1bULU7h2RE5PDwbNf88tuzHjAAQgEZSvgtXVnLZnviQGiwgsgdx",
      0);

void RunTest(const TestVector &test) {
    std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
    CExtKey key;
    CExtPubKey pubkey;
    key.SetMaster(seed.data(), seed.size());
    pubkey = key.Neuter();
    for (const TestDerivation &derive : test.vDerive) {
        unsigned char data[74];
        key.Encode(data);
        pubkey.Encode(data);

        // Test private key
        BOOST_CHECK(EncodeExtKey(key) == derive.prv);
        BOOST_CHECK(DecodeExtKey(derive.prv) == key); //ensure a base58 decoded key also matches

        // Test public key
        BOOST_CHECK(EncodeExtPubKey(pubkey) == derive.pub);
        BOOST_CHECK(DecodeExtPubKey(derive.pub) == pubkey); //ensure a base58 decoded pubkey also matches

        // Derive new keys
        CExtKey keyNew;
        BOOST_CHECK(key.Derive(keyNew, derive.nChild));
        CExtPubKey pubkeyNew = keyNew.Neuter();
        if (!(derive.nChild & 0x80000000)) {
            // Compare with public derivation
            CExtPubKey pubkeyNew2;
            BOOST_CHECK(pubkey.Derive(pubkeyNew2, derive.nChild));
            BOOST_CHECK(pubkeyNew == pubkeyNew2);
        }
        key = keyNew;
        pubkey = pubkeyNew;

        CDataStream ssPub(SER_DISK, CLIENT_VERSION);
        ssPub << pubkeyNew;
        BOOST_CHECK(ssPub.size() == 75);

        CDataStream ssPriv(SER_DISK, CLIENT_VERSION);
        ssPriv << keyNew;
        BOOST_CHECK(ssPriv.size() == 75);

        CExtPubKey pubCheck;
        CExtKey privCheck;
        ssPub >> pubCheck;
        ssPriv >> privCheck;

        BOOST_CHECK(pubCheck == pubkeyNew);
        BOOST_CHECK(privCheck == keyNew);
    }
}

BOOST_FIXTURE_TEST_SUITE(bip32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bip32_test1) {
    RunTest(test1);
}

BOOST_AUTO_TEST_CASE(bip32_test2) {
    RunTest(test2);
}

BOOST_AUTO_TEST_CASE(bip32_test3) {
    RunTest(test3);
}

BOOST_AUTO_TEST_SUITE_END()
