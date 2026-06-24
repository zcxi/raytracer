//
// Created by chenx on 2020-06-03.
//

#ifndef RAYTRACER_QUATERNION_H
#define RAYTRACER_QUATERNION_H


#include "Vec3.h"

//partial implementation of the quaternion class to execute rotations in 3d space
class Quaternion {
public:

    //Constructor
    Quaternion(double s, const Vec3& v);

    Quaternion operator*(const Quaternion& rhs) const;

    Quaternion getUnitNormQuaternion() const;
    Quaternion getConjugate() const;
    Quaternion getInverse() const;
    double getNorm() const;

    static Vec3 rotateVector(const Vec3& vector, const double& angleRadians, const Vec3& axis);

    double S() const {return s;}
    const Vec3& V() const {return v;}

private:
    //q = s + v

    //scalar
    double s;
    //vector
    Vec3 v;
};


#endif //RAYTRACER_QUATERNION_H
