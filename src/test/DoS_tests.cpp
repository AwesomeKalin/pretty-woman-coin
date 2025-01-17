// Copyright (c) 2011-2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

// Unit tests for denial-of-service detection/prevention code

#include "chainparams.h"
#include "config.h"
#include "keystore.h"
#include "net/net.h"
#include "net/net_processing.h"
#include "pow.h"
#include "script/sign.h"
#include "serialize.h"
#include "txn_validator.h"
#include "util.h"
#include "validation.h"
#include "test/test_prettywomancoin.h"

#include <cstdint>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>

namespace {
    CService ip(uint32_t i) {
        struct in_addr s;
        s.s_addr = i;
        return CService(CNetAddr(s), Params().GetDefaultPort());
    }
    NodeId id = 0;
}

BOOST_FIXTURE_TEST_SUITE(DoS_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(DoS_banning) {
    const Config &config = GlobalConfig::GetConfig();
    std::atomic<bool> interruptDummy(false);

    CConnman::CAsyncTaskPool asyncTaskPool{config};
    connman->ClearBanned();
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNodePtr dummyNode1 =
        CNode::Make(
            id++,
            NODE_NETWORK,
            0,
            INVALID_SOCKET,
            addr1,
            0u,
            0u,
            asyncTaskPool,
            "",
            true);
    dummyNode1->SetSendVersion(PROTOCOL_VERSION);
    GetNodeSignals().InitializeNode(dummyNode1, *connman, nullptr);
    dummyNode1->nVersion = 1;
    dummyNode1->fSuccessfullyConnected = true;
    // Should get banned.
    Misbehaving(dummyNode1->GetId(), 100, "");
    SendMessages(config, dummyNode1, *connman, interruptDummy);
    BOOST_CHECK(connman->IsBanned(addr1));
    // Different IP, not banned.
    BOOST_CHECK(!connman->IsBanned(ip(0xa0b0c001 | 0x0000ff00)));

    CAddress addr2(ip(0xa0b0c002), NODE_NONE);
    CNodePtr dummyNode2 =
        CNode::Make(
            id++,
            NODE_NETWORK,
            0,
            INVALID_SOCKET,
            addr2,
            1u,
            1u,
            asyncTaskPool,
            "",
            true);
    dummyNode2->SetSendVersion(PROTOCOL_VERSION);
    GetNodeSignals().InitializeNode(dummyNode2, *connman, nullptr);
    dummyNode2->nVersion = 1;
    dummyNode2->fSuccessfullyConnected = true;
    Misbehaving(dummyNode2->GetId(), 50, "");
    SendMessages(config, dummyNode2, *connman, interruptDummy);
    // 2 not banned yet...
    BOOST_CHECK(!connman->IsBanned(addr2));
    // ... but 1 still should be.
    BOOST_CHECK(connman->IsBanned(addr1));
    Misbehaving(dummyNode2->GetId(), 50, "");
    SendMessages(config, dummyNode2, *connman, interruptDummy);
    BOOST_CHECK(connman->IsBanned(addr2));
}

BOOST_AUTO_TEST_CASE(DoS_banscore)
{
    auto& config { GlobalConfig::GetModifiableGlobalConfig() };
    std::atomic<bool> interruptDummy(false);

    CConnman::CAsyncTaskPool asyncTaskPool{config};
    connman->ClearBanned();
    // because 11 is my favorite number.
    config.SetBanScoreThreshold(111);
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNodePtr dummyNode1 =
        CNode::Make(
            id++,
            NODE_NETWORK,
            0,
            INVALID_SOCKET,
            addr1,
            3u,
            1u,
            asyncTaskPool,
            "",
            true);
    dummyNode1->SetSendVersion(PROTOCOL_VERSION);
    GetNodeSignals().InitializeNode(dummyNode1, *connman, nullptr);
    dummyNode1->nVersion = 1;
    dummyNode1->fSuccessfullyConnected = true;
    Misbehaving(dummyNode1->GetId(), 100, "");
    SendMessages(config, dummyNode1, *connman, interruptDummy);
    BOOST_CHECK(!connman->IsBanned(addr1));
    Misbehaving(dummyNode1->GetId(), 10, "");
    SendMessages(config, dummyNode1, *connman, interruptDummy);
    BOOST_CHECK(!connman->IsBanned(addr1));
    Misbehaving(dummyNode1->GetId(), 1, "");
    SendMessages(config, dummyNode1, *connman, interruptDummy);
    BOOST_CHECK(connman->IsBanned(addr1));
}

BOOST_AUTO_TEST_CASE(DoS_bantime) {
    const Config &config = GlobalConfig::GetConfig();
    std::atomic<bool> interruptDummy(false);

    CConnman::CAsyncTaskPool asyncTaskPool{config};
    connman->ClearBanned();
    int64_t nStartTime = GetTime();
    // Overrides future calls to GetTime()
    SetMockTime(nStartTime);

    CAddress addr(ip(0xa0b0c001), NODE_NONE);
    CNodePtr dummyNode =
        CNode::Make(
            id++,
            NODE_NETWORK,
            0,
            INVALID_SOCKET,
            addr,
            4u,
            4u,
            asyncTaskPool,
            "",
            true);
    dummyNode->SetSendVersion(PROTOCOL_VERSION);
    GetNodeSignals().InitializeNode(dummyNode, *connman, nullptr);
    dummyNode->nVersion = 1;
    dummyNode->fSuccessfullyConnected = true;

    Misbehaving(dummyNode->GetId(), 100, "");
    SendMessages(config, dummyNode, *connman, interruptDummy);
    BOOST_CHECK(connman->IsBanned(addr));

    SetMockTime(nStartTime + 60 * 60);
    BOOST_CHECK(connman->IsBanned(addr));

    SetMockTime(nStartTime + 60 * 60 * 24 + 1);
    BOOST_CHECK(!connman->IsBanned(addr));
}

BOOST_AUTO_TEST_CASE(DoS_mapOrphans) {
    CBasicKeyStore keystore;
    CAddress dummy_addr(ip(0xa0b0c001), NODE_NONE);

    size_t maxExtraTxnsForCompactBlock {
        static_cast<size_t>(
            gArgs.GetArg("-blockreconstructionextratxn",
                      COrphanTxns::DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN))
    };
    size_t maxTxSizePolicy {
        static_cast<size_t>(
            gArgs.GetArgAsBytes("-maxtxsizepolicy",
                      MAX_TX_SIZE_POLICY_BEFORE_GENESIS))
    };
    size_t maxOrphanPercent {
        static_cast<size_t>(
            gArgs.GetArg("-maxorphansinbatchpercent",
                      COrphanTxns::DEFAULT_MAX_PERCENTAGE_OF_ORPHANS_IN_BATCH))
    };
    size_t maxInputsOutputs {
        static_cast<size_t>(
            gArgs.GetArg("-maxinputspertransactionoutoffirstlayerorphan",
                      COrphanTxns::DEFAULT_MAX_INPUTS_OUTPUTS_PER_TRANSACTION))
    };
    // A common buffer with orphan txns
    std::shared_ptr<COrphanTxns> orphanTxns {
        std::make_shared<COrphanTxns>(
                            maxExtraTxnsForCompactBlock,
                            maxTxSizePolicy,
                            maxOrphanPercent,
                            maxInputsOutputs)
    };

    CConnman::CAsyncTaskPool asyncTaskPool{GlobalConfig::GetConfig()};

    std::vector<CNodePtr> nodes {};
    for(auto i = 0; i < 50; ++i) {
        CNodePtr pNode =
            CNode::Make(
                i,
                NODE_NETWORK,
                0,
                INVALID_SOCKET,
                dummy_addr,
                0u,
                0u,
                asyncTaskPool,
                "",
                true);
        nodes.push_back(pNode);
    }

    // Get a pointer to the TxIdTracker.
    const TxIdTrackerSPtr& pTxIdTracker = connman->GetTxIdTracker();
    // 50 orphan transactions:
    for (NodeId i = 0; i < 50; i++) {
        CKey key;
        key.MakeNewKey(true);
        keystore.AddKey(key);

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout = COutPoint(InsecureRand256(), 0);
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1 * CENT;
        tx.vout[0].scriptPubKey =
            GetScriptForDestination(key.GetPubKey().GetID());

        // Add txn input data to the queue
        orphanTxns->addTxn(
            std::make_shared<CTxInputData>(
                pTxIdTracker, // a pointer to the TxIdTracker
                MakeTransactionRef(tx),  // a pointer to the tx
                TxSource::p2p, // tx source
                TxValidationPriority::normal, // tx validation priority
                TxStorage::memory, // tx storage
                GetTime(),     // nAcceptTime
                Amount(0),     // nAbsurdFee
                nodes[i]));    // pNode
    }
    BOOST_CHECK(orphanTxns->getTxnsNumber() == 50);

    // ... and 50 that depend on other orphans:
    for (NodeId i = 0; i < 50; i++) {
        CKey key;
        key.MakeNewKey(true);
        keystore.AddKey(key);
        // Get a random orphan txn
        TxInputDataSPtr pRndTxInputData { orphanTxns->getRndOrphan() };
        BOOST_CHECK(pRndTxInputData);

        CTransactionRef txPrev = pRndTxInputData->GetTxnPtr();
        // Create a dependant txn
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout = COutPoint(txPrev->GetId(), 0);
        tx.vout.resize(1);
        tx.vout[0].nValue = 1 * CENT;
        tx.vout[0].scriptPubKey =
            GetScriptForDestination(key.GetPubKey().GetID());
        SignSignature(testConfig, keystore, false, false, *txPrev, tx, 0, SigHashType());
        // Add txn input data to the queue
        orphanTxns->addTxn(
            std::make_shared<CTxInputData>(
                pTxIdTracker, // a pointer to the TxIdTracker
                MakeTransactionRef(tx),  // a pointer to the tx
                TxSource::p2p, // tx source
                TxValidationPriority::normal, // tx validation priority
                TxStorage::memory, // tx storage
                GetTime(),     // nAcceptTime
                Amount(0),     // nAbsurdFee
                pRndTxInputData->GetNodePtr())); // pNode
    }
    BOOST_CHECK(orphanTxns->getTxnsNumber() == 100);

    // This really-big orphan should be ignored:
    for (NodeId i = 0; i < 10; i++) {
        CKey key;
        key.MakeNewKey(true);
        keystore.AddKey(key);
        // Get a random orphan txn
        TxInputDataSPtr pRndTxInputData { orphanTxns->getRndOrphan() };
        BOOST_CHECK(pRndTxInputData);

        CTransactionRef txPrev = pRndTxInputData->GetTxnPtr();
        CMutableTransaction tx;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1 * CENT;
        tx.vout[0].scriptPubKey =
            GetScriptForDestination(key.GetPubKey().GetID());
        tx.vin.resize(2777);
        for (size_t j = 0; j < tx.vin.size(); j++) {
            tx.vin[j].prevout = COutPoint(txPrev->GetId(), j);
        }
        SignSignature(testConfig, keystore, false, false, *txPrev, tx, 0, SigHashType());
        // Re-use same signature for other inputs
        // (they don't have to be valid for this test)
        for (unsigned int j = 1; j < tx.vin.size(); j++)
            tx.vin[j].scriptSig = tx.vin[0].scriptSig;
        // Create a shared object with txn input data
        auto pTxInputData {
            std::make_shared<CTxInputData>(
                pTxIdTracker, // a pointer to the TxIdTracker
                MakeTransactionRef(tx),  // a pointer to the tx
                TxSource::p2p, // tx source
                TxValidationPriority::normal, // tx validation priority
                TxStorage::memory, // tx storage
                GetTime(),     // nAcceptTime
                Amount(0),     // nAbsurdFee
                pRndTxInputData->GetNodePtr()) // pNode
        };
        // Add txn input data to the queue
        orphanTxns->addTxn(pTxInputData);
        BOOST_CHECK(!orphanTxns->checkTxnExists(pTxInputData->GetTxnPtr()->GetId()));
    }
    // Test erase orphans from a given peer:
    for (NodeId i = 0; i < 3; i++) {
        size_t sizeBefore = orphanTxns->getTxnsNumber();
        orphanTxns->eraseTxnsFromPeer(i);
        BOOST_CHECK(orphanTxns->getTxnsNumber() < sizeBefore);
    }
}

BOOST_AUTO_TEST_SUITE_END()
