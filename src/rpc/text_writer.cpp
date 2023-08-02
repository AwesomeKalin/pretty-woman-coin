// Copyright (c) 2022 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "rpc/text_writer.h"

void CStringWriter::ReserveAdditional(size_t size)
{
    if(strBuffer.size() + size > strBuffer.capacity())
    {
        strBuffer.reserve(strBuffer.size() + size);
    }
}


