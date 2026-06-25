#include "Bvh.h"
#include "../Diagnostics/TraceStats.h"

#include <algorithm>
#include <array>
#include <limits>

namespace {

constexpr std::size_t BIN_COUNT = 12;
constexpr std::size_t MAX_LEAF_SIZE = 2;
constexpr std::size_t TRAVERSAL_STACK_SIZE = 128;

double component(const Vec3& vector, int axis) {
    return axis == 0 ? vector.X() :
           axis == 1 ? vector.Y() : vector.Z();
}

struct Bin {
    Aabb bounds;
    std::size_t count;
    bool initialized;

    Bin() : bounds(), count(0), initialized(false) {}

    void add(const Aabb& value) {
        bounds = initialized
            ? Aabb::surrounding(bounds, value)
            : value;
        initialized = true;
        ++count;
    }
};

struct StackEntry {
    int index;
    double nearDistance;
};

} // namespace

Bvh::Bvh() : primitives(), nodes() {}

void Bvh::build(const std::vector<const Shape*>& shapes) {
    primitives.clear();
    nodes.clear();
    primitives.reserve(shapes.size());
    for (const Shape* shape : shapes) {
        Aabb bounds;
        shape->boundingBox(bounds);
        primitives.push_back(Primitive{
            shape, bounds, bounds.centroid()
        });
    }
    nodes.reserve(primitives.empty() ? 0 : primitives.size() * 2 - 1);
    if (!primitives.empty()) {
        buildNode(0, primitives.size());
    }
}

Aabb Bvh::rangeBounds(std::size_t start, std::size_t end) const {
    Aabb bounds = primitives[start].bounds;
    for (std::size_t index = start + 1; index < end; ++index) {
        bounds = Aabb::surrounding(bounds, primitives[index].bounds);
    }
    return bounds;
}

std::size_t Bvh::chooseSplit(
        std::size_t start, std::size_t end, const Aabb& bounds) {
    Vec3 centroidMinimum = primitives[start].centroid;
    Vec3 centroidMaximum = primitives[start].centroid;
    for (std::size_t index = start + 1; index < end; ++index) {
        const Vec3& centroid = primitives[index].centroid;
        centroidMinimum = Vec3(
            std::min(centroidMinimum.X(), centroid.X()),
            std::min(centroidMinimum.Y(), centroid.Y()),
            std::min(centroidMinimum.Z(), centroid.Z()));
        centroidMaximum = Vec3(
            std::max(centroidMaximum.X(), centroid.X()),
            std::max(centroidMaximum.Y(), centroid.Y()),
            std::max(centroidMaximum.Z(), centroid.Z()));
    }

    double bestCost = std::numeric_limits<double>::infinity();
    int bestAxis = -1;
    std::size_t bestBin = 0;
    const double parentArea =
        std::max(bounds.surfaceArea(), Vec3::EPSILON);

    for (int axis = 0; axis < 3; ++axis) {
        const double minimum = component(centroidMinimum, axis);
        const double extent =
            component(centroidMaximum, axis) - minimum;
        if (extent <= Vec3::EPSILON) {
            continue;
        }

        std::array<Bin, BIN_COUNT> bins;
        for (std::size_t index = start; index < end; ++index) {
            const double normalized =
                (component(primitives[index].centroid, axis) - minimum) /
                extent;
            const std::size_t bin = std::min(
                BIN_COUNT - 1,
                static_cast<std::size_t>(normalized * BIN_COUNT));
            bins[bin].add(primitives[index].bounds);
        }

        std::array<Aabb, BIN_COUNT> leftBounds;
        std::array<Aabb, BIN_COUNT> rightBounds;
        std::array<std::size_t, BIN_COUNT> leftCounts{};
        std::array<std::size_t, BIN_COUNT> rightCounts{};
        Aabb runningLeft;
        Aabb runningRight;
        bool leftInitialized = false;
        bool rightInitialized = false;
        std::size_t leftCount = 0;
        std::size_t rightCount = 0;

        for (std::size_t bin = 0; bin < BIN_COUNT; ++bin) {
            if (bins[bin].initialized) {
                runningLeft = leftInitialized
                    ? Aabb::surrounding(runningLeft, bins[bin].bounds)
                    : bins[bin].bounds;
                leftInitialized = true;
            }
            leftCount += bins[bin].count;
            leftBounds[bin] = runningLeft;
            leftCounts[bin] = leftCount;

            const std::size_t reverse = BIN_COUNT - 1 - bin;
            if (bins[reverse].initialized) {
                runningRight = rightInitialized
                    ? Aabb::surrounding(
                          runningRight, bins[reverse].bounds)
                    : bins[reverse].bounds;
                rightInitialized = true;
            }
            rightCount += bins[reverse].count;
            rightBounds[reverse] = runningRight;
            rightCounts[reverse] = rightCount;
        }

        for (std::size_t bin = 0; bin + 1 < BIN_COUNT; ++bin) {
            if (leftCounts[bin] == 0 || rightCounts[bin + 1] == 0) {
                continue;
            }
            const double cost =
                (leftBounds[bin].surfaceArea() * leftCounts[bin] +
                 rightBounds[bin + 1].surfaceArea() *
                     rightCounts[bin + 1]) /
                parentArea;
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestBin = bin;
            }
        }
    }

    const std::size_t count = end - start;
    if (bestAxis < 0 || bestCost >= static_cast<double>(count)) {
        return 0;
    }

    const double minimum = component(centroidMinimum, bestAxis);
    const double extent =
        component(centroidMaximum, bestAxis) - minimum;
    const double splitPosition =
        minimum + extent *
            (static_cast<double>(bestBin + 1) / BIN_COUNT);
    const auto middle = std::partition(
        primitives.begin() + start,
        primitives.begin() + end,
        [bestAxis, splitPosition](const Primitive& primitive) {
            return component(primitive.centroid, bestAxis) <
                   splitPosition;
        });
    const std::size_t split =
        static_cast<std::size_t>(middle - primitives.begin());
    return split == start || split == end ? 0 : split;
}

