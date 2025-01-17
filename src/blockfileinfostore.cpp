// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "blockfileinfostore.h"

#include "block_file_access.h"
#include "block_file_info.h"
#include "config.h"
#include "util.h"
#include "txdb.h"  // CBlockTreeDB
#include "consensus/validation.h" // CValidationState

/** Access to info about block files */
std::unique_ptr<CBlockFileInfoStore> pBlockFileInfoStore = std::make_unique<CBlockFileInfoStore>();

void CBlockFileInfoStore::FindNextFileWithEnoughEmptySpace(const Config &config,
    uint64_t nAddSize, unsigned int& nFile)
{
    // this while instead of if is here because first commit introduced it
    // and vinfoBlockFile.size() can exceed nLastBlockFile at least in
    // LoadBlockIndexDB function where block file info is being loaded
    // and we can't be certain that it's the only case without more tests
    // and extensive refactoring
    while (vinfoBlockFile[nFile].Size() &&
           // >= is here for legacy purposes - could possibly be changed to > as
           // currently max file size is one byte less than preferred block file size
           // but larger code analisys would be required
           vinfoBlockFile[nFile].Size() + nAddSize >= config.GetPreferredBlockFileSize()) {
        nFile++;
        if (vinfoBlockFile.size() <= nFile) {
            vinfoBlockFile.resize(nFile + 1);
        }
    }
}

void CBlockFileInfoStore::FlushBlockFile(bool fFinalize) {
    LOCK(cs_LastBlockFile);

    if ( !vinfoBlockFile.empty() )
    {
        assert( nLastBlockFile >= 0 );
        assert( vinfoBlockFile.size() > static_cast<std::size_t>(nLastBlockFile) );

        BlockFileAccess::FlushBlockFile(
            nLastBlockFile,
            vinfoBlockFile[nLastBlockFile],
            fFinalize );
    }
    else
    {
        assert( nLastBlockFile == 0 );
    }
}

std::vector<std::pair<int, const CBlockFileInfo *>> CBlockFileInfoStore::GetAndClearDirtyFileInfo()
{
    std::vector<std::pair<int, const CBlockFileInfo *>> vFiles;
    vFiles.reserve(setDirtyFileInfo.size());
    for (std::set<int>::iterator it = setDirtyFileInfo.begin();
        it != setDirtyFileInfo.end();) {
        vFiles.push_back(
            std::make_pair(*it, &vinfoBlockFile[*it]));
        setDirtyFileInfo.erase(it++);
    }
    return vFiles;
}


