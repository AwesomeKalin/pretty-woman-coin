// Copyright (c) 2017 Amaury SÉCHET
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "processing_block_index.h"
#include "undo.h"
#include "validation.h"

#include "test/test_prettywomancoin.h"

#include <boost/test/unit_test.hpp>

namespace{ class undo_tests_uid; } // only used as unique identifier

template <>
struct ProcessingBlockIndex::UnitTestAccess<undo_tests_uid>
{
    UnitTestAccess() = delete;

    static void ApplyBlockUndo(const CBlockUndo &blockUndo,
                            const CBlock &block,
                            CBlockIndex* index,
                            CCoinsViewCache &view,
                            const task::CCancellationToken& shutdownToken)
    {
        ProcessingBlockIndex idx{ *index };
        idx.ApplyBlockUndo(
            blockUndo,
            block,
            view,
            task::CCancellationSource::Make()->GetToken());

    }

};
using TestAccessProcessingBlockIndex = ProcessingBlockIndex::UnitTestAccess<undo_tests_uid>;

template <>
struct CBlockIndex::UnitTestAccess<undo_tests_uid>
{
    UnitTestAccess() = delete;

    static void SetHeight( CBlockIndex& index, int32_t height)
    {
        index.nHeight = height;
    }
};
using TestAccessCBlockIndex = CBlockIndex::UnitTestAccess<undo_tests_uid>;

BOOST_FIXTURE_TEST_SUITE(undo_tests, BasicTestingSetup)

static void UpdateUTXOSet(const CBlock &block, CCoinsViewCache &view,
                          CBlockUndo &blockundo,
                          const CChainParams &chainparams, uint32_t nHeight) {
    auto &coinbaseTx = *block.vtx[0];
    UpdateCoins(coinbaseTx, view, nHeight);

    for (size_t i = 1; i < block.vtx.size(); i++) {
        auto &tx = *block.vtx[1];

        blockundo.vtxundo.push_back(CTxUndo());
        UpdateCoins(tx, view, blockundo.vtxundo.back(), nHeight);
    }

    view.SetBestBlock(block.GetHash());
}

static void UndoBlock(const CBlock &block, CCoinsViewCache &view,
                      const CBlockUndo &blockUndo,
                      const CChainParams &chainparams, uint32_t nHeight) {

    CBlockIndex::TemporaryBlockIndex index{ {} };

    TestAccessCBlockIndex::SetHeight( index, nHeight );
    TestAccessProcessingBlockIndex::ApplyBlockUndo(blockUndo, block, index.get(), view, task::CCancellationSource::Make()->GetToken());
}

static bool HasSpendableCoin(const CCoinsViewCache &view, const uint256 &txid) {
    auto coin = view.GetCoin(COutPoint(txid, 0));
    return (coin.has_value() && !coin->IsSpent());
}

BOOST_AUTO_TEST_CASE(connect_utxo_extblock) {
    SelectParams(CBaseChainParams::MAIN);
    const CChainParams &chainparams = Params();

    CBlock block;
    CMutableTransaction tx;

    CCoinsViewEmpty coinsDummy;
    CCoinsViewCache view(coinsDummy);

    block.hashPrevBlock = InsecureRand256();
    view.SetBestBlock(block.hashPrevBlock);

    // Create a block with coinbase and resolution transaction.
    tx.vin.resize(1);
    tx.vin[0].scriptSig.resize(10);
    tx.vout.resize(1);
    tx.vout[0].nValue = Amount(42);
    auto coinbaseTx = CTransaction(tx);

    block.vtx.resize(2);
    block.vtx[0] = MakeTransactionRef(tx);

    tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    tx.vin[0].prevout = COutPoint(InsecureRand256(), 0);
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    tx.vin[0].scriptSig.resize(0);
    tx.nVersion = 2;

    auto prevTx0 = CTransaction(tx);
    AddCoins(view, prevTx0, false, 100, 0);

    tx.vin[0].prevout = COutPoint(prevTx0.GetId(), 0);
    auto tx0 = CTransaction(tx);
    block.vtx[1] = MakeTransactionRef(tx0);

    // Now update the UTXO set.
    CBlockUndo blockundo;
    UpdateUTXOSet(block, view, blockundo, chainparams, 123456);

    BOOST_CHECK(view.GetBestBlock() == block.GetHash());
    BOOST_CHECK(HasSpendableCoin(view, coinbaseTx.GetId()));
    BOOST_CHECK(HasSpendableCoin(view, tx0.GetId()));
    BOOST_CHECK(!HasSpendableCoin(view, prevTx0.GetId()));

    UndoBlock(block, view, blockundo, chainparams, 123456);

    BOOST_CHECK(view.GetBestBlock() == block.hashPrevBlock);
    BOOST_CHECK(!HasSpendableCoin(view, coinbaseTx.GetId()));
    BOOST_CHECK(!HasSpendableCoin(view, tx0.GetId()));
    BOOST_CHECK(HasSpendableCoin(view, prevTx0.GetId()));
}

BOOST_AUTO_TEST_SUITE_END()
