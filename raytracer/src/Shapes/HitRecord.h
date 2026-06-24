#ifndef RAYTRACER_HITRECORD_H
#define RAYTRACER_HITRECORD_H

#include "../Math/Ray.h"

class Shape;

struct HitRecord {
    double distance;
    Vec3 point;
    Vec3 normal;
    bool frontFace;
    const Shape* shape;

    HitRecord()
        : distance(0.0), point(), normal(), frontFace(true), shape(nullptr) {
    }

    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal) {
        frontFace = ray.direction().dot(outwardNormal) < 0.0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

#endif

