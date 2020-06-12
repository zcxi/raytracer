//
// Created by chenx on 2020-05-24.
//

#include "Shape.h"

int Shape::getId() {
    return id;
}

void Shape::setId(int id) {
    this->id = id;
}

Shape::Shape(Vec3 surfaceColor, Vec3 emissionColor, double transparency, double refractiveIndex)
    :transparency(transparency), refractiveIndex(refractiveIndex)
{
        this->surfaceColor = Vec3(surfaceColor);
        this->emissionColor = Vec3(emissionColor);
}
