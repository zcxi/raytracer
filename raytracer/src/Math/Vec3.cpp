//
// Created by chenx on 2020-05-22.
//

#include <cmath>
#include <stdexcept>
#include "Vec3.h"

constexpr double Vec3::EPSILON;
constexpr double Vec3::EPISLON;

Vec3::Vec3() : x(0.0), y(0.0), z(0.0) {}

Vec3::Vec3(double xValue, double yValue, double zValue)
    : x(xValue), y(yValue), z(zValue) {}

Vec3::Vec3(const std::vector<double>& coordinates)
    : x(0.0), y(0.0), z(0.0) {
    if (coordinates.size() != 3) {
        throw std::invalid_argument("Vec3 requires exactly three coordinates.");
    }
    x = coordinates[0];
    y = coordinates[1];
    z = coordinates[2];
}

bool Vec3::operator== (const Vec3 &rhs) const {

    return x == rhs.x && y == rhs.y && z == rhs.z;
}

bool Vec3::operator != (const Vec3 &rhs) const{

    return !(*this == rhs);
}

Vec3 Vec3::operator + (const Vec3 &rhs) const{

    return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vec3 Vec3::operator - (const Vec3 &rhs) const{

    return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}
Vec3 Vec3::operator * (const double &scalar) const{

    return Vec3(scalar * x, scalar * y, scalar * z);
}
Vec3 Vec3::elementwiseMultiply (const Vec3 &rhs) const{

    return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
}
Vec3 Vec3::operator / (const double &scalar) const{
    if (std::abs(scalar) <= EPSILON) {
        throw std::invalid_argument("Cannot divide a vector by zero.");
    }
    return Vec3(x / scalar, y / scalar, z / scalar);
}
Vec3 Vec3::operator - () const {
    return Vec3(-X(), -Y(), -Z());
}


double Vec3::dot(const Vec3 &rhs) const{

    return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vec3 Vec3::cross(const Vec3 &rhs) const{

    return Vec3(
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x);
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

        return Vec3(x / length, y / length, z / length);
    }
    //TODO else throw exception on usage of invalid vec3
    return Vec3();
}

double Vec3::getLength() const {

    return std::sqrt(x * x + y * y + z * z);
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
