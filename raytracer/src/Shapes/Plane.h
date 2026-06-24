//
// Created by chenx on 2020-06-10.
//

#ifndef RAYTRACERV2_PLANE_H
#define RAYTRACERV2_PLANE_H


#include "Shape.h"

class Plane: public Shape {

    public:

        Plane(const Vec3& normal, const Vec3& point,
              const Material& material);
        Plane(const Vec3& normal, const Vec3& point, const Vec3& surfaceColor,
              const Vec3& emissionColor, double transparency,
              double refractiveIndex);
        bool intersect(const Ray& ray, double minDistance,
                       double maxDistance, HitRecord& hit) const override;

    private:
        Vec3 normal;
        Vec3 point;
};

#endif //RAYTRACERV2_PLANE_H
