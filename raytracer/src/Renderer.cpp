//
// Created by chenx on 2020-05-25.
//

#include <algorithm>
#include <atomic>
#include <cstdint>
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
}

double Renderer::sampleOffset(std::size_t pixelIndex,
                              unsigned int sampleIndex,
                              std::uint64_t seed,
                              unsigned int dimension) {
    return Sampler::sample(
        pixelIndex, sampleIndex, seed, dimension);
}

void Renderer::renderPass(unsigned int sampleIndex) {
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    const std::size_t pixelCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::atomic<std::size_t> nextPixel(0);

    unsigned int numThreads = settings.workerCount;
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 1;
        }
    }
    numThreads = static_cast<unsigned int>(
        std::min<std::size_t>(numThreads, pixelCount));
    std::vector<std::thread> pool;
    pool.reserve(numThreads);

    for (unsigned int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        pool.emplace_back(
            [this, width, pixelCount, sampleIndex, &nextPixel]() {
            while (true) {
                const std::size_t index = nextPixel.fetch_add(1);
                if (index >= pixelCount) {
                    break;
                }

                const int row = static_cast<int>(index / width);
                const int column = static_cast<int>(index % width);
                Sampler sampler(
                    index, sampleIndex, settings.randomSeed);
                const double offsetX = sampler.next();
                const double offsetY = sampler.next();
                const Ray ray =
                    camera.makeRay(column + offsetX, row + offsetY);
                accumulationBuffer[row][column] =
                    accumulationBuffer[row][column] +
                    scene.trace(
                        ray, sampler,
                        PathTraceSettings(
                            settings.maxBounces,
                            settings.russianRouletteStart));
            }
        });
    }
    for (auto& worker : pool) {
        worker.join();
    }

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
    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    accumulationBuffer.assign(
        height, std::vector<Vec3>(width, Vec3()));

    for (unsigned int sample = 0;
         sample < settings.samplesPerPixel; ++sample) {
        renderPass(sample);
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
                      << " samples per pixel." << std::endl;
        }
    }
}
