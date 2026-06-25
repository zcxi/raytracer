#include "Environment.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

Environment::Environment(
        const Vec3& horizon, const Vec3& zenith, double intensity)
    : horizon(horizon),
      zenith(zenith),
      intensity(intensity),
      pixels(),
      width(0),
      height(0),
      importanceCdf(),
      importanceTotal(0.0) {
    if (horizon.X() < 0.0 || horizon.Y() < 0.0 || horizon.Z() < 0.0 ||
        zenith.X() < 0.0 || zenith.Y() < 0.0 || zenith.Z() < 0.0) {
        throw std::invalid_argument(
            "Environment colors cannot contain negative values.");
    }
    if (intensity < 0.0) {
        throw std::invalid_argument(
            "Environment intensity cannot be negative.");
    }
}

Vec3 Environment::evaluate(const Vec3& direction) const {
    const double pi = 3.14159265358979323846;
    const Vec3 normalized = direction.normalize();
    if (!pixels.empty()) {
        const double longitude =
            std::atan2(normalized.Z(), normalized.X());
        const double latitude =
            std::asin(std::max(-1.0, std::min(1.0, normalized.Y())));
        const double u = longitude / (2.0 * pi) + 0.5;
        const double v = 0.5 - latitude / pi;
        const unsigned int x = std::min(
            width - 1,
            static_cast<unsigned int>(u * width));
        const unsigned int y = std::min(
            height - 1,
            static_cast<unsigned int>(v * height));
        return pixels[static_cast<std::size_t>(y) * width + x] *
               intensity;
    }
    const double blend =
        std::max(0.0, std::min(1.0, normalized.Y() * 0.5 + 0.5));
    return (horizon * (1.0 - blend) + zenith * blend) * intensity;
}

EnvironmentSample Environment::sample(Sampler& sampler) const {
    const double pi = 3.14159265358979323846;
    if (!pixels.empty() && importanceTotal > 0.0) {
        const double target = sampler.next() * importanceTotal;
        const auto found = std::lower_bound(
            importanceCdf.begin() + 1, importanceCdf.end(), target);
        const std::size_t index = static_cast<std::size_t>(
            std::distance(importanceCdf.begin() + 1, found));
        const unsigned int x =
            static_cast<unsigned int>(index % width);
        const unsigned int y =
            static_cast<unsigned int>(index / width);
        const double u = (x + sampler.next()) / width;
        const double v = (y + sampler.next()) / height;
        const double longitude = (u - 0.5) * 2.0 * pi;
        const double latitude = (0.5 - v) * pi;
        const double cosineLatitude = std::cos(latitude);
        EnvironmentSample result;
        result.direction = Vec3(
            cosineLatitude * std::cos(longitude),
            std::sin(latitude),
            cosineLatitude * std::sin(longitude));
        result.radiance = evaluate(result.direction);
        result.pdf = pdf(result.direction);
        return result;
    }
    const double y = 1.0 - 2.0 * sampler.next();
    const double angle = 2.0 * pi * sampler.next();
    const double radial = std::sqrt(std::max(0.0, 1.0 - y * y));
    EnvironmentSample result;
    result.direction =
        Vec3(radial * std::cos(angle), y, radial * std::sin(angle));
    result.radiance = evaluate(result.direction);
    result.pdf = 1.0 / (4.0 * pi);
    return result;
}

double Environment::pdf(const Vec3& direction) const {
    const double pi = 3.14159265358979323846;
    if (!pixels.empty() && importanceTotal > 0.0) {
        const Vec3 normalized = direction.normalize();
        const double longitude =
            std::atan2(normalized.Z(), normalized.X());
        const double latitude =
            std::asin(std::clamp(normalized.Y(), -1.0, 1.0));
        const double u = longitude / (2.0 * pi) + 0.5;
        const double v = 0.5 - latitude / pi;
        const unsigned int x = std::min(
            width - 1, static_cast<unsigned int>(u * width));
        const unsigned int y = std::min(
            height - 1, static_cast<unsigned int>(v * height));
        const double probability = texelWeight(x, y) / importanceTotal;
        const double solidAngle =
            (2.0 * pi / width) * (pi / height) *
            std::max(1e-8, std::cos(latitude));
        return probability / solidAngle;
    }
    return 1.0 / (4.0 * pi);
}

bool Environment::isBlack() const {
    if (!pixels.empty()) {
        return false;
    }
    return horizon.near(Vec3()) && zenith.near(Vec3());
}

