#include "SimdAabb.h"

#include <algorithm>
#include <cmath>
#include <limits>

bool SimdAabb::intersectScalar(
        const Aabb& aabb,
        const double originsX[4],
        const double originsY[4],
        const double originsZ[4],
        const double invDirX[4],
        const double invDirY[4],
        const double invDirZ[4],
        const double tMin[4],
        const double tMax[4],
        bool results[4]) {
    Vec3 boxMin = aabb.min();
    Vec3 boxMax = aabb.max();
    bool anyHit = false;
    for (int i = 0; i < 4; ++i) {
        double tx0 = (boxMin.X() - originsX[i]) * invDirX[i];
        double tx1 = (boxMax.X() - originsX[i]) * invDirX[i];
        double tmin = std::min(tx0, tx1);
        double tmax = std::max(tx0, tx1);

        double ty0 = (boxMin.Y() - originsY[i]) * invDirY[i];
        double ty1 = (boxMax.Y() - originsY[i]) * invDirY[i];
        tmin = std::max(tmin, std::min(ty0, ty1));
        tmax = std::min(tmax, std::max(ty0, ty1));

        double tz0 = (boxMin.Z() - originsZ[i]) * invDirZ[i];
        double tz1 = (boxMax.Z() - originsZ[i]) * invDirZ[i];
        tmin = std::max(tmin, std::min(tz0, tz1));
        tmax = std::min(tmax, std::max(tz0, tz1));

        results[i] = tmin <= tmax && tmax >= tMin[i] && tmin <= tMax[i];
        anyHit = anyHit || results[i];
    }
    return anyHit;
}

#ifdef RAYTRACER_SIMD_ENABLED

bool SimdAabb::intersect4(
        const Aabb& aabb,
        const double originsX[4],
        const double originsY[4],
        const double originsZ[4],
        const double invDirX[4],
        const double invDirY[4],
        const double invDirZ[4],
        const double tMin[4],
        const double tMax[4],
        bool results[4]) {
    Vec3 boxMin = aabb.min();
    Vec3 boxMax = aabb.max();

    __m256d ox = _mm256_loadu_pd(originsX);
    __m256d oy = _mm256_loadu_pd(originsY);
    __m256d oz = _mm256_loadu_pd(originsZ);
    __m256d idx = _mm256_loadu_pd(invDirX);
    __m256d idy = _mm256_loadu_pd(invDirY);
    __m256d idz = _mm256_loadu_pd(invDirZ);
    __m256d tmin = _mm256_loadu_pd(tMin);
    __m256d tmax = _mm256_loadu_pd(tMax);

    __m256d tx0 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMin.X()), ox), idx);
    __m256d tx1 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMax.X()), ox), idx);
    __m256d near = _mm256_min_pd(tx0, tx1);
    __m256d far = _mm256_max_pd(tx0, tx1);

    __m256d ty0 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMin.Y()), oy), idy);
    __m256d ty1 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMax.Y()), oy), idy);
    near = _mm256_max_pd(near, _mm256_min_pd(ty0, ty1));
    far = _mm256_min_pd(far, _mm256_max_pd(ty0, ty1));

    __m256d tz0 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMin.Z()), oz), idz);
    __m256d tz1 = _mm256_mul_pd(
        _mm256_sub_pd(_mm256_set1_pd(boxMax.Z()), oz), idz);
    near = _mm256_max_pd(near, _mm256_min_pd(tz0, tz1));
    far = _mm256_min_pd(far, _mm256_max_pd(tz0, tz1));

    __m256d cmp = _mm256_and_pd(
        _mm256_cmp_pd(near, far, _CMP_LE_OQ),
        _mm256_cmp_pd(far, tmin, _CMP_GE_OQ));
    cmp = _mm256_and_pd(
        cmp, _mm256_cmp_pd(near, tmax, _CMP_LE_OQ));

    int mask = _mm256_movemask_pd(cmp);
    bool anyHit = mask != 0;
    for (int i = 0; i < 4; ++i) {
        results[i] = (mask & (1 << i)) != 0;
    }
    return anyHit;
}

#endif // RAYTRACER_SIMD_ENABLED
