//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_LIGHTSOURCE_H
#define RAYTRACER_LIGHTSOURCE_H


#include "../../Math/Vec3.h"

struct LightSample {
    Vec3 direction;
    Vec3 radiance;
    double maximumDistance;
    bool valid;

    LightSample()
        : direction(), radiance(), maximumDistance(0.0), valid(false) {}
};

class LightSource {

    public:
        LightSource(const Vec3& position, const Vec3& color, double intensity);
        virtual ~LightSource() {}
        const Vec3& getPosition() const;
        double getIntensity() const;
        const Vec3& getColor() const;

        virtual double getIncidentBrightness(const Vec3 & pos) const = 0;
        virtual bool sampleIncident(
            const Vec3& point, const Vec3& surfaceNormal,
            LightSample& sample) const = 0;
        virtual bool isFinite() const { return true; }

protected:
        Vec3 position;
        Vec3 color;
        double intensity;

};


#endif //RAYTRACER_LIGHTSOURCE_H
