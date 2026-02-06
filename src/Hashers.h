#pragma once

#include <cstdint>
#include <string_view>

struct PolyHash32
{
    PolyHash32(uint32_t base, uint32_t seed) noexcept
        : m_base(base),
          m_seed(seed)
    {
    }

    uint32_t operator()(std::string_view value) const noexcept
    {
        uint32_t hashValue = m_seed;

        for (unsigned char ch : value)
        {
            hashValue = hashValue * m_base + static_cast<uint32_t>(ch);
        }

        return hashValue;
    }

private:
    uint32_t m_base;
    uint32_t m_seed;
};

struct Fnv1a32
{
    Fnv1a32(uint32_t base, uint32_t seed) noexcept
        : m_base(base),
          m_seed(seed)
    {
    }

    uint32_t operator()(std::string_view value) const noexcept
    {
        constexpr uint32_t kOffsetBasis = 2166136261u;

        uint32_t hashValue = kOffsetBasis ^ m_seed;

        for (unsigned char ch : value)
        {
            hashValue ^= static_cast<uint32_t>(ch);
            hashValue *= m_base;
        }

        return hashValue;
    }

private:
    uint32_t m_base;
    uint32_t m_seed;
};

class HashFuncGen
{
public:
    static PolyHash32 MakePolyHash32(uint32_t base, uint32_t seed) noexcept
    {
        return PolyHash32(base, seed);
    }

    static Fnv1a32 MakeFnv1a32(uint32_t base, uint32_t seed) noexcept
    {
        return Fnv1a32(base, seed);
    }
};
