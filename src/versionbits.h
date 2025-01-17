// Copyright (c) 2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#ifndef BITCOIN_CONSENSUS_VERSIONBITS
#define BITCOIN_CONSENSUS_VERSIONBITS

#include <cstdint>

/** Version bits are not used anymore.
    This variable is used in assembler.cpp for consistency with old code and to set the version of block that we are going to mine. */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;

#endif
