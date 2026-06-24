//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_RENDERER_H
#define RAYTRACER_RENDERER_H


#include "Writer/ImageWriter.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"
#include "Math/Sampler.h"
#include <atomic>
#include <cstdint>

struct RenderSettings {
    unsigned int samplesPerPixel;
    unsigned int previewInterval;
    std::uint64_t randomSeed;
    unsigned int workerCount;
    unsigned int maxBounces;
    unsigned int russianRouletteStart;

    RenderSettings(unsigned int samplesPerPixel = 1,
                   unsigned int previewInterval = 0,
                   std::uint64_t randomSeed = 1,
                   unsigned int workerCount = 0,
                   unsigned int maxBounces = 8,
                   unsigned int russianRouletteStart = 4)
        : samplesPerPixel(samplesPerPixel),
          previewInterval(previewInterval),
          randomSeed(randomSeed),
          workerCount(workerCount),
          maxBounces(maxBounces),
          russianRouletteStart(russianRouletteStart) {
    }
};

class Renderer {

    public:
        Renderer(ImageWriter& imageWriter, const Scene& scene,
                 const Camera& camera,
                 const RenderSettings& settings = RenderSettings());

        void render();
        static double sampleOffset(std::size_t pixelIndex,
                                   unsigned int sampleIndex,
                                   std::uint64_t seed,
                                   unsigned int dimension);


    private:
        void renderPass(unsigned int sampleIndex);
        std::vector<std::vector<Vec3>> averagedFrame(
            unsigned int completedSamples) const;

        ImageWriter& imageWriter;
        const Scene& scene;
        const Camera& camera;
        RenderSettings settings;
        std::vector<std::vector<Vec3>> accumulationBuffer;
};


#endif //RAYTRACER_RENDERER_H
