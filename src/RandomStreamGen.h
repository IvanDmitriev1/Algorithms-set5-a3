#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <random>

class RandomStreamGen
{
    static constexpr std::string_view kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-";

public:
    RandomStreamGen(uint64_t seed, size_t streamSize);

    void Regenerate(size_t streamSize) noexcept;
    inline size_t Size() const noexcept { return m_data.size(); }

    std::span<const std::string> Prefix(size_t prefixCount) const noexcept;
    size_t PrefixSizeByPercent(size_t percent) const noexcept;
    std::vector<size_t> PrefixSizesByStepPercent(size_t stepPercent) const;

private:
    std::string GenerateRandomString();

private:
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<size_t> m_lenDist;
    std::uniform_int_distribution<size_t> m_charDist;
    std::vector<std::string> m_data;
};