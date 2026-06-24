#include "Bvh.h"
#include "../Diagnostics/TraceStats.h"

#include <algorithm>
#include <array>
#include <limits>

namespace {

double component(const Vec3& vector, int axis) {
    return axis == 0 ? vector.X() :
           axis == 1 ? vector.Y() : vector.Z();
}

} // namespace

Bvh::Bvh() : orderedShapes(), shapeBounds(), nodes() {}

void Bvh::build(const std::vector<const Shape*>& shapes) {
    orderedShapes = shapes;
    shapeBounds.clear();
    nodes.clear();
    shapeBounds.reserve(orderedShapes.size());
    for (const Shape* shape : orderedShapes) {
        Aabb bounds;
        shape->boundingBox(bounds);
        shapeBounds.push_back(bounds);
    }
    if (!orderedShapes.empty()) {
        buildNode(0, orderedShapes.size());
    }
}

Aabb Bvh::rangeBounds(std::size_t start, std::size_t end) const {
    Aabb bounds = shapeBounds[start];
    for (std::size_t index = start + 1; index < end; ++index) {
        bounds = Aabb::surrounding(bounds, shapeBounds[index]);
    }
    return bounds;
}

int Bvh::buildNode(std::size_t start, std::size_t end) {
    const int nodeIndex = static_cast<int>(nodes.size());
    nodes.push_back(Node());
    nodes[nodeIndex].bounds = rangeBounds(start, end);
    const std::size_t count = end - start;
    if (count <= 2) {
        nodes[nodeIndex].start = start;
        nodes[nodeIndex].count = count;
        return nodeIndex;
    }

    const Vec3 extent =
        nodes[nodeIndex].bounds.max() - nodes[nodeIndex].bounds.min();
    int axis = 0;
    if (extent.Y() > extent.X() && extent.Y() >= extent.Z()) {
        axis = 1;
    } else if (extent.Z() > extent.X()) {
        axis = 2;
    }
    const std::size_t middle = start + count / 2;
    std::nth_element(
        orderedShapes.begin() + start,
        orderedShapes.begin() + middle,
        orderedShapes.begin() + end,
        [axis](const Shape* first, const Shape* second) {
            Aabb firstBounds;
            Aabb secondBounds;
            first->boundingBox(firstBounds);
            second->boundingBox(secondBounds);
            return component(firstBounds.centroid(), axis) <
                   component(secondBounds.centroid(), axis);
        });
    for (std::size_t index = start; index < end; ++index) {
        orderedShapes[index]->boundingBox(shapeBounds[index]);
    }
    const int left = buildNode(start, middle);
    const int right = buildNode(middle, end);
    nodes[nodeIndex].left = left;
    nodes[nodeIndex].right = right;
    return nodeIndex;
}

bool Bvh::intersect(const Ray& ray, double minDistance,
                    double maxDistance, HitRecord& closestHit) const {
    if (nodes.empty()) {
        return false;
    }
    bool found = false;
    double closest = maxDistance;
    std::array<int, 64> stack;
    std::size_t stackSize = 1;
    stack[0] = 0;
    while (stackSize > 0) {
        const int index = stack[--stackSize];
        TraceStats* stats = currentTraceStats();
        if (stats) {
            ++stats->bvhNodeVisits;
        }
        const Node& node = nodes[index];
        if (!node.bounds.intersect(ray, minDistance, closest)) {
            continue;
        }
        if (node.isLeaf()) {
            for (std::size_t offset = 0; offset < node.count; ++offset) {
                if (stats) {
                    ++stats->primitiveTests;
                }
                HitRecord candidate;
                if (orderedShapes[node.start + offset]->intersect(
                        ray, minDistance, closest, candidate)) {
                    closest = candidate.distance;
                    closestHit = candidate;
                    found = true;
                }
            }
        } else {
            stack[stackSize++] = node.left;
            stack[stackSize++] = node.right;
        }
    }
    return found;
}

bool Bvh::occluded(const Ray& ray, double minDistance,
                   double maxDistance) const {
    if (nodes.empty()) {
        return false;
    }
    std::array<int, 64> stack;
    std::size_t stackSize = 1;
    stack[0] = 0;
    while (stackSize > 0) {
        const int index = stack[--stackSize];
        TraceStats* stats = currentTraceStats();
        if (stats) {
            ++stats->bvhNodeVisits;
        }
        const Node& node = nodes[index];
        if (!node.bounds.intersect(ray, minDistance, maxDistance)) {
            continue;
        }
        if (node.isLeaf()) {
            for (std::size_t offset = 0; offset < node.count; ++offset) {
                if (stats) {
                    ++stats->primitiveTests;
                }
                const Shape* shape = orderedShapes[node.start + offset];
                HitRecord hit;
                if (shape->getMaterial().transmission <= 0.0 &&
                    shape->intersect(
                        ray, minDistance, maxDistance, hit)) {
                    return true;
                }
            }
        } else {
            stack[stackSize++] = node.left;
            stack[stackSize++] = node.right;
        }
    }
    return false;
}
