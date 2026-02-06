#include "RandomStreamGen.h"

#include <algorithm>

RandomStreamGen::RandomStreamGen(uint64_t seed, size_t streamSize)
    : m_rng(seed),
    m_lenDist(1, 30),
    m_charDist(0, static_cast<int>(kAlphabet.size() - 1))
{
    Regenerate(streamSize);
}

void RandomStreamGen::Regenerate(size_t streamSize) noexcept
{
    m_data.clear();
    m_data.reserve(streamSize);

    for (size_t i = 0; i < streamSize; ++i)
    {
        m_data.push_back(GenerateRandomString());
    }
}

std::span<const std::string> RandomStreamGen::Prefix(size_t prefixCount) const noexcept
{
    prefixCount = std::min(prefixCount, m_data.size());
    return { m_data.data(), prefixCount };
}

size_t RandomStreamGen::PrefixSizeByPercent(size_t percent) const noexcept
{
    percent = std::min<size_t>(percent, 100);
    return (m_data.size() * percent) / 100;
}

std::vector<size_t> RandomStreamGen::PrefixSizesByStepPercent(size_t stepPercent) const
{
    std::vector<size_t> sizes;
    if (stepPercent == 0)
    {
        return sizes;
    }

    for (size_t p = 0; p <= 100; p += stepPercent)
    {
        sizes.push_back(PrefixSizeByPercent(p));
    }

    if (sizes.empty() || sizes.back() != m_data.size())
    {
        sizes.push_back(m_data.size());
    }

    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    return sizes;
}

std::string RandomStreamGen::GenerateRandomString()
{
    const size_t length = m_lenDist(m_rng);

    std::string result;
    result.resize(length);

    for (size_t i = 0; i < length; ++i)
    {
        const size_t index = m_charDist(m_rng);
        result[i] = kAlphabet[index];
    }

    return result;
}
