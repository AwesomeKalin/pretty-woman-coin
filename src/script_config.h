// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#pragma once

#include <cstdint>

/**
 * Configuration interface that contains limits used when evaluating scripts.
 * Class must be defined outside config.h as it is used by a dynamic library
 * (libprettywomancoinconsensus) which is not connected to the rest of prettywomancoin code.
 */
class CScriptConfig
{
public:
    virtual uint64_t GetMaxOpsPerScript(bool isGenesisEnabled, bool isConsensus) const = 0;
    virtual uint64_t GetMaxScriptNumLength(bool isGenesisEnabled, bool isConsensus) const = 0;
    virtual uint64_t GetMaxScriptSize(bool isGenesisEnabled, bool isConsensus) const = 0;
    virtual uint64_t GetMaxPubKeysPerMultiSig(bool isGenesisEnabled, bool isConsensus) const = 0;
    virtual uint64_t GetMaxStackMemoryUsage(bool isGenesisEnabled, bool isConsensus) const = 0;

protected:
    ~CScriptConfig() = default;
};