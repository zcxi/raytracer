//
// Created by chenx on 2020-05-24.
//

#ifndef RAYTRACER_SPHERE_H
#define RAYTRACER_SPHERE_H


#include "Shape.h"

class Sphere: public Shape {

    public:
        Sphere(Vec3 center, double radius, Vec3 surfaceColor,
               Vec3 emissionColor, double transparency, double refractiveIndex);
        Vec3* getRayIntersection(const Vec3 &rayOrigin, const Vec3 &rayDirection) const;
        Vec3 getNormal(const Vec3 &point) const;

        double getRadius() const {return radius;};
        Vec3 getCenter() const {return center;};
        Vec3 getSurfaceColor() const {return Shape::getSurfaceColor();};

    private:
        Vec3 center;
        double radius;
        double radiusSquared;
};


#endif //RAYTRACER_SPHERE_H
