//
// Created by chenx on 2020-05-24.
//

#include "Shape.h"
#include <stdexcept>

int Shape::getId() {
    return id;
}

void Shape::setId(int id) {
    this->id = id;
}

Shape::Shape(const Vec3& surfaceColor, const Vec3& emissionColor,
             double transparency, double refractiveIndex)
    : id(-1),
      transparency(transparency),
      refractiveIndex(refractiveIndex)
{
        if (transparency < 0.0 || transparency > 1.0) {
            throw std::invalid_argument("Transparency must be between zero and one.");
        }
        if (refractiveIndex <= 0.0) {
            throw std::invalid_argument("Refractive index must be positive.");
        }
        this->surfaceColor = Vec3(surfaceColor);
        this->emissionColor = Vec3(emissionColor);
}
