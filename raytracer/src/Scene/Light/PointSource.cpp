//
// Created by chenx on 2020-05-25.
//
#define _USE_MATH_DEFINES
#include <math.h>
#include "PointSource.h"

PointSource::PointSource(Vec3 position, Vec3 color, double intensity): LightSource(position, color, intensity) {

}

double PointSource::getIncidentBrightness(const Vec3 & incidentPosition) const{
    double rayDist = LightSource::getPosition().distanceTo(incidentPosition);

    return LightSource::getIntensity() / (4 * M_PI * rayDist * rayDist);
}
