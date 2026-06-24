#ifndef RAYTRACER_OBJ_MESH_H
#define RAYTRACER_OBJ_MESH_H

#include "Triangle.h"
#include "../Acceleration/Bvh.h"
#include <string>
#include <vector>

class ObjMesh : public Shape {
public:
    ObjMesh(const std::string& path, const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool intersectBruteForce(const Ray& ray, double minDistance,
                             double maxDistance, HitRecord& hit) const;
    bool boundingBox(Aabb& bounds) const override;
    std::size_t triangleCount() const { return triangles.size(); }
    std::size_t bvhNodeCount() const { return bvh.nodeCount(); }

private:
    std::vector<Triangle> triangles;
    std::vector<const Shape*> triangleShapes;
    Bvh bvh;
    Aabb bounds;
};

#endif
