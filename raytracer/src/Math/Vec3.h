//
// Created by chenx on 2020-05-22.
//

#ifndef RAYTRACER_VEC3_H
#define RAYTRACER_VEC3_H

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

class Vec3
{

public:

    constexpr static const double EPSILON = 1e-6;
    constexpr static const double EPISLON = EPSILON;

    constexpr Vec3() noexcept : x(0.0), y(0.0), z(0.0) {}
    constexpr Vec3(double x, double y, double z) noexcept : x(x), y(y), z(z) {}
    explicit Vec3(const std::vector<double>& coordinates);

    [[nodiscard]] constexpr bool operator == (const Vec3 &rhs) const noexcept {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    [[nodiscard]] constexpr bool operator != (const Vec3 &rhs) const noexcept {
        return !(*this == rhs);
    }
    [[nodiscard]] constexpr Vec3 operator + (const Vec3 &rhs) const noexcept {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    [[nodiscard]] constexpr Vec3 operator - (const Vec3 &rhs) const noexcept {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }
    [[nodiscard]] constexpr Vec3 operator * (const double &scalar) const noexcept {
        return Vec3(scalar * x, scalar * y, scalar * z);
    }
    [[nodiscard]] constexpr Vec3 elementwiseMultiply (const Vec3 &rhs) const noexcept {
        return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
    }
    [[nodiscard]] constexpr Vec3 operator / (const double &scalar) const {
        if (std::abs(scalar) <= EPSILON) {
            throw std::invalid_argument("Cannot divide a vector by zero.");
        }
        return Vec3(x / scalar, y / scalar, z / scalar);
    }
    [[nodiscard]] constexpr Vec3 operator - () const noexcept {
        return Vec3(-x, -y, -z);
    }

    [[nodiscard]] constexpr double dot(const Vec3 &rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    [[nodiscard]] constexpr Vec3 cross(const Vec3 &rhs) const noexcept {
        return Vec3(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x);
    }
    [[nodiscard]] constexpr Vec3 projectOnto(const Vec3 &rhs) const noexcept {
        const double denominator = rhs.dot(rhs);
        if (denominator <= EPSILON) {
            return Vec3();
        }
        return rhs * (this->dot(rhs) / denominator);
    }
    [[nodiscard]] constexpr Vec3 normalize() const noexcept {
        double length = getLength();
        if (length > 0) {
            return Vec3(x / length, y / length, z / length);
        }
        return Vec3();
    }
    [[nodiscard]] constexpr double getLength() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr double distanceTo(const Vec3& to) const noexcept {
        Vec3 distance = *this - to;
        return distance.getLength();
    }
    [[nodiscard]] constexpr bool near(const Vec3& rhs, double epsilon = EPSILON) const noexcept {
        return std::abs(x - rhs.x) <= epsilon &&
               std::abs(y - rhs.y) <= epsilon &&
               std::abs(z - rhs.z) <= epsilon;
    }

    [[nodiscard]] constexpr double X() const noexcept { return x; }
    [[nodiscard]] constexpr double Y() const noexcept { return y; }
    [[nodiscard]] constexpr double Z() const noexcept { return z; }
    [[nodiscard]] constexpr std::array<double, 3> getCoords() const noexcept {
        return {{x, y, z}};
    }

private:
    double x;
    double y;
    double z;


};


#endif //RAYTRACER_VEC3_H
