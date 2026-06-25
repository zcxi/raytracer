//
// Created by chenx on 2020-05-25.
//

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <cmath>
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
    if (settings.adaptiveSampling) {
        if (settings.adaptiveMinSamples == 0 ||
            settings.adaptiveMinSamples > settings.adaptiveMaxSamples) {
            throw std::invalid_argument(
                "Adaptive min samples must be positive and <= max samples.");
        }
        if (settings.adaptiveBatch == 0) {
            throw std::invalid_argument(
                "Adaptive sample batch must be positive.");
        }
        if (!std::isfinite(settings.adaptiveRelativeError) ||
            settings.adaptiveRelativeError < 0.0 ||
            !std::isfinite(settings.adaptiveAbsoluteError) ||
            settings.adaptiveAbsoluteError < 0.0 ||
            !std::isfinite(settings.adaptiveLuminanceFloor) ||
            settings.adaptiveLuminanceFloor <= 0.0) {
            throw std::invalid_argument(
                "Adaptive error thresholds must be finite and non-negative, "
                "and the luminance floor must be positive.");
        }
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
                AccumulationRecord& record =
                    accumulationBuffer[row][column];
                if (settings.adaptiveSampling && record.converged) {
                    continue;
                }
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
                    const PathResult pathResult = scene.trace(
                        ray, sampler,
                        PathTraceSettings(
                            settings.maxBounces,
                            settings.russianRouletteStart));
                    record.addSample(
                        pathResult.radiance, pathResult.albedo,
                        pathResult.normal, pathResult.position);
                }
                if (settings.adaptiveSampling) {
                    record.checkConvergence(
                        settings.adaptiveMinSamples,
                        settings.adaptiveRelativeError,
                        settings.adaptiveAbsoluteError,
                        settings.adaptiveLuminanceFloor);
                }
            }
        }
    }
}

unsigned int Renderer::convergedPixelCount() const {
    unsigned int count = 0;
    for (const auto& row : accumulationBuffer) {
        for (const auto& record : row) {
            if (record.converged) {
                ++count;
            }
        }
    }
    return count;
}

void Renderer::writeAdaptiveOutput(unsigned int maxSamples) {
    std::vector<std::vector<Vec3>> result = prepareOutput();
    if (!imageWriter.writeAdaptive(result, maxSamples)) {
        throw std::runtime_error(
            "Failed to write adaptive rendered image.");
    }
}

