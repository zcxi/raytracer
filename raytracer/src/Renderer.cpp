//
// Created by chenx on 2020-05-25.
//

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "Renderer.h"

Renderer::Renderer(ImageWriter& writer, const Scene& sceneRef,
                   const Camera& cameraRef, const RenderSettings& renderSettings)
    : imageWriter(writer),
      scene(sceneRef),
      camera(cameraRef),
      settings(renderSettings),
      nextTile(0),
      stopping(false),
      workGeneration(0),
      activeWorkers(0),
      currentFirstSample(0),
      currentSampleCount(0),
      tilesX(0),
      tileCount(0) {
    if (settings.samplesPerPixel == 0) {
        throw std::invalid_argument("Samples per pixel must be positive.");
    }
    if (settings.maxBounces == 0) {
        throw std::invalid_argument("Maximum bounces must be positive.");
    }
    if (settings.tileSize == 0) {
        throw std::invalid_argument("Tile size must be positive.");
    }
}

Renderer::~Renderer() {
    stopWorkers();
}

double Renderer::sampleOffset(std::size_t pixelIndex,
                              unsigned int sampleIndex,
                              std::uint64_t seed,
                              unsigned int dimension) {
    return Sampler::sample(
        pixelIndex, sampleIndex, seed, dimension);
}

double Renderer::renderPass(
        unsigned int firstSample, unsigned int sampleCount) {
    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(workMutex);
        currentFirstSample = firstSample;
        currentSampleCount = sampleCount;
        nextTile.store(0);
        activeWorkers = static_cast<unsigned int>(workers.size());
        ++workGeneration;
    }
    workAvailable.notify_all();
    {
        std::unique_lock<std::mutex> lock(workMutex);
        passFinished.wait(lock, [this]() {
            return activeWorkers == 0;
        });
    }

    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;
    return elapsed.count();
}

void Renderer::renderTiles(
        unsigned int firstSample, unsigned int sampleCount,
        TraceStats* stats) {
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    const unsigned int tileSize = settings.tileSize;
    TraceStatsScope statsScope(stats);
    while (true) {
        const std::size_t tileIndex = nextTile.fetch_add(1);
        if (tileIndex >= tileCount) {
            break;
        }
        const unsigned int tileX =
            static_cast<unsigned int>(tileIndex % tilesX);
        const unsigned int tileY =
            static_cast<unsigned int>(tileIndex / tilesX);
        const int startX = static_cast<int>(tileX * tileSize);
        const int startY = static_cast<int>(tileY * tileSize);
        const int endX = std::min(
            width, startX + static_cast<int>(tileSize));
        const int endY = std::min(
            height, startY + static_cast<int>(tileSize));
        for (int row = startY; row < endY; ++row) {
            for (int column = startX; column < endX; ++column) {
                const std::size_t index =
                    static_cast<std::size_t>(row) * width + column;
                Vec3 accumulated = accumulationBuffer[row][column];
                for (unsigned int offset = 0;
                     offset < sampleCount; ++offset) {
                    const unsigned int sampleIndex =
                        firstSample + offset;
                    Sampler sampler(
                        index, sampleIndex, settings.randomSeed);
                    const double offsetX = sampler.next();
                    const double offsetY = sampler.next();
                    const Ray ray = camera.makeRay(
                        column + offsetX, row + offsetY, sampler);
                    accumulated = accumulated + scene.trace(
                        ray, sampler,
                        PathTraceSettings(
                            settings.maxBounces,
                            settings.russianRouletteStart));
                }
                accumulationBuffer[row][column] = accumulated;
            }
        }
    }
}

void Renderer::workerLoop(unsigned int workerIndex) {
    std::size_t observedGeneration = 0;
    while (true) {
        unsigned int firstSample = 0;
        unsigned int sampleCount = 0;
        {
            std::unique_lock<std::mutex> lock(workMutex);
            workAvailable.wait(lock, [this, observedGeneration]() {
                return stopping || workGeneration != observedGeneration;
            });
            if (stopping) {
                return;
            }
            observedGeneration = workGeneration;
            firstSample = currentFirstSample;
            sampleCount = currentSampleCount;
        }
        renderTiles(
            firstSample, sampleCount,
            settings.collectStats ? &workerStats[workerIndex] : nullptr);
        {
            std::lock_guard<std::mutex> lock(workMutex);
            --activeWorkers;
            if (activeWorkers == 0) {
                passFinished.notify_one();
            }
        }
    }
}

