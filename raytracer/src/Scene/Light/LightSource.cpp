//
// Created by chenx on 2020-05-25.
//

#include "LightSource.h"

LightSource::LightSource(Vec3 _position, Vec3 _color, double _intensity){

    this->position = _position;
    this->color = _color;
    this->intensity = _intensity;
}


Vec3 LightSource::getPosition() const{

    return position;
}
double LightSource::getIntensity() const{

    return intensity;
}


Vec3 LightSource::getColor() const{

    return color;
};