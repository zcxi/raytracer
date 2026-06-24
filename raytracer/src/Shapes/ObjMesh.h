#ifndef RAYTRACER_OBJ_MESH_H
#define RAYTRACER_OBJ_MESH_H

#include "Triangle.h"
#include <string>
#include <vector>

class ObjMesh : public Shape {
public:
    ObjMesh(const std::string& path, const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;
    std::size_t triangleCount() const { return triangles.size(); }

private:
    std::vector<Triangle> triangles;
    Aabb bounds;
};

#endif

