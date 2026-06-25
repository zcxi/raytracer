//
// Created by chenx on 2020-05-25.
//
#include <cmath>
#include "PointSource.h"

PointSource::PointSource(const Vec3& position, const Vec3& color,
                         double intensity)
    : LightSource(position, color, intensity) {

}

double PointSource::getIncidentBrightness(const Vec3 & incidentPosition) const{
    const double rayDist = LightSource::getPosition().distanceTo(incidentPosition);
    if (rayDist <= Vec3::EPSILON) {
        return 0.0;
    }
    const double pi = 3.14159265358979323846;
    return LightSource::getIntensity() / (4.0 * pi * rayDist * rayDist);
}

bool PointSource::sampleIncident(
        const Vec3& point, const Vec3& surfaceNormal,
        LightSample& sample) const {
    const Vec3 toLight = position - point;
    const double distance = toLight.getLength();
    if (distance <= Vec3::EPSILON) {
        return false;
    }
    const Vec3 direction = toLight / distance;
    if (surfaceNormal.dot(direction) <= 0.0) {
        return false;
    }
    const double attenuation = intensity / (4.0 * 3.14159265358979323846 * distance * distance);
    sample.direction = direction;
    sample.radiance = color * attenuation;
    sample.maximumDistance = distance - 1e-4;
    sample.valid = true;
    return true;
}
