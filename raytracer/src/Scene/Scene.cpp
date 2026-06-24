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
            return true;
        }
    }
    return false;
}

Vec3 Scene::trace(const Ray& ray) const {
    const double pi = 3.14159265358979323846;
    HitRecord hit;
    if (!findClosestHit(ray, RAY_EPSILON,
                        std::numeric_limits<double>::infinity(), hit)) {
        return backgroundColor;
    }

    Vec3 rayColor = hit.shape->getEmissionColor();
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
        rayColor = rayColor +
            hit.shape->getSurfaceColor().elementwiseMultiply(incomingLight) / pi;
    }
    return rayColor;
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
