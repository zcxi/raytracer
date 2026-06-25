#ifndef RAYTRACER_SIMD_AABB_H
#define RAYTRACER_SIMD_AABB_H

#include "Aabb.h"
#include "../Math/Ray.h"

#ifdef RAYTRACER_SIMD_ENABLED
#include <immintrin.h>
#endif

class SimdAabb {
public:
#ifdef RAYTRACER_SIMD_ENABLED
    static bool intersect4(
        const Aabb& aabb,
        const double originsX[4],
        const double originsY[4],
        const double originsZ[4],
        const double invDirX[4],
        const double invDirY[4],
        const double invDirZ[4],
        const double tMin[4],
        const double tMax[4],
        bool results[4]);
#endif

    static bool intersectScalar(
        const Aabb& aabb,
        const double originsX[4],
        const double originsY[4],
        const double originsZ[4],
        const double invDirX[4],
        const double invDirY[4],
        const double invDirZ[4],
        const double tMin[4],
        const double tMax[4],
        bool results[4]);
};

#endif
