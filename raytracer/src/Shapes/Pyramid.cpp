#include "Pyramid.h"

#include <cmath>
#include <stdexcept>

std::array<Pyramid::Triangle, 6> Pyramid::makeTriangles(
        const Vec3& baseCenter, double width, double depth, double height) {
    const double halfWidth = width * 0.5;
    const double halfDepth = depth * 0.5;
    const double y = baseCenter.Y();

    const Vec3 backLeft(baseCenter.X() - halfWidth, y,
                        baseCenter.Z() - halfDepth);
    const Vec3 backRight(baseCenter.X() + halfWidth, y,
                         baseCenter.Z() - halfDepth);
    const Vec3 frontRight(baseCenter.X() + halfWidth, y,
                          baseCenter.Z() + halfDepth);
    const Vec3 frontLeft(baseCenter.X() - halfWidth, y,
                         baseCenter.Z() + halfDepth);
    const Vec3 apex(baseCenter.X(), y + height, baseCenter.Z());

    return {{
        Pyramid::Triangle(backLeft, backRight, frontRight),
        Pyramid::Triangle(backLeft, frontRight, frontLeft),
        Pyramid::Triangle(backLeft, apex, backRight),
        Pyramid::Triangle(backRight, apex, frontRight),
        Pyramid::Triangle(frontRight, apex, frontLeft),
        Pyramid::Triangle(frontLeft, apex, backLeft)
    }};
}

Pyramid::Triangle::Triangle(const Vec3& first, const Vec3& second,
                            const Vec3& third)
    : first(first),
      second(second),
      third(third),
      normal((second - first).cross(third - first).normalize()) {
}

Pyramid::Pyramid(const Vec3& baseCenter, double baseWidth,
                 double baseDepth, double height,
                 const Material& material)
    : Shape(material),
      baseCenter(baseCenter),
      baseWidth(baseWidth),
      baseDepth(baseDepth),
      height(height),
      triangles(makeTriangles(
          baseCenter, baseWidth, baseDepth, height)) {
    if (baseWidth <= Vec3::EPSILON ||
        baseDepth <= Vec3::EPSILON ||
        height <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Pyramid width, depth, and height must all be positive.");
    }
}

Pyramid::Pyramid(const Vec3& baseCenter, double baseWidth, double baseDepth,
                 double height, const Vec3& surfaceColor,
                 const Vec3& emissionColor, double transparency,
                 double refractiveIndex)
    : Pyramid(
          baseCenter, baseWidth, baseDepth, height,
          transparency > 0.0
              ? Material::dielectric(
                    refractiveIndex, surfaceColor, emissionColor)
              : Material::diffuse(surfaceColor, emissionColor)) {
    if (transparency < 0.0 || transparency > 1.0) {
        throw std::invalid_argument(
            "Transparency must be between zero and one.");
    }
}

bool Pyramid::intersectTriangle(const Ray& ray, const Triangle& triangle,
                                double minDistance, double maxDistance,
                                double& distance) {
    const Vec3 firstEdge = triangle.second - triangle.first;
    const Vec3 secondEdge = triangle.third - triangle.first;
    const Vec3 perpendicular = ray.direction().cross(secondEdge);
    const double determinant = firstEdge.dot(perpendicular);

    if (std::abs(determinant) <= Vec3::EPSILON) {
        return false;
    }

    const double inverseDeterminant = 1.0 / determinant;
    const Vec3 originOffset = ray.origin() - triangle.first;
    const double firstCoordinate =
        originOffset.dot(perpendicular) * inverseDeterminant;
    if (firstCoordinate < -Vec3::EPSILON ||
        firstCoordinate > 1.0 + Vec3::EPSILON) {
        return false;
    }

    const Vec3 crossOffset = originOffset.cross(firstEdge);
    const double secondCoordinate =
        ray.direction().dot(crossOffset) * inverseDeterminant;
    if (secondCoordinate < -Vec3::EPSILON ||
        firstCoordinate + secondCoordinate > 1.0 + Vec3::EPSILON) {
        return false;
    }

    distance = secondEdge.dot(crossOffset) * inverseDeterminant;
    return distance >= minDistance && distance <= maxDistance;
}

bool Pyramid::intersect(const Ray& ray, double minDistance,
                        double maxDistance, HitRecord& hit) const {
    bool foundHit = false;
    double closestDistance = maxDistance;
    Vec3 closestNormal;

    for (const Triangle& triangle : triangles) {
        double distance = 0.0;
        if (intersectTriangle(ray, triangle, minDistance,
                              closestDistance, distance)) {
            foundHit = true;
            closestDistance = distance;
            closestNormal = triangle.normal;
        }
    }

    if (!foundHit) {
        return false;
    }

    hit.distance = closestDistance;
    hit.point = ray.at(closestDistance);
    hit.setFaceNormal(ray, closestNormal);
    hit.shape = this;
    return true;
}

bool Pyramid::boundingBox(Aabb& bounds) const {
    const double halfWidth = baseWidth * 0.5;
    const double halfDepth = baseDepth * 0.5;
    bounds = Aabb(
        Vec3(baseCenter.X() - halfWidth,
             baseCenter.Y(),
             baseCenter.Z() - halfDepth),
        Vec3(baseCenter.X() + halfWidth,
             baseCenter.Y() + height,
             baseCenter.Z() + halfDepth));
    return true;
}
