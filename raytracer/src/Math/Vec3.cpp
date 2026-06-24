//
// Created by chenx on 2020-05-22.
//

#include <stdexcept>
#include "Vec3.h"

constexpr double Vec3::EPSILON;
constexpr double Vec3::EPISLON;

Vec3::Vec3(const std::vector<double>& coordinates)
    : x(0.0), y(0.0), z(0.0) {
    if (coordinates.size() != 3) {
        throw std::invalid_argument("Vec3 requires exactly three coordinates.");
    }
    x = coordinates[0];
    y = coordinates[1];
    z = coordinates[2];
}
