#ifndef RAYTRACER_BSDF_H
#define RAYTRACER_BSDF_H

#include "Material.h"
#include "../Math/Sampler.h"

struct BsdfSample {
    Vec3 direction;
    Vec3 weight;
    double pdf;
    bool delta;
    bool valid;

    BsdfSample()
        : direction(), weight(), pdf(0.0), delta(false), valid(false) {}
};

class Bsdf {
public:
    static Vec3 evaluate(const Material& material,
                         const Vec3& normal,
                         const Vec3& outgoing,
                         const Vec3& incoming);
    static double pdf(const Material& material,
                      const Vec3& normal,
                      const Vec3& outgoing,
                      const Vec3& incoming);
    static BsdfSample sample(const Material& material,
                             const Vec3& normal,
                             const Vec3& outgoing,
                             bool frontFace,
                             Sampler& sampler);
    static Vec3 fresnelSchlick(
        double cosine, const Vec3& baseReflectance);
    static double ggxDistribution(
        double normalHalfCosine, double roughness);
    static double smithMasking(
        double normalViewCosine,
        double normalLightCosine,
        double roughness);
    static bool hasNonDeltaLobe(const Material& material);
    static Vec3 reflect(const Vec3& direction, const Vec3& normal);
    static Vec3 refract(const Vec3& direction, const Vec3& normal,
                        double refractionRatio);
    static bool hasTotalInternalReflection(
        const Vec3& direction, const Vec3& normal,
        double refractionRatio);
    static double schlickReflectance(double cosine,
                                     double refractionRatio);

private:
    static Vec3 cosineHemisphere(
        const Vec3& normal, Sampler& sampler);
    static Vec3 sampleGgxNormal(
        const Vec3& normal, double roughness, Sampler& sampler);
};

#endif

