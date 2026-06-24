#include "Sampler.h"

double Sampler::next() {
    return sample(pixelIndex, sampleIndex, seed, dimension++);
}

double Sampler::sample(std::size_t pixelIndex, unsigned int sampleIndex,
                       std::uint64_t seed, unsigned int dimension) {
    std::uint64_t value = seed;
    value ^= static_cast<std::uint64_t>(pixelIndex) +
             0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
    value ^= static_cast<std::uint64_t>(sampleIndex) *
             0xbf58476d1ce4e5b9ULL;
    value ^= static_cast<std::uint64_t>(dimension) *
             0x94d049bb133111ebULL;
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;

    const std::uint64_t mantissa = value >> 11;
    return static_cast<double>(mantissa) *
           (1.0 / 9007199254740992.0);
}