int Bvh::buildNode(std::size_t start, std::size_t end) {
    const int nodeIndex = static_cast<int>(nodes.size());
    nodes.push_back(Node());
    nodes[nodeIndex].bounds = rangeBounds(start, end);
    const std::size_t count = end - start;
    if (count <= MAX_LEAF_SIZE) {
        nodes[nodeIndex].start = start;
        nodes[nodeIndex].count = count;
        return nodeIndex;
    }

    std::size_t middle = chooseSplit(
        start, end, nodes[nodeIndex].bounds);
    if (middle == 0) {
        const Vec3 extent =
            nodes[nodeIndex].bounds.max() -
            nodes[nodeIndex].bounds.min();
        int axis = 0;
        if (extent.Y() > extent.X() && extent.Y() >= extent.Z()) {
            axis = 1;
        } else if (extent.Z() > extent.X()) {
            axis = 2;
        }
        middle = start + count / 2;
        std::nth_element(
            primitives.begin() + start,
            primitives.begin() + middle,
            primitives.begin() + end,
            [axis](const Primitive& first, const Primitive& second) {
                return component(first.centroid, axis) <
                       component(second.centroid, axis);
            });
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

    TraceStats* stats = currentTraceStats();
    const Aabb::RayData rayData(ray);
    double rootNear = minDistance;
    if (!nodes[0].bounds.intersect(
            rayData, minDistance, maxDistance, &rootNear, stats)) {
        return false;
    }

    bool found = false;
    double closest = maxDistance;
    std::array<StackEntry, TRAVERSAL_STACK_SIZE> stack;
    std::size_t stackSize = 1;
    stack[0] = StackEntry{0, rootNear};

    while (stackSize > 0) {
        const StackEntry entry = stack[--stackSize];
        if (entry.nearDistance > closest) {
            continue;
        }
        if (stats) {
            ++stats->bvhNodeVisits;
        }
        const Node& node = nodes[entry.index];
        if (node.isLeaf()) {
            for (std::size_t offset = 0; offset < node.count; ++offset) {
                if (stats) {
                    ++stats->primitiveTests;
                }
                HitRecord candidate;
                if (primitives[node.start + offset].shape->intersect(
                        ray, minDistance, closest, candidate)) {
                    closest = candidate.distance;
                    closestHit = candidate;
                    found = true;
                }
            }
            continue;
        }

        double leftNear = minDistance;
        double rightNear = minDistance;
        const bool hitLeft = nodes[node.left].bounds.intersect(
            rayData, minDistance, closest, &leftNear, stats);
        const bool hitRight = nodes[node.right].bounds.intersect(
            rayData, minDistance, closest, &rightNear, stats);
        if (hitLeft && hitRight) {
            if (leftNear < rightNear) {
                stack[stackSize++] = StackEntry{node.right, rightNear};
                stack[stackSize++] = StackEntry{node.left, leftNear};
            } else {
                stack[stackSize++] = StackEntry{node.left, leftNear};
                stack[stackSize++] = StackEntry{node.right, rightNear};
            }
        } else if (hitLeft) {
            stack[stackSize++] = StackEntry{node.left, leftNear};
        } else if (hitRight) {
            stack[stackSize++] = StackEntry{node.right, rightNear};
        }
    }
    return found;
}

bool Bvh::occluded(const Ray& ray, double minDistance,
                   double maxDistance) const {
    if (nodes.empty()) {
        return false;
    }

    TraceStats* stats = currentTraceStats();
    const Aabb::RayData rayData(ray);
    double rootNear = minDistance;
    if (!nodes[0].bounds.intersect(
            rayData, minDistance, maxDistance, &rootNear, stats)) {
        return false;
    }

    std::array<StackEntry, TRAVERSAL_STACK_SIZE> stack;
    std::size_t stackSize = 1;
    stack[0] = StackEntry{0, rootNear};

    while (stackSize > 0) {
        const StackEntry entry = stack[--stackSize];
        if (stats) {
            ++stats->bvhNodeVisits;
        }
        const Node& node = nodes[entry.index];
        if (node.isLeaf()) {
            for (std::size_t offset = 0; offset < node.count; ++offset) {
                if (stats) {
                    ++stats->primitiveTests;
                }
                const Shape* shape =
                    primitives[node.start + offset].shape;
                HitRecord hit;
                if (shape->getMaterial().transmission <= 0.0 &&
                    shape->intersect(
                        ray, minDistance, maxDistance, hit)) {
                    return true;
                }
            }
            continue;
        }

        double leftNear = minDistance;
        double rightNear = minDistance;
        const bool hitLeft = nodes[node.left].bounds.intersect(
            rayData, minDistance, maxDistance, &leftNear, stats);
        const bool hitRight = nodes[node.right].bounds.intersect(
            rayData, minDistance, maxDistance, &rightNear, stats);
        if (hitLeft && hitRight) {
            if (leftNear < rightNear) {
                stack[stackSize++] = StackEntry{node.right, rightNear};
                stack[stackSize++] = StackEntry{node.left, leftNear};
            } else {
                stack[stackSize++] = StackEntry{node.left, leftNear};
                stack[stackSize++] = StackEntry{node.right, rightNear};
            }
        } else if (hitLeft) {
            stack[stackSize++] = StackEntry{node.left, leftNear};
        } else if (hitRight) {
            stack[stackSize++] = StackEntry{node.right, rightNear};
        }
    }
    return false;
}
