// Copyright (c) 2020 Prettywomancoin Association.
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#pragma once

#include <cstdio>
#include <memory>

// Helpers for use with std::unique_ptr to enable RAII closing of FILE*
struct CloseFileDeleter
{
    void operator()(FILE* file) { ::fclose(file); }
};
using UniqueCFile = std::unique_ptr<FILE, CloseFileDeleter>;

/**
 * A very simple RAII wrapper for a file-descriptor.
 *
 * Ensures only a single wrapped copy of the file-descriptor exists, and
 * closes that descriptor on destruction.
 */
class UniqueFileDescriptor final
{
  public:
    UniqueFileDescriptor() = default;
    UniqueFileDescriptor(int fd) : mFd{fd} {}
    UniqueFileDescriptor(UniqueFileDescriptor&& that) noexcept : mFd{that.Release()} {}
    UniqueFileDescriptor(const UniqueFileDescriptor&) = delete;
    UniqueFileDescriptor& operator=(UniqueFileDescriptor&& that) noexcept;
    UniqueFileDescriptor& operator=(const UniqueFileDescriptor& that) = delete;
    ~UniqueFileDescriptor() noexcept;

    // Get the managed file-descriptor
    [[nodiscard]] int Get() const noexcept { return mFd; }
    // Release ownership of the managed file-descriptor
    [[nodiscard]] int Release() noexcept;
    // Free and clear our file-descriptor
    void Reset() noexcept;

  private:
    int mFd {-1};
};

