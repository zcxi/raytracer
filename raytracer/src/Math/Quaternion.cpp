//
// Created by chenx on 2020-06-03.
//
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <stdexcept>
#include "Quaternion.h"

Quaternion::Quaternion(double s, const Vec3& v): s(s), v(v){}

Quaternion Quaternion::getUnitNormQuaternion() const{
    const double norm = getNorm();
    if (norm <= Vec3::EPSILON) {
        throw std::invalid_argument("Cannot normalize a zero quaternion.");
    }
    return Quaternion(s / norm, v / norm);
}

Quaternion Quaternion::operator*(const Quaternion& rhs) const{

    double scalar = (s * rhs.s) - v.dot(rhs.v);

    Vec3 imaginary = (rhs.v * s) + (v * rhs.s) + v.cross(rhs.v);

    return Quaternion(scalar, imaginary);
}

//https://www.haroldserrano.com/blog/developing-a-math-engine-in-c-implementing-quaternions
Vec3 Quaternion::rotateVector(const Vec3& vector, const double& angleRadians, const Vec3& axis){
    const Vec3 normalizedAxis = axis.normalize();
    if (normalizedAxis.getLength() <= Vec3::EPSILON) {
        return vector;
    }

    const double halfAngle = angleRadians * 0.5;
    const Quaternion q(std::cos(halfAngle), normalizedAxis * std::sin(halfAngle));
    const Quaternion p(0.0, vector);
    const Quaternion rotatedVector = q * p * q.getInverse();
    return rotatedVector.V();

}

Quaternion Quaternion::getConjugate() const {

    Vec3 imaginary = v*(-1);

    return Quaternion(s,imaginary);
}

Quaternion Quaternion::getInverse() const{
    const double norm = getNorm();
    const double normSquared = norm * norm;
    if (normSquared <= Vec3::EPSILON) {
        throw std::invalid_argument("Cannot invert a zero quaternion.");
    }

    const Quaternion conjugateValue = getConjugate();
    return Quaternion(conjugateValue.s / normSquared,
                      conjugateValue.v / normSquared);
}

double Quaternion::getNorm() const{

    double scalar = s*s;
    double imaginary = v.X()*v.X() + v.Y()*v.Y() + v.Z()*v.Z();

    return sqrt(scalar+imaginary);
}
