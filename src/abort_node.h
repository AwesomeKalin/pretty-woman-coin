// Copyright (c) 2021 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#ifndef BITCOIN_ABORTNODE_H
#define BITCOIN_ABORTNODE_H

#include <iostream>
#include "consensus/validation.h"

bool AbortNode(const std::string &strMessage,
               const std::string &userMessage = "");

bool AbortNode(CValidationState &state, const std::string &strMessage,
               const std::string &userMessage = "");

#endif
