#include "Math/Quaternion.h"
#include "Math/Ray.h"
#include "Scene/Camera.h"
#include "Scene/Light/PointSource.h"
#include "Scene/Scene.h"
#include "Shapes/Cube.h"
#include "Shapes/Plane.h"
#include "Shapes/Pyramid.h"
#include "Shapes/RectangularPrism.h"
#include "Shapes/Sphere.h"
#include "Writer/ImageWriter.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const double PI = 3.14159265358979323846;
int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << std::endl;
    }
}

void expectNear(double actual, double expected, double epsilon,
                const std::string& message) {
    expect(std::abs(actual - expected) <= epsilon,
           message + " (actual " + std::to_string(actual) +
           ", expected " + std::to_string(expected) + ")");
}

void expectVecNear(const Vec3& actual, const Vec3& expected, double epsilon,
                   const std::string& message) {
    expect(actual.near(expected, epsilon), message);
}

void expectThrows(const std::function<void()>& action,
                  const std::string& message) {
    try {
        action();
        expect(false, message);
    } catch (const std::invalid_argument&) {
    }
}

void testVectorMath() {
    expectVecNear(Vec3(6.0, 9.0, 12.0) / 3.0, Vec3(2.0, 3.0, 4.0),
                  1e-9, "Vector division divides components by the scalar.");
    expectVecNear(Vec3(2.0, 2.0, 0.0).projectOnto(Vec3(1.0, 0.0, 0.0)),
                  Vec3(2.0, 0.0, 0.0), 1e-9,
                  "Vector projection uses the correct scalar division.");
    expectVecNear(Vec3(1.0, 2.0, 3.0).projectOnto(Vec3()), Vec3(), 1e-9,
                  "Projection onto a zero vector is safely zero.");
    expectThrows([]() {
        const Vec3 ignored = Vec3(1.0, 2.0, 3.0) / 0.0;
        (void)ignored;
    }, "Vector division by zero is rejected in all build modes.");
    expectThrows([]() {
        const Vec3 invalid(std::vector<double>{1.0, 2.0});
        (void)invalid;
    }, "Vec3 rejects coordinate arrays with the wrong size.");
}

void testQuaternionRotation() {
    const Vec3 rotated = Quaternion::rotateVector(
        Vec3(1.0, 0.0, 0.0), PI * 0.5, Vec3(0.0, 0.0, 1.0));
    expectVecNear(rotated, Vec3(0.0, 1.0, 0.0), 1e-6,
                  "Quaternion rotation preserves radians and uses a valid inverse.");
}

void testRayAndSphereIntersection() {
    const Ray normalizedRay(Vec3(), Vec3(0.0, 0.0, -5.0));
    expectNear(normalizedRay.direction().getLength(), 1.0, 1e-9,
               "Ray directions are normalized.");
    expectThrows([]() {
        Ray invalid{Vec3(), Vec3()};
        (void)invalid;
    }, "Zero-length ray directions are rejected.");

    const Sphere sphere(Vec3(0.0, 0.0, -5.0), 1.0,
                        Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0);
    HitRecord hit;
    expect(sphere.intersect(normalizedRay, 1e-4, 100.0, hit),
           "A forward ray intersects a sphere.");
    expectNear(hit.distance, 4.0, 1e-9,
               "Sphere intersection chooses the nearest positive root.");
    expect(hit.frontFace, "Outside sphere hit is a front face.");
    expectVecNear(hit.normal, Vec3(0.0, 0.0, 1.0), 1e-9,
                  "Outside sphere normal faces against the ray.");

    const Sphere enclosingSphere(Vec3(), 2.0, Vec3(1.0, 1.0, 1.0),
                                 Vec3(), 0.0, 1.0);
    const Ray insideRay(Vec3(), Vec3(1.0, 0.0, 0.0));
    expect(enclosingSphere.intersect(insideRay, 1e-4, 100.0, hit),
           "A ray originating inside a sphere finds the exit root.");
    expectNear(hit.distance, 2.0, 1e-9, "Inside-sphere exit distance is correct.");
    expect(!hit.frontFace, "Inside sphere hit is a back face.");
    expectVecNear(hit.normal, Vec3(-1.0, 0.0, 0.0), 1e-9,
                  "Inside sphere normal is oriented against the ray.");

    expect(!sphere.intersect(normalizedRay, 4.1, 4.9, hit),
           "Ray intervals reject roots outside the requested range.");
    expectThrows([]() {
        Sphere invalid(Vec3(), 0.0, Vec3(), Vec3(), 0.0, 1.0);
    }, "Zero-radius spheres are rejected.");
}

