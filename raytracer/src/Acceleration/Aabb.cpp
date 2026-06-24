#include "Aabb.h"
#include "../Diagnostics/TraceStats.h"

#include <algorithm>
#include <cmath>

namespace {

double component(const Vec3& vector, int axis) {
    return axis == 0 ? vector.X() :
           axis == 1 ? vector.Y() : vector.Z();
}

} // namespace

Aabb::Aabb() : minimum(), maximum() {}

Aabb::Aabb(const Vec3& minimum, const Vec3& maximum)
    : minimum(minimum), maximum(maximum) {
}

bool Aabb::intersect(const Ray& ray, double minDistance,
                     double maxDistance) const {
    TraceStats* stats = currentTraceStats();
    if (stats) {
        ++stats->aabbTests;
    }
    for (int axis = 0; axis < 3; ++axis) {
        const double direction = component(ray.direction(), axis);
        const double origin = component(ray.origin(), axis);
        const double slabMinimum = component(minimum, axis);
        const double slabMaximum = component(maximum, axis);
        if (std::abs(direction) <= Vec3::EPSILON) {
            if (origin < slabMinimum || origin > slabMaximum) {
                return false;
            }
            continue;
        }
        const double inverse = 1.0 / direction;
        double nearDistance = (slabMinimum - origin) * inverse;
        double farDistance = (slabMaximum - origin) * inverse;
        if (nearDistance > farDistance) {
            std::swap(nearDistance, farDistance);
        }
        minDistance = std::max(minDistance, nearDistance);
        maxDistance = std::min(maxDistance, farDistance);
        if (maxDistance < minDistance) {
            return false;
        }
    }
    return true;
}

Aabb Aabb::surrounding(const Aabb& first, const Aabb& second) {
    return Aabb(
        Vec3(std::min(first.min().X(), second.min().X()),
             std::min(first.min().Y(), second.min().Y()),
             std::min(first.min().Z(), second.min().Z())),
        Vec3(std::max(first.max().X(), second.max().X()),
             std::max(first.max().Y(), second.max().Y()),
             std::max(first.max().Z(), second.max().Z())));
}
