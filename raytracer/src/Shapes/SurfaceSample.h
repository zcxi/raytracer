#ifndef RAYTRACER_SURFACE_SAMPLE_H
#define RAYTRACER_SURFACE_SAMPLE_H

#include "../Math/Vec3.h"

struct SurfaceSample {
    Vec3 point;
    Vec3 normal;
    double areaPdf;

    SurfaceSample() : point(), normal(), areaPdf(0.0) {}
};

#endif

