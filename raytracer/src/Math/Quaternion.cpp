//
// Created by chenx on 2020-06-03.
//
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include "Quaternion.h"

Quaternion::Quaternion(double s, Vec3 v): s(s), v(v){}

Quaternion Quaternion::getUnitNormQuaternion() const{

    //require s in radians
    v.normalize();
    double normS = cosf(s*0.5);
    Vec3 normV = v*sinf(s*0.5);

    return Quaternion(normS, normV);
}

Quaternion Quaternion::operator*(const Quaternion& rhs) const{

    double scalar = (s * rhs.s) - v.dot(rhs.v);

    Vec3 imaginary = (rhs.v * s) + (v * rhs.s) + v.cross(rhs.v);

    return Quaternion(scalar, imaginary);
}

//https://www.haroldserrano.com/blog/developing-a-math-engine-in-c-implementing-quaternions
Vec3 Quaternion::rotateVector(const Vec3& vector, const double& angleRadians, const Vec3& axis){

    //angleRadians from radian to degrees
    double angleDegrees = (angleRadians * 180 ) / M_PI;

    //convert our vector to a pure quaternion
    Quaternion p(0,(vector));

    axis.normalize();

    //create the real quaternion
    Quaternion q(angleDegrees, axis);

    //convert quaternion to unit norm quaternion
    Quaternion qNorm = q.getUnitNormQuaternion();

    //Get the inverse of the quaternion
    Quaternion qInverse = q.getInverse();

    //rotate the quaternion
    Quaternion rotatedVector=q*p*qInverse;

    //return the vector part of the quaternion
    return rotatedVector.V();

}

Quaternion Quaternion::getConjugate() const {

    Vec3 imaginary = v*(-1);

    return Quaternion(s,imaginary);
}

Quaternion Quaternion::getInverse() const{

    double absNorm = abs(this->getNorm());

    Quaternion conjugateValue = this->getConjugate();

    double scalar= conjugateValue.s * absNorm;
    Vec3 imaginary= conjugateValue.v * absNorm;

    return Quaternion(scalar,imaginary);
}

double Quaternion::getNorm() const{

    double scalar = s*s;
    double imaginary = v.X()*v.X() + v.Y()*v.Y() + v.Z()*v.Z();

    return sqrt(scalar+imaginary);
}