//
// Created by chenx on 2020-05-24.
//

#ifndef RAYTRACER_SHAPE_H
#define RAYTRACER_SHAPE_H


#include "../Math/Vec3.h"
#include "HitRecord.h"

class Shape {

    public:

        Shape(const Vec3& surfaceColor, const Vec3& emissionColor,
              double transparency, double refractiveIndex);
        int getId();
        void setId(int id);

        virtual ~Shape() {}
        virtual bool intersect(const Ray& ray, double minDistance,
                               double maxDistance, HitRecord& hit) const = 0;

        const Vec3& getSurfaceColor() const {
            return surfaceColor;
        }

        const Vec3& getEmissionColor() const { return emissionColor; }
        double getTransparency() const { return transparency; }
        double getRefractiveIndex() const { return refractiveIndex; }

    private:
        int id;
        Vec3 surfaceColor;
        Vec3 emissionColor;
        double transparency;
        double refractiveIndex;

};


#endif //RAYTRACER_SHAPE_H
