// Copyright (c) 2016 The Prettywomancoin Core developers
// Copyright (c) 2018-2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "bench.h"

#include "config.h"
#include "consensus/validation.h"
#include "streams.h"
#include "validation.h"
#include "chainparams.h"
#include "init.h"

namespace block_bench {
#include "bench/data/block413567.raw.h"
}

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::State &state) {
    CDataStream stream((const char *)block_bench::block413567,
                       (const char *)&block_bench::block413567[sizeof(
                           block_bench::block413567)],
                       SER_NETWORK, PROTOCOL_VERSION);
    char a;
    stream.write(&a, 1); // Prevent compaction

    while (state.KeepRunning()) {
        CBlock block;
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block413567)));
    }
}

static void DeserializeAndCheckBlockTest(benchmark::State &state) {
    CDataStream stream((const char *)block_bench::block413567,
                       (const char *)&block_bench::block413567[sizeof(
                           block_bench::block413567)],
                       SER_NETWORK, PROTOCOL_VERSION);
    char a;
    stream.write(&a, 1); // Prevent compaction

    SelectParams(CBaseChainParams::MAIN);
    ConfigInit &config = GlobalConfig::GetModifiableGlobalConfig();
    config.SetDefaultBlockSizeParams(Params().GetDefaultBlockSizeParams());

    assert(AppInitParameterInteraction(config));

    while (state.KeepRunning()) {
        // Note that CBlock caches its checked state, so we need to recreate it
        // here.
        CBlock block;
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block413567)));

        CValidationState validationState;
        assert(CheckBlock(config, block, validationState, 413567));
    }
}

BENCHMARK(DeserializeBlockTest)
BENCHMARK(DeserializeAndCheckBlockTest)
