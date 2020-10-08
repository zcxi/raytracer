//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_LIGHTSOURCE_H
#define RAYTRACER_LIGHTSOURCE_H


#include "../../Math/Vec3.h"

class LightSource {

    public:
        LightSource(Vec3 position, Vec3 color, double intensity);
        Vec3 getPosition() const;
        double getIntensity() const;
        Vec3 getColor() const;

        virtual double getIncidentBrightness(const Vec3 & pos) const = 0;

protected:
        Vec3 position;
        Vec3 color;
        double intensity;

};
//Types of lightSources: Point, Sun, SpotLight, Area

#endif //RAYTRACER_LIGHTSOURCE_H
