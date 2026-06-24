//
// Created by chenx on 2020-05-24.
//

#include <cmath>
#include <stdexcept>
#include "Sphere.h"
#include "../Math/Sampler.h"

Sphere::Sphere(const Vec3& center, double radius,
               const Material& material)
    : Shape(material),
      center(center),
      radius(radius),
      radiusSquared(radius * radius) {
    if (radius <= Vec3::EPSILON) {
        throw std::invalid_argument("Sphere radius must be positive.");
    }
}

Sphere::Sphere(const Vec3& center, double radius, const Vec3& surfaceColor,
        const Vec3& emissionColor, double transparency, double refractiveIndex)
    : Sphere(center, radius,
             transparency > 0.0
                 ? Material::dielectric(
                       refractiveIndex, surfaceColor, emissionColor)
                 : Material::diffuse(surfaceColor, emissionColor)) {
    if (transparency < 0.0 || transparency > 1.0) {
        throw std::invalid_argument(
            "Transparency must be between zero and one.");
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
    const Vec3 outward = (hit.point - center) / radius;
    hit.setFaceNormal(ray, outward);
    const double pi = 3.14159265358979323846;
    hit.u = 0.5 + std::atan2(outward.Z(), outward.X()) / (2.0 * pi);
    hit.v = 0.5 - std::asin(outward.Y()) / pi;
    hit.shape = this;
    return true;
}

bool Sphere::sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const {
    const double pi = 3.14159265358979323846;
    const double z = 1.0 - 2.0 * sampler.next();
    const double angle = 2.0 * pi * sampler.next();
    const double radial = std::sqrt(std::max(0.0, 1.0 - z * z));
    sample.normal = Vec3(
        radial * std::cos(angle), z, radial * std::sin(angle));
    sample.point = center + sample.normal * radius;
    sample.areaPdf = 1.0 / surfaceArea();
    return true;
}

double Sphere::surfaceArea() const {
    const double pi = 3.14159265358979323846;
    return 4.0 * pi * radiusSquared;
}

bool Sphere::boundingBox(Aabb& bounds) const {
    const Vec3 extent(radius, radius, radius);
    bounds = Aabb(center - extent, center + extent);
    return true;
}
