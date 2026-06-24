//
// Created by chenx on 2020-05-22.
//

#ifndef RAYTRACER_VEC3_H
#define RAYTRACER_VEC3_H

#include <vector>

class Vec3
{

public:

    constexpr static const double EPSILON = 1e-6;
    constexpr static const double EPISLON = EPSILON;

    Vec3();
    Vec3(double x, double y, double z);
    explicit Vec3(const std::vector<double>& coordinates);

    bool operator == (const Vec3 &rhs) const;
    bool operator != (const Vec3 &rhs) const;
    Vec3 operator + (const Vec3 &rhs) const;
    Vec3 operator - (const Vec3 &rhs) const;
    Vec3 operator * (const double &scalar) const;
    Vec3 elementwiseMultiply (const Vec3 &rhs) const;
    Vec3 operator / (const double &scalar) const;
    Vec3 operator - () const;

    double dot(const Vec3 &rhs) const;
    Vec3 cross(const Vec3 &rhs) const;
    Vec3 projectOnto(const Vec3 &rhs) const;
    Vec3 normalize() const;
    double getLength() const;

    double distanceTo(const Vec3& to) const;
    bool near(const Vec3& rhs, double epsilon = EPSILON) const;

    double X() const {return this->coord[0];}
    double Y() const {return this->coord[1];}
    double Z() const {return this->coord[2];}
    const std::vector<double>& getCoords() const {return this->coord;}

private:
    std::vector<double> coord;


};


#endif //RAYTRACER_VEC3_H
