//
// Directional light implementation.
//
#include <cmath>
#include <limits>
#include <stdexcept>
#include "DirectionalSource.h"

DirectionalSource::DirectionalSource(
        const Vec3& incomingDir, const Vec3& color,
        double irradiance)
    : LightSource(Vec3(), color, irradiance),
      dir(incomingDir.normalize()),
      irradiance(irradiance) {
    if (irradiance < 0.0) {
        throw std::invalid_argument(
            "Directional light irradiance cannot be negative.");
    }
    if (dir.getLength() <= Vec3::EPSILON) {
        throw std::invalid_argument(
            "Directional light direction must be non-zero.");
    }
}

double DirectionalSource::getIncidentBrightness(
        const Vec3 & incidentPosition) const {
    (void)incidentPosition;
    return irradiance;
}

bool DirectionalSource::sampleIncident(
        const Vec3& point, const Vec3& surfaceNormal,
        LightSample& sample) const {
    (void)point;
    sample = LightSample();
    const Vec3 towardLight = -dir;
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
