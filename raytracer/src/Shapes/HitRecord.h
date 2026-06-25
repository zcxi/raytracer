#ifndef RAYTRACER_HITRECORD_H
#define RAYTRACER_HITRECORD_H

#include "../Math/Ray.h"

class Shape;

struct HitRecord {
    double distance;
    Vec3 point;
    Vec3 normal;
    Vec3 geometricNormal;
    bool frontFace;
    const Shape* shape;
    double u;
    double v;

    HitRecord()
        : distance(0.0), point(), normal(), geometricNormal(),
          frontFace(true), shape(nullptr),
          u(0.0), v(0.0) {
    }

    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal) {
        frontFace = ray.direction().dot(outwardNormal) < 0.0;
        geometricNormal = frontFace ? outwardNormal : -outwardNormal;
        normal = geometricNormal;
    }
};

#endif
