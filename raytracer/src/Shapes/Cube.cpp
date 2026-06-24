#include "Cube.h"

Cube::Cube(const Vec3& center, double sideLength,
           const Vec3& surfaceColor, const Vec3& emissionColor,
           double transparency, double refractiveIndex)
    : RectangularPrism(center,
                       Vec3(sideLength, sideLength, sideLength),
                       surfaceColor,
                       emissionColor,
                       transparency,
                       refractiveIndex) {
}

