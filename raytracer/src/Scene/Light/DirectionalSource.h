//
// Directional light with constant radiance from a fixed direction.
//

#ifndef RAYTRACER_DIRECTIONALSOURCE_H
#define RAYTRACER_DIRECTIONALSOURCE_H


#include "LightSource.h"

class DirectionalSource: public LightSource {

    public:
        using LightSource::sampleIncident;
        DirectionalSource(const Vec3& dir, const Vec3& color,
                          double irradiance,
                          double angularRadius = 0.0);
        double getIncidentBrightness(const Vec3 & incidentPosition) const override;
        bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            Sampler& sampler, LightSample& sample) const override;
        bool isFinite() const override { return false; }
        const Vec3& getDirection() const { return dir; }
        double getAngularRadius() const { return angularRadius; }

    private:
        Vec3 dir;
        double irradiance;
        double angularRadius;
};


#endif
