//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_POINTSOURCE_H
#define RAYTRACER_POINTSOURCE_H


#include "LightSource.h"

class PointSource: public LightSource{

    public:
        PointSource(const Vec3& position, const Vec3& color, double intensity);
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
        bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            LightSample& sample) const override;
};


#endif //RAYTRACER_POINTSOURCE_H
