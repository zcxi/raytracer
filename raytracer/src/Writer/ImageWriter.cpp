//
// Created by chenx on 2020-05-25.
//

#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include "ImageWriter.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

double ImageWriter::applyExposure(double linearValue, double exposure) {
    return std::max(0.0, linearValue) * std::pow(2.0, exposure);
}

double ImageWriter::toneMap(double linearValue, ToneMapper toneMapper) {
    const double value = std::max(0.0, linearValue);
    if (toneMapper == ToneMapper::Reinhard) {
        return value / (1.0 + value);
    }
    if (toneMapper == ToneMapper::Aces) {
        const double a = 2.51;
        const double b = 0.03;
        const double c = 2.43;
        const double d = 0.59;
        const double e = 0.14;
        return std::max(
            0.0, std::min(1.0, (value * (a * value + b)) /
                                      (value * (c * value + d) + e)));
    }
    return value;
}

double ImageWriter::linearToSrgb(double linearValue) {
    const double clamped = std::max(0.0, std::min(1.0, linearValue));
    if (clamped <= 0.0031308) {
        return 12.92 * clamped;
    }
    return 1.055 * std::pow(clamped, 1.0 / 2.4) - 0.055;
}

unsigned char ImageWriter::toByte(double linearValue, double exposure,
                                  ToneMapper toneMapper) {
    const double exposed = applyExposure(linearValue, exposure);
    const double mapped = toneMap(exposed, toneMapper);
    const double encoded = linearToSrgb(mapped);
    return static_cast<unsigned char>(std::round(encoded * 255.0));
}

bool ImageWriter::write(const std::vector<std::vector<Vec3>>& image) const {
    return writeScaled(image, 1.0);
}

bool ImageWriter::writeAccumulation(
        const std::vector<std::vector<Vec3>>& accumulation,
        unsigned int completedSamples) const {
    if (completedSamples == 0) {
        return false;
    }
    return writeScaled(
        accumulation, static_cast<double>(completedSamples));
}

bool ImageWriter::writeAdaptive(
        const std::vector<std::vector<Vec3>>& averaged,
        unsigned int maxSamples) const {
    (void)maxSamples;
    return writeScaled(averaged, 1.0);
}

static bool hasExtension(const std::string& path, const char* ext) {
    const std::size_t pathLen = path.size();
    const std::size_t extLen = std::strlen(ext);
    if (pathLen < extLen + 1) return false;
    return path.compare(pathLen - extLen, extLen, ext) == 0;
}

bool ImageWriter::writeScaled(
        const std::vector<std::vector<Vec3>>& image,
        double divisor) const {
    if (image.empty() || image.front().empty()) {
        return false;
    }

    const int width = static_cast<int>(image.front().size());
    const int height = static_cast<int>(image.size());
    for (const auto& row : image) {
        if (row.size() != static_cast<std::size_t>(width)) {
            return false;
        }
    }

    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * height * 3);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const Vec3& pixel = image[row][col];
            const std::size_t idx =
                (static_cast<std::size_t>(row) * width + col) * 3;
            pixels[idx + 0] = toByte(
                pixel.X() / divisor, settings.exposure, settings.toneMapper);
            pixels[idx + 1] = toByte(
                pixel.Y() / divisor, settings.exposure, settings.toneMapper);
            pixels[idx + 2] = toByte(
                pixel.Z() / divisor, settings.exposure, settings.toneMapper);
        }
    }

    if (hasExtension(outputPath, ".png")) {
        return stbi_write_png(
            outputPath.c_str(), width, height, 3, pixels.data(),
            width * 3) != 0;
    }
    if (hasExtension(outputPath, ".jpg") || hasExtension(outputPath, ".jpeg")) {
        return stbi_write_jpg(
            outputPath.c_str(), width, height, 3, pixels.data(), 92) != 0;
    }

    std::ofstream ofs(outputPath.c_str(), std::ios::out | std::ios::binary);
    if (!ofs) {
        return false;
    }
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write(reinterpret_cast<const char*>(pixels.data()),
              static_cast<std::streamsize>(pixels.size()));
    return static_cast<bool>(ofs);
}
