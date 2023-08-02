// Copyright (c) 2021 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#ifndef BITCOIN_PROCESSINGBLOCKINDEX_H
#define BITCOIN_PROCESSINGBLOCKINDEX_H

#include "primitives/block.h"
#include "undo.h"

class ProcessingBlockIndex
{
public:
    template<typename T> struct UnitTestAccess;

    ProcessingBlockIndex( CBlockIndex& index ) : mIndex(index) {}

    DisconnectResult DisconnectBlock(
        const CBlock& block,
        CCoinsViewCache& view,
        const task::CCancellationToken& shutdownToken) const;

private:

    DisconnectResult ApplyBlockUndo(
        const CBlockUndo& blockUndo,
        const CBlock& block,
        CCoinsViewCache& view,
        const task::CCancellationToken& shutdownToken) const;

    CBlockIndex& mIndex;
};

#endif
