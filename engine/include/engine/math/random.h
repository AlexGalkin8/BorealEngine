#pragma once

#include <cstdint>

namespace math {

class Random
{
public:
    explicit Random(uint64_t seed = 0x853c49e6748fea9bull) : m_State(seed ? seed : 0x853c49e6748fea9bull) {}

    void seed(uint64_t s) { m_State = s ? s : 0x853c49e6748fea9bull; }

    uint32_t nextU32()
    {
        uint64_t x = m_State;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        m_State = x;
        return static_cast<uint32_t>((x * 2685821657736338717ull) >> 32);
    }

    float nextFloat01()
    {
        return static_cast<float>((nextU32() >> 8) * (1.0 / 16777216.0));
    }

    float range(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * nextFloat01();
    }

    int32_t rangeInt(int32_t minValue, int32_t maxValue)
    {
        if (maxValue <= minValue) return minValue;
        uint32_t span = static_cast<uint32_t>(maxValue - minValue + 1);
        return minValue + static_cast<int32_t>(nextU32() % span);
    }

private:
    uint64_t m_State;
};

} // namespace math
