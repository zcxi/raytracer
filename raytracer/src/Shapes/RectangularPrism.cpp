#include "RectangularPrism.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

double component(const Vec3& vector, int axis) {
    if (axis == 0) {
        return vector.X();
    }
    if (axis == 1) {
        return vector.Y();
    }
    return vector.Z();
}

Vec3 axisNormal(int axis, double sign) {
    if (axis == 0) {
        return Vec3(sign, 0.0, 0.0);
    }
    if (axis == 1) {
        return Vec3(0.0, sign, 0.0);
    }
    return Vec3(0.0, 0.0, sign);
}

} // namespace

RectangularPrism::RectangularPrism(
        const Vec3& center, const Vec3& dimensions,
        const Material& material)
    : Shape(material),
      center(center),
      dimensions(dimensions),
      minimum(center - dimensions * 0.5),
      maximum(center + dimensions * 0.5) {
    if (dimensions.X() <= Vec3::EPSILON ||
        dimensions.Y() <= Vec3::EPSILON ||
        dimensions.Z() <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Rectangular prism dimensions must all be positive.");
    }
}

RectangularPrism::RectangularPrism(
        const Vec3& center, const Vec3& dimensions,
        const Vec3& surfaceColor, const Vec3& emissionColor,
        double transparency, double refractiveIndex)
    : RectangularPrism(
          center, dimensions,
          transparency > 0.0
              ? Material::dielectric(
                    refractiveIndex, surfaceColor, emissionColor)
              : Material::diffuse(surfaceColor, emissionColor)) {
    if (transparency < 0.0 || transparency > 1.0) {
        throw std::invalid_argument(
            "Transparency must be between zero and one.");
    }
}

bool RectangularPrism::intersect(const Ray& ray, double minDistance,
                                 double maxDistance, HitRecord& hit) const {
    double entryDistance = minDistance;
    double exitDistance = maxDistance;
    Vec3 entryNormal;
    Vec3 exitNormal;

    for (int axis = 0; axis < 3; ++axis) {
        const double origin = component(ray.origin(), axis);
        const double direction = component(ray.direction(), axis);
        const double slabMinimum = component(minimum, axis);
        const double slabMaximum = component(maximum, axis);

        if (std::abs(direction) <= Vec3::EPSILON) {
            if (origin < slabMinimum || origin > slabMaximum) {
                return false;
            }
            continue;
        }

        double nearDistance = (slabMinimum - origin) / direction;
        double farDistance = (slabMaximum - origin) / direction;
        Vec3 nearNormal = axisNormal(axis, -1.0);
        Vec3 farNormal = axisNormal(axis, 1.0);

        if (nearDistance > farDistance) {
            std::swap(nearDistance, farDistance);
            std::swap(nearNormal, farNormal);
        }

        if (nearDistance > entryDistance) {
            entryDistance = nearDistance;
            entryNormal = nearNormal;
        }
        if (farDistance < exitDistance) {
            exitDistance = farDistance;
            exitNormal = farNormal;
        }
        if (entryDistance > exitDistance) {
            return false;
        }
    }

    double distance = entryDistance;
    Vec3 outwardNormal = entryNormal;
    if (outwardNormal.getLength() <= Vec3::EPSILON) {
        distance = exitDistance;
        outwardNormal = exitNormal;
    }
    if (distance < minDistance || distance > maxDistance ||
        outwardNormal.getLength() <= Vec3::EPSILON) {
        return false;
    }

    hit.distance = distance;
    hit.point = ray.at(distance);
    hit.setFaceNormal(ray, outwardNormal);
    hit.shape = this;
    return true;
}
