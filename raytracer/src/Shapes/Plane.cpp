//
// Created by chenx on 2020-06-10.
//

#include "Plane.h"
#include <cmath>
#include <stdexcept>

Plane::Plane(const Vec3& normal, const Vec3& point,
             const Material& material)
    : Shape(material), normal(normal.normalize()), point(point) {
    if (normal.getLength() <= Vec3::EPSILON) {
        throw std::invalid_argument("Plane normal must be non-zero.");
    }
}

Plane::Plane(const Vec3& normal, const Vec3& point, const Vec3& surfaceColor,
             const Vec3& emissionColor, double transparency,
             double refractiveIndex)
    : Plane(normal, point,
            transparency > 0.0
                ? Material::dielectric(
                      refractiveIndex, surfaceColor, emissionColor)
                : Material::diffuse(surfaceColor, emissionColor)) {
    if (transparency < 0.0 || transparency > 1.0) {
        throw std::invalid_argument(
            "Transparency must be between zero and one.");
    }
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
