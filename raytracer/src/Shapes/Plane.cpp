//
// Created by chenx on 2020-06-10.
//

#include "Plane.h"
#include <cmath>
#include <stdexcept>

Plane::Plane(const Vec3& normal, const Vec3& point, const Vec3& surfaceColor,
             const Vec3& emissionColor, double transparency,
             double refractiveIndex)
    :Shape(surfaceColor, emissionColor, transparency, refractiveIndex) {
    if (normal.getLength() <= Vec3::EPSILON) {
        throw std::invalid_argument("Plane normal must be non-zero.");
    }
    this->normal = normal.normalize();
    this->point = point;
}

bool Plane::intersect(const Ray& ray, double minDistance,
                      double maxDistance, HitRecord& hit) const {
    const double denominator = normal.dot(ray.direction());
    if (std::abs(denominator) <= Vec3::EPSILON) {
        return false;
    }

    const double distance = (point - ray.origin()).dot(normal) / denominator;
    if (distance < minDistance || distance > maxDistance) {
        return false;
    }

    hit.distance = distance;
    hit.point = ray.at(distance);
    hit.setFaceNormal(ray, normal);
    hit.shape = this;
    return true;
}
