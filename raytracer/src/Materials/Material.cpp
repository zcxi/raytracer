#include "Material.h"

#include <algorithm>
#include <cmath>
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
                   double transmission, double clearcoat,
                   double clearcoatRoughness, double normalStrength,
                   std::shared_ptr<Texture> texture)
    : type(type),
      albedo(albedo),
      emission(emission),
      refractiveIndex(refractiveIndex),
      roughness(roughness),
      metallic(metallic),
      transmission(transmission),
      clearcoat(clearcoat),
      clearcoatRoughness(clearcoatRoughness),
      normalStrength(normalStrength),
      texture(texture) {
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
        transmission < 0.0 || transmission > 1.0 ||
        clearcoat < 0.0 || clearcoat > 1.0 ||
        clearcoatRoughness < 0.0 || clearcoatRoughness > 1.0 ||
        normalStrength < 0.0) {
        throw std::invalid_argument(
            "Material scalar parameters are outside their valid range.");
    }
}

Material Material::withTexture(
        std::shared_ptr<Texture> newTexture) const {
    Material copy = *this;
    copy.texture = newTexture;
    return copy;
}

Material Material::withRoughnessTexture(
        std::shared_ptr<Texture> newTexture) const {
    Material copy = *this;
    copy.roughnessTexture = newTexture;
    return copy;
}

Material Material::withMetallicTexture(
        std::shared_ptr<Texture> newTexture) const {
    Material copy = *this;
    copy.metallicTexture = newTexture;
    return copy;
}

Material Material::withNormalTexture(
        std::shared_ptr<Texture> newTexture, double strength) const {
    if (strength < 0.0) {
        throw std::invalid_argument("Normal-map strength cannot be negative.");
    }
    Material copy = *this;
    copy.normalTexture = newTexture;
    copy.normalStrength = strength;
    return copy;
}

Vec3 Material::colorAt(
        double u, double v, const Vec3& point) const {
    return texture ? texture->value(u, v, point) : albedo;
}

double Material::roughnessAt(
        double u, double v, const Vec3& point) const {
    if (!roughnessTexture) return roughness;
    const Vec3 sample = roughnessTexture->value(u, v, point);
    return std::clamp(roughness * sample.X(), 0.0, 1.0);
}

double Material::metallicAt(
        double u, double v, const Vec3& point) const {
    if (!metallicTexture) return metallic;
    const Vec3 sample = metallicTexture->value(u, v, point);
    return std::clamp(metallic * sample.X(), 0.0, 1.0);
}

Vec3 Material::normalAt(
        double u, double v, const Vec3& point,
        const Vec3& geometricNormal) const {
    if (!normalTexture || normalStrength <= 0.0) {
        return geometricNormal;
    }
    const Vec3 encoded = normalTexture->value(u, v, point);
    Vec3 tangentNormal(
        (encoded.X() * 2.0 - 1.0) * normalStrength,
        (encoded.Y() * 2.0 - 1.0) * normalStrength,
        encoded.Z() * 2.0 - 1.0);
    tangentNormal = tangentNormal.normalize();
    const Vec3 helper = std::abs(geometricNormal.X()) > 0.9
        ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
    const Vec3 tangent = helper.cross(geometricNormal).normalize();
    const Vec3 bitangent = geometricNormal.cross(tangent);
    Vec3 mapped = (
        tangent * tangentNormal.X() +
        bitangent * tangentNormal.Y() +
        geometricNormal * tangentNormal.Z()).normalize();
    if (mapped.dot(geometricNormal) < 0.0) mapped = -mapped;
    return mapped;
}

Material Material::diffuse(const Vec3& albedo, const Vec3& emission) {
    return Material(
        MaterialType::Diffuse, albedo, emission, 1.0,
        1.0, 0.0, 0.0, 0.0, 0.1, 1.0);
}

Material Material::mirror(const Vec3& tint, const Vec3& emission) {
    return Material(
        MaterialType::Mirror, tint, emission, 1.0,
        0.0, 1.0, 0.0, 0.0, 0.1, 1.0);
}

Material Material::dielectric(double refractiveIndex, const Vec3& tint,
                              const Vec3& emission) {
    return Material(
        MaterialType::Dielectric, tint, emission, refractiveIndex,
        0.0, 0.0, 1.0, 0.0, 0.1, 1.0);
}

Material Material::principled(
        const Vec3& baseColor, double roughness,
        double metallic, double transmission,
        double refractiveIndex, const Vec3& emission,
        double clearcoat, double clearcoatRoughness) {
    return Material(
        MaterialType::Principled, baseColor, emission,
        refractiveIndex, roughness, metallic, transmission,
        clearcoat, clearcoatRoughness, 1.0);
}
