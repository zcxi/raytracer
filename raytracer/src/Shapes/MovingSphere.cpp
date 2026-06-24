#include "MovingSphere.h"

#include <cmath>
#include <stdexcept>

MovingSphere::MovingSphere(
        const Vec3& startCenter, const Vec3& endCenter,
        double startTime, double endTime, double radius,
        const Material& material)
    : Shape(material), startCenter(startCenter), endCenter(endCenter),
      startTime(startTime), endTime(endTime), radius(radius) {
    if (endTime <= startTime || radius <= Vec3::EPSILON)
        throw std::invalid_argument("Invalid moving sphere.");
}

Vec3 MovingSphere::centerAt(double time) const {
    const double t = std::max(
        0.0, std::min(1.0, (time - startTime) / (endTime - startTime)));
    return startCenter + (endCenter - startCenter) * t;
}

bool MovingSphere::intersect(const Ray& ray, double minDistance,
                             double maxDistance, HitRecord& hit) const {
    const Vec3 center = centerAt(ray.time());
    const Vec3 offset = ray.origin() - center;
    const double halfB = offset.dot(ray.direction());
    const double c = offset.dot(offset) - radius * radius;
    const double discriminant = halfB * halfB - c;
    if (discriminant < 0.0) return false;
    const double rootValue = std::sqrt(discriminant);
    double root = -halfB - rootValue;
    if (root < minDistance || root > maxDistance) {
        root = -halfB + rootValue;
        if (root < minDistance || root > maxDistance) return false;
    }
    hit.distance = root;
    hit.point = ray.at(root);
    hit.setFaceNormal(ray, (hit.point - center) / radius);
    hit.shape = this;
    return true;
}

bool MovingSphere::boundingBox(Aabb& bounds) const {
    const Vec3 extent(radius, radius, radius);
    bounds = Aabb::surrounding(
        Aabb(startCenter - extent, startCenter + extent),
        Aabb(endCenter - extent, endCenter + extent));
    return true;
}

