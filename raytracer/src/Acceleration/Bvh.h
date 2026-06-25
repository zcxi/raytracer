#ifndef RAYTRACER_BVH_H
#define RAYTRACER_BVH_H

#include "../Shapes/Shape.h"
#include <vector>

class Bvh {
public:
    Bvh();

    void build(const std::vector<const Shape*>& shapes);
    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const;
    bool occluded(const Ray& ray, double minDistance,
                  double maxDistance) const;
    std::size_t nodeCount() const { return nodes.size(); }
    bool empty() const { return nodes.empty(); }

private:
    struct Primitive {
        const Shape* shape;
        Aabb bounds;
        Vec3 centroid;
    };

    struct Node {
        Aabb bounds;
        int left;
        int right;
        std::size_t start;
        std::size_t count;

        Node() : bounds(), left(-1), right(-1), start(0), count(0) {}
        bool isLeaf() const { return count > 0; }
    };

    int buildNode(std::size_t start, std::size_t end);
    Aabb rangeBounds(std::size_t start, std::size_t end) const;
    std::size_t chooseSplit(
        std::size_t start, std::size_t end, const Aabb& bounds);

    std::vector<Primitive> primitives;
    std::vector<Node> nodes;
};

#endif

