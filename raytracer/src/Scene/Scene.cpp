//
// Created by chenx on 2020-05-25.
//

#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include "Scene.h"

constexpr double Scene::RAY_EPSILON;

Scene::Scene(const Vec3& background)
    : backgroundColor(background) {
}

bool Scene::findClosestHit(const Ray& ray, double minDistance,
                           double maxDistance, HitRecord& closestHit) const {
    bool foundHit = false;
    double closestDistance = maxDistance;
    HitRecord candidate;

    for (const auto& object : shapes) {
        if (object->intersect(ray, minDistance, closestDistance, candidate)) {
            foundHit = true;
            closestDistance = candidate.distance;
            closestHit = candidate;
        }
    }
    return foundHit;
}

bool Scene::isOccluded(const Ray& ray, double maxDistance) const {
    HitRecord ignored;
    for (const auto& object : shapes) {
        if (object->intersect(ray, RAY_EPSILON, maxDistance, ignored)) {
            if (object->getMaterial().type != MaterialType::Dielectric) {
                return true;
            }
        }
    }
    return false;
}

Vec3 Scene::trace(const Ray& ray) const {
    Sampler sampler(0, 0, 1);
    return trace(ray, sampler, PathTraceSettings(1, 1));
}

Vec3 Scene::directLighting(const HitRecord& hit) const {
    const double pi = 3.14159265358979323846;
    Vec3 result;
    for (const auto& lightSource : lightSources) {
        const Vec3 pointToLight = lightSource->getPosition() - hit.point;
        const double lightDistance = pointToLight.getLength();
        if (lightDistance <= RAY_EPSILON) {
            continue;
        }

        const Vec3 lightDirection = pointToLight / lightDistance;
        const double angleIntensity =
            std::max(0.0, hit.normal.dot(lightDirection));
        if (angleIntensity <= 0.0) {
            continue;
        }

        const Vec3 shadowOrigin = hit.point + hit.normal * RAY_EPSILON;
        const Ray shadowRay(shadowOrigin, lightDirection);
        if (isOccluded(shadowRay, lightDistance - RAY_EPSILON)) {
            continue;
        }

        const double distanceIntensity =
            lightSource->getIncidentBrightness(hit.point);
        const Vec3 incomingLight =
            lightSource->getColor() * (angleIntensity * distanceIntensity);
        result = result +
            hit.shape->getSurfaceColor().elementwiseMultiply(incomingLight) / pi;
    }
    return result;
}

Vec3 Scene::reflect(const Vec3& direction, const Vec3& normal) {
    return direction - normal * (2.0 * direction.dot(normal));
}

Vec3 Scene::refract(const Vec3& direction, const Vec3& normal,
                    double refractionRatio) {
    const double cosine =
        std::min((-direction).dot(normal), 1.0);
    const Vec3 perpendicular =
        (direction + normal * cosine) * refractionRatio;
    const double parallelLengthSquared =
        std::max(0.0, 1.0 - perpendicular.dot(perpendicular));
    const Vec3 parallel =
        normal * -std::sqrt(parallelLengthSquared);
    return (perpendicular + parallel).normalize();
}

bool Scene::hasTotalInternalReflection(
        const Vec3& direction, const Vec3& normal,
        double refractionRatio) {
    const double cosine =
        std::min((-direction).dot(normal), 1.0);
    const double sine =
        std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    return refractionRatio * sine > 1.0;
}

double Scene::schlickReflectance(double cosine,
                                 double refractionRatio) {
    double base = (1.0 - refractionRatio) /
                  (1.0 + refractionRatio);
    base *= base;
    const double oneMinusCosine = 1.0 - cosine;
    return base + (1.0 - base) *
        oneMinusCosine * oneMinusCosine * oneMinusCosine *
        oneMinusCosine * oneMinusCosine;
}

Vec3 Scene::cosineHemisphere(const Vec3& normal, Sampler& sampler) {
    const double pi = 3.14159265358979323846;
    const double first = sampler.next();
    const double second = sampler.next();
    const double radius = std::sqrt(first);
    const double angle = 2.0 * pi * second;
    const double localX = radius * std::cos(angle);
    const double localY = radius * std::sin(angle);
    const double localZ = std::sqrt(std::max(0.0, 1.0 - first));

    const Vec3 helper =
        std::abs(normal.X()) > 0.9
            ? Vec3(0.0, 1.0, 0.0)
            : Vec3(1.0, 0.0, 0.0);
    const Vec3 tangent = helper.cross(normal).normalize();
    const Vec3 bitangent = normal.cross(tangent);
    return (tangent * localX +
            bitangent * localY +
            normal * localZ).normalize();
}

Vec3 Scene::trace(const Ray& ray, Sampler& sampler,
                  const PathTraceSettings& settings) const {
    if (settings.maxBounces == 0) {
        return Vec3();
    }

    Vec3 radiance;
    Vec3 throughput(1.0, 1.0, 1.0);
    Ray currentRay = ray;

    for (unsigned int bounce = 0;
         bounce < settings.maxBounces; ++bounce) {
        HitRecord hit;
        if (!findClosestHit(
                currentRay, RAY_EPSILON,
                std::numeric_limits<double>::infinity(), hit)) {
            radiance = radiance +
                throughput.elementwiseMultiply(backgroundColor);
            break;
        }

        const Material& material = hit.shape->getMaterial();
        radiance = radiance +
            throughput.elementwiseMultiply(material.emission);

        Vec3 scatteredDirection;
        if (material.type == MaterialType::Diffuse) {
            radiance = radiance +
                throughput.elementwiseMultiply(directLighting(hit));
            scatteredDirection = cosineHemisphere(hit.normal, sampler);
        } else if (material.type == MaterialType::Mirror) {
            scatteredDirection =
                reflect(currentRay.direction(), hit.normal).normalize();
        } else {
            const double refractionRatio = hit.frontFace
                ? 1.0 / material.refractiveIndex
                : material.refractiveIndex;
            const double cosine = std::min(
                (-currentRay.direction()).dot(hit.normal), 1.0);
            const bool cannotRefract =
                hasTotalInternalReflection(
                    currentRay.direction(), hit.normal,
                    refractionRatio);
            if (cannotRefract ||
                sampler.next() <
                    schlickReflectance(cosine, refractionRatio)) {
                scatteredDirection =
                    reflect(currentRay.direction(), hit.normal).normalize();
            } else {
                scatteredDirection =
                    refract(currentRay.direction(), hit.normal,
                            refractionRatio);
            }
        }

        throughput =
            throughput.elementwiseMultiply(material.albedo);

        if (bounce + 1 >= settings.russianRouletteStart) {
            const double maximumComponent = std::max(
                throughput.X(),
                std::max(throughput.Y(), throughput.Z()));
            const double survivalProbability =
                std::max(0.05, std::min(0.95, maximumComponent));
            if (sampler.next() > survivalProbability) {
                break;
            }
            throughput = throughput / survivalProbability;
        }

        const double side =
            scatteredDirection.dot(hit.normal) >= 0.0 ? 1.0 : -1.0;
        currentRay = Ray(
            hit.point + hit.normal * (RAY_EPSILON * side),
            scatteredDirection);
    }
    return radiance;
}

void Scene::addShape(std::unique_ptr<Shape> shape) {
    if (shape) {
        shapes.push_back(std::move(shape));
    }
}

void Scene::addLight(std::unique_ptr<LightSource> source) {
    if (source) {
        lightSources.push_back(std::move(source));
    }
}