std::vector<std::vector<Vec3>> Renderer::prepareOutput() const {
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    std::vector<std::vector<Vec3>> color(
        height, std::vector<Vec3>(width));
    std::vector<std::vector<Vec3>> albedo(
        height, std::vector<Vec3>(width));
    std::vector<std::vector<Vec3>> normal(
        height, std::vector<Vec3>(width));
    std::vector<std::vector<Vec3>> position(
        height, std::vector<Vec3>(width));

    double luminanceSum = 0.0;
    std::size_t validPixels = 0;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            const AccumulationRecord& record =
                accumulationBuffer[r][c];
            Vec3 pixel = record.averaged();
            color[r][c] = pixel;
            albedo[r][c] = record.averagedAlbedo();
            Vec3 avgN = record.averagedNormal();
            double nlen = avgN.getLength();
            normal[r][c] = nlen > 1e-9 ? avgN / nlen : Vec3();
            position[r][c] = record.averagedPosition();

            double lum = 0.2126 * pixel.X() + 0.7152 * pixel.Y() +
                         0.0722 * pixel.Z();
            if (lum > 0.0) {
                luminanceSum += std::log(lum + 1e-10);
                ++validPixels;
            }
        }
    }

    if (settings.denoise.enabled) {
        ATrousDenoiser::denoise(
            color, albedo, normal, position,
            settings.denoise, color);
    }

    if (validPixels > 0) {
        double avgLogLuminance =
            std::exp(luminanceSum / validPixels);
        double targetLum = imageWriter.getSettings().targetLuminance;
        double autoEv = std::log(targetLum / avgLogLuminance) /
                        std::log(2.0);
        double baseExposure = imageWriter.getSettings().autoExposure
                                  ? autoEv
                                  : 0.0;
        double finalExposure =
            baseExposure + imageWriter.getConfiguredExposure();
        const_cast<ImageWriter&>(imageWriter).setExposure(finalExposure);
    }
    return color;
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
        height,
        std::vector<AccumulationRecord>(width, AccumulationRecord()));
    scene.finalize();
    startWorkers();

    try {
        if (settings.adaptiveSampling) {
            unsigned int completedSamples = 0;
            unsigned int totalPixels =
                static_cast<unsigned int>(width) * height;
            unsigned int previewCountdown = settings.previewInterval > 0
                                                ? settings.previewInterval
                                                : settings.adaptiveBatch;
            while (completedSamples < settings.adaptiveMaxSamples) {
                unsigned int batchSize = settings.adaptiveBatch;
                if (completedSamples + batchSize >
                    settings.adaptiveMaxSamples) {
                    batchSize = settings.adaptiveMaxSamples -
                                completedSamples;
                }
                unsigned int converged = convergedPixelCount();
                if (converged >= totalPixels &&
                    completedSamples >= settings.adaptiveMinSamples) {
                    break;
                }
                double passSeconds =
                    renderPass(completedSamples, batchSize);
                completedSamples += batchSize;
                unsigned int passConverged = convergedPixelCount();
                const bool previewDue =
                    settings.previewInterval > 0 &&
                    batchSize >= previewCountdown;
                previewCountdown = previewDue
                    ? settings.previewInterval
                    : previewCountdown - batchSize;
                bool finalPass =
                    completedSamples >= settings.adaptiveMaxSamples ||
                    (passConverged >= totalPixels &&
                     completedSamples >= settings.adaptiveMinSamples);
                if (finalPass || previewDue) {
                    writeAdaptiveOutput(completedSamples);
                    unsigned int activePixels = totalPixels - passConverged;
                    std::cout << "Rendered " << completedSamples
                              << "/" << settings.adaptiveMaxSamples
                              << " spp; " << passConverged << "/"
                              << totalPixels << " converged"
                              << " (" << activePixels << " active)"
                              << ". Last pass: " << std::fixed
                              << std::setprecision(3) << passSeconds
                              << " s." << std::endl;
                }
                if (passConverged >= totalPixels &&
                    completedSamples >= settings.adaptiveMinSamples) {
                    break;
                }
            }
        } else {
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
                    std::vector<std::vector<Vec3>> result =
                        prepareOutput();
                    if (!imageWriter.write(result)) {
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
        }
    } catch (...) {
        stopWorkers();
        throw;
    }
    stopWorkers();

    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - renderStart;
    double primarySamples = 0.0;
    if (settings.adaptiveSampling) {
        for (const auto& row : accumulationBuffer) {
            for (const auto& record : row) {
                primarySamples += record.sampleCount;
            }
        }
    } else {
        primarySamples =
            static_cast<double>(width) * height * settings.samplesPerPixel;
    }
    std::cout << "Render time: " << std::fixed
              << std::setprecision(3) << elapsed.count()
              << " s; primary samples: "
              << std::setprecision(0) << primarySamples
              << "; throughput: " << std::setprecision(2)
              << primarySamples / std::max(0.001, elapsed.count()) / 1e6
              << " M samples/s; BVH nodes: "
              << scene.bvhNodeCount();
    if (settings.adaptiveSampling) {
        unsigned int totalPixels =
            static_cast<unsigned int>(width) * height;
        unsigned int converged = convergedPixelCount();
        const double avgSamples = primarySamples / totalPixels;
        const double fixedBudget =
            static_cast<double>(totalPixels) *
            settings.adaptiveMaxSamples;
        const double savedPercent = fixedBudget > 0.0
            ? 100.0 * (fixedBudget - primarySamples) / fixedBudget
            : 0.0;
        std::cout << "; converged: " << converged << "/" << totalPixels
                  << " (" << std::setprecision(1)
                  << 100.0 * converged / totalPixels << "%)"
                  << "; avg spp: " << avgSamples
                  << "; samples saved: " << savedPercent << "%";
    }
    if (settings.collectStats) {
        const TraceStats stats = combinedStats();
        std::cout << "; rays: " << stats.rays
                  << "; shadow rays: " << stats.shadowRays
                  << "; AABB tests: " << stats.aabbTests
                  << "; primitive tests: " << stats.primitiveTests
                  << "; BVH node visits: " << stats.bvhNodeVisits
                  << "; avg path depth: " << std::setprecision(2)
                  << stats.averagePathDepth()
                  << "; lights: " << stats.consideredLights
                  << "; emitted shadow rays: " << stats.emittedShadowRays
                  << "; occluded: " << stats.occludedShadowRays
                  << "; range rejects: " << stats.rangeRejects
                  << "; cone rejects: " << stats.coneRejects
                  << "; backface rejects: " << stats.backfaceRejects;
    }
    std::cout << "." << std::endl;
}
