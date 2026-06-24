#ifndef RAYTRACER_PYRAMID_H
#define RAYTRACER_PYRAMID_H

#include "Shape.h"

#include <array>

class Pyramid : public Shape {
public:
    Pyramid(const Vec3& baseCenter, double baseWidth, double baseDepth,
            double height, const Material& material);
    Pyramid(const Vec3& baseCenter, double baseWidth, double baseDepth,
            double height, const Vec3& surfaceColor,
            const Vec3& emissionColor, double transparency,
            double refractiveIndex);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;

    const Vec3& getBaseCenter() const { return baseCenter; }
    double getBaseWidth() const { return baseWidth; }
    double getBaseDepth() const { return baseDepth; }
    double getHeight() const { return height; }

private:
    struct Triangle {
        Vec3 first;
        Vec3 second;
        Vec3 third;
        Vec3 normal;

        Triangle(const Vec3& first, const Vec3& second, const Vec3& third);
    };

    static std::array<Triangle, 6> makeTriangles(
        const Vec3& baseCenter, double width, double depth, double height);
    static bool intersectTriangle(const Ray& ray, const Triangle& triangle,
                                  double minDistance, double maxDistance,
                                  double& distance);

    Vec3 baseCenter;
    double baseWidth;
    double baseDepth;
    double height;
    std::array<Triangle, 6> triangles;
};

#endif
