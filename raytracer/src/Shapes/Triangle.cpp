#include "Triangle.h"

#include <algorithm>
#include <cmath>
#include "SurfaceSample.h"
#include "../Math/Sampler.h"

Triangle::Triangle(
        const Vertex& first, const Vertex& second,
        const Vertex& third, const Material& material)
    : Shape(material), first(first), second(second), third(third),
      faceNormal((second.position - first.position)
          .cross(third.position - first.position).normalize()) {
}

bool Triangle::intersect(const Ray& ray, double minDistance,
                         double maxDistance, HitRecord& hit) const {
    const Vec3 edgeOne = second.position - first.position;
    const Vec3 edgeTwo = third.position - first.position;
    const Vec3 perpendicular = ray.direction().cross(edgeTwo);
    const double determinant = edgeOne.dot(perpendicular);
    if (std::abs(determinant) <= Vec3::EPSILON) return false;
    const double inverse = 1.0 / determinant;
    const Vec3 offset = ray.origin() - first.position;
    const double b1 = offset.dot(perpendicular) * inverse;
    if (b1 < 0.0 || b1 > 1.0) return false;
    const Vec3 crossOffset = offset.cross(edgeOne);
    const double b2 = ray.direction().dot(crossOffset) * inverse;
    if (b2 < 0.0 || b1 + b2 > 1.0) return false;
    const double distance = edgeTwo.dot(crossOffset) * inverse;
    if (distance < minDistance || distance > maxDistance) return false;
    const double b0 = 1.0 - b1 - b2;
    Vec3 normal = first.normal * b0 +
                  second.normal * b1 + third.normal * b2;
    if (normal.getLength() <= Vec3::EPSILON) normal = faceNormal;
    hit.distance = distance;
    hit.point = ray.at(distance);
    hit.setFaceNormal(ray, normal.normalize());
    hit.u = first.u * b0 + second.u * b1 + third.u * b2;
    hit.v = first.v * b0 + second.v * b1 + third.v * b2;
    hit.shape = this;
    return true;
}

bool Triangle::boundingBox(Aabb& bounds) const {
    const double epsilon = 1e-5;
    bounds = Aabb(
        Vec3(std::min(first.position.X(),
                      std::min(second.position.X(), third.position.X())) - epsilon,
             std::min(first.position.Y(),
                      std::min(second.position.Y(), third.position.Y())) - epsilon,
             std::min(first.position.Z(),
                      std::min(second.position.Z(), third.position.Z())) - epsilon),
        Vec3(std::max(first.position.X(),
                      std::max(second.position.X(), third.position.X())) + epsilon,
             std::max(first.position.Y(),
                      std::max(second.position.Y(), third.position.Y())) + epsilon,
             std::max(first.position.Z(),
                      std::max(second.position.Z(), third.position.Z())) + epsilon));
    return true;
}

double Triangle::computeArea() const {
    Vec3 edge1 = second.position - first.position;
    Vec3 edge2 = third.position - first.position;
    return 0.5 * edge1.cross(edge2).getLength();
}

bool Triangle::sampleSurface(Sampler& sampler,
                             SurfaceSample& sample) const {
    double r1 = sampler.next();
    double r2 = sampler.next();
    double sqrtR1 = std::sqrt(r1);
    double b0 = 1.0 - sqrtR1;
    double b1 = sqrtR1 * (1.0 - r2);
    double b2 = sqrtR1 * r2;
    sample.point = first.position * b0 +
                   second.position * b1 + third.position * b2;
    Vec3 n = first.normal * b0 +
             second.normal * b1 + third.normal * b2;
    if (n.getLength() > Vec3::EPSILON) {
        sample.normal = n.normalize();
    } else {
        sample.normal = faceNormal;
    }
    double area = computeArea();
    sample.areaPdf = area > 0.0 ? 1.0 / area : 0.0;
    return area > 0.0;
}

