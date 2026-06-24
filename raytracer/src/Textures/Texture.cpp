#include "Texture.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

Vec3 SolidTexture::value(
        double u, double v, const Vec3& point) const {
    (void)u; (void)v; (void)point;
    return color;
}

CheckerTexture::CheckerTexture(
        const Vec3& first, const Vec3& second, double scale)
    : first(first), second(second), scale(scale) {
    if (scale <= 0.0) {
        throw std::invalid_argument("Checker scale must be positive.");
    }
}

Vec3 CheckerTexture::value(
        double u, double v, const Vec3& point) const {
    (void)u; (void)v;
    const double pattern =
        std::sin(point.X() * scale) *
        std::sin(point.Y() * scale) *
        std::sin(point.Z() * scale);
    return pattern < 0.0 ? first : second;
}

ImageTexture::ImageTexture() : pixels(), width(0), height(0) {}

bool ImageTexture::loadPpm(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    std::string magic;
    unsigned int maximum = 0;
    if (!input || !(input >> magic >> width >> height >> maximum) ||
        magic != "P6" || width == 0 || height == 0 ||
        maximum == 0 || maximum > 255) {
        return false;
    }
    input.get();
    std::vector<unsigned char> bytes(
        static_cast<std::size_t>(width) * height * 3);
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input) return false;
    pixels.clear();
    const double scale = 1.0 / maximum;
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        pixels.push_back(Vec3(
            bytes[i] * scale, bytes[i + 1] * scale,
            bytes[i + 2] * scale));
    }
    return true;
}

Vec3 ImageTexture::value(
        double u, double v, const Vec3& point) const {
    (void)point;
    if (pixels.empty()) return Vec3(1.0, 0.0, 1.0);
    u = u - std::floor(u);
    v = std::max(0.0, std::min(1.0, v));
    const unsigned int x = std::min(
        width - 1, static_cast<unsigned int>(u * width));
    const unsigned int y = std::min(
        height - 1,
        static_cast<unsigned int>((1.0 - v) * height));
    return pixels[static_cast<std::size_t>(y) * width + x];
}