bool Environment::loadPpm(
        const std::string& path, double mapIntensity) {
    if (mapIntensity < 0.0) {
        return false;
    }
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        return false;
    }
    std::string magic;
    unsigned int mapWidth = 0;
    unsigned int mapHeight = 0;
    unsigned int maximum = 0;
    input >> magic >> mapWidth >> mapHeight >> maximum;
    if (magic != "P6" || mapWidth == 0 || mapHeight == 0 ||
        maximum == 0 || maximum > 255) {
        return false;
    }
    input.get();
    std::vector<unsigned char> bytes(
        static_cast<std::size_t>(mapWidth) * mapHeight * 3);
    input.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        return false;
    }
    std::vector<Vec3> loaded;
    loaded.reserve(static_cast<std::size_t>(mapWidth) * mapHeight);
    for (std::size_t index = 0; index < bytes.size(); index += 3) {
        const double scale = 1.0 / static_cast<double>(maximum);
        loaded.push_back(Vec3(
            bytes[index] * scale,
            bytes[index + 1] * scale,
            bytes[index + 2] * scale));
    }
    pixels.swap(loaded);
    width = mapWidth;
    height = mapHeight;
    intensity = mapIntensity;
    buildImportanceDistribution();
    return true;
}

bool Environment::loadHdr(
        const std::string& path, double mapIntensity) {
    if (mapIntensity < 0.0) return false;
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) return false;
    std::string line;
    if (!std::getline(input, line) ||
        line.rfind("#?", 0) != 0) {
        return false;
    }
    bool formatFound = false;
    while (std::getline(input, line) && !line.empty() && line != "\r") {
        if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos) {
            formatFound = true;
        }
    }
    if (!formatFound || !std::getline(input, line)) return false;
    int mapHeight = 0;
    int mapWidth = 0;
    char ySign = 0;
    char xSign = 0;
    char yAxis = 0;
    char xAxis = 0;
    std::stringstream resolution(line);
    resolution >> ySign >> yAxis >> mapHeight >>
                  xSign >> xAxis >> mapWidth;
    if (!resolution || yAxis != 'Y' || xAxis != 'X' ||
        mapWidth <= 0 || mapHeight <= 0) {
        return false;
    }

    std::vector<Vec3> loaded(
        static_cast<std::size_t>(mapWidth) * mapHeight);
    std::vector<unsigned char> scanline(
        static_cast<std::size_t>(mapWidth) * 4);
    for (int y = 0; y < mapHeight; ++y) {
        unsigned char header[4];
        input.read(reinterpret_cast<char*>(header), 4);
        if (!input || header[0] != 2 || header[1] != 2 ||
            ((header[2] << 8) | header[3]) != mapWidth) {
            return false;
        }
        for (int channel = 0; channel < 4; ++channel) {
            int x = 0;
            while (x < mapWidth) {
                unsigned char count = 0;
                input.read(reinterpret_cast<char*>(&count), 1);
                if (!input || count == 0) return false;
                if (count > 128) {
                    unsigned char value = 0;
                    input.read(reinterpret_cast<char*>(&value), 1);
                    const int run = count - 128;
                    if (!input || x + run > mapWidth) return false;
                    for (int i = 0; i < run; ++i) {
                        scanline[(x++ * 4) + channel] = value;
                    }
                } else {
                    if (x + count > mapWidth) return false;
                    for (int i = 0; i < count; ++i) {
                        input.read(reinterpret_cast<char*>(
                            &scanline[(x++ * 4) + channel]), 1);
                    }
                    if (!input) return false;
                }
            }
        }
        const int destinationY =
            ySign == '-' ? y : mapHeight - 1 - y;
        for (int x = 0; x < mapWidth; ++x) {
            const unsigned char exponent = scanline[x * 4 + 3];
            Vec3 color;
            if (exponent != 0) {
                const double scale = std::ldexp(1.0, exponent - (128 + 8));
                color = Vec3(
                    scanline[x * 4] * scale,
                    scanline[x * 4 + 1] * scale,
                    scanline[x * 4 + 2] * scale);
            }
            const int destinationX =
                xSign == '+' ? x : mapWidth - 1 - x;
            loaded[static_cast<std::size_t>(destinationY) * mapWidth +
                   destinationX] = color;
        }
    }
    pixels.swap(loaded);
    width = static_cast<unsigned int>(mapWidth);
    height = static_cast<unsigned int>(mapHeight);
    intensity = mapIntensity;
    buildImportanceDistribution();
    return true;
}

double Environment::texelWeight(
        unsigned int x, unsigned int y) const {
    const double pi = 3.14159265358979323846;
    const Vec3& pixel = pixels[static_cast<std::size_t>(y) * width + x];
    const double luminance =
        0.2126 * pixel.X() + 0.7152 * pixel.Y() + 0.0722 * pixel.Z();
    const double theta = pi * (y + 0.5) / height;
    return std::max(0.0, luminance) * std::sin(theta);
}

void Environment::buildImportanceDistribution() {
    importanceCdf.assign(pixels.size() + 1, 0.0);
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const std::size_t index =
                static_cast<std::size_t>(y) * width + x;
            importanceCdf[index + 1] =
                importanceCdf[index] + texelWeight(x, y);
        }
    }
    importanceTotal =
        importanceCdf.empty() ? 0.0 : importanceCdf.back();
}

void Environment::setIntensity(double value) {
    if (!std::isfinite(value) || value < 0.0) {
        throw std::invalid_argument(
            "Environment intensity must be finite and non-negative.");
    }
    intensity = value;
}
