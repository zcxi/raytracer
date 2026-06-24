//
// Created by chenx on 2020-05-25.
//

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include "Renderer.h"
#include <thread>

Renderer::Renderer(ImageWriter& writer, const Scene& sceneRef,
                   const Camera& cameraRef)
    : imageWriter(writer), scene(sceneRef), camera(cameraRef) {
}

void Renderer::render(){

    const int height = camera.getImageHeight();
    const int width = camera.getImageWidth();
    frameBuffer.assign(height, std::vector<Vec3>(width, Vec3()));

    const std::size_t pixelCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::atomic<std::size_t> nextPixel(0);

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 1;
    }
    numThreads = static_cast<unsigned int>(
        std::min<std::size_t>(numThreads, pixelCount));
    std::vector<std::thread> pool;
    pool.reserve(numThreads);

    for (unsigned int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        pool.emplace_back([this, width, pixelCount, &nextPixel]() {
            while (true) {
                const std::size_t index = nextPixel.fetch_add(1);
                if (index >= pixelCount) {
                    break;
                }

                const int row = static_cast<int>(index / width);
                const int column = static_cast<int>(index % width);
                const Ray ray = camera.makeRay(column + 0.5, row + 0.5);
                frameBuffer[row][column] = scene.trace(ray);
            }
        });
    }
    for (auto& worker : pool) {
        worker.join();
    }

    if (!imageWriter.write(frameBuffer)) {
        throw std::runtime_error("Failed to write rendered image.");
    }
}
