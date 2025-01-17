// Copyright (c) 2017 The Prettywomancoin developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.
#include "dstencode.h"
#include "base58.h"
#include "chainparams.h"
#include "config.h"
#include "script/standard.h"

std::string EncodeDestination(const CTxDestination &dest,
                              const Config &config) {
    const CChainParams &params = config.GetChainParams();
    return EncodeBase58Addr(dest, params);
}

CTxDestination DecodeDestination(const std::string &addr,
                                 const CChainParams &params) {
    return DecodeBase58Addr(addr, params);
}

bool IsValidDestinationString(const std::string &addr,
                              const CChainParams &params) {
    return IsValidDestination(DecodeDestination(addr, params));
}

std::string EncodeDestination(const CTxDestination &dst) {
    return EncodeDestination(dst, GlobalConfig::GetConfig());
}

CTxDestination DecodeDestination(const std::string &addr) {
    return DecodeDestination(addr, Params());
}

bool IsValidDestinationString(const std::string &addr) {
    return IsValidDestinationString(addr, Params());
}
