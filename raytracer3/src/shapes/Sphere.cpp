//
// Created by chenx on 2020-05-24.
//

#include <cmath>
#include "Sphere.h"

Sphere::Sphere(Vec3 center, double radius, Vec3 surfaceColor,
        Vec3 emissionColor, double transparency, double refractiveIndex)
        :Shape(surfaceColor, emissionColor, transparency, refractiveIndex){

    this->center = Vec3(center);
    this->radius = radius;
    this->radiusSquared = radius * radius;
}

Vec3* Sphere::getRayIntersection(const Vec3 &rayOrigin, const Vec3 &rayDirection) const{

    Vec3 rayDir = rayDirection.normalize();
    Vec3 centerToRay = center - rayOrigin;

    Vec3 projection = centerToRay.projectOnto(rayDir) + rayOrigin;

    double intersectDistance = centerToRay.dot(rayDir);
    if (intersectDistance <= 0)
    {
        return nullptr;
    }
    //project origin to origin onto ray
    double perpendicularSquared = centerToRay.dot(centerToRay) - intersectDistance * intersectDistance;
    if (perpendicularSquared > radiusSquared){
        return nullptr;
    }
    //half the distance that the ray will travel inside the sphere
    double dist = sqrt(radiusSquared - perpendicularSquared);

    return new Vec3(projection - rayDir * dist);
}

Vec3 Sphere::getNormal(const Vec3 &point) const {
    Vec3 normal = point - center;

    return normal.normalize();
}
