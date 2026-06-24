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
    : environment(background, background),
      boundedShapes(),
      unboundedShapes(),
      emissiveShapes(),
      bvh(),
      accelerationEnabled(true) {
}

bool Scene::findClosestHit(const Ray& ray, double minDistance,
                           double maxDistance, HitRecord& closestHit) const {
    bool foundHit = false;
    if (accelerationEnabled) {
        foundHit =
            bvh.intersect(ray, minDistance, maxDistance, closestHit);
    } else {
        HitRecord candidate;
        double closest = maxDistance;
        for (const Shape* shape : boundedShapes) {
            if (shape->intersect(
                    ray, minDistance, closest, candidate)) {
                closest = candidate.distance;
                closestHit = candidate;
                foundHit = true;
            }
        }
    }
    double closestDistance =
        foundHit ? closestHit.distance : maxDistance;
    HitRecord candidate;

    for (const Shape* object : unboundedShapes) {
        if (object->intersect(ray, minDistance, closestDistance, candidate)) {
            foundHit = true;
            closestDistance = candidate.distance;
            closestHit = candidate;
        }
    }
    return foundHit;
}

bool Scene::isOccluded(const Ray& ray, double maxDistance) const {
    if (accelerationEnabled) {
        if (bvh.occluded(ray, RAY_EPSILON, maxDistance)) {
            return true;
        }
    } else {
        HitRecord hit;
        for (const Shape* shape : boundedShapes) {
            if (shape->getMaterial().transmission <= 0.0 &&
                shape->intersect(
                    ray, RAY_EPSILON, maxDistance, hit)) {
                return true;
            }
        }
    }
    HitRecord ignored;
    for (const Shape* object : unboundedShapes) {
        if (object->intersect(ray, RAY_EPSILON, maxDistance, ignored)) {
            if (object->getMaterial().transmission <= 0.0) {
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

Vec3 Scene::pointLighting(
        const HitRecord& hit, const Vec3& outgoing) const {
    Vec3 result;
    for (const auto& lightSource : lightSources) {
        const Vec3 pointToLight = lightSource->getPosition() - hit.point;
        const double lightDistance = pointToLight.getLength();
        if (lightDistance <= RAY_EPSILON) {
            continue;
        }

        const Vec3 lightDirection = pointToLight / lightDistance;
        const double surfaceCosine =
            std::max(0.0, hit.normal.dot(lightDirection));
        if (surfaceCosine <= 0.0) {
            continue;
        }

        const Vec3 shadowOrigin = hit.point + hit.normal * RAY_EPSILON;
        const Ray shadowRay(shadowOrigin, lightDirection);
        if (isOccluded(shadowRay, lightDistance - RAY_EPSILON)) {
            continue;
        }

        const double distanceIntensity =
            lightSource->getIncidentBrightness(hit.point);
        const Vec3 incomingRadiance =
            lightSource->getColor() * distanceIntensity;
        const Vec3 bsdf = Bsdf::evaluate(
            hit.shape->getMaterial(), hit.normal,
            outgoing, lightDirection);
        result = result + bsdf.elementwiseMultiply(incomingRadiance) *
            surfaceCosine;
    }
    return result;
}

std::size_t Scene::sampledLightCount() const {
    return emissiveShapes.size() +
        (environment.isBlack() ? 0u : 1u);
}

double Scene::powerHeuristic(double firstPdf, double secondPdf) {
    const double firstSquared = firstPdf * firstPdf;
    const double secondSquared = secondPdf * secondPdf;
    const double sum = firstSquared + secondSquared;
    return sum > 0.0 ? firstSquared / sum : 0.0;
}

double Scene::emissiveLightPdf(
        const Vec3& origin, const HitRecord& lightHit) const {
    const std::size_t lightCount = sampledLightCount();
    if (lightCount == 0 ||
        lightHit.shape->surfaceArea() <= Vec3::EPSILON) {
        return 0.0;
    }
    const Vec3 offset = lightHit.point - origin;
    const double distanceSquared = offset.dot(offset);
    const Vec3 direction = offset.normalize();
    const double lightCosine =
        std::abs(lightHit.normal.dot(-direction));
    if (lightCosine <= Vec3::EPSILON) {
        return 0.0;
    }
    return (1.0 / static_cast<double>(lightCount)) *
        (1.0 / lightHit.shape->surfaceArea()) *
        distanceSquared / lightCosine;
}

double Scene::environmentLightPdf(const Vec3& direction) const {
    const std::size_t lightCount = sampledLightCount();
    if (lightCount == 0 || environment.isBlack()) {
        return 0.0;
    }
    return environment.pdf(direction) /
        static_cast<double>(lightCount);
}

Vec3 Scene::directLighting(
        const HitRecord& hit, const Vec3& outgoing,
        Sampler& sampler) const {
    Vec3 result = pointLighting(hit, outgoing);
    const std::size_t lightCount = sampledLightCount();
    if (lightCount == 0) {
        return result;
    }

    const std::size_t selected = std::min(
        lightCount - 1,
        static_cast<std::size_t>(sampler.next() * lightCount));
    Vec3 lightDirection;
    Vec3 incomingRadiance;
    double lightPdf = 0.0;
    double maximumDistance = std::numeric_limits<double>::infinity();

    if (selected < emissiveShapes.size()) {
        const Shape* light = emissiveShapes[selected];
        SurfaceSample lightSample;
        if (!light->sampleSurface(sampler, lightSample)) {
            return result;
        }
        const Vec3 toLight = lightSample.point - hit.point;
        const double distanceSquared = toLight.dot(toLight);
        const double distance = std::sqrt(distanceSquared);
        if (distance <= RAY_EPSILON) {
            return result;
        }
        lightDirection = toLight / distance;
        const double lightCosine =
            std::abs(lightSample.normal.dot(-lightDirection));
        if (lightCosine <= Vec3::EPSILON) {
            return result;
        }
        lightPdf =
            lightSample.areaPdf * distanceSquared / lightCosine /
            static_cast<double>(lightCount);
        incomingRadiance = light->getMaterial().emission;
        maximumDistance = distance - RAY_EPSILON;
    } else {
        const EnvironmentSample sample = environment.sample(sampler);
        lightDirection = sample.direction;
        incomingRadiance = sample.radiance;
        lightPdf = sample.pdf / static_cast<double>(lightCount);
    }

    const double surfaceCosine =
        std::max(0.0, hit.normal.dot(lightDirection));
    if (surfaceCosine <= 0.0 || lightPdf <= 0.0) {
        return result;
    }
    const Ray shadowRay(
        hit.point + hit.normal * RAY_EPSILON, lightDirection);
    if (isOccluded(shadowRay, maximumDistance)) {
        return result;
    }

    const Material& material = hit.shape->getMaterial();
    const double bsdfPdf = Bsdf::pdf(
        material, hit.normal, outgoing, lightDirection);
    const double weight = powerHeuristic(lightPdf, bsdfPdf);
    const Vec3 bsdf = Bsdf::evaluate(
        material, hit.normal, outgoing, lightDirection);
    return result + bsdf.elementwiseMultiply(incomingRadiance) *
        (surfaceCosine * weight / lightPdf);
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

Vec3 Scene::trace(const Ray& ray, Sampler& sampler,
                  const PathTraceSettings& settings) const {
    if (settings.maxBounces == 0) {
        return Vec3();
    }

    Vec3 radiance;
    Vec3 throughput(1.0, 1.0, 1.0);
    Ray currentRay = ray;
    Vec3 previousPoint;
    double previousBsdfPdf = 0.0;
    bool previousWasDelta = true;

    for (unsigned int bounce = 0;
         bounce < settings.maxBounces; ++bounce) {
        HitRecord hit;
        if (!findClosestHit(
                currentRay, RAY_EPSILON,
                std::numeric_limits<double>::infinity(), hit)) {
            const double weight = previousWasDelta
                ? 1.0
                : powerHeuristic(
                      previousBsdfPdf,
                      environmentLightPdf(currentRay.direction()));
            radiance = radiance +
                throughput.elementwiseMultiply(
                    environment.evaluate(currentRay.direction())) * weight;
            break;
        }

        const Material& material = hit.shape->getMaterial();
        double emissionWeight = 1.0;
        if (bounce > 0 && !previousWasDelta &&
            !material.emission.near(Vec3())) {
            emissionWeight = powerHeuristic(
                previousBsdfPdf,
                emissiveLightPdf(previousPoint, hit));
        }
        radiance = radiance +
            throughput.elementwiseMultiply(material.emission) *
            emissionWeight;

        const Vec3 outgoing = -currentRay.direction();
        if (Bsdf::hasNonDeltaLobe(material)) {
            radiance = radiance +
                throughput.elementwiseMultiply(
                    directLighting(hit, outgoing, sampler));
        }

        const BsdfSample bsdfSample = Bsdf::sample(
            material, hit.normal, outgoing,
            hit.frontFace, sampler);
        if (!bsdfSample.valid) {
            break;
        }
        throughput =
            throughput.elementwiseMultiply(bsdfSample.weight);
        previousBsdfPdf = bsdfSample.pdf;
        previousWasDelta = bsdfSample.delta;

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
            bsdfSample.direction.dot(hit.normal) >= 0.0 ? 1.0 : -1.0;
        previousPoint = hit.point;
        currentRay = Ray(
            hit.point + hit.normal * (RAY_EPSILON * side),
            bsdfSample.direction);
    }
    return radiance;
}

void Scene::addShape(std::unique_ptr<Shape> shape) {
    if (shape) {
        Aabb bounds;
        if (shape->boundingBox(bounds)) {
            boundedShapes.push_back(shape.get());
        } else {
            unboundedShapes.push_back(shape.get());
        }
        if (!shape->getMaterial().emission.near(Vec3()) &&
            shape->surfaceArea() > Vec3::EPSILON) {
            emissiveShapes.push_back(shape.get());
        }
        shapes.push_back(std::move(shape));
        bvh.build(boundedShapes);
    }
}

void Scene::setEnvironment(const Environment& newEnvironment) {
    environment = newEnvironment;
}

void Scene::addLight(std::unique_ptr<LightSource> source) {
    if (source) {
        lightSources.push_back(std::move(source));
    }
}
