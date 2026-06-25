#include "Bsdf.h"

#include <algorithm>
#include <cmath>

namespace {

const double PI = 3.14159265358979323846;

Vec3 mix(const Vec3& first, const Vec3& second, double amount) {
    return first * (1.0 - amount) + second * amount;
}

double specularProbability(const Material& material) {
    return std::max(0.1, std::min(0.9, 0.25 + 0.65 * material.metallic));
}

Vec3 tangentFor(const Vec3& normal) {
    const Vec3 helper = std::abs(normal.X()) > 0.9
        ? Vec3(0.0, 1.0, 0.0)
        : Vec3(1.0, 0.0, 0.0);
    return helper.cross(normal).normalize();
}

} // namespace

Vec3 Bsdf::fresnelSchlick(
        double cosine, const Vec3& baseReflectance) {
    const double factor = std::pow(
        std::max(0.0, 1.0 - cosine), 5.0);
    return baseReflectance +
        (Vec3(1.0, 1.0, 1.0) - baseReflectance) * factor;
}

double Bsdf::ggxDistribution(
        double normalHalfCosine, double roughness) {
    const double alpha = std::max(0.001, roughness * roughness);
    const double alphaSquared = alpha * alpha;
    const double cosineSquared =
        normalHalfCosine * normalHalfCosine;
    const double denominator =
        cosineSquared * (alphaSquared - 1.0) + 1.0;
    return alphaSquared /
        (PI * denominator * denominator);
}

double Bsdf::smithMasking(
        double normalViewCosine,
        double normalLightCosine,
        double roughness) {
    const double alpha = roughness * roughness;
    const double k = (alpha + 1.0) * (alpha + 1.0) / 8.0;
    const double view = normalViewCosine /
        (normalViewCosine * (1.0 - k) + k);
    const double light = normalLightCosine /
        (normalLightCosine * (1.0 - k) + k);
    return view * light;
}

Vec3 Bsdf::evaluate(const Material& material,
                    const Vec3& normal,
                    const Vec3& outgoing,
                    const Vec3& incoming) {
    const double normalView =
        std::max(0.0, normal.dot(outgoing));
    const double normalLight =
        std::max(0.0, normal.dot(incoming));
    if (normalView <= 0.0 || normalLight <= 0.0 ||
        material.type == MaterialType::Mirror ||
        material.type == MaterialType::Dielectric) {
        return Vec3();
    }

    const Vec3 halfVector = (outgoing + incoming).normalize();
    const double normalHalf =
        std::max(0.0, normal.dot(halfVector));
    const double viewHalf =
        std::max(0.0, outgoing.dot(halfVector));
    const Vec3 dielectricF0(0.04, 0.04, 0.04);
    const Vec3 baseReflectance =
        mix(dielectricF0, material.albedo, material.metallic);
    const Vec3 fresnel =
        fresnelSchlick(viewHalf, baseReflectance);
    const double distribution =
        ggxDistribution(normalHalf, material.roughness);
    const double masking = smithMasking(
        normalView, normalLight, material.roughness);
    const Vec3 specular = fresnel *
        (distribution * masking /
         std::max(Vec3::EPSILON, 4.0 * normalView * normalLight));

    const Vec3 diffuseWeight =
        (Vec3(1.0, 1.0, 1.0) - fresnel) *
        ((1.0 - material.metallic) *
         (1.0 - material.transmission));
    const Vec3 diffuse =
        diffuseWeight.elementwiseMultiply(material.albedo) / PI;
    return diffuse + specular;
}

double Bsdf::pdf(const Material& material,
                 const Vec3& normal,
                 const Vec3& outgoing,
                 const Vec3& incoming) {
    const double normalLight =
        std::max(0.0, normal.dot(incoming));
    if (normalLight <= 0.0 ||
        material.type == MaterialType::Mirror ||
        material.type == MaterialType::Dielectric) {
        return 0.0;
    }
    const Vec3 halfVector = (outgoing + incoming).normalize();
    const double normalHalf =
        std::max(0.0, normal.dot(halfVector));
    const double viewHalf =
        std::max(Vec3::EPSILON, outgoing.dot(halfVector));
    const double specularPdf =
        ggxDistribution(normalHalf, material.roughness) *
        normalHalf / (4.0 * viewHalf);
    const double diffusePdf = normalLight / PI;
    const double specularChance = specularProbability(material);
    return (1.0 - material.transmission) *
        (specularChance * specularPdf +
         (1.0 - specularChance) * diffusePdf);
}

Vec3 Bsdf::cosineHemisphere(
        const Vec3& normal, Sampler& sampler) {
    const double first = sampler.next();
    const double second = sampler.next();
    const double radius = std::sqrt(first);
    const double angle = 2.0 * PI * second;
    const Vec3 tangent = tangentFor(normal);
    const Vec3 bitangent = normal.cross(tangent);
    return (tangent * (radius * std::cos(angle)) +
            bitangent * (radius * std::sin(angle)) +
            normal * std::sqrt(std::max(0.0, 1.0 - first))).normalize();
}

Vec3 Bsdf::sampleGgxNormal(
        const Vec3& normal, double roughness, Sampler& sampler) {
    const double alpha = std::max(0.001, roughness * roughness);
    const double first = sampler.next();
    const double second = sampler.next();
    const double phi = 2.0 * PI * first;
    const double cosine = std::sqrt(
        (1.0 - second) /
        (1.0 + (alpha * alpha - 1.0) * second));
    const double sine =
        std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    const Vec3 tangent = tangentFor(normal);
    const Vec3 bitangent = normal.cross(tangent);
    return (tangent * (sine * std::cos(phi)) +
            bitangent * (sine * std::sin(phi)) +
            normal * cosine).normalize();
}

Vec3 Bsdf::reflect(const Vec3& direction, const Vec3& normal) {
    return direction - normal * (2.0 * direction.dot(normal));
}

Vec3 Bsdf::refract(const Vec3& direction, const Vec3& normal,
                   double refractionRatio) {
    const double cosine =
        std::min((-direction).dot(normal), 1.0);
    const Vec3 perpendicular =
        (direction + normal * cosine) * refractionRatio;
    const Vec3 parallel = normal * -std::sqrt(std::max(
        0.0, 1.0 - perpendicular.dot(perpendicular)));
    return (perpendicular + parallel).normalize();
}

bool Bsdf::hasTotalInternalReflection(
        const Vec3& direction, const Vec3& normal,
        double refractionRatio) {
    const double cosine =
        std::min((-direction).dot(normal), 1.0);
    const double sine =
        std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    return refractionRatio * sine > 1.0;
}

double Bsdf::schlickReflectance(double cosine,
                                double refractionRatio) {
    double base = (1.0 - refractionRatio) /
                  (1.0 + refractionRatio);
    base *= base;
    const double oneMinusCosine = 1.0 - cosine;
    return base + (1.0 - base) *
        oneMinusCosine * oneMinusCosine * oneMinusCosine *
        oneMinusCosine * oneMinusCosine;
}

BsdfSample Bsdf::sample(
        const Material& material, const Vec3& normal,
        const Vec3& outgoing, bool frontFace, Sampler& sampler) {
    BsdfSample result;
    const Vec3 incident = -outgoing;

    if (material.type == MaterialType::Mirror) {
        result.direction = reflect(incident, normal).normalize();
        result.weight = material.albedo;
        result.delta = true;
        result.valid = true;
        return result;
    }

    const bool isDielectric =
        material.type == MaterialType::Dielectric;
    const bool sampleTransmission =
        isDielectric || sampler.next() < material.transmission;
    if (sampleTransmission) {
        const double ratio = frontFace
            ? 1.0 / material.refractiveIndex
            : material.refractiveIndex;
        const double cosine =
            std::min(incident.dot(-normal), 1.0);
        const double sine =
            std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
        const double r0 = std::pow(
            (1.0 - ratio) / (1.0 + ratio), 2.0);
        const double reflectance =
            r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
        if (ratio * sine > 1.0 || sampler.next() < reflectance) {
            result.direction = reflect(incident, normal).normalize();
        } else {
            result.direction = refract(incident, normal, ratio);
        }
        result.weight = material.albedo;
        result.delta = true;
        result.valid = true;
        return result;
    }

    const double specularChance = specularProbability(material);
    if (sampler.next() < specularChance) {
        const Vec3 halfVector =
            sampleGgxNormal(normal, material.roughness, sampler);
        result.direction = reflect(incident, halfVector).normalize();
    } else {
        result.direction = cosineHemisphere(normal, sampler);
    }
    if (normal.dot(result.direction) <= 0.0) {
        return result;
    }
    result.pdf = pdf(
        material, normal, outgoing, result.direction);
    if (result.pdf <= Vec3::EPSILON) {
        return result;
    }
    const Vec3 value = evaluate(
        material, normal, outgoing, result.direction);
    result.weight = value *
        (std::max(0.0, normal.dot(result.direction)) / result.pdf);
    result.delta = false;
    result.valid = true;
    return result;
}

bool Bsdf::hasNonDeltaLobe(const Material& material) {
    return (material.type == MaterialType::Diffuse ||
            material.type == MaterialType::Principled) &&
        material.transmission < 1.0;
}
