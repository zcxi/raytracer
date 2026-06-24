#ifndef RAYTRACER_MATERIAL_H
#define RAYTRACER_MATERIAL_H

#include "../Math/Vec3.h"

enum class MaterialType {
    Diffuse,
    Mirror,
    Dielectric
};

struct Material {
    MaterialType type;
    Vec3 albedo;
    Vec3 emission;
    double refractiveIndex;

    Material(MaterialType type = MaterialType::Diffuse,
             const Vec3& albedo = Vec3(0.8, 0.8, 0.8),
             const Vec3& emission = Vec3(),
             double refractiveIndex = 1.5);

    static Material diffuse(const Vec3& albedo,
                            const Vec3& emission = Vec3());
    static Material mirror(const Vec3& tint = Vec3(1.0, 1.0, 1.0),
                           const Vec3& emission = Vec3());
    static Material dielectric(
        double refractiveIndex = 1.5,
        const Vec3& tint = Vec3(1.0, 1.0, 1.0),
        const Vec3& emission = Vec3());
};

#endif

