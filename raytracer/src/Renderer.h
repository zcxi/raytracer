//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_RENDERER_H
#define RAYTRACER_RENDERER_H


#include "Writer/ImageWriter.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"
#include "Math/Sampler.h"
#include "Diagnostics/TraceStats.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

struct RenderSettings {
    unsigned int samplesPerPixel;
    unsigned int previewInterval;
    std::uint64_t randomSeed;
    unsigned int workerCount;
    unsigned int maxBounces;
    unsigned int russianRouletteStart;
    unsigned int tileSize;

    RenderSettings(unsigned int samplesPerPixel = 1,
                   unsigned int previewInterval = 0,
                   std::uint64_t randomSeed = 1,
                   unsigned int workerCount = 0,
                   unsigned int maxBounces = 8,
                   unsigned int russianRouletteStart = 4,
                   unsigned int tileSize = 16)
        : samplesPerPixel(samplesPerPixel),
          previewInterval(previewInterval),
          randomSeed(randomSeed),
          workerCount(workerCount),
          maxBounces(maxBounces),
          russianRouletteStart(russianRouletteStart),
          tileSize(tileSize) {
    }
};

class Renderer {

    public:
        Renderer(ImageWriter& imageWriter, const Scene& scene,
                 const Camera& camera,
                 const RenderSettings& settings = RenderSettings());
        ~Renderer();

        void render();
        static double sampleOffset(std::size_t pixelIndex,
                                   unsigned int sampleIndex,
                                   std::uint64_t seed,
                                   unsigned int dimension);


    private:
        double renderPass(unsigned int sampleIndex);
        void startWorkers();
        void stopWorkers();
        void workerLoop(unsigned int workerIndex);
        void renderTiles(unsigned int sampleIndex, TraceStats& stats);
        TraceStats combinedStats() const;

        ImageWriter& imageWriter;
        const Scene& scene;
        const Camera& camera;
        RenderSettings settings;
        std::vector<std::vector<Vec3>> accumulationBuffer;
        std::vector<std::thread> workers;
        std::vector<TraceStats> workerStats;
        std::atomic<std::size_t> nextTile;
        std::mutex workMutex;
        std::condition_variable workAvailable;
        std::condition_variable passFinished;
        bool stopping;
        std::size_t workGeneration;
        unsigned int activeWorkers;
        unsigned int currentSample;
        unsigned int tilesX;
        std::size_t tileCount;
};


#endif //RAYTRACER_RENDERER_H
