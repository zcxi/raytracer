//
// Directional light implementation.
//
#include <cmath>
#include <limits>
#include <stdexcept>
#include "DirectionalSource.h"
#include "../../Math/Sampler.h"

DirectionalSource::DirectionalSource(
        const Vec3& incomingDir, const Vec3& color,
        double irradiance, double angularRadius)
    : LightSource(Vec3(), color, irradiance),
      dir(incomingDir.normalize()),
      irradiance(irradiance),
      angularRadius(angularRadius) {
    if (irradiance < 0.0) {
        throw std::invalid_argument(
            "Directional light irradiance cannot be negative.");
    }
    if (dir.getLength() <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Directional light direction must be non-zero.");
    }
    if (angularRadius < 0.0 ||
        angularRadius >= 3.14159265358979323846 * 0.5) {
        throw std::invalid_argument(
            "Directional light angular radius must be in [0, PI/2).");
    }
}

double DirectionalSource::getIncidentBrightness(
        const Vec3 & incidentPosition) const {
    (void)incidentPosition;
    return irradiance;
}

bool DirectionalSource::sampleIncident(
        const Vec3& point, const Vec3& surfaceNormal,
        Sampler& sampler, LightSample& sample) const {
    (void)point;
    sample = LightSample();
    const Vec3 center = -dir;
    Vec3 towardLight = center;
    if (angularRadius > Vec3::EPSILON) {
        const double cosineMaximum = std::cos(angularRadius);
        const double cosine =
            1.0 - sampler.next() * (1.0 - cosineMaximum);
        const double sine =
            std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
        const double angle =
            2.0 * 3.14159265358979323846 * sampler.next();
        const Vec3 helper = std::abs(center.X()) > 0.9
            ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
        const Vec3 tangent = helper.cross(center).normalize();
        const Vec3 bitangent = center.cross(tangent);
        towardLight = (
            center * cosine +
            tangent * (sine * std::cos(angle)) +
            bitangent * (sine * std::sin(angle))).normalize();
    }
    if (surfaceNormal.dot(towardLight) <= 0.0) {
        sample.rejection = LightRejection::Backface;
        return false;
    }
    sample.direction = towardLight;
    sample.radiance = color * irradiance;
    sample.maximumDistance = std::numeric_limits<double>::infinity();
    sample.valid = true;
    return true;
}
