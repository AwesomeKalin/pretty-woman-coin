// Copyright (c) 2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include <iostream>

#include "bench.h"
#include "bloom.h"

static void RollingBloom(benchmark::State &state) {
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<uint8_t> data(32);
    uint32_t count = 0;
    uint32_t nEntriesPerGeneration = (120000 + 1) / 2;
    uint32_t countnow = 0;
    uint64_t match = 0;
    while (state.KeepRunning()) {
        count++;
        data[0] = count;
        data[1] = count >> 8;
        data[2] = count >> 16;
        data[3] = count >> 24;
        if (countnow == nEntriesPerGeneration) {
            auto b = benchmark::clock::now();
            filter.insert(data);
            auto total = std::chrono::duration_cast<std::chrono::nanoseconds>(benchmark::clock::now() - b).count();
            std::cout << "RollingBloom-refresh,1," << total << "," << total << "," << total << "\n";
            countnow = 0;
        } else {
            filter.insert(data);
        }
        countnow++;
        data[0] = count >> 24;
        data[1] = count >> 16;
        data[2] = count >> 8;
        data[3] = count;
        match += filter.contains(data);
    }
}

BENCHMARK(RollingBloom)
