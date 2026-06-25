//
// Created by chenx on 2020-05-25.
//

#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include "Scene.h"
#include "../Diagnostics/TraceStats.h"

constexpr double Scene::RAY_EPSILON;

Scene::Scene(const Vec3& background)
    : environment(background, background),
      boundedShapes(),
      unboundedShapes(),
      emissiveShapes(),
      aggregateBounds(),
      hasAggregateBounds(false),
      bvh(),
      accelerationDirty(false),
      accelerationBuildCount(0),
      accelerationEnabled(true) {
}

bool Scene::findClosestHit(const Ray& ray, double minDistance,
                           double maxDistance, HitRecord& closestHit) const {
    finalize();
    bool foundHit = false;
    if (accelerationEnabled) {
        foundHit =
            bvh.intersect(ray, minDistance, maxDistance, closestHit);
    } else {
        HitRecord candidate;
        double closest = maxDistance;
        for (const Shape* shape : boundedShapes) {
            TraceStats* stats = currentTraceStats();
            if (stats) {
                ++stats->primitiveTests;
            }
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
        TraceStats* stats = currentTraceStats();
        if (stats) {
            ++stats->primitiveTests;
        }
        if (object->intersect(ray, minDistance, closestDistance, candidate)) {
            foundHit = true;
            closestDistance = candidate.distance;
            closestHit = candidate;
        }
    }
    return foundHit;
}

bool Scene::isOccluded(const Ray& ray, double maxDistance) const {
    TraceStats* stats = currentTraceStats();
    if (stats) {
        ++stats->rays;
        ++stats->shadowRays;
    }
    finalize();
    if (accelerationEnabled) {
        if (bvh.occluded(ray, RAY_EPSILON, maxDistance)) {
            return true;
        }
    } else {
        HitRecord hit;
        for (const Shape* shape : boundedShapes) {
            if (stats) {
                ++stats->primitiveTests;
            }
            if (shape->getMaterial().transmission <= 0.0 &&
                shape->intersect(
                    ray, RAY_EPSILON, maxDistance, hit)) {
                return true;
            }
        }
    }
    HitRecord ignored;
    for (const Shape* object : unboundedShapes) {
        if (stats) {
            ++stats->primitiveTests;
        }
        if (object->intersect(ray, RAY_EPSILON, maxDistance, ignored)) {
            if (object->getMaterial().transmission <= 0.0) {
                return true;
            }
        }
    }
    return false;
}

PathResult Scene::trace(const Ray& ray) const {
    Sampler sampler(0, 0, 1);
    return trace(ray, sampler, PathTraceSettings(1, 1));
}

Vec3 Scene::pointLighting(
        const HitRecord& hit, const Material& material,
        const Vec3& outgoing, Sampler& sampler) const {
    Vec3 result;
    LightSample sample;
    TraceStats* stats = currentTraceStats();
    for (const auto& lightSource : lightSources) {
        if (stats) {
            ++stats->consideredLights;
        }
        if (!lightSource->sampleIncident(
                hit.point, hit.normal, sampler, sample)) {
            if (stats) {
                if (sample.rejection == LightRejection::Range) {
                    ++stats->rangeRejects;
                } else if (sample.rejection == LightRejection::Cone) {
                    ++stats->coneRejects;
                } else if (sample.rejection == LightRejection::Backface) {
                    ++stats->backfaceRejects;
                }
            }
            continue;
        }
        const Vec3 shadowOrigin =
            hit.point + hit.geometricNormal * RAY_EPSILON;
        const Ray shadowRay(shadowOrigin, sample.direction);
        if (!lightSource->isFinite()) {
            sample.maximumDistance = directionalShadowDistance(
                shadowOrigin, sample.direction);
        }
        if (stats) {
            ++stats->emittedShadowRays;
        }
        if (isOccluded(shadowRay, sample.maximumDistance)) {
            if (stats) {
                ++stats->occludedShadowRays;
            }
            continue;
        }
        const double surfaceCosine =
            std::max(0.0, hit.normal.dot(sample.direction));
        const Vec3 bsdf = Bsdf::evaluate(
            material, hit.normal,
            outgoing, sample.direction);
        result = result + bsdf.elementwiseMultiply(sample.radiance) *
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
        const HitRecord& hit, const Material& material,
        const Vec3& outgoing,
        Sampler& sampler) const {
    Vec3 result = pointLighting(hit, material, outgoing, sampler);
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
        hit.point + hit.geometricNormal * RAY_EPSILON, lightDirection);
    if (isOccluded(shadowRay, maximumDistance)) {
        return result;
    }

    const double bsdfPdf = Bsdf::pdf(
        material, hit.normal, outgoing, lightDirection);
    const double weight = powerHeuristic(lightPdf, bsdfPdf);
    const Vec3 bsdf = Bsdf::evaluate(
        material, hit.normal, outgoing, lightDirection);
    return result + bsdf.elementwiseMultiply(incomingRadiance) *
        (surfaceCosine * weight / lightPdf);
}

PathResult Scene::trace(const Ray& ray, Sampler& sampler,
                   const PathTraceSettings& settings) const {
    PathResult result;
    if (settings.maxBounces == 0) {
        return result;
    }

    Vec3 radiance;
    Vec3 throughput(1.0, 1.0, 1.0);
    Ray currentRay = ray;
    Vec3 previousPoint;
    double previousBsdfPdf = 0.0;
    bool previousWasDelta = true;
    bool firstBounce = true;
    TraceStats* stats = currentTraceStats();
    if (stats) {
        ++stats->paths;
    }

    for (unsigned int bounce = 0;
         bounce < settings.maxBounces; ++bounce) {
        if (stats) {
            ++stats->rays;
            ++stats->pathVertices;
        }
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

        Material material = hit.shape->getMaterial();
        material.albedo =
            material.colorAt(hit.u, hit.v, hit.point);
        material.roughness =
            material.roughnessAt(hit.u, hit.v, hit.point);
        material.metallic =
            material.metallicAt(hit.u, hit.v, hit.point);
        hit.normal = material.normalAt(
            hit.u, hit.v, hit.point, hit.normal);

        if (firstBounce) {
            result.albedo = material.albedo;
            result.normal = hit.normal;
            result.position = hit.point;
            firstBounce = false;
        }

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
                    directLighting(hit, material, outgoing, sampler));
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
            bsdfSample.direction.dot(hit.geometricNormal) >= 0.0
                ? 1.0 : -1.0;
        previousPoint = hit.point;
        currentRay = Ray(
            hit.point + hit.geometricNormal * (RAY_EPSILON * side),
            bsdfSample.direction);
    }
    result.radiance = radiance;
    return result;
}

void Scene::addShape(std::unique_ptr<Shape> shape) {
    if (shape) {
        Aabb bounds;
        if (shape->boundingBox(bounds)) {
            boundedShapes.push_back(shape.get());
            aggregateBounds = hasAggregateBounds
                ? Aabb::surrounding(aggregateBounds, bounds)
                : bounds;
            hasAggregateBounds = true;
        } else {
            unboundedShapes.push_back(shape.get());
        }
        if (!shape->getMaterial().emission.near(Vec3()) &&
            shape->surfaceArea() > Vec3::EPSILON) {
            emissiveShapes.push_back(shape.get());
        }
        shapes.push_back(std::move(shape));
        accelerationDirty = true;
    }
}

void Scene::finalize() const {
    if (accelerationDirty) {
        bvh.build(boundedShapes);
        accelerationDirty = false;
        ++accelerationBuildCount;
    }
}

void Scene::setEnvironment(const Environment& newEnvironment) {
    environment = newEnvironment;
}

bool Scene::loadEnvironmentMap(
        const std::string& path, double intensity) {
    const std::size_t dot = path.find_last_of('.');
    const std::string extension =
        dot == std::string::npos ? std::string() : path.substr(dot);
    if (extension == ".hdr" || extension == ".HDR") {
        return environment.loadHdr(path, intensity);
    }
    return environment.loadPpm(path, intensity);
}

void Scene::addLight(std::unique_ptr<LightSource> source) {
    if (source) {
        lightSources.push_back(std::move(source));
    }
}

double Scene::directionalShadowDistance(
        const Vec3& origin, const Vec3& direction) const {
    if (!hasAggregateBounds || !unboundedShapes.empty()) {
        return std::numeric_limits<double>::infinity();
    }
    const Vec3 minimum = aggregateBounds.min();
    const Vec3 maximum = aggregateBounds.max();
    double farthest = 0.0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const Vec3 corner(
                    x ? maximum.X() : minimum.X(),
                    y ? maximum.Y() : minimum.Y(),
                    z ? maximum.Z() : minimum.Z());
                farthest = std::max(
                    farthest, (corner - origin).dot(direction));
            }
        }
    }
    return std::max(RAY_EPSILON, farthest + RAY_EPSILON);
}