void Renderer::startWorkers() {
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    tilesX = (static_cast<unsigned int>(width) + settings.tileSize - 1) /
        settings.tileSize;
    const unsigned int tilesY =
        (static_cast<unsigned int>(height) + settings.tileSize - 1) /
        settings.tileSize;
    tileCount = static_cast<std::size_t>(tilesX) * tilesY;

    unsigned int numThreads = settings.workerCount;
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 1;
        }
    }
    numThreads = static_cast<unsigned int>(
        std::min<std::size_t>(numThreads, tileCount));
    workerStats.assign(numThreads, TraceStats());
    {
        std::lock_guard<std::mutex> lock(workMutex);
        stopping = false;
        workGeneration = 0;
        activeWorkers = 0;
    }
    workers.reserve(numThreads);
    for (unsigned int index = 0; index < numThreads; ++index) {
        workers.emplace_back(&Renderer::workerLoop, this, index);
    }
}

void Renderer::stopWorkers() {
    {
        std::lock_guard<std::mutex> lock(workMutex);
        stopping = true;
    }
    workAvailable.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
}

TraceStats Renderer::combinedStats() const {
    TraceStats result;
    for (const TraceStats& stats : workerStats) {
        result += stats;
    }
    return result;
}

void Renderer::render() {
    const std::chrono::steady_clock::time_point renderStart =
        std::chrono::steady_clock::now();
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    accumulationBuffer.assign(
        height, std::vector<Vec3>(width, Vec3()));
    scene.finalize();
    startWorkers();

    try {
        unsigned int completedSamples = 0;
        while (completedSamples < settings.samplesPerPixel) {
            const unsigned int remaining =
                settings.samplesPerPixel - completedSamples;
            const unsigned int batchSize =
                settings.previewInterval == 0
                    ? remaining
                    : std::min(settings.previewInterval, remaining);
            const double passSeconds =
                renderPass(completedSamples, batchSize);
            completedSamples += batchSize;
            const bool finalPass =
                completedSamples == settings.samplesPerPixel;
            const bool previewPass =
                settings.previewInterval > 0 &&
                completedSamples % settings.previewInterval == 0;
            if (finalPass || previewPass) {
                if (!imageWriter.writeAccumulation(
                        accumulationBuffer, completedSamples)) {
                    throw std::runtime_error(
                        "Failed to write rendered image.");
                }
                std::cout << "Rendered " << completedSamples << "/"
                          << settings.samplesPerPixel
                          << " samples per pixel. Last pass: "
                          << std::fixed << std::setprecision(3)
                          << passSeconds << " s." << std::endl;
            }
        }
    } catch (...) {
        stopWorkers();
        throw;
    }
    stopWorkers();
    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - renderStart;
    const double primarySamples =
        static_cast<double>(width) * height *
        settings.samplesPerPixel;
    std::cout << "Render time: " << std::fixed
              << std::setprecision(3) << elapsed.count()
              << " s; primary samples: "
              << std::setprecision(0) << primarySamples
              << "; throughput: " << std::setprecision(2)
              << primarySamples / std::max(0.001, elapsed.count()) / 1e6
              << " M samples/s; BVH nodes: "
              << scene.bvhNodeCount();
    if (settings.collectStats) {
        const TraceStats stats = combinedStats();
        std::cout << "; rays: " << stats.rays
                  << "; shadow rays: " << stats.shadowRays
                  << "; AABB tests: " << stats.aabbTests
                  << "; primitive tests: " << stats.primitiveTests
                  << "; BVH node visits: " << stats.bvhNodeVisits
                  << "; average path depth: " << std::setprecision(2)
                  << stats.averagePathDepth();
    }
    std::cout << "." << std::endl;
}
