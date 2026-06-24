//
// Created by chenx on 2020-05-24.
//

#ifndef RAYTRACER_SPHERE_H
#define RAYTRACER_SPHERE_H


#include "Shape.h"

class Sphere: public Shape {

    public:
        Sphere(const Vec3& center, double radius,
               const Material& material);
        Sphere(const Vec3& center, double radius, const Vec3& surfaceColor,
               const Vec3& emissionColor, double transparency,
               double refractiveIndex);
        bool intersect(const Ray& ray, double minDistance,
                       double maxDistance, HitRecord& hit) const override;

        double getRadius() const {return radius;}
        const Vec3& getCenter() const {return center;}
    private:
        Vec3 center;
        double radius;
        double radiusSquared;
};


#endif //RAYTRACER_SPHERE_H
