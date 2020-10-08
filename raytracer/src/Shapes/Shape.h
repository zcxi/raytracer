//
// Created by chenx on 2020-05-24.
//

#ifndef RAYTRACER_SHAPE_H
#define RAYTRACER_SHAPE_H


#include "../Math/Vec3.h"

class Shape {

    public:

        Shape(Vec3 surfaceColor, Vec3 emissionColor, double transparency, double refractiveIndex);
        int getId();
        void setId(int id);

        virtual Vec3* getRayIntersection(const Vec3 &rayOrigin, const Vec3 &rayDirection) const = 0;
        virtual Vec3 getNormal(const Vec3 &vector) const = 0;

        const Vec3& getSurfaceColor() const {
            return surfaceColor;
        }

        Vec3 getReflectionDir(const Vec3 &hit, const Vec3 &dir);

    private:
        int id;
        Vec3 surfaceColor;
        Vec3 emissionColor;
        double transparency;
        double refractiveIndex;

};


#endif //RAYTRACER_SHAPE_H
