#ifndef RAYTRACER_CUBE_H
#define RAYTRACER_CUBE_H

#include "RectangularPrism.h"

class Cube : public RectangularPrism {
public:
    Cube(const Vec3& center, double sideLength,
         const Material& material);
    Cube(const Vec3& center, double sideLength,
         const Vec3& surfaceColor, const Vec3& emissionColor,
         double transparency, double refractiveIndex);
};

#endif
