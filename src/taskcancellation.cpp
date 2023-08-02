// Copyright (c) 2023 Prettywomancoin Association.
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#include "taskcancellation.h"

namespace task
{   

bool CCancellationToken::IsCanceled() const
{
    return
        std::any_of(
            mSource.begin(),
            mSource.end(),
            [](auto source){ return source->IsCanceled(); });
}

}
