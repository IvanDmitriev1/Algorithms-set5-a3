#pragma once
#include <algorithm>
#include <bit>
#include <concepts>
#include <string_view>
#include <cstdint>
#include <vector>
#include <cmath>
#include <utility>
#include <cstddef>
#include <limits>

template <typename THashFunction>
concept HashFunction =
    std::invocable<const THashFunction&, std::string_view> &&
    std::unsigned_integral<std::invoke_result_t<const THashFunction&, std::string_view>>;

template <HashFunction THashFunction>
class HyperLogLog
{
    using HashType = std::invoke_result_t<const THashFunction&, std::string_view>;
    static constexpr int kHashBits = std::numeric_limits<HashType>::digits;

public:
    explicit HyperLogLog(uint8_t precisionBits, THashFunction hasher = THashFunction{})
        : m_precisionBits(precisionBits),
        m_registerCount(size_t{ 1 } << precisionBits),
        m_hasher(std::move(hasher)),
        m_registers(m_registerCount, 0)
    {}

    void Reset()
    {
        std::fill(m_registers.begin(), m_registers.end(), std::uint8_t{ 0 });
    }

    inline size_t RegisterCount() const noexcept { return m_registerCount; }
    inline uint8_t PrecisionBits() const noexcept { return m_precisionBits; }

    void Add(std::string_view value);
    double Estimate() const;

private:
    static double AlphaForRegisterCount(size_t registerCount) noexcept;
    uint8_t ComputeRank(HashType remainingBits) noexcept;

private:
    const uint8_t m_precisionBits = 0;
    const size_t m_registerCount = 0;
    const THashFunction m_hasher;
    std::vector<uint8_t> m_registers;
};

template<HashFunction THashFunction>
inline void HyperLogLog<THashFunction>::Add(std::string_view value)
{
    const HashType hash = m_hasher(value);
    const int shift = kHashBits - static_cast<int>(m_precisionBits);
    const size_t registerIndex = static_cast<size_t>(hash >> shift);
    const HashType remainingBits = static_cast<HashType>(hash << m_precisionBits);
    const uint8_t rank = ComputeRank(remainingBits);

    uint8_t& reg = m_registers[registerIndex];
    reg = std::max(reg, rank);
}

template<HashFunction THashFunction>
inline double HyperLogLog<THashFunction>::Estimate() const
{
    const double registerCount = static_cast<double>(m_registerCount);
    const double alpha = AlphaForRegisterCount(m_registerCount);

    double sum = 0.0;
    size_t zeroRegisterCount = 0;

    for (uint8_t r : m_registers)
    {
        if (r == 0)
        {
            ++zeroRegisterCount;
        }

        sum += std::ldexp(1.0, -static_cast<int>(r));
    }

    double estimate = alpha * registerCount * registerCount / sum;
    if (estimate <= 2.5 * registerCount && zeroRegisterCount > 0)
    {
        estimate = registerCount * std::log(registerCount / static_cast<double>(zeroRegisterCount));
    }

    return estimate;
}

template<HashFunction THashFunction>
inline double HyperLogLog<THashFunction>::AlphaForRegisterCount(size_t registerCount) noexcept
{
    if (registerCount == 16)
        return 0.673;
    if (registerCount == 32)
        return 0.697;
    if (registerCount == 64)
        return 0.709;

    return 0.7213 / (1.0 + 1.079 / static_cast<double>(registerCount));
}

template<HashFunction THashFunction>
inline uint8_t HyperLogLog<THashFunction>::ComputeRank(HashType remainingBits) noexcept
{
    const int remainingBitCount = kHashBits - static_cast<int>(m_precisionBits);

    if (remainingBits == 0)
    {
        return static_cast<uint8_t>(remainingBitCount + 1);
    }

    return static_cast<uint8_t>(std::countl_zero(remainingBits) + 1);
}
