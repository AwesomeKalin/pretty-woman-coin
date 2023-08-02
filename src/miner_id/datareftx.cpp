// Copyright (c) 2022 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "datareftx.h"

bool operator==(const DataRefTx& msg1, const DataRefTx& msg2)
{
    return ((msg1.mTxn && msg2.mTxn && *msg1.mTxn == *msg2.mTxn) ||
            (!msg1.mTxn && !msg2.mTxn)) &&
           msg1.mMerkleProof == msg2.mMerkleProof;
}

