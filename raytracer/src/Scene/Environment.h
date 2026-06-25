#ifndef RAYTRACER_ENVIRONMENT_H
#define RAYTRACER_ENVIRONMENT_H

#include "../Math/Sampler.h"
#include "../Math/Vec3.h"
#include <string>
#include <vector>

struct EnvironmentSample {
    Vec3 direction;
    Vec3 radiance;
    double pdf;

    EnvironmentSample() : direction(), radiance(), pdf(0.0) {}
};

class Environment {
public:
    Environment(const Vec3& horizon = Vec3(),
                const Vec3& zenith = Vec3(),
                double intensity = 1.0);

    Vec3 evaluate(const Vec3& direction) const;
    EnvironmentSample sample(Sampler& sampler) const;
    double pdf(const Vec3& direction) const;
    bool isBlack() const;
    bool loadPpm(const std::string& path, double mapIntensity = 1.0);
    bool loadHdr(const std::string& path, double mapIntensity = 1.0);
    void setIntensity(double value);

private:
    void buildImportanceDistribution();
    double texelWeight(unsigned int x, unsigned int y) const;
    Vec3 horizon;
    Vec3 zenith;
    double intensity;
    std::vector<Vec3> pixels;
    unsigned int width;
    unsigned int height;
    std::vector<double> importanceCdf;
    double importanceTotal;
};

#endif

