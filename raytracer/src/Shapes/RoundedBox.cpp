#include "RoundedBox.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

RoundedBox::RoundedBox(
        const Vec3& center, const Vec3& dimensions,
        double radius, const Material& material)
    : Shape(material),
      center(center),
      halfDimensions(dimensions * 0.5),
      radius(radius) {
    const double smallest = std::min(
        halfDimensions.X(),
        std::min(halfDimensions.Y(), halfDimensions.Z()));
    if (dimensions.X() <= 0.0 || dimensions.Y() <= 0.0 ||
        dimensions.Z() <= 0.0 || radius < 0.0 || radius >= smallest) {
        throw std::invalid_argument(
            "Rounded-box dimensions must be positive and radius must fit.");
    }
}

double RoundedBox::signedDistance(const Vec3& point) const {
    const Vec3 local = point - center;
    const Vec3 core = halfDimensions - Vec3(radius, radius, radius);
    const Vec3 q(
        std::abs(local.X()) - core.X(),
        std::abs(local.Y()) - core.Y(),
        std::abs(local.Z()) - core.Z());
    const Vec3 outside(
        std::max(0.0, q.X()),
        std::max(0.0, q.Y()),
        std::max(0.0, q.Z()));
    return outside.getLength() +
        std::min(0.0, std::max(q.X(), std::max(q.Y(), q.Z()))) -
        radius;
}

Vec3 RoundedBox::normalAt(const Vec3& point) const {
    const double epsilon = 1e-5;
    return Vec3(
        signedDistance(point + Vec3(epsilon, 0.0, 0.0)) -
            signedDistance(point - Vec3(epsilon, 0.0, 0.0)),
        signedDistance(point + Vec3(0.0, epsilon, 0.0)) -
            signedDistance(point - Vec3(0.0, epsilon, 0.0)),
        signedDistance(point + Vec3(0.0, 0.0, epsilon)) -
            signedDistance(point - Vec3(0.0, 0.0, epsilon))).normalize();
}

bool RoundedBox::intersect(
        const Ray& ray, double minDistance,
        double maxDistance, HitRecord& hit) const {
    Aabb bounds;
    boundingBox(bounds);
    double distance = minDistance;
    if (!bounds.intersect(ray, minDistance, maxDistance, &distance)) {
        return false;
    }
    constexpr int maximumSteps = 160;
    constexpr double hitEpsilon = 1e-5;
    for (int step = 0; step < maximumSteps && distance <= maxDistance; ++step) {
        const Vec3 point = ray.at(distance);
        const double sdf = signedDistance(point);
        if (std::abs(sdf) <= hitEpsilon) {
            hit.distance = distance;
            hit.point = point;
            const Vec3 outward = normalAt(point);
            hit.setFaceNormal(ray, outward);
            hit.u = 0.5 + std::atan2(outward.Z(), outward.X()) /
                (2.0 * 3.14159265358979323846);
            hit.v = 0.5 - std::asin(std::clamp(
                outward.Y(), -1.0, 1.0)) /
                3.14159265358979323846;
            hit.shape = this;
            return true;
        }
        distance += std::max(hitEpsilon, std::abs(sdf) * 0.8);
    }
    return false;
}

bool RoundedBox::boundingBox(Aabb& bounds) const {
    bounds = Aabb(center - halfDimensions, center + halfDimensions);
    return true;
}
