#ifndef RAYTRACER_MATERIAL_H
#define RAYTRACER_MATERIAL_H

#include "../Math/Vec3.h"
#include "../Textures/Texture.h"
#include <memory>

enum class MaterialType {
    Diffuse,
    Mirror,
    Dielectric,
    Principled
};

struct Material {
    MaterialType type;
    Vec3 albedo;
    Vec3 emission;
    double refractiveIndex;
    double roughness;
    double metallic;
    double transmission;
    double clearcoat;
    double clearcoatRoughness;
    double normalStrength;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> roughnessTexture;
    std::shared_ptr<Texture> metallicTexture;
    std::shared_ptr<Texture> normalTexture;

    Material(MaterialType type = MaterialType::Diffuse,
             const Vec3& albedo = Vec3(0.8, 0.8, 0.8),
             const Vec3& emission = Vec3(),
             double refractiveIndex = 1.5,
             double roughness = 1.0,
             double metallic = 0.0,
             double transmission = 0.0,
             double clearcoat = 0.0,
             double clearcoatRoughness = 0.1,
             double normalStrength = 1.0,
             std::shared_ptr<Texture> texture =
                 std::shared_ptr<Texture>());

    static Material diffuse(const Vec3& albedo,
                            const Vec3& emission = Vec3());
    static Material mirror(const Vec3& tint = Vec3(1.0, 1.0, 1.0),
                           const Vec3& emission = Vec3());
    static Material dielectric(
        double refractiveIndex = 1.5,
        const Vec3& tint = Vec3(1.0, 1.0, 1.0),
        const Vec3& emission = Vec3());
    static Material principled(
        const Vec3& baseColor,
        double roughness = 0.5,
        double metallic = 0.0,
        double transmission = 0.0,
        double refractiveIndex = 1.5,
        const Vec3& emission = Vec3(),
        double clearcoat = 0.0,
        double clearcoatRoughness = 0.1);
    Material withTexture(std::shared_ptr<Texture> texture) const;
    Material withRoughnessTexture(std::shared_ptr<Texture> texture) const;
    Material withMetallicTexture(std::shared_ptr<Texture> texture) const;
    Material withNormalTexture(
        std::shared_ptr<Texture> texture, double strength = 1.0) const;
    Vec3 colorAt(double u, double v, const Vec3& point) const;
    double roughnessAt(double u, double v, const Vec3& point) const;
    double metallicAt(double u, double v, const Vec3& point) const;
    Vec3 normalAt(
        double u, double v, const Vec3& point,
        const Vec3& geometricNormal) const;
};

#endif
