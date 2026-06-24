//
// Created by chenx on 2020-05-22.
//

#include <cmath>
#include <stdexcept>
#include "Vec3.h"


Vec3::Vec3() : coord(3, 0.0) {}

Vec3::Vec3(double x, double y , double z) : coord{x, y, z} {}

Vec3::Vec3(const std::vector<double>& coordinates) : coord(coordinates) {
    if (coordinates.size() != 3) {
        throw std::invalid_argument("Vec3 requires exactly three coordinates.");
    }
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
    if (std::abs(scalar) <= EPSILON) {
        throw std::invalid_argument("Cannot divide a vector by zero.");
    }
    return Vec3(coord[0] / scalar, coord[1] / scalar, coord[2] / scalar);
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
    const double denominator = rhs.dot(rhs);
    if (denominator <= EPSILON) {
        return Vec3();
    }
    return rhs * (this->dot(rhs) / denominator);
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

double Vec3::distanceTo(const Vec3& to) const{

    Vec3 distance = *this - to;
    return distance.getLength();
}

bool Vec3::near(const Vec3& rhs, double epsilon) const {
    return std::abs(X() - rhs.X()) <= epsilon &&
           std::abs(Y() - rhs.Y()) <= epsilon &&
           std::abs(Z() - rhs.Z()) <= epsilon;
}
