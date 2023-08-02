// Copyright (c) 2021 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file
// LICENSE.

#pragma once

#include <cstdint>

/**
 * Default configuration values used in dsdetected message processing.
 */
struct DSDetectedDefaults
{
    // Default maximum transaction size to report over a dsdetected webhook (in
    // MB)
    static constexpr uint64_t DEFAULT_MAX_WEBHOOK_TXN_SIZE{100};
};
