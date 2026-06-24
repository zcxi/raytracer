//
// Created by chenx on 2020-05-24.
//

#include <cmath>
#include <stdexcept>
#include "Sphere.h"

Sphere::Sphere(const Vec3& center, double radius, const Vec3& surfaceColor,
        const Vec3& emissionColor, double transparency, double refractiveIndex)
        :Shape(surfaceColor, emissionColor, transparency, refractiveIndex){

    this->center = Vec3(center);
    this->radius = radius;
    this->radiusSquared = radius * radius;
    if (radius <= Vec3::EPSILON) {
        throw std::invalid_argument("Sphere radius must be positive.");
    }
}

bool Sphere::intersect(const Ray& ray, double minDistance,
                       double maxDistance, HitRecord& hit) const {
    const Vec3 originToCenter = ray.origin() - center;
    const double halfB = originToCenter.dot(ray.direction());
    const double c = originToCenter.dot(originToCenter) - radiusSquared;
    const double discriminant = halfB * halfB - c;

    if (discriminant < 0.0) {
        return false;
    }

    const double squareRoot = std::sqrt(discriminant);
    double root = -halfB - squareRoot;
    if (root < minDistance || root > maxDistance) {
        root = -halfB + squareRoot;
        if (root < minDistance || root > maxDistance) {
            return false;
        }
    }

    hit.distance = root;
    hit.point = ray.at(root);
    hit.setFaceNormal(ray, (hit.point - center) / radius);
    hit.shape = this;
    return true;
}
