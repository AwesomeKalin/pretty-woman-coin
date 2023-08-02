// Copyright (c) 2020 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "zmq_sink.h"

#include "validationinterface.h"

using namespace InvalidTxnPublisher;

#if ENABLE_ZMQ
void CInvalidTxnZmqSink::Publish(const InvalidTxnInfo& invalidTxInfo)
{
    const auto messageSize = EstimateMessageSize(invalidTxInfo, true);
    CStringWriter sw;
    sw.ReserveAdditional(messageSize); 
    CJSONWriter jw(sw, false);
    invalidTxInfo.ToJson(jw, messageSize <= maxMessageSize);
    std::string jsonString = sw.MoveOutString();
    GetMainSignals().InvalidTxMessageZMQ(jsonString);
}
#endif
