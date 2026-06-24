//
// Created by chenx on 2020-05-24.
//

#ifndef RAYTRACER_SHAPE_H
#define RAYTRACER_SHAPE_H


#include "../Math/Vec3.h"
#include "../Materials/Material.h"
#include "HitRecord.h"

class Shape {

    public:

        explicit Shape(const Material& material);
        Shape(const Vec3& surfaceColor, const Vec3& emissionColor,
              double transparency, double refractiveIndex);
        int getId();
        void setId(int id);

        virtual ~Shape() {}
        virtual bool intersect(const Ray& ray, double minDistance,
                               double maxDistance, HitRecord& hit) const = 0;

        const Material& getMaterial() const { return material; }
        const Vec3& getSurfaceColor() const { return material.albedo; }
        const Vec3& getEmissionColor() const { return material.emission; }
        double getTransparency() const {
            return material.type == MaterialType::Dielectric ? 1.0 : 0.0;
        }
        double getRefractiveIndex() const {
            return material.refractiveIndex;
        }

    private:
        int id;
        Material material;

};


#endif //RAYTRACER_SHAPE_H
