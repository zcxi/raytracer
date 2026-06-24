#include "Rectangle.h"

#include "../Math/Sampler.h"
#include <cmath>
#include <stdexcept>

Rectangle::Rectangle(
        const Vec3& center, const Vec3& normal,
        const Vec3& upDirection, double width, double height,
        const Material& material)
    : Shape(material),
      center(center),
      normal(normal.normalize()),
      right(upDirection.cross(normal).normalize()),
      up(normal.cross(right).normalize()),
      width(width),
      height(height) {
    if (normal.getLength() <= Vec3::EPSILON ||
        upDirection.getLength() <= Vec3::EPSILON ||
        right.getLength() <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Rectangle normal and up direction must define a basis.");
    }
    if (width <= Vec3::EPSILON || height <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Rectangle dimensions must be positive.");
    }
}

bool Rectangle::intersect(const Ray& ray, double minDistance,
                          double maxDistance, HitRecord& hit) const {
    const double denominator = normal.dot(ray.direction());
    if (std::abs(denominator) <= Vec3::EPSILON) {
        return false;
    }
    const double distance =
        (center - ray.origin()).dot(normal) / denominator;
    if (distance < minDistance || distance > maxDistance) {
        return false;
    }
    const Vec3 point = ray.at(distance);
    const Vec3 offset = point - center;
    if (std::abs(offset.dot(right)) > width * 0.5 ||
        std::abs(offset.dot(up)) > height * 0.5) {
        return false;
    }
    hit.distance = distance;
    hit.point = point;
    hit.setFaceNormal(ray, normal);
    hit.shape = this;
    return true;
}

bool Rectangle::sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const {
    const double horizontal = (sampler.next() - 0.5) * width;
    const double vertical = (sampler.next() - 0.5) * height;
    sample.point = center + right * horizontal + up * vertical;
    sample.normal = normal;
    sample.areaPdf = 1.0 / surfaceArea();
    return true;
}

