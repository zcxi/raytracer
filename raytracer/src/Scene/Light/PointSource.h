//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_POINTSOURCE_H
#define RAYTRACER_POINTSOURCE_H


#include "LightSource.h"

class PointSource: public LightSource{

    public:
        PointSource(Vec3 position, Vec3 color, double intensity);
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
};


#endif //RAYTRACER_POINTSOURCE_H