bool CBlockFileInfoStore::FindBlockPos(const Config &config, CValidationState &state,
    CDiskBlockPos &pos, uint64_t nAddSize, int32_t nHeight,
    uint64_t nTime, bool& fCheckForPruning, bool fKnown) {
    LOCK(cs_LastBlockFile);

    unsigned int nFile = fKnown ? pos.File() : nLastBlockFile;
    if (vinfoBlockFile.size() <= nFile) {
        vinfoBlockFile.resize(nFile + 1);
    }

    if (!fKnown) {
        FindNextFileWithEnoughEmptySpace(config, nAddSize, nFile);
        pos = { static_cast<int>(nFile), static_cast<unsigned int>(vinfoBlockFile[nFile].Size()) };
    }

    if ((int)nFile != nLastBlockFile) {
        if (!fKnown) {
            LogPrintf("Leaving block file %i: %s\n", nLastBlockFile,
                vinfoBlockFile[nLastBlockFile].ToString());
        }
        pBlockFileInfoStore->FlushBlockFile(!fKnown);
        nLastBlockFile = nFile;
    }

    if (fKnown) {
        vinfoBlockFile[nFile].AddKnownBlock( nHeight, nTime, nAddSize, pos.Pos() );
    }
    else {
        vinfoBlockFile[nFile].AddNewBlock( nHeight, nTime, nAddSize );
    }

    if (!fKnown) {
        uint64_t nOldChunks =
            (pos.Pos() + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        uint64_t nNewChunks =
            (vinfoBlockFile[nFile].Size() + BLOCKFILE_CHUNK_SIZE - 1) /
            BLOCKFILE_CHUNK_SIZE;
        if (nNewChunks > nOldChunks) {
            if (fPruneMode) {
                fCheckForPruning = true;
            }

            if (!BlockFileAccess::PreAllocateBlock( nNewChunks, pos ))
            {
                return state.Error("out of disk space");
            }
        }
    }

    setDirtyFileInfo.insert(nFile);
    return true;
}

bool CBlockFileInfoStore::FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos,
    uint64_t nAddSize, bool& fCheckForPruning) {

    LOCK(cs_LastBlockFile);

    pos = { nFile, static_cast<unsigned int>(vinfoBlockFile[nFile].UndoSize()) };
    uint64_t nNewSize = vinfoBlockFile[nFile].AddUndoSize( nAddSize );
    setDirtyFileInfo.insert(nFile);

    uint64_t nOldChunks =
        (pos.Pos() + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    uint64_t nNewChunks =
        (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (fPruneMode) {
            fCheckForPruning = true;
        }

        if (!BlockFileAccess::PreAllocateUndoBlock( nNewChunks, pos ))
        {
            return state.Error("out of disk space");
        }
    }

    return true;
}

/**
 * Calculate the amount of disk space the block & undo files currently use.
 */
uint64_t CBlockFileInfoStore::CalculateCurrentUsage() {
    // TODO: this method currently required cs_LastBlockFile to be held. Consider moving locking code insied this method  
    uint64_t retval = 0;
    for (const CBlockFileInfo &file : vinfoBlockFile) {
        retval += file.Size() + file.UndoSize();
    }
    return retval;
}

void CBlockFileInfoStore::ClearFileInfo(int fileNumber)
{
    vinfoBlockFile[fileNumber] = {};
    setDirtyFileInfo.insert(fileNumber);
}


/**
 * Calculate the block/rev files to delete based on height specified by user
 * with RPC command pruneblockchain
 */
void CBlockFileInfoStore::FindFilesToPruneManual(
    const Config& config,
    std::set<int> &setFilesToPrune,
    int32_t nManualPruneHeight)
{
    assert(fPruneMode && nManualPruneHeight > 0);

    LOCK2(cs_main, cs_LastBlockFile);
    if (chainActive.Tip() == nullptr) {
        return;
    }

    // last block to prune is the lesser of (user-specified height,
    // configured minimum blocks to keep from the tip)
    int32_t nLastBlockWeCanPrune =
        (chainActive.Tip()->GetHeight() < config.GetMinBlocksToKeep()) ?
        nManualPruneHeight :
        std::min(nManualPruneHeight, chainActive.Tip()->GetHeight() - config.GetMinBlocksToKeep());
    int count = 0;
    for (int fileNumber = 0; fileNumber < nLastBlockFile; fileNumber++) {
        if (vinfoBlockFile[fileNumber].Size() == 0 ||
            vinfoBlockFile[fileNumber].HeightLast() > nLastBlockWeCanPrune) {
            continue;
        }
        setFilesToPrune.insert(fileNumber);
        count++;
    }
    LogPrintf("Prune (Manual): prune_height=%d found %d blk/rev pairs for removal\n",
        nLastBlockWeCanPrune, count);
}


/**
 * Prune block and undo files (blk???.dat and undo???.dat) so that the disk
 * space used is less than a user-defined target. The user sets the target (in
 * MB) on the command line or in config file.  This will be run on startup and
 * whenever new space is allocated in a block or undo file, staying below the
 * target. Changing back to unpruned requires a reindex (which in this case
 * means the blockchain must be re-downloaded.)
 *
 * Pruning functions are called from FlushStateToDisk when the global
 * fCheckForPruning flag has been set. Block and undo files are deleted in
 * lock-step (when blk00003.dat is deleted, so is rev00003.dat.). Pruning cannot
 * take place until the longest chain is at least a certain length (100000 on
 * mainnet, 1000 on testnet, 1000 on regtest). Pruning will never delete a block
 * within a defined distance (currently 288) from the active chain's tip. The
 * block index is updated by unsetting HAVE_DATA and HAVE_UNDO for any blocks
 * that were stored in the deleted files. A db flag records the fact that at
 * least some block files have been pruned.
 *
 * @param[out]   setFilesToPrune   The set of file indices that can be unlinked
 * will be returned
 */
void CBlockFileInfoStore::FindFilesToPrune(
    const Config& config,
    std::set<int> &setFilesToPrune,
    int32_t nPruneAfterHeight)
{
    LOCK2(cs_main, cs_LastBlockFile);
    if (chainActive.Tip() == nullptr || nPruneTarget == 0) {
        return;
    }
    if (chainActive.Tip()->GetHeight() <= nPruneAfterHeight) {
        return;
    }

    int32_t nLastBlockWeCanPrune =
        chainActive.Tip()->GetHeight() - config.GetMinBlocksToKeep();
    uint64_t nCurrentUsage = pBlockFileInfoStore->CalculateCurrentUsage();
    // We don't check to prune until after we've allocated new space for files,
    // so we should leave a buffer under our target to account for another
    // allocation before the next pruning.
    uint64_t nBuffer = BLOCKFILE_CHUNK_SIZE + UNDOFILE_CHUNK_SIZE;
    uint64_t nBytesToPrune;
    int count = 0;

    if (nCurrentUsage + nBuffer >= nPruneTarget) {
        for (int fileNumber = 0; fileNumber < nLastBlockFile; fileNumber++) {
            nBytesToPrune = vinfoBlockFile[fileNumber].Size() +
                vinfoBlockFile[fileNumber].UndoSize();

            if (vinfoBlockFile[fileNumber].Size() == 0)
            {
                continue;
            }

            // are we below our target?
            if (nCurrentUsage + nBuffer < nPruneTarget) {
                break;
            }

            // don't prune files that could have a block within
            // configured minimum number of blocks to keep of the main chain's tip but keep scanning
            if (vinfoBlockFile[fileNumber].HeightLast() > nLastBlockWeCanPrune) {
                continue;
            }

            // Queue up the files for removal
            setFilesToPrune.insert(fileNumber);
            nCurrentUsage -= nBytesToPrune;
            count++;
        }
    }

    LogPrint(BCLog::PRUNE, "Prune: target=%dMiB actual=%dMiB diff=%dMiB "
        "max_prune_height=%d found %d blk/rev pairs for removal\n",
        nPruneTarget / ONE_MEBIBYTE, nCurrentUsage / ONE_MEBIBYTE,
        ((int64_t)nPruneTarget - (int64_t)nCurrentUsage) / ONE_MEBIBYTE,
        nLastBlockWeCanPrune, count);
}

void CBlockFileInfoStore::LoadBlockFileInfo(int nLastBlockFile, CBlockTreeDB& blockTreeDb)
{
    this->nLastBlockFile = nLastBlockFile;
    vinfoBlockFile.resize(nLastBlockFile + 1);
    LogPrintf("%s: last block file = %i\n", __func__, nLastBlockFile);
    for (int nFile = 0; nFile <= nLastBlockFile; nFile++) {
        blockTreeDb.ReadBlockFileInfo(nFile, vinfoBlockFile[nFile]);
    }
    LogPrintf("%s: last block file info: %s\n", __func__,
        vinfoBlockFile[nLastBlockFile].ToString());
    for (int nFile = nLastBlockFile + 1; true; nFile++) {
        CBlockFileInfo info;
        if (blockTreeDb.ReadBlockFileInfo(nFile, info)) {
            vinfoBlockFile.push_back(info);
        }
        else {
            break;
        }
    }
}

void CBlockFileInfoStore::Clear()
{
    vinfoBlockFile.clear();
    nLastBlockFile = 0;
    setDirtyFileInfo.clear();
}

CBlockFileInfo *CBlockFileInfoStore::GetBlockFileInfo(size_t n)
{
    return &vinfoBlockFile.at(n);
}
