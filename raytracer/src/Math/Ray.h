#ifndef RAYTRACER_RAY_H
#define RAYTRACER_RAY_H

#include "Vec3.h"
#include <stdexcept>

class Ray {
public:
    Ray(const Vec3& origin, const Vec3& direction)
        : origin_(origin), direction_(direction.normalize()) {
        if (direction.getLength() <= Vec3::EPSILON) {
            throw std::invalid_argument("Ray direction must be non-zero.");
        }
    }

    const Vec3& origin() const { return origin_; }
    const Vec3& direction() const { return direction_; }
    Vec3 at(double distance) const { return origin_ + direction_ * distance; }

private:
    Vec3 origin_;
    Vec3 direction_;
};

#endif
