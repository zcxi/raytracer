#include "Denoiser.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

const double SPATIAL_FILTER[5][5] = {
    {1.0/256.0, 4.0/256.0,  6.0/256.0,  4.0/256.0,  1.0/256.0},
    {4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0},
    {6.0/256.0, 24.0/256.0, 36.0/256.0, 24.0/256.0, 6.0/256.0},
    {4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0},
    {1.0/256.0, 4.0/256.0,  6.0/256.0,  4.0/256.0,  1.0/256.0}
};

} // namespace

void ATrousDenoiser::denoise(
        const std::vector<std::vector<Vec3>>& colorIn,
        const std::vector<std::vector<Vec3>>&,
        const std::vector<std::vector<Vec3>>& normalIn,
        const std::vector<std::vector<Vec3>>& positionIn,
        const DenoiseSettings& settings,
        std::vector<std::vector<Vec3>>& output) {
    if (!settings.enabled || settings.iterations <= 0) {
        output = colorIn;
        return;
    }
    int height = static_cast<int>(colorIn.size());
    if (height == 0) { output = colorIn; return; }
    int width = static_cast<int>(colorIn[0].size());

    std::vector<std::vector<Vec3>> color = colorIn;
    std::vector<std::vector<Vec3>> work(
        height, std::vector<Vec3>(width));

    for (int iteration = 0; iteration < settings.iterations; ++iteration) {
        int stepSize = 1 << iteration;
        for (int row = 0; row < height; ++row) {
            for (int column = 0; column < width; ++column) {
                const Vec3& centerColor = color[row][column];
                const Vec3& centerNormal = normalIn[row][column];
                const Vec3& centerPosition = positionIn[row][column];
                double totalWeight = 0.0;
                Vec3 result;

                for (int dy = -2; dy <= 2; ++dy) {
                    int ny = row + dy * stepSize;
                    if (ny < 0 || ny >= height) continue;
                    for (int dx = -2; dx <= 2; ++dx) {
                        int nx = column + dx * stepSize;
                        if (nx < 0 || nx >= width) continue;

                        double spatialW =
                            SPATIAL_FILTER[dy + 2][dx + 2];

                        const Vec3& sampleColor = color[ny][nx];
                        double colorDiff =
                            (sampleColor - centerColor).getLength();
                        double colorW = std::exp(
                            -colorDiff /
                            (settings.colorPhi * settings.colorPhi + 1e-10));

                        const Vec3& sampleNormal = normalIn[ny][nx];
                        double normalDot = std::max(
                            0.0, centerNormal.dot(sampleNormal));
                        double normalW = 1.0;
                        double centerLen = centerNormal.getLength();
                        double sampleLen = sampleNormal.getLength();
                        if (centerLen > 1e-9 && sampleLen > 1e-9) {
                            normalW = normalDot > 0.999
                                ? 1.0
                                : std::pow(
                                      normalDot,
                                      32.0 / (settings.normalPhi + 1e-10));
                        }

                        const Vec3& samplePos = positionIn[ny][nx];
                        double posDiff =
                            (samplePos - centerPosition).getLength();
                        double posW = std::exp(
                            -posDiff /
                            (settings.positionPhi + 1e-10));

                        double weight =
                            spatialW * colorW * normalW * posW;
                        result = result + sampleColor * weight;
                        totalWeight += weight;
                    }
                }
                if (totalWeight > 0.0) {
                    work[row][column] = result / totalWeight;
                } else {
                    work[row][column] = centerColor;
                }
            }
        }
        color.swap(work);
    }
    output = color;
}
