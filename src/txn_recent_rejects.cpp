// Copyright (c) 2019 The Pretty Woman Coin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txn_recent_rejects.h"

#include <mutex>

CTxnRecentRejects::CTxnRecentRejects() {
    // Create a bloom filter
    mpRecentRejects = std::make_unique<CRollingBloomFilter>(120000, 0.000001);
}

void CTxnRecentRejects::insert(const uint256& txHash) {
    std::unique_lock lock {mRecentRejectsMtx};
    mpRecentRejects->insert(txHash);
}

bool CTxnRecentRejects::isRejected(const uint256& txHash) const {
    std::shared_lock lock {mRecentRejectsMtx};
    return mpRecentRejects->contains(txHash);
}

void CTxnRecentRejects::reset() {
    std::unique_lock lock {mRecentRejectsMtx};
    mpRecentRejects->reset();
}
