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
