#include "Environment.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

Environment::Environment(
        const Vec3& horizon, const Vec3& zenith, double intensity)
    : horizon(horizon),
      zenith(zenith),
      intensity(intensity),
      pixels(),
      width(0),
      height(0) {
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
    (void)direction;
    const double pi = 3.14159265358979323846;
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
    return true;
}

void Environment::setIntensity(double value) {
    if (!std::isfinite(value) || value < 0.0) {
        throw std::invalid_argument(
            "Environment intensity must be finite and non-negative.");
    }
    intensity = value;
}
