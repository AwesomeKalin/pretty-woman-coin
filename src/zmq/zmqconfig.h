// Copyright (c) 2014 The Prettywomancoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQCONFIG_H
#define BITCOIN_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include "config/prettywomancoin-config.h"
#endif

#include <string>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include "primitives/block.h"

void zmqError(const char *str);

#endif // BITCOIN_ZMQ_ZMQCONFIG_H
