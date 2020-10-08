//
// Created by chenx on 2020-06-10.
//

#include "Plane.h"

Plane::Plane(Vec3 normal, Vec3 point, Vec3 surfaceColor, Vec3 emissionColor, double transparency, double refractiveIndex)
    :Shape(surfaceColor, emissionColor, transparency, refractiveIndex) {
    this->normal = normal.normalize();
    this->point = point;
}

Vec3* Plane::getRayIntersection(const Vec3 &rayOrigin, const Vec3 &rayDirection) const{

    Vec3 rayDir = rayDirection.normalize();
    double var = normal.dot(rayDirection);
    if (var > Vec3::EPISLON){
        Vec3 rayToPlane = point - rayOrigin;
        double dist = rayToPlane.dot(normal) / var;
        if (dist >= 0){
            Vec3* ret = new Vec3(rayDirection * dist + rayOrigin);
            return ret;
        }
    }
    return nullptr;
}


Vec3 Plane::getNormal(const Vec3 &point) const {
    if (point.dot(normal) == 0){
        return Vec3();
    }
    return normal;
}