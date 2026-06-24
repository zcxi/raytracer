#include "Material.h"

#include <stdexcept>

namespace {

void validateNonNegativeColor(const Vec3& color, const char* name) {
    if (color.X() < 0.0 || color.Y() < 0.0 || color.Z() < 0.0) {
        throw std::invalid_argument(
            std::string(name) + " cannot contain negative values.");
    }
}

} // namespace

Material::Material(MaterialType type, const Vec3& albedo,
                   const Vec3& emission, double refractiveIndex,
                   double roughness, double metallic,
                   double transmission)
    : type(type),
      albedo(albedo),
      emission(emission),
      refractiveIndex(refractiveIndex),
      roughness(roughness),
      metallic(metallic),
      transmission(transmission) {
    validateNonNegativeColor(albedo, "Material albedo");
    validateNonNegativeColor(emission, "Material emission");
    if (albedo.X() > 1.0 || albedo.Y() > 1.0 || albedo.Z() > 1.0) {
        throw std::invalid_argument(
            "Material albedo components cannot exceed one.");
    }
    if (refractiveIndex <= 0.0) {
        throw std::invalid_argument(
            "Material refractive index must be positive.");
    }
    if (roughness < 0.0 || roughness > 1.0 ||
        metallic < 0.0 || metallic > 1.0 ||
        transmission < 0.0 || transmission > 1.0) {
        throw std::invalid_argument(
            "Roughness, metallic, and transmission must be between zero and one.");
    }
}

Material Material::diffuse(const Vec3& albedo, const Vec3& emission) {
    return Material(
        MaterialType::Diffuse, albedo, emission, 1.0,
        1.0, 0.0, 0.0);
}

Material Material::mirror(const Vec3& tint, const Vec3& emission) {
    return Material(
        MaterialType::Mirror, tint, emission, 1.0,
        0.0, 1.0, 0.0);
}

Material Material::dielectric(double refractiveIndex, const Vec3& tint,
                              const Vec3& emission) {
    return Material(
        MaterialType::Dielectric, tint, emission, refractiveIndex,
        0.0, 0.0, 1.0);
}

Material Material::principled(
        const Vec3& baseColor, double roughness,
        double metallic, double transmission,
        double refractiveIndex, const Vec3& emission) {
    return Material(
        MaterialType::Principled, baseColor, emission,
        refractiveIndex, roughness, metallic, transmission);
}
