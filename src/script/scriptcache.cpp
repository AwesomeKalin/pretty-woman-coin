// Copyright (c) 2017 The Prettywomancoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scriptcache.h"
#include "crypto/sha256.h"
#include "cuckoocache.h"
#include "primitives/transaction.h"
#include "random.h"
#include "script/sigcache.h"
#include "util.h"
#include <mutex>

std::mutex cs_script_cache;
static auto scriptExecutionCache =
    std::make_unique<CuckooCache::cache<uint256, SignatureCacheHasher>>();
static uint256 scriptExecutionCacheNonce(GetRandHash());

static void InitScriptExecutionCacheUnlocked() 
{
    // nMaxCacheSize is unsigned. If -maxscriptcachesize is set to zero,
    // setup_bytes creates the minimum possible cache (2 elements).
    size_t nMaxCacheSize =
        std::min(static_cast<uint64_t>(std::max(int64_t(0),
                          gArgs.GetArgAsBytes("-maxscriptcachesize",
                                       DEFAULT_MAX_SCRIPT_CACHE_SIZE, ONE_MEBIBYTE))),
                 MAX_MAX_SCRIPT_CACHE_SIZE * ONE_MEBIBYTE);
    size_t nElems = scriptExecutionCache->setup_bytes(nMaxCacheSize);
    LogPrintf("Using %zu MiB out of %zu requested for script execution cache, "
              "able to store %zu elements\n",
              (nElems * sizeof(uint256)) >> 20, nMaxCacheSize >> 20, nElems);
}

void InitScriptExecutionCache()
{
    std::lock_guard lock{cs_script_cache};
    InitScriptExecutionCacheUnlocked();
}

void ClearCache() 
{
    std::lock_guard lock{cs_script_cache};
    scriptExecutionCache = std::make_unique<CuckooCache::cache<uint256, SignatureCacheHasher>>();
    InitScriptExecutionCacheUnlocked();
}

uint256 GetScriptCacheKey(const CTransaction &tx, uint32_t flags) {
    uint256 key;
    // We only use the first 19 bytes of nonce to avoid a second SHA round -
    // giving us 19 + 32 + 4 = 55 bytes (+ 8 + 1 = 64)
    static_assert(55 - sizeof(flags) - 32 >= 128 / 8,
                  "Want at least 128 bits of nonce for script execution cache");
    CSHA256()
        .Write(scriptExecutionCacheNonce.begin(), 55 - sizeof(flags) - 32)
        .Write(tx.GetHash().begin(), 32)
        .Write((uint8_t *)&flags, sizeof(flags))
        .Finalize(key.begin());

    return key;
}

bool IsKeyInScriptCache(uint256 key, bool erase) {
    std::lock_guard lock{cs_script_cache};
    return scriptExecutionCache->contains(key, erase);
}

void AddKeyInScriptCache(uint256 key) {
    std::lock_guard lock{cs_script_cache};
    scriptExecutionCache->insert(key);
}