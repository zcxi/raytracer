//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_POINTSOURCE_H
#define RAYTRACER_POINTSOURCE_H


#include "LightSource.h"
#include <limits>

class PointSource: public LightSource{

    public:
        PointSource(
            const Vec3& position, const Vec3& color, double intensity,
            double range = std::numeric_limits<double>::infinity());
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
        bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            LightSample& sample) const override;
        double getRange() const { return range; }

    private:
        double range;
};


#endif //RAYTRACER_POINTSOURCE_H
