//
// Created by chenx on 2020-05-22.
//

#include <cmath>
#include <assert.h>
#include "Vec3.h"


Vec3::Vec3() {
    coord = std::vector<double> (3, 0);
}

Vec3::Vec3(double x, double y , double z) {
    coord = {x, y, z};
}

Vec3::Vec3(std::vector<double> coordinates){
    assert(coordinates.size() == 3);
    coord = coordinates;
}

bool Vec3::operator== (const Vec3 &rhs) const {

    return coord == rhs.coord;
}

bool Vec3::operator != (const Vec3 &rhs) const{

    return coord != rhs.coord;
}

Vec3 Vec3::operator + (const Vec3 &rhs) const{

    std::vector<double> sum = {coord[0] + rhs.coord[0], coord[1] + rhs.coord[1], coord[2] + rhs.coord[2]};
    return Vec3(sum);
}

Vec3 Vec3::operator - (const Vec3 &rhs) const{

    std::vector<double> sum = {coord[0] - rhs.coord[0], coord[1] - rhs.coord[1], coord[2] - rhs.coord[2]};
    return Vec3(sum);
}
Vec3 Vec3::operator * (const double &scalar) const{

    return Vec3(scalar* coord[0], scalar* coord[1], scalar* coord[2]);
}
Vec3 Vec3::elementwiseMultiply (const Vec3 &rhs) const{

    return Vec3(coord[0]* rhs.X(), coord[1]* rhs.Y(), coord[2]* rhs.Z());
}
Vec3 Vec3::operator / (const double &scalar) const{

    return Vec3(scalar/ coord[0], scalar/ coord[1], scalar/ coord[2]);
}
Vec3 Vec3::operator - () const {
    return Vec3(-X(), -Y(), -Z());
}


double Vec3::dot(const Vec3 &rhs) const{

    return  coord[0] * rhs.coord[0] + coord[1] * rhs.coord[1] + coord[2] * rhs.coord[2];
}

Vec3 Vec3::cross(const Vec3 &rhs) const{

    return Vec3(coord[1] * rhs.coord[2] - coord[2] * rhs.coord[1], coord[2] * rhs.coord[0] - coord[0] * rhs.coord[2], coord[0] * rhs.coord[1] - coord[1] * rhs.coord[0]);
}

Vec3 Vec3::projectOnto(const Vec3 &rhs) const{

    return rhs * this->dot(rhs.coord) / rhs.dot(rhs.coord);
}

Vec3 Vec3::normalize() const{

    double length = getLength();
    if (length > 0) {

        return Vec3(coord[0]/ length, coord[1]/ length, coord[2]/ length);
    }
    //TODO else throw exception on usage of invalid vec3
    return Vec3();
}

double Vec3::getLength() const {

    return sqrt(coord[0] * coord[0] + coord[1] * coord[1]+ coord[2] * coord[2]);
}

double Vec3::distanceTo(const Vec3 to){

    Vec3 distance = *this - to;
    return distance.getLength();
}