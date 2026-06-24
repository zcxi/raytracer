//
// Created by chenx on 2020-05-25.
//

#include <fstream>
#include <algorithm>
#include <cmath>
#include "ImageWriter.h"


double ImageWriter::linearToSrgb(double linearValue) {
    const double clamped = std::max(0.0, std::min(1.0, linearValue));
    if (clamped <= 0.0031308) {
        return 12.92 * clamped;
    }
    return 1.055 * std::pow(clamped, 1.0 / 2.4) - 0.055;
}

unsigned char ImageWriter::toByte(double linearValue) {
    const double encoded = linearToSrgb(linearValue);
    return static_cast<unsigned char>(std::round(encoded * 255.0));
}

bool ImageWriter::write(const std::vector<std::vector<Vec3>>& image) const {
    if (image.empty() || image.front().empty()) {
        return false;
    }

    const std::size_t width = image.front().size();
    const std::size_t height = image.size();
    for (const auto& row : image) {
        if (row.size() != width) {
            return false;
        }
    }

    std::ofstream ofs(outputPath.c_str(), std::ios::out | std::ios::binary);
    if (!ofs) {
        return false;
    }
    ofs << "P6\n" << width << " " << height << "\n255\n";

    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t column = 0; column < width; ++column) {
            const Vec3& pixel = image[row][column];
            const unsigned char bytes[3] = {
                toByte(pixel.X()),
                toByte(pixel.Y()),
                toByte(pixel.Z())
            };
            ofs.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
        }
    }

    return static_cast<bool>(ofs);
}
