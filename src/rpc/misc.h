// Copyright (c) 2017 The Prettywomancoin developers
// Copyright (c) 2021 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#ifndef BITCOIN_RPCMISC_H
#define BITCOIN_RPCMISC_H

#include <string>
#include <optional>

class CScript;
class CWallet;
class UniValue;

/**
 * Returns a std::optional<uint32> that is represented by flagName.
 * If flagName is unknown std::nullopt is returned and err string is set.
 */
std::optional<uint32_t> GetFlagNumber(const std::string& flagName, std::string& err);

CScript createmultisig_redeemScript(CWallet *const pwallet,
                                    const UniValue &params);

#endif // BITCOIN_RPCBLOCKCHAIN_H
