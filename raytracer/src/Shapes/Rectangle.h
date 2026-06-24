#ifndef RAYTRACER_RECTANGLE_H
#define RAYTRACER_RECTANGLE_H

#include "Shape.h"

class Rectangle : public Shape {
public:
    Rectangle(const Vec3& center, const Vec3& normal,
              const Vec3& upDirection, double width, double height,
              const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const override;
    double surfaceArea() const override { return width * height; }

private:
    Vec3 center;
    Vec3 normal;
    Vec3 right;
    Vec3 up;
    double width;
    double height;
};

#endif

