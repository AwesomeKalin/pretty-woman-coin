// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file LICENSE.

#ifndef BITCOIN_AMOUNT_H
#define BITCOIN_AMOUNT_H

#include "serialize.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

struct Amount {
private:
    int64_t amount;

public:
    constexpr Amount() : amount(0) {}

    template <typename T>
    explicit constexpr Amount(T _camount) : amount(_camount) {
        static_assert(std::is_integral<T>(),
                      "Only integer types can be used as amounts");
    }

    constexpr Amount(const Amount &_camount) : amount(_camount.amount) {}
    constexpr Amount& operator=(const Amount&) = default;

    // Allow access to underlying value for non-monetary operations
    int64_t GetSatoshis() const { return amount; }

    /**
     * Implement standard operators
     */
    Amount &operator+=(const Amount a) {
        amount += a.amount;
        return *this;
    }
    Amount &operator-=(const Amount a) {
        amount -= a.amount;
        return *this;
    }

    /**
     * Equality
     */
    friend constexpr bool operator==(const Amount a, const Amount b) {
        return a.amount == b.amount;
    }
    friend constexpr bool operator!=(const Amount a, const Amount b) {
        return !(a == b);
    }

    /**
     * Comparison
     */
    friend constexpr bool operator<(const Amount a, const Amount b) {
        return a.amount < b.amount;
    }
    friend constexpr bool operator>(const Amount a, const Amount b) {
        return b < a;
    }
    friend constexpr bool operator<=(const Amount a, const Amount b) {
        return !(a > b);
    }
    friend constexpr bool operator>=(const Amount a, const Amount b) {
        return !(a < b);
    }

    /**
     * Unary minus
     */
    constexpr Amount operator-() const { return Amount(-amount); }

    /**
     * Addition and subtraction.
     */
    friend constexpr Amount operator+(const Amount a, const Amount b) {
        return Amount(a.amount + b.amount);
    }
    friend constexpr Amount operator-(const Amount a, const Amount b) {
        return a + -b;
    }

    /**
     * Multiplication
     */
    friend constexpr Amount operator*(const int64_t a, const Amount b) {
        return Amount(a * b.amount);
    }
    friend constexpr Amount operator*(const int a, const Amount b) {
        return Amount(a * b.amount);
    }

    /**
     * Division
     */
    constexpr int64_t operator/(const Amount b) const {
        return amount / b.amount;
    }
    constexpr Amount operator/(const int64_t b) const {
        return Amount(amount / b);
    }
    constexpr Amount operator/(const int b) const { return Amount(amount / b); }

    /**
     * Modulus
     */
    constexpr int64_t operator%(const Amount b) const {
        return amount % b.amount;
    }
    constexpr Amount operator%(const int64_t b) const {
        return Amount(amount % b);
    }
    constexpr Amount operator%(const int b) const { return Amount(amount % b); }

    /**
     * Do not implement double ops to get an error with double and ensure
     * casting to integer is explicit.
     */
    friend constexpr Amount operator*(const double a, const Amount b) = delete;
    constexpr Amount operator/(const double b) const = delete;
    constexpr Amount operator%(const double b) const = delete;

    // ostream support
    friend std::ostream &operator<<(std::ostream &stream, const Amount &ca) {
        return stream << ca.amount;
    }

    std::string ToString() const;

    // serialization support
    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(amount);
    }
};

static const Amount COIN(100000000);
static const Amount CENT(1000000);

extern const std::string CURRENCY_UNIT;

/**
 * No amount larger than this (in satoshi) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Pretty Woman Coin
 * currently happens to be less than 21,000,000 PWC for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 */
static const Amount MAX_MONEY = 21000000 * COIN;
inline bool MoneyRange(const Amount nValue) {
    return (nValue >= Amount(0) && nValue <= MAX_MONEY);
}

/**
 * Fee rate in satoshis per kilobyte: Amount / kB
 */
class CFeeRate {
private:
    // unit is satoshis-per-1,000-bytes
    Amount nSatoshisPerK {0};

public:
    CFeeRate () = default;
    explicit CFeeRate(const Amount _nSatoshisPerK)
        : nSatoshisPerK(_nSatoshisPerK) {}
    /**
     * Constructor for a fee rate in satoshis per kB. The size in bytes must not
     * exceed (2^63 - 1)
     */
    CFeeRate(const Amount nFeePaid, size_t nBytes);
    /**
     * Return the fee in satoshis for the given size in bytes.
     */
    Amount GetFee(size_t nBytes) const;
    /**
     * Return the fee in satoshis for a size of 1000 bytes
     */
    Amount GetFeePerK() const { return nSatoshisPerK; }
    friend bool operator<(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK < b.nSatoshisPerK;
    }
    friend bool operator>(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK > b.nSatoshisPerK;
    }
    friend bool operator==(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK == b.nSatoshisPerK;
    }
    friend bool operator<=(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK <= b.nSatoshisPerK;
    }
    friend bool operator>=(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK >= b.nSatoshisPerK;
    }
    CFeeRate &operator+=(const CFeeRate &a) {
        nSatoshisPerK += a.nSatoshisPerK;
        return *this;
    }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(nSatoshisPerK);
    }
};

#endif //  BITCOIN_AMOUNT_H
