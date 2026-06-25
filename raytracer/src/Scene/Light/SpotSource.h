//
// Spot light with cone angle and range.
//

#ifndef RAYTRACER_SPOTSOURCE_H
#define RAYTRACER_SPOTSOURCE_H


#include "LightSource.h"

class SpotSource: public LightSource {

    public:
        SpotSource(const Vec3& position, const Vec3& dir,
                   const Vec3& color, double intensity,
                   double range, double innerAngle, double outerAngle);
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
        bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            LightSample& sample) const override;
        double getRange() const { return range; }
        double getInnerAngle() const { return innerAngle; }
        double getOuterAngle() const { return outerAngle; }

    private:
        Vec3 dir;
        double range;
        double innerAngle;
        double outerAngle;
};


#endif
