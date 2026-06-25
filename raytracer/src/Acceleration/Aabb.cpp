#include "Aabb.h"
#include "../Diagnostics/TraceStats.h"

#include <algorithm>
#include <cmath>
#include <limits>
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

Aabb::RayData::RayData(const Ray& ray)
    : origin{{ray.origin().X(), ray.origin().Y(), ray.origin().Z()}},
      inverseDirection(),
      negative(),
      parallel() {
    const double direction[3] = {
        ray.direction().X(), ray.direction().Y(), ray.direction().Z()
    };
    for (int axis = 0; axis < 3; ++axis) {
        parallel[axis] = std::abs(direction[axis]) <= Vec3::EPSILON;
        inverseDirection[axis] = parallel[axis]
            ? std::numeric_limits<double>::infinity()
            : 1.0 / direction[axis];
        negative[axis] = inverseDirection[axis] < 0.0;
    }
}

bool Aabb::intersect(const Ray& ray, double minDistance,
                     double maxDistance, double* nearDistance,
                     TraceStats* stats) const {
    return intersect(
        RayData(ray), minDistance, maxDistance, nearDistance, stats);
}

bool Aabb::intersect(const RayData& ray, double minDistance,
                     double maxDistance, double* nearDistance,
                     TraceStats* stats) const {
    if (stats) {
        ++stats->aabbTests;
    }
    for (int axis = 0; axis < 3; ++axis) {
        const double origin = ray.origin[axis];
        const double slabMinimum = component(minimum, axis);
        const double slabMaximum = component(maximum, axis);
        if (ray.parallel[axis]) {
            if (origin < slabMinimum || origin > slabMaximum) {
                return false;
            }
            continue;
        }
        const double inverse = ray.inverseDirection[axis];
        const double nearSlab = ray.negative[axis]
            ? slabMaximum : slabMinimum;
        const double farSlab = ray.negative[axis]
            ? slabMinimum : slabMaximum;
        minDistance = std::max(
            minDistance, (nearSlab - origin) * inverse);
        maxDistance = std::min(
            maxDistance, (farSlab - origin) * inverse);
        if (maxDistance < minDistance) {
            return false;
        }
    }
    if (nearDistance) {
        *nearDistance = minDistance;
    }
    return true;
}

double Aabb::surfaceArea() const {
    const Vec3 extent = maximum - minimum;
    return 2.0 * (
        extent.X() * extent.Y() +
        extent.X() * extent.Z() +
        extent.Y() * extent.Z());
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
