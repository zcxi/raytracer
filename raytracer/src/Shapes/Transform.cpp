#include "Transform.h"

#include "../Math/Quaternion.h"
#include <algorithm>
#include <stdexcept>

Transform::Transform(
        std::unique_ptr<Shape> child, const Vec3& translation,
        const Vec3& rotationRadians, const Vec3& scale)
    : Shape(child ? child->getMaterial() : Material()),
      child(std::move(child)), translation(translation),
      rotation(rotationRadians), scale(scale) {
    if (!this->child ||
        std::abs(scale.X()) <= Vec3::EPSILON ||
        std::abs(scale.Y()) <= Vec3::EPSILON ||
        std::abs(scale.Z()) <= Vec3::EPSILON) {
        throw std::invalid_argument("Invalid transformed shape.");
    }
}

Vec3 Transform::rotateForward(const Vec3& vector) const {
    Vec3 result = Quaternion::rotateVector(
        vector, rotation.X(), Vec3(1.0, 0.0, 0.0));
    result = Quaternion::rotateVector(
        result, rotation.Y(), Vec3(0.0, 1.0, 0.0));
    return Quaternion::rotateVector(
        result, rotation.Z(), Vec3(0.0, 0.0, 1.0));
}

Vec3 Transform::rotateInverse(const Vec3& vector) const {
    Vec3 result = Quaternion::rotateVector(
        vector, -rotation.Z(), Vec3(0.0, 0.0, 1.0));
    result = Quaternion::rotateVector(
        result, -rotation.Y(), Vec3(0.0, 1.0, 0.0));
    return Quaternion::rotateVector(
        result, -rotation.X(), Vec3(1.0, 0.0, 0.0));
}

Vec3 Transform::toLocalPoint(const Vec3& point) const {
    const Vec3 rotated = rotateInverse(point - translation);
    return Vec3(rotated.X() / scale.X(),
                rotated.Y() / scale.Y(),
                rotated.Z() / scale.Z());
}

Vec3 Transform::toLocalDirection(const Vec3& direction) const {
    const Vec3 rotated = rotateInverse(direction);
    return Vec3(rotated.X() / scale.X(),
                rotated.Y() / scale.Y(),
                rotated.Z() / scale.Z());
}

Vec3 Transform::toWorldPoint(const Vec3& point) const {
    return translation + rotateForward(Vec3(
        point.X() * scale.X(),
        point.Y() * scale.Y(),
        point.Z() * scale.Z()));
}

Vec3 Transform::toWorldNormal(const Vec3& normal) const {
    return rotateForward(Vec3(
        normal.X() / scale.X(),
        normal.Y() / scale.Y(),
        normal.Z() / scale.Z())).normalize();
}

bool Transform::intersect(const Ray& ray, double minDistance,
                          double maxDistance, HitRecord& hit) const {
    const Vec3 localDirection = toLocalDirection(ray.direction());
    const Ray localRay(
        toLocalPoint(ray.origin()), localDirection, ray.time());
    HitRecord localHit;
    if (!child->intersect(
            localRay, 0.0, maxDistance * 10.0, localHit)) {
        return false;
    }
    const Vec3 worldPoint = toWorldPoint(localHit.point);
    const double worldDistance = worldPoint.distanceTo(ray.origin());
    if (worldDistance < minDistance || worldDistance > maxDistance) {
        return false;
    }
    hit = localHit;
    hit.distance = worldDistance;
    hit.point = worldPoint;
    hit.setFaceNormal(ray, toWorldNormal(
        localHit.frontFace ? localHit.normal : -localHit.normal));
    hit.shape = this;
    return true;
}

bool Transform::boundingBox(Aabb& bounds) const {
    Aabb local;
    if (!child->boundingBox(local)) return false;
    Vec3 minimum(1e30, 1e30, 1e30);
    Vec3 maximum(-1e30, -1e30, -1e30);
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const Vec3 corner(
                    x ? local.max().X() : local.min().X(),
                    y ? local.max().Y() : local.min().Y(),
                    z ? local.max().Z() : local.min().Z());
                const Vec3 world = toWorldPoint(corner);
                minimum = Vec3(
                    std::min(minimum.X(), world.X()),
                    std::min(minimum.Y(), world.Y()),
                    std::min(minimum.Z(), world.Z()));
                maximum = Vec3(
                    std::max(maximum.X(), world.X()),
                    std::max(maximum.Y(), world.Y()),
                    std::max(maximum.Z(), world.Z()));
            }
        }
    }
    bounds = Aabb(minimum, maximum);
    return true;
}

