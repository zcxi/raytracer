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
#include <thread>

Renderer::Renderer(ImageWriter& writer, const Scene& sceneRef,
                   const Camera& cameraRef, const RenderSettings& renderSettings)
    : imageWriter(writer),
      scene(sceneRef),
      camera(cameraRef),
      settings(renderSettings) {
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

double Renderer::sampleOffset(std::size_t pixelIndex,
                              unsigned int sampleIndex,
                              std::uint64_t seed,
                              unsigned int dimension) {
    return Sampler::sample(
        pixelIndex, sampleIndex, seed, dimension);
}

double Renderer::renderPass(unsigned int sampleIndex) {
    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    const unsigned int tileSize = settings.tileSize;
    const unsigned int tilesX =
        (static_cast<unsigned int>(width) + tileSize - 1) / tileSize;
    const unsigned int tilesY =
        (static_cast<unsigned int>(height) + tileSize - 1) / tileSize;
    const std::size_t tileCount =
        static_cast<std::size_t>(tilesX) * tilesY;
    std::atomic<std::size_t> nextTile(0);

    unsigned int numThreads = settings.workerCount;
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 1;
        }
    }
    numThreads = static_cast<unsigned int>(
        std::min<std::size_t>(numThreads, tileCount));
    std::vector<std::thread> pool;
    pool.reserve(numThreads);

    for (unsigned int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        pool.emplace_back(
            [this, width, height, tilesX, tileSize,
             tileCount, sampleIndex, &nextTile]() {
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
                        Sampler sampler(
                            index, sampleIndex, settings.randomSeed);
                        const double offsetX = sampler.next();
                        const double offsetY = sampler.next();
                        const Ray ray = camera.makeRay(
                            column + offsetX, row + offsetY);
                        accumulationBuffer[row][column] =
                            accumulationBuffer[row][column] +
                            scene.trace(
                                ray, sampler,
                                PathTraceSettings(
                                    settings.maxBounces,
                                    settings.russianRouletteStart));
                    }
                }
            }
        });
    }
    for (auto& worker : pool) {
        worker.join();
    }

    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;
    return elapsed.count();
}

std::vector<std::vector<Vec3>> Renderer::averagedFrame(
        unsigned int completedSamples) const {
    std::vector<std::vector<Vec3>> result = accumulationBuffer;
    const double sampleCount = static_cast<double>(completedSamples);
    for (auto& row : result) {
        for (auto& pixel : row) {
            pixel = pixel / sampleCount;
        }
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

    for (unsigned int sample = 0;
         sample < settings.samplesPerPixel; ++sample) {
        const double passSeconds = renderPass(sample);
        const unsigned int completedSamples = sample + 1;
        const bool finalPass =
            completedSamples == settings.samplesPerPixel;
        const bool previewPass =
            settings.previewInterval > 0 &&
            completedSamples % settings.previewInterval == 0;
        if (finalPass || previewPass) {
            if (!imageWriter.write(averagedFrame(completedSamples))) {
                throw std::runtime_error("Failed to write rendered image.");
            }
            std::cout << "Rendered " << completedSamples << "/"
                      << settings.samplesPerPixel
                      << " samples per pixel. Last pass: "
                      << std::fixed << std::setprecision(3)
                      << passSeconds << " s." << std::endl;
        }
    }
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
              << scene.bvhNodeCount() << "." << std::endl;
}
