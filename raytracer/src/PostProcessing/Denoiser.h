#ifndef RAYTRACER_DENOISER_H
#define RAYTRACER_DENOISER_H

#include "../Math/Vec3.h"
#include <vector>

struct DenoiseSettings {
    bool enabled;
    int iterations;
    double colorPhi;
    double normalPhi;
    double positionPhi;

    DenoiseSettings()
        : enabled(false), iterations(5), colorPhi(0.5),
          normalPhi(0.2), positionPhi(1.0) {}
};

class ATrousDenoiser {
public:
    static void denoise(
        const std::vector<std::vector<Vec3>>& color,
        const std::vector<std::vector<Vec3>>& albedo,
        const std::vector<std::vector<Vec3>>& normal,
        const std::vector<std::vector<Vec3>>& position,
        const DenoiseSettings& settings,
        std::vector<std::vector<Vec3>>& output);
};

#endif
