//
// Created by chenx on 2020-06-03.
//

#ifndef RAYTRACER_QUATERNION_H
#define RAYTRACER_QUATERNION_H


#include <cmath>
#include <stdexcept>
#include "Vec3.h"

//partial implementation of the quaternion class to execute rotations in 3d space
class Quaternion {
public:

    //Constructor
    constexpr Quaternion(double s, const Vec3& v) noexcept : s(s), v(v) {}

    [[nodiscard]] constexpr Quaternion operator*(const Quaternion& rhs) const noexcept {
        double scalar = (s * rhs.s) - v.dot(rhs.v);
        Vec3 imaginary = (rhs.v * s) + (v * rhs.s) + v.cross(rhs.v);
        return Quaternion(scalar, imaginary);
    }

    [[nodiscard]] constexpr Quaternion getUnitNormQuaternion() const {
        const double norm = getNorm();
        if (norm <= Vec3::EPSILON) {
            throw std::invalid_argument("Cannot normalize a zero quaternion.");
        }
        return Quaternion(s / norm, v / norm);
    }

    [[nodiscard]] constexpr Quaternion getConjugate() const noexcept {
        Vec3 imaginary = v * (-1);
        return Quaternion(s, imaginary);
    }

    [[nodiscard]] constexpr Quaternion getInverse() const {
        const double norm = getNorm();
        const double normSquared = norm * norm;
        if (normSquared <= Vec3::EPSILON) {
            throw std::invalid_argument("Cannot invert a zero quaternion.");
        }
        const Quaternion conjugateValue = getConjugate();
        return Quaternion(conjugateValue.s / normSquared,
                          conjugateValue.v / normSquared);
    }

    [[nodiscard]] constexpr double getNorm() const noexcept {
        double scalar = s * s;
        double imaginary = v.X() * v.X() + v.Y() * v.Y() + v.Z() * v.Z();
        return std::sqrt(scalar + imaginary);
    }

    [[nodiscard]] static constexpr Vec3 rotateVector(const Vec3& vector, double angleRadians, const Vec3& axis) {
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

    [[nodiscard]] constexpr double S() const noexcept { return s; }
    [[nodiscard]] constexpr const Vec3& V() const noexcept { return v; }

private:
    //q = s + v

    //scalar
    double s;
    //vector
    Vec3 v;
};


#endif //RAYTRACER_QUATERNION_H
