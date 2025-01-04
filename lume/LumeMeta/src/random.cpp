/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "random.h"

META_BEGIN_NAMESPACE()
namespace {
// for more information see https://prng.di.unimi.it/
// xoshiro256++ implementation based on the code from https://prng.di.unimi.it/xoshiro256plusplus.c
class Xoroshiro128 : public IRandom {
public:
    Xoroshiro128(uint64_t seed)
    {
        Initialize(seed);
    }
    ~Xoroshiro128() override = default;

    void Initialize(uint64_t seed) override
    {
        // as recommended for xoshiro256++ initialize the state with splitmix64.
        uint64_t z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        s_[0] = z ^ (z >> 31);
        z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        s_[1] = z ^ (z >> 31);
        z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        s_[2] = z ^ (z >> 31);
        z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        s_[3] = z ^ (z >> 31);
    }

    uint64_t GetRandom() override
    {
        const uint64_t result = Rotl(s_[0] + s_[3], 23) + s_[0];
        const uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = Rotl(s_[3], 45);
        return result;
    }

private:
    inline uint64_t Rotl(const uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t s_[4];
};
} // namespace

BASE_NS::unique_ptr<IRandom> CreateXoroshiro128(uint64_t seed)
{
    return BASE_NS::unique_ptr<IRandom>(new Xoroshiro128(seed));
}

META_END_NAMESPACE()
