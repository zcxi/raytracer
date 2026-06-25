#include "Lathe.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "../Math/Sampler.h"
#include "SurfaceSample.h"

namespace {

const double PI = 3.14159265358979323846;

} // namespace

Lathe::Lathe(const std::vector<ProfilePoint>& profile,
             unsigned int segments,
             const Material& material)
    : Shape(material), triangles(), triangleShapes(),
      triangleCdf(), bvh(), bounds(), totalArea(0.0) {
    if (profile.size() < 3) {
        throw std::invalid_argument(
            "Lathe profile must have at least 3 points.");
    }
    if (segments < 4 || segments > 512) {
        throw std::invalid_argument(
            "Lathe segments must be in range [4, 512].");
    }

    double minR = std::numeric_limits<double>::infinity();
    double maxR = -std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto& p : profile) {
        minR = std::min(minR, p.r);
        maxR = std::max(maxR, p.r);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    if (maxR <= 0.0 || minR < 0.0) {
        throw std::invalid_argument(
            "Lathe profile radii must be non-negative with at least one positive.");
    }

    for (unsigned int s = 0; s < segments; ++s) {
        double theta0 = 2.0 * PI * s / segments;
        double theta1 = 2.0 * PI * (s + 1) / segments;
        double c0 = std::cos(theta0), s0 = std::sin(theta0);
        double c1 = std::cos(theta1), s1 = std::sin(theta1);

        for (std::size_t j = 0; j + 1 < profile.size(); ++j) {
            const auto& p0 = profile[j];
            const auto& p1 = profile[j + 1];
            double dr = p1.r - p0.r;
            double dy = p1.y - p0.y;

            Vec3 v00(p0.r * c0, p0.y, p0.r * s0);
            Vec3 v10(p0.r * c1, p0.y, p0.r * s1);
            Vec3 v01(p1.r * c0, p1.y, p1.r * s0);
            Vec3 v11(p1.r * c1, p1.y, p1.r * s1);

            double edgeLen = std::sqrt(dr * dr + dy * dy);
            double nx = edgeLen > 1e-12 ? dy / edgeLen : 0.0;
            double ny = edgeLen > 1e-12 ? -dr / edgeLen : 0.0;

            double u0 = static_cast<double>(s) / segments;
            double u1 = static_cast<double>(s + 1) / segments;
            double v0 = (maxY > minY)
                ? (p0.y - minY) / (maxY - minY) : 0.0;
            double v1 = (maxY > minY)
                ? (p1.y - minY) / (maxY - minY) : 0.0;

            Vertex vert00(v00, Vec3(nx * c0, ny, nx * s0), u0, v0);
            Vertex vert10(v10, Vec3(nx * c1, ny, nx * s1), u1, v0);
            Vertex vert01(v01, Vec3(nx * c0, ny, nx * s0), u0, v1);
            Vertex vert11(v11, Vec3(nx * c1, ny, nx * s1), u1, v1);

            triangles.emplace_back(vert00, vert10, vert11, material);
            triangles.emplace_back(vert00, vert11, vert01, material);
        }
    }

    if (triangles.empty()) {
        throw std::invalid_argument("Lathe produced no triangles.");
    }

    triangles[0].boundingBox(bounds);
    totalArea = 0.0;
    triangleCdf.reserve(triangles.size());
    for (std::size_t i = 0; i < triangles.size(); ++i) {
        Aabb triBounds;
        triangles[i].boundingBox(triBounds);
        if (i > 0) bounds = Aabb::surrounding(bounds, triBounds);
        double area = triangles[i].computeArea();
        totalArea += area;
        triangleCdf.push_back(totalArea);
    }

    triangleShapes.reserve(triangles.size());
    for (const Triangle& tri : triangles) {
        triangleShapes.push_back(&tri);
    }
    bvh.build(triangleShapes);
}

bool Lathe::intersect(const Ray& ray, double minDistance,
                      double maxDistance, HitRecord& hit) const {
    if (bvh.intersect(ray, minDistance, maxDistance, hit)) {
        hit.shape = this;
        return true;
    }
    return false;
}

bool Lathe::boundingBox(Aabb& output) const {
    output = bounds;
    return true;
}

bool Lathe::sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const {
    if (totalArea <= 0.0 || triangleCdf.empty()) return false;
    double target = sampler.next() * totalArea;
    auto it = std::lower_bound(
        triangleCdf.begin(), triangleCdf.end(), target);
    if (it == triangleCdf.end()) return false;
    std::size_t index = static_cast<std::size_t>(
        it - triangleCdf.begin());
    if (!triangles[index].sampleSurface(sampler, sample)) return false;
    sample.normal = sample.normal.normalize();
    return true;
}
