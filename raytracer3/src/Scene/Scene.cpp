//
// Created by chenx on 2020-05-25.
//

#include <cmath>
#include <algorithm>
#include <iostream>
#include "Scene.h"

Scene::Scene(){
    lightSources = std::vector<LightSource*>();
    shapes = std::vector<Shape*>();
}


Vec3 Scene::trace(const Vec3 &rayOrigin, const Vec3 &rayDirection, int depth) const
{
    //std::cout << rayDirection.X() << rayDirection.Y() << rayDirection.Z() << " dir " << rayOrigin.X() << rayOrigin.Y()<< rayOrigin.Z();
    Vec3* incidentPointPtr = nullptr;
    Shape* incidentObjectPtr = nullptr;
    Vec3 rayColor = Vec3();
    double minDist = INFINITY;

    for (auto object: shapes){
        Vec3* intersectPointPtr = object->getRayIntersection(rayOrigin, rayDirection);

        //check if we hit
        if (intersectPointPtr != nullptr){

            double dist = intersectPointPtr->distanceTo(rayOrigin);
            if (dist < minDist) {

                incidentObjectPtr = object;
                minDist = dist;
                incidentPointPtr = intersectPointPtr;
            } else {
                delete intersectPointPtr;
                intersectPointPtr = nullptr;
            }
        }
    }
    if (incidentPointPtr == nullptr){
        return Vec3(BACKGROUND_COLOR);
    }
    for (auto lightSource: lightSources){

        Vec3 rayDirection = lightSource->getPosition() - *incidentPointPtr;

        bool obstructed = false;

        for (auto shape: shapes){
            //TODO check for obstuction by self
            //TODO optimize by organizing objects by distance. possible boost r-tree
            Vec3* intersectionPoint = shape->getRayIntersection(*incidentPointPtr, rayDirection);
            if (intersectionPoint != nullptr){

                if (*intersectionPoint == *incidentPointPtr){
                    //TODO verify rounding errors don't break functionality
                    continue;
                }
                if (intersectionPoint->distanceTo(*incidentPointPtr) <
                    lightSource->getPosition().distanceTo(*incidentPointPtr)){

                    obstructed = true;
                    delete intersectionPoint;
                    break;
                }
            }
        }

        if (obstructed){
            continue;
        }
        Vec3 lightDirection = (lightSource->getPosition() - *incidentPointPtr).normalize();
        Vec3 pointColor = incidentObjectPtr->getSurfaceColor();
        double angleIntensity = incidentObjectPtr->getNormal(*incidentPointPtr).dot(lightDirection);
        double distanceIntensity = lightSource->getIncidentBrightness(*incidentPointPtr);
        rayColor = rayColor + pointColor * angleIntensity * distanceIntensity;

    }
    //TODO implement recursive tracing
    return rayColor;
}

void Scene::addShapes(const std::vector<Shape*>& _shapes){
    //reserve for performance
    shapes.reserve(shapes.size() + _shapes.size());
    shapes.insert(shapes.end(), _shapes.begin(), _shapes.end());
}

void Scene::addLights(const std::vector<LightSource*>& sources){
    lightSources.reserve(lightSources.size() + sources.size());
    lightSources.insert(lightSources.end(), sources.begin(), sources.end());
}
