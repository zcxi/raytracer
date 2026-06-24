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
                   const Vec3& emission, double refractiveIndex)
    : type(type),
      albedo(albedo),
      emission(emission),
      refractiveIndex(refractiveIndex) {
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
}

Material Material::diffuse(const Vec3& albedo, const Vec3& emission) {
    return Material(MaterialType::Diffuse, albedo, emission, 1.0);
}

Material Material::mirror(const Vec3& tint, const Vec3& emission) {
    return Material(MaterialType::Mirror, tint, emission, 1.0);
}

Material Material::dielectric(double refractiveIndex, const Vec3& tint,
                              const Vec3& emission) {
    return Material(
        MaterialType::Dielectric, tint, emission, refractiveIndex);
}
