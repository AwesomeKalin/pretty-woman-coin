// Copyright (c) 2021 Prettywomancoin Association.
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#pragma once

#include "uint256.h"

struct BlockHasher {
    size_t operator()(const uint256 &hash) const { return hash.GetCheapHash(); }
};
