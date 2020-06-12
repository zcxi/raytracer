//
// Created by chenx on 2020-06-10.
//

#ifndef RAYTRACERV2_PLANE_H
#define RAYTRACERV2_PLANE_H


#include "Shape.h"

class Plane: public Shape {

    public:

        Plane(Vec3 normal, Vec3 point, Vec3 surfaceColor,
            Vec3 emissionColor, double transparency, double refractiveIndex);
        Vec3* getRayIntersection(const Vec3 &rayOrigin, const Vec3 &rayDirection) const;
        Vec3 getNormal(const Vec3 &rayDir) const;

        Vec3 getSurfaceColor() const {return Shape::getSurfaceColor();};

    private:
        Vec3 normal;
        Vec3 point;
};

#endif //RAYTRACERV2_PLANE_H
