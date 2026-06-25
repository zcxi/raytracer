//
// Directional light with constant radiance from a fixed direction.
//

#ifndef RAYTRACER_DIRECTIONALSOURCE_H
#define RAYTRACER_DIRECTIONALSOURCE_H


#include "LightSource.h"

class DirectionalSource: public LightSource {

    public:
        DirectionalSource(const Vec3& dir, const Vec3& color,
                          double irradiance);
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
        bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            LightSample& sample) const override;
        bool isFinite() const override { return false; }
        const Vec3& getDirection() const { return dir; }

    private:
        Vec3 dir;
        double irradiance;
};


#endif
