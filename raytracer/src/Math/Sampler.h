#ifndef RAYTRACER_SAMPLER_H
#define RAYTRACER_SAMPLER_H

#include <cstddef>
#include <cstdint>

class Sampler {
public:
    Sampler(std::size_t pixelIndex, unsigned int sampleIndex,
            std::uint64_t seed)
        : pixelIndex(pixelIndex),
          sampleIndex(sampleIndex),
          seed(seed),
          dimension(0) {
    }

    double next();
    static double sample(std::size_t pixelIndex, unsigned int sampleIndex,
                         std::uint64_t seed, unsigned int dimension);

private:
    std::size_t pixelIndex;
    unsigned int sampleIndex;
    std::uint64_t seed;
    unsigned int dimension;
};

#endif

