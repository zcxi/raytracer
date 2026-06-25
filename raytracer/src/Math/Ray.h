#ifndef RAYTRACER_RAY_H
#define RAYTRACER_RAY_H

#include "Vec3.h"
#include <cmath>
#include <stdexcept>

class Ray {
public:
    constexpr Ray(const Vec3& origin, const Vec3& direction, double time = 0.0)
        : origin_(origin), direction_(), time_(time) {
        const double lengthSquared = direction.dot(direction);
        if (lengthSquared <= Vec3::EPSILON * Vec3::EPSILON) {
            throw std::invalid_argument("Ray direction must be non-zero.");
        }
        direction_ = direction * (1.0 / std::sqrt(lengthSquared));
    }

    [[nodiscard]] constexpr const Vec3& origin() const noexcept { return origin_; }
    [[nodiscard]] constexpr const Vec3& direction() const noexcept { return direction_; }
    [[nodiscard]] constexpr double time() const noexcept { return time_; }
    [[nodiscard]] constexpr Vec3 at(double distance) const noexcept { return origin_ + direction_ * distance; }

private:
    Vec3 origin_;
    Vec3 direction_;
    double time_;
};

#endif
