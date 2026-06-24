//
// Created by chenx on 2020-05-25.
//

#include "LightSource.h"
#include <stdexcept>

LightSource::LightSource(const Vec3& _position, const Vec3& _color,
                         double _intensity){
    if (_intensity < 0.0) {
        throw std::invalid_argument("Light intensity cannot be negative.");
    }
    this->position = _position;
    this->color = _color;
    this->intensity = _intensity;
}


const Vec3& LightSource::getPosition() const{

    return position;
}
double LightSource::getIntensity() const{

    return intensity;
}


const Vec3& LightSource::getColor() const{

    return color;
}