void testPlaneIntersection() {
    const Plane plane(Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -2.0),
                      Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0);
    HitRecord hit;
    expect(plane.intersect(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
                           1e-4, 100.0, hit),
           "Plane can be intersected from its front side.");
    expectNear(hit.distance, 2.0, 1e-9, "Front plane distance is correct.");
    expect(hit.frontFace, "Front plane hit is marked as a front face.");

    expect(plane.intersect(Ray(Vec3(0.0, 0.0, -4.0), Vec3(0.0, 0.0, 1.0)),
                           1e-4, 100.0, hit),
           "Plane can be intersected from its back side.");
    expect(!hit.frontFace, "Back plane hit is marked as a back face.");
    expect(!plane.intersect(Ray(Vec3(), Vec3(1.0, 0.0, 0.0)),
                            1e-4, 100.0, hit),
           "Parallel rays do not intersect a plane.");
    expectThrows([]() {
        Plane invalid(Vec3(), Vec3(), Vec3(), Vec3(), 0.0, 1.0);
    }, "Zero normals are rejected.");
}

void testRectangularPrismIntersection() {
    const RectangularPrism prism(
        Vec3(0.0, 0.0, -5.0), Vec3(2.0, 4.0, 6.0),
        Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0);
    HitRecord hit;

    expect(prism.intersect(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
                           1e-4, 100.0, hit),
           "A ray intersects the nearest rectangular-prism slab.");
    expectNear(hit.distance, 2.0, 1e-9,
               "Rectangular-prism entry distance is correct.");
    expectVecNear(hit.normal, Vec3(0.0, 0.0, 1.0), 1e-9,
                  "Rectangular-prism entry normal is correct.");

    expect(prism.intersect(
               Ray(Vec3(0.0, 0.0, -5.0), Vec3(1.0, 0.0, 0.0)),
               1e-4, 100.0, hit),
           "A ray starting inside a rectangular prism finds an exit.");
    expectNear(hit.distance, 1.0, 1e-9,
               "Rectangular-prism inside exit distance is correct.");
    expect(!hit.frontFace,
           "A rectangular-prism exit from inside is a back-face hit.");
    expectVecNear(hit.normal, Vec3(-1.0, 0.0, 0.0), 1e-9,
                  "Inside rectangular-prism normals face against the ray.");

    expect(!prism.intersect(
               Ray(Vec3(2.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, hit),
           "A parallel ray outside a prism slab misses.");
    expectThrows([]() {
        RectangularPrism invalid(
            Vec3(), Vec3(1.0, 0.0, 1.0), Vec3(), Vec3(), 0.0, 1.0);
    }, "Rectangular prisms reject non-positive dimensions.");
}

void testCubeIntersection() {
    const Cube cube(Vec3(0.0, 0.0, -5.0), 2.0,
                    Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0);
    HitRecord hit;
    expect(cube.intersect(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
                          1e-4, 100.0, hit),
           "A cube intersects through its rectangular-prism implementation.");
    expectNear(hit.distance, 4.0, 1e-9,
               "Cube entry distance is correct.");
    expectVecNear(cube.getDimensions(), Vec3(2.0, 2.0, 2.0), 1e-9,
                  "Cube dimensions are equal on every axis.");
    expectThrows([]() {
        Cube invalid(Vec3(), 0.0, Vec3(), Vec3(), 0.0, 1.0);
    }, "Cubes reject non-positive side lengths.");
}

void testPyramidIntersection() {
    const Pyramid pyramid(
        Vec3(0.0, -1.0, -5.0), 2.0, 2.0, 2.0,
        Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0);
    HitRecord hit;

    expect(pyramid.intersect(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
                             1e-4, 100.0, hit),
           "A ray intersects a pyramid side.");
    expectNear(hit.distance, 4.5, 1e-9,
               "Pyramid side intersection distance is correct.");
    expect(hit.normal.Z() > 0.0,
           "The front pyramid face has an outward positive-Z normal.");

    expect(pyramid.intersect(
               Ray(Vec3(0.0, -3.0, -5.0), Vec3(0.0, 1.0, 0.0)),
               1e-4, 100.0, hit),
           "A ray intersects the pyramid base.");
    expectNear(hit.distance, 2.0, 1e-9,
               "Pyramid base intersection distance is correct.");
    expectVecNear(hit.normal, Vec3(0.0, -1.0, 0.0), 1e-9,
                  "Pyramid base normal points outward.");

    expect(pyramid.intersect(
               Ray(Vec3(0.0, -0.5, -5.0), Vec3(1.0, 0.0, 0.0)),
               1e-4, 100.0, hit),
           "A ray starting inside a pyramid finds a side exit.");
    expect(!hit.frontFace, "A pyramid exit from inside is a back-face hit.");
    expect(hit.normal.X() < 0.0,
           "Inside pyramid normals are oriented against the exiting ray.");

    expectThrows([]() {
        Pyramid invalid(
            Vec3(), 1.0, 1.0, 0.0, Vec3(), Vec3(), 0.0, 1.0);
    }, "Pyramids reject non-positive dimensions.");
}

void testCamera() {
    const Camera camera(Vec3(), Vec3(), 200, 100, PI * 0.5);
    expectVecNear(camera.makeRay(100.0, 50.0).direction(),
                  Vec3(0.0, 0.0, -1.0), 1e-9,
                  "The center camera ray follows the forward direction.");
    const Vec3 rightEdge = camera.makeRay(200.0, 50.0).direction();
    expectNear(rightEdge.X() / -rightEdge.Z(), 2.0, 1e-9,
               "Camera projection uses floating-point aspect ratio.");

    const Camera turned(Vec3(), Vec3(0.0, PI * 0.5, 0.0),
                        100, 100, PI * 0.5);
    expectVecNear(turned.makeRay(50.0, 50.0).direction(),
                  Vec3(1.0, 0.0, 0.0), 1e-6,
                  "Camera yaw is applied to primary rays.");
    expectThrows([]() {
        Camera invalid(Vec3(), Vec3(), 0, 100, PI * 0.5);
    }, "Invalid camera dimensions are rejected.");
}

void testDirectLightingAndShadows() {
    Scene litScene(Vec3(0.1, 0.2, 0.3));
    litScene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -1.0),
                  Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0)));
    litScene.addLight(std::unique_ptr<LightSource>(
        new PointSource(Vec3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0),
                        4.0 * PI * PI)));

    const Vec3 lit = litScene.trace(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)));
    expectVecNear(lit, Vec3(1.0, 0.0, 0.0), 1e-6,
                  "Direct lighting clamps N dot L and includes light color.");

    const Vec3 background =
        litScene.trace(Ray(Vec3(), Vec3(0.0, 1.0, 0.0)));
    expectVecNear(background, Vec3(0.1, 0.2, 0.3), 1e-9,
                  "Missed rays return the configured linear background.");

    Scene shadowScene;
    shadowScene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -5.0),
                  Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0)));
    shadowScene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(1.0, 0.0, -2.5), 0.45,
                   Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0)));
    shadowScene.addLight(std::unique_ptr<LightSource>(
        new PointSource(Vec3(2.0, 0.0, 0.0), Vec3(1.0, 1.0, 1.0), 100.0)));
    const Vec3 shadowed =
        shadowScene.trace(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)));
    expectVecNear(shadowed, Vec3(), 1e-9,
                  "Shadow rays respect their interval and detect blockers.");
}

void testImageEncoding() {
    expect(ImageWriter::toByte(-1.0) == 0,
           "Negative linear colors clamp to black.");
    expect(ImageWriter::toByte(0.0) == 0, "Zero encodes to zero.");
    expect(ImageWriter::toByte(1.0) == 255, "One encodes to 255.");
    expect(ImageWriter::toByte(2.0) == 255,
           "HDR values clamp safely during image conversion.");
    expect(std::abs(static_cast<int>(ImageWriter::toByte(0.18)) - 118) <= 1,
           "Linear values are converted to sRGB rather than truncated.");
}

} // namespace

int main() {
    try {
        testVectorMath();
        testQuaternionRotation();
        testRayAndSphereIntersection();
        testPlaneIntersection();
        testRectangularPrismIntersection();
        testCubeIntersection();
        testPyramidIntersection();
        testCamera();
        testDirectLightingAndShadows();
        testImageEncoding();
    } catch (const std::exception& error) {
        std::cerr << "Unexpected test exception: " << error.what() << std::endl;
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed." << std::endl;
        return 1;
    }
    std::cout << "All ray tracer correctness tests passed." << std::endl;
    return 0;
}
