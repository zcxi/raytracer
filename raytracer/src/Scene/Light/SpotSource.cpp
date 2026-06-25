//
// Spot light implementation.
//
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "SpotSource.h"

SpotSource::SpotSource(const Vec3& position, const Vec3& dir,
                       const Vec3& color, double intensity,
                       double range, double innerAngle, double outerAngle)
    : LightSource(position, color, intensity),
      dir(dir.normalize()),
      range(range),
      innerAngle(innerAngle),
      outerAngle(outerAngle) {
    if (intensity < 0.0) {
        throw std::invalid_argument(
            "Spot light intensity cannot be negative.");
    }
    if (range <= 0.0) {
        throw std::invalid_argument(
            "Spot light range must be positive.");
    }
    if (innerAngle < 0.0 || innerAngle > outerAngle ||
        outerAngle > 3.14159265358979323846) {
        throw std::invalid_argument(
            "Spot light cone angles must satisfy 0 <= innerAngle <= outerAngle <= PI.");
    }
    if (innerAngle >= 3.14159265358979323846 * 0.5) {
        throw std::invalid_argument(
            "Spot light cone angles must produce a visible beam.");
    }
}

double SpotSource::getIncidentBrightness(
        const Vec3 & incidentPosition) const {
    const double distance = position.distanceTo(incidentPosition);
    if (distance <= Vec3::EPSILON || distance > range) {
        return 0.0;
    }
    const Vec3 toLight = (position - incidentPosition).normalize();
    const double cosine = std::max(0.0, (-toLight).dot(dir));
    const double innerCosine = std::cos(innerAngle);
    const double outerCosine = std::cos(outerAngle);
    if (cosine < outerCosine) {
        return 0.0;
    }
    const double pi = 3.14159265358979323846;
    double falloff = intensity / (4.0 * pi * distance * distance);
    if (cosine > innerCosine + Vec3::EPSILON) {
        const double t = (cosine - outerCosine) /
            std::max(Vec3::EPSILON, innerCosine - outerCosine);
        falloff *= t * t * (3.0 - 2.0 * t);
    }
    return falloff;
}

bool SpotSource::sampleIncident(
        const Vec3& point, const Vec3& surfaceNormal,
        LightSample& sample) const {
    const Vec3 toLight = position - point;
    const double distance = toLight.getLength();
    if (distance <= Vec3::EPSILON || distance > range) {
        return false;
    }
    const Vec3 direction = toLight / distance;
    if (surfaceNormal.dot(direction) <= 0.0) {
        return false;
    }
    const double cosine = (-direction).dot(dir);
    const double outerCosine = std::cos(outerAngle);
    if (cosine < outerCosine) {
        return false;
    }
    const double innerCosine = std::cos(innerAngle);
    double coneWeight = 1.0;
    if (cosine < innerCosine) {
        const double t = (cosine - outerCosine) /
            std::max(Vec3::EPSILON, innerCosine - outerCosine);
        coneWeight = t * t * (3.0 - 2.0 * t);
    }
    const double pi = 3.14159265358979323846;
    const double attenuation =
        intensity * coneWeight / (4.0 * pi * distance * distance);
    sample.direction = direction;
    sample.radiance = color * attenuation;
    sample.maximumDistance = distance - 1e-4;
    sample.valid = true;
    return true;
}
