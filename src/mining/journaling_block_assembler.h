// Copyright (c) 2019 Prettywomancoin Association.
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#pragma once

#include <mining/assembler.h>
#include <mining/group_builder.h>
#include <mining/journal.h>

#include <future>
#include <mutex>

namespace mining
{

/**
* A mining candidate builder that utilises the mempool journal.
*/
class JournalingBlockAssembler : public BlockAssembler
{
  public:

    // Default config values
    static constexpr uint64_t DEFAULT_MAX_SLOT_TRANSACTIONS {25000};
    static constexpr bool DEFAULT_NEW_BLOCK_FILL {false};
    static constexpr unsigned DEFAULT_THROTTLE_THRESHOLD {100};
    static constexpr unsigned DEFAULT_RUN_FREQUENCY_MILLIS {100};

    // Construction/destruction
    JournalingBlockAssembler(const Config& config);
    ~JournalingBlockAssembler();

    // Construct a new block template with coinbase to scriptPubKeyIn
    std::unique_ptr<CBlockTemplate> CreateNewBlock(const CScript& scriptPubKeyIn, CBlockIndex*& pindexPrev) override;

    // Get the maximum generated block size for the current config and chain tip
    uint64_t GetMaxGeneratedBlockSize() const override;

    // Get (and reset) whether we might produce an updated template
    bool GetTemplateUpdated() override;

    // (Re)read our configuration parameters (for unit testing)
    void ReadConfigParameters();

    BlockStats getLastBlockStats() const override { return mLastBlockStats; }

    // Unit test access
    template<typename T> struct UnitTestAccess;

  private:

    // Thread entry point for block update processing
    void threadBlockUpdate() noexcept;

    // Update our block template with some new transactions
    void updateBlock(const CBlockIndex* pindex, uint64_t maxTxns);

    // Create a new block for us to start working on
    void newBlock();

    // Result of attempt to add a transaction or group
    struct AddTransactionResult
    {
        enum class Result { SUCCESS, ERRORED, SKIPPED, BLOCKFULL };
        Result result { Result::SUCCESS };
        size_t numAdded {0};
    };

    // Test whether we can add another transaction to the next block and
    // return the number of transactions actually added
    AddTransactionResult addTransactionOrGroup(const CBlockIndex* pindex,
                                               const CJournal::Index& journalEnd,
                                               uint64_t maxBlockSizeComputed);
    AddTransactionResult addTransaction(const CBlockIndex* pindex,
                                        uint64_t maxBlockSizeComputed,
                                        const CJournalEntry& entry);

    // Our internal mutex
    mutable std::mutex mMtx {};

    // Flag to indicate whether we have been updated
    std::atomic_bool mRecentlyUpdated {false};
    // Time most recent txn was added to the latest template
    int64_t mLastUpdateTime {0};


    // Chain context for the block
    int64_t mLockTimeCutoff {0};

    // Worker thread management
    std::future<void> mFuture {};
    std::promise<void> mPromise {};

    // Frequency we run
    std::chrono::milliseconds mRunFrequency {DEFAULT_RUN_FREQUENCY_MILLIS};

    // Maximum number of transactions to process per time slot
    std::atomic_uint64_t mMaxSlotTransactions {DEFAULT_MAX_SLOT_TRANSACTIONS};
    // Whether every call to CreateNewBlock returns all txns from the journal,
    // or whether sometimes only a subset may be returned.
    std::atomic_bool mNewBlockFill {DEFAULT_NEW_BLOCK_FILL};

    // Selfish mining prevention throttling threshold
    unsigned mThrottlingThreshold {DEFAULT_THROTTLE_THRESHOLD};
    bool mEnteredThrottling {false};

    // The journal we're reading from and our current position in that journal
    CJournalPtr mJournal {nullptr};

    // Variables used for mining statistics
    BlockStats mLastBlockStats{};

    // All details for the block we are currently building
    static constexpr uint64_t COINBASE_SIG_OPS {100};
    static constexpr uint64_t COINBASE_SIZE {1000};

    // Position where we're reading from the index
    CJournal::Index mJournalPos {};

    // Block assembly state, without the huge vectors
    struct BlockAssemblyState
    {
        uint64_t mBlockSigOps {COINBASE_SIG_OPS};
        uint64_t mBlockSize {COINBASE_SIZE};
        // Amount of fees in the current block template
        Amount mBlockFees {0};
    };
    std::vector<CTransactionRef> mBlockTxns {};
    std::vector<Amount> mTxFees {};

    BlockAssemblyState mState {};

    TxnGroupBuilder mGroupBuilder {};

    // When adding transaction group we optimize for the happy case
    // and do serious extra work only when we need to rollback() when
    // the group would push the block over the limit
    class GroupCheckpoint {

    private:
        // track whether we should roll back the group.
        bool mShouldRollback {true};

        // reference of the block assembler
        JournalingBlockAssembler& mAssembler;

        // copy of internal state of the block assember that we need to restore
        BlockAssemblyState mAssemblerStateCheckpoint {};

        template<class T> class VectorCheckpoint {
        private:
            std::vector<T> &mVector;
            size_t mVectorSize {0};
        public:
            VectorCheckpoint(std::vector<T> &vector)
            : mVector(vector)
            , mVectorSize(vector.size())
            {
            }
            void trimToSize() {
                mVector.erase(std::next(mVector.begin(), mVectorSize), mVector.end());
            }
        };
        // for vectors, just remember the size as the iterators are very unstable
        VectorCheckpoint<CTransactionRef> mBlockTxnsCheckpoint;
        VectorCheckpoint<Amount> mTxFeesCheckpoint;

    public:
        GroupCheckpoint(JournalingBlockAssembler& assembler);

        GroupCheckpoint(const GroupCheckpoint&) = delete;
        GroupCheckpoint& operator=(const GroupCheckpoint&) = delete;

        ~GroupCheckpoint() { rollback(); }

        void rollback();
        void commit() { mShouldRollback = false; }
    };
};
}
