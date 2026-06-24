#ifndef RAYTRACER_RECTANGULAR_PRISM_H
#define RAYTRACER_RECTANGULAR_PRISM_H

#include "Shape.h"

class RectangularPrism : public Shape {
public:
    RectangularPrism(const Vec3& center, const Vec3& dimensions,
                     const Material& material);
    RectangularPrism(const Vec3& center, const Vec3& dimensions,
                     const Vec3& surfaceColor, const Vec3& emissionColor,
                     double transparency, double refractiveIndex);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override {
        bounds = Aabb(minimum, maximum);
        return true;
    }

    const Vec3& getCenter() const { return center; }
    const Vec3& getDimensions() const { return dimensions; }

private:
    Vec3 center;
    Vec3 dimensions;
    Vec3 minimum;
    Vec3 maximum;
};

#endif
