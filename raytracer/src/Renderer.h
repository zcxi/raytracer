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
#include "PostProcessing/Denoiser.h"
#include <atomic>
#include <cmath>
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
    bool collectStats;

    bool adaptiveSampling;
    unsigned int adaptiveMinSamples;
    unsigned int adaptiveMaxSamples;
    unsigned int adaptiveBatch;
    double adaptiveRelativeError;
    double adaptiveAbsoluteError;
    double adaptiveLuminanceFloor;

    DenoiseSettings denoise;

    RenderSettings(unsigned int samplesPerPixel = 1,
                   unsigned int previewInterval = 0,
                   std::uint64_t randomSeed = 1,
                   unsigned int workerCount = 0,
                   unsigned int maxBounces = 8,
                   unsigned int russianRouletteStart = 4,
                   unsigned int tileSize = 16,
                   bool collectStats = false,
                   bool adaptive = false,
                   unsigned int minAdaptiveSamples = 16,
                   unsigned int maxAdaptiveSamples = 1024,
                   unsigned int adaptBatchSize = 8,
                   double adaptRelativeErr = 0.05,
                   double adaptAbsoluteErr = 0.005,
                   double adaptLuminanceFloor = 0.01)
        : samplesPerPixel(samplesPerPixel),
          previewInterval(previewInterval),
          randomSeed(randomSeed),
          workerCount(workerCount),
          maxBounces(maxBounces),
          russianRouletteStart(russianRouletteStart),
          tileSize(tileSize),
          collectStats(collectStats),
          adaptiveSampling(adaptive),
          adaptiveMinSamples(minAdaptiveSamples),
          adaptiveMaxSamples(maxAdaptiveSamples),
          adaptiveBatch(adaptBatchSize),
          adaptiveRelativeError(adaptRelativeErr),
          adaptiveAbsoluteError(adaptAbsoluteErr),
          adaptiveLuminanceFloor(adaptLuminanceFloor) {
    }
};

struct AccumulationRecord {
    Vec3 rgbSum;
    Vec3 albedoSum;
    Vec3 normalSum;
    Vec3 positionSum;
    double luminanceMean;
    double luminanceM2;
    unsigned int sampleCount;
    bool converged;

    AccumulationRecord()
        : rgbSum(), albedoSum(), normalSum(), positionSum(),
          luminanceMean(0.0), luminanceM2(0.0),
          sampleCount(0), converged(false) {}

    void addSample(const Vec3& rgb, const Vec3& albedo,
                   const Vec3& normal, const Vec3& position) {
        rgbSum = rgbSum + rgb;
        albedoSum = albedoSum + albedo;
        normalSum = normalSum + normal;
        positionSum = positionSum + position;
        double luminance = 0.2126 * rgb.X() + 0.7152 * rgb.Y() +
                           0.0722 * rgb.Z();
        ++sampleCount;
        double delta = luminance - luminanceMean;
        luminanceMean += delta / sampleCount;
        double delta2 = luminance - luminanceMean;
        luminanceM2 += delta * delta2;
    }

    void addSample(const Vec3& rgb) {
        addSample(rgb, Vec3(), Vec3(), Vec3());
    }

    Vec3 averaged() const {
        return sampleCount == 0 ? Vec3() : rgbSum / sampleCount;
    }
    Vec3 averagedAlbedo() const {
        return sampleCount == 0 ? Vec3() : albedoSum / sampleCount;
    }
    Vec3 averagedNormal() const {
        return sampleCount == 0 ? Vec3() : normalSum / sampleCount;
    }
    Vec3 averagedPosition() const {
        return sampleCount == 0 ? Vec3() : positionSum / sampleCount;
    }

    double luminanceVariance() const {
        return sampleCount < 2 ? 0.0
                               : luminanceM2 / (sampleCount - 1);
    }

    double relativeError(double luminanceFloor) const {
        double variance = luminanceVariance();
        if (variance <= 0.0) return 0.0;
        double stdError = std::sqrt(variance / sampleCount);
        double ref = std::max(luminanceFloor, luminanceMean);
        return stdError / ref;
    }

    double absoluteError() const {
        double variance = luminanceVariance();
        return variance <= 0.0
                   ? 0.0
                   : std::sqrt(variance / sampleCount);
    }

    bool checkConvergence(unsigned int minSamples,
                          double relativeThreshold,
                          double absoluteThreshold,
                          double luminanceFloor) {
        if (converged) return true;
        if (sampleCount < minSamples) return false;
        converged = relativeError(luminanceFloor) <= relativeThreshold &&
                    absoluteError() <= absoluteThreshold;
        return converged;
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
        double renderPass(
            unsigned int firstSample, unsigned int sampleCount);
        void startWorkers();
        void stopWorkers();
        void workerLoop(unsigned int workerIndex);
        void renderTiles(
            unsigned int firstSample, unsigned int sampleCount,
            TraceStats* stats);
        TraceStats combinedStats() const;
        unsigned int convergedPixelCount() const;
        void writeAdaptiveOutput(unsigned int completedSamples);
        std::vector<std::vector<Vec3>> prepareOutput() const;

        ImageWriter& imageWriter;
        const Scene& scene;
        const Camera& camera;
        RenderSettings settings;
        std::vector<std::vector<AccumulationRecord>> accumulationBuffer;
        std::vector<std::thread> workers;
        std::vector<TraceStats> workerStats;
        std::atomic<std::size_t> nextTile;
        std::mutex workMutex;
        std::condition_variable workAvailable;
        std::condition_variable passFinished;
        bool stopping;
        std::size_t workGeneration;
        unsigned int activeWorkers;
        unsigned int currentFirstSample;
        unsigned int currentSampleCount;
        unsigned int tilesX;
        std::size_t tileCount;
};


#endif //RAYTRACER_RENDERER_H
