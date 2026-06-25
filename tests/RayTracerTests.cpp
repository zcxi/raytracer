#include "Math/Quaternion.h"
#include "Math/Ray.h"
#include "Math/Sampler.h"
#include "Materials/Material.h"
#include "Materials/Bsdf.h"
#include "Acceleration/Aabb.h"
#include "Acceleration/Bvh.h"
#include "Diagnostics/TraceStats.h"
#include "Renderer.h"
#include "Scene/Camera.h"
#include "Scene/Environment.h"
#include "Scene/Light/PointSource.h"
#include "Scene/Scene.h"
#include "Shapes/Cube.h"
#include "Shapes/Plane.h"
#include "Shapes/Pyramid.h"
#include "Shapes/Rectangle.h"
#include "Shapes/Triangle.h"
#include "Shapes/ObjMesh.h"
#include "Shapes/MovingSphere.h"
#include "Shapes/Transform.h"
#include "Shapes/RectangularPrism.h"
#include "Shapes/Sphere.h"
#include "Writer/ImageWriter.h"
#include "Textures/Texture.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <fstream>
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
    expect(sizeof(Vec3) == sizeof(double) * 3,
           "Vec3 stores exactly three inline doubles.");
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

std::vector<unsigned char> readBytes(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

void testPerformanceQuickWins() {
    expect(!RenderSettings().collectStats,
           "Detailed tracing counters are disabled by default.");
    expect(RenderSettings(1, 0, 1, 1, 1, 1, 16, true).collectStats,
           "Tracing counters can be enabled explicitly.");

    const Aabb areaBox(
        Vec3(-1.0, -2.0, -3.0), Vec3(1.0, 2.0, 3.0));
    expectNear(areaBox.surfaceArea(), 88.0, 1e-9,
               "AABB surface area supports SAH construction.");

    Scene scene;
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(
            Vec3(-1.0, 0.0, -5.0), 1.0,
            Material::diffuse(Vec3(1.0, 0.0, 0.0)))));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(
            Vec3(1.0, 0.0, -5.0), 1.0,
            Material::diffuse(Vec3(0.0, 1.0, 0.0)))));
    expect(scene.bvhBuildCount() == 0,
           "Adding shapes defers scene BVH construction.");
    scene.finalize();
    expect(scene.bvhBuildCount() == 1,
           "Scene finalization builds the BVH once.");
    scene.finalize();
    expect(scene.bvhBuildCount() == 1,
           "Repeated finalization does not rebuild a clean BVH.");

    Scene recursiveScene;
    for (int index = 0; index < 8; ++index) {
        recursiveScene.addShape(std::unique_ptr<Shape>(
            new Sphere(
                Vec3(static_cast<double>(index) * 3.0 - 10.5,
                     0.0, -8.0),
                1.0,
                Material::diffuse(Vec3(0.5, 0.5, 0.5)))));
    }
    recursiveScene.finalize();
    for (int index = 0; index < 8; ++index) {
        const Ray ray(
            Vec3(static_cast<double>(index) * 3.0 - 10.5,
                 0.0, 0.0),
            Vec3(0.0, 0.0, -1.0));
        HitRecord accelerated;
        HitRecord brute;
        recursiveScene.setAccelerationEnabled(true);
        const bool acceleratedFound = recursiveScene.findClosestHit(
            ray, 1e-4, 100.0, accelerated);
        recursiveScene.setAccelerationEnabled(false);
        const bool bruteFound = recursiveScene.findClosestHit(
            ray, 1e-4, 100.0, brute);
        expect(acceleratedFound == bruteFound &&
                   accelerated.distance == brute.distance,
               "Recursive BVH construction matches brute-force traversal.");
    }

    const std::string objPath = "test-performance-mesh.obj";
    {
        std::ofstream obj(objPath.c_str());
        obj << "v -1 -1 -5\nv 1 -1 -5\nv 1 1 -5\nv -1 1 -5\n";
        obj << "f 1 2 3 4\n";
    }
    ObjMesh mesh(objPath, Material::diffuse(Vec3(0.5, 0.5, 0.5)));
    expect(mesh.bvhNodeCount() > 0,
           "OBJ meshes build a local triangle BVH.");
    TraceStats stats;
    {
        TraceStatsScope scope(&stats);
        HitRecord hit;
        expect(mesh.intersect(
                   Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
                   1e-4, 100.0, hit),
               "Mesh BVH preserves triangle intersections.");
    }
    expect(stats.aabbTests > 0 && stats.bvhNodeVisits > 0 &&
               stats.primitiveTests > 0,
           "Mesh traversal contributes reproducible tracing counters.");
    std::remove(objPath.c_str());

    const std::string averagedPath = "test-averaged.ppm";
    const std::string accumulationPath = "test-accumulation.ppm";
    const std::vector<std::vector<Vec3>> averaged(
        1, std::vector<Vec3>(2, Vec3(0.25, 0.5, 0.75)));
    const std::vector<std::vector<Vec3>> accumulation(
        1, std::vector<Vec3>(2, Vec3(1.0, 2.0, 3.0)));
    ImageWriter averagedWriter(averagedPath);
    ImageWriter accumulationWriter(accumulationPath);
    expect(averagedWriter.write(averaged),
           "Regular image output succeeds.");
    expect(accumulationWriter.writeAccumulation(accumulation, 4),
           "Accumulation output succeeds without a framebuffer copy.");
    expect(readBytes(averagedPath) == readBytes(accumulationPath),
           "Direct accumulation output is byte-identical to averaged output.");
    std::remove(averagedPath.c_str());
    std::remove(accumulationPath.c_str());

    const std::string batchedPath = "test-batched-render.ppm";
    const std::string passPath = "test-pass-render.ppm";
    Scene renderScene(Vec3(0.2, 0.3, 0.4));
    Camera renderCamera(Vec3(), Vec3(), 4, 4, PI * 0.5);
    ImageWriter batchedWriter(batchedPath);
    ImageWriter passWriter(passPath);
    Renderer batchedRenderer(
        batchedWriter, renderScene, renderCamera,
        RenderSettings(4, 0, 9, 2, 2, 2, 2));
    Renderer passRenderer(
        passWriter, renderScene, renderCamera,
        RenderSettings(4, 1, 9, 2, 2, 2, 2));
    batchedRenderer.render();
    passRenderer.render();
    expect(readBytes(batchedPath) == readBytes(passPath),
           "Tile sample batching preserves deterministic accumulation.");
    std::remove(batchedPath.c_str());
    std::remove(passPath.c_str());
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
    Scene litScene;
    litScene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -1.0),
                  Vec3(1.0, 1.0, 1.0), Vec3(), 0.0, 1.0)));
    litScene.addLight(std::unique_ptr<LightSource>(
        new PointSource(Vec3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0),
                        4.0 * PI * PI)));

    const Vec3 lit = litScene.trace(Ray(Vec3(), Vec3(0.0, 0.0, -1.0)));
    expectVecNear(lit, Vec3(0.97, 0.0, 0.0), 1e-6,
                  "PBR direct lighting includes light color and conserved energy.");

    Scene backgroundScene(Vec3(0.1, 0.2, 0.3));
    const Vec3 background =
        backgroundScene.trace(Ray(Vec3(), Vec3(0.0, 1.0, 0.0)));
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

    expectNear(ImageWriter::applyExposure(0.25, 2.0), 1.0, 1e-9,
               "Exposure is measured in photographic stops.");
    expectNear(ImageWriter::toneMap(1.0, ToneMapper::Reinhard),
               0.5, 1e-9, "Reinhard tone mapping compresses highlights.");
    expect(ImageWriter::toneMap(4.0, ToneMapper::Aces) < 1.0,
           "ACES tone mapping keeps HDR highlights in display range.");
    expect(ImageWriter::toneMap(4.0, ToneMapper::Aces) >
               ImageWriter::toneMap(1.0, ToneMapper::Aces),
           "ACES tone mapping remains monotonic for common HDR values.");
    expect(ImageWriter::toByte(4.0, 0.0, ToneMapper::Reinhard) < 255,
           "Tone mapping preserves detail that direct clamping would lose.");
}

void testDeterministicSampling() {
    const double first =
        Renderer::sampleOffset(42, 3, 1234, 0);
    const double repeated =
        Renderer::sampleOffset(42, 3, 1234, 0);
    const double otherDimension =
        Renderer::sampleOffset(42, 3, 1234, 1);
    const double otherSample =
        Renderer::sampleOffset(42, 4, 1234, 0);

    expectNear(first, repeated, 0.0,
               "Jitter is deterministic for a pixel, sample, and seed.");
    expect(first >= 0.0 && first < 1.0,
           "Horizontal jitter remains inside the pixel.");
    expect(otherDimension >= 0.0 && otherDimension < 1.0,
           "Vertical jitter remains inside the pixel.");
    expect(first != otherDimension,
           "Sampling dimensions receive independent jitter.");
    expect(first != otherSample,
           "Successive samples receive different jitter.");
    expectThrows([]() {
        Scene scene;
        Camera camera(Vec3(), Vec3(), 1, 1, PI * 0.5);
        ImageWriter writer("unused.ppm");
        Renderer invalid(writer, scene, camera, RenderSettings(0));
        (void)invalid;
    }, "Renderers reject zero samples per pixel.");
    expectThrows([]() {
        Scene scene;
        Camera camera(Vec3(), Vec3(), 1, 1, PI * 0.5);
        ImageWriter writer("unused.ppm");
        Renderer invalid(
            writer, scene, camera,
            RenderSettings(1, 0, 1, 1, 0, 0));
        (void)invalid;
    }, "Renderers reject zero maximum bounce depth.");
}

void testMaterialsAndOptics() {
    const Material diffuse =
        Material::diffuse(Vec3(0.2, 0.4, 0.6));
    const Material mirror =
        Material::mirror(Vec3(0.9, 0.9, 0.9));
    const Material glass =
        Material::dielectric(1.5);
    expect(diffuse.type == MaterialType::Diffuse,
           "Diffuse material factory sets the correct type.");
    expect(mirror.type == MaterialType::Mirror,
           "Mirror material factory sets the correct type.");
    expect(glass.type == MaterialType::Dielectric,
           "Dielectric material factory sets the correct type.");
    expectNear(glass.refractiveIndex, 1.5, 1e-9,
               "Dielectric stores its index of refraction.");
    expectThrows([]() {
        const Material invalid =
            Material::dielectric(0.0);
        (void)invalid;
    }, "Dielectrics reject non-positive refractive indices.");
    expectThrows([]() {
        const Material invalid =
            Material::diffuse(Vec3(-0.1, 0.0, 0.0));
        (void)invalid;
    }, "Materials reject negative albedo.");
    expectThrows([]() {
        const Material invalid =
            Material::diffuse(Vec3(1.1, 0.0, 0.0));
        (void)invalid;
    }, "Materials reject albedo above one.");

    expectVecNear(
        Scene::reflect(
            Vec3(1.0, -1.0, 0.0).normalize(),
            Vec3(0.0, 1.0, 0.0)),
        Vec3(1.0, 1.0, 0.0).normalize(), 1e-9,
        "Mirror reflection follows the surface normal.");
    expectVecNear(
        Scene::refract(
            Vec3(0.0, -1.0, 0.0),
            Vec3(0.0, 1.0, 0.0), 1.0 / 1.5),
        Vec3(0.0, -1.0, 0.0), 1e-9,
        "Normal-incidence refraction does not bend.");
    expectNear(
        Scene::schlickReflectance(1.0, 1.0 / 1.5),
        0.04, 1e-9,
        "Schlick Fresnel gives four-percent glass reflectance head-on.");
    expect(!Scene::hasTotalInternalReflection(
               Vec3(0.0, -1.0, 0.0),
               Vec3(0.0, 1.0, 0.0), 1.5),
           "A normal-incidence ray can exit a dielectric.");
    expect(Scene::hasTotalInternalReflection(
               Vec3(0.98, -0.2, 0.0).normalize(),
               Vec3(0.0, 1.0, 0.0), 1.5),
           "A steep inside ray triggers total internal reflection.");
}

void testPhysicallyBasedMaterials() {
    const Material plastic = Material::principled(
        Vec3(0.2, 0.4, 0.8), 0.35, 0.0, 0.0, 1.5);
    const Material metal = Material::principled(
        Vec3(0.9, 0.6, 0.2), 0.2, 1.0, 0.0, 1.5);
    const Material transmission = Material::principled(
        Vec3(1.0, 1.0, 1.0), 0.05, 0.0, 0.8, 1.45);
    expect(plastic.type == MaterialType::Principled,
           "Principled factory creates a PBR material.");
    expectNear(plastic.roughness, 0.35, 1e-9,
               "Principled material stores roughness.");
    expectNear(metal.metallic, 1.0, 1e-9,
               "Principled material stores metallic weight.");
    expectNear(transmission.transmission, 0.8, 1e-9,
               "Principled material stores transmission weight.");
    expectThrows([]() {
        const Material invalid = Material::principled(
            Vec3(0.5, 0.5, 0.5), 1.1, 0.0, 0.0);
        (void)invalid;
    }, "Principled materials reject roughness above one.");

    expectVecNear(
        Bsdf::fresnelSchlick(
            1.0, Vec3(0.04, 0.04, 0.04)),
        Vec3(0.04, 0.04, 0.04), 1e-9,
        "Schlick Fresnel equals F0 at normal incidence.");
    expectVecNear(
        Bsdf::fresnelSchlick(
            0.0, Vec3(0.04, 0.04, 0.04)),
        Vec3(1.0, 1.0, 1.0), 1e-9,
        "Schlick Fresnel approaches one at grazing angles.");
    expect(Bsdf::ggxDistribution(1.0, 0.1) >
               Bsdf::ggxDistribution(1.0, 0.8),
           "Smoother GGX surfaces have sharper normal peaks.");
    const double masking = Bsdf::smithMasking(0.6, 0.7, 0.4);
    expect(masking > 0.0 && masking <= 1.0,
           "Smith masking remains a valid attenuation factor.");

    const Vec3 normal(0.0, 0.0, 1.0);
    const Vec3 view(0.0, 0.0, 1.0);
    const Vec3 light(0.0, 0.0, 1.0);
    const Vec3 plasticValue =
        Bsdf::evaluate(plastic, normal, view, light);
    const Vec3 metalValue =
        Bsdf::evaluate(metal, normal, view, light);
    expect(plasticValue.X() > 0.0 &&
               plasticValue.Y() > plasticValue.X(),
           "PBR plastic retains colored diffuse reflection.");
    expect(metalValue.X() > metalValue.Z(),
           "Metal Fresnel reflection is tinted by base color.");

    Sampler firstSampler(20, 3, 77);
    Sampler secondSampler(20, 3, 77);
    const BsdfSample first =
        Bsdf::sample(plastic, normal, view, true, firstSampler);
    const BsdfSample second =
        Bsdf::sample(plastic, normal, view, true, secondSampler);
    expect(first.valid && first.pdf > 0.0 && !first.delta,
           "Principled opaque BSDF produces a valid sampled lobe.");
    expectVecNear(first.direction, second.direction, 0.0,
                  "GGX importance sampling is deterministic.");
    expectVecNear(first.weight, second.weight, 0.0,
                  "Sampled PBR throughput is deterministic.");
}

void testSecondaryRayTransport() {
    Scene mirrorScene(Vec3(0.2, 0.3, 0.4));
    mirrorScene.addShape(std::unique_ptr<Shape>(
        new Plane(
            Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -1.0),
            Material::mirror())));
    Sampler mirrorSampler(0, 0, 7);
    const Vec3 reflected = mirrorScene.trace(
        Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
        mirrorSampler, PathTraceSettings(2, 99));
    expectVecNear(reflected, Vec3(0.2, 0.3, 0.4), 1e-9,
                  "A perfect mirror reflects environment radiance.");

    Scene emissiveScene;
    emissiveScene.addShape(std::unique_ptr<Shape>(
        new Sphere(
            Vec3(0.0, 0.0, -3.0), 1.0,
            Material::diffuse(
                Vec3(), Vec3(3.0, 2.0, 1.0)))));
    Sampler emissiveSampler(0, 0, 9);
    const Vec3 emitted = emissiveScene.trace(
        Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
        emissiveSampler, PathTraceSettings(1, 99));
    expectVecNear(emitted, Vec3(3.0, 2.0, 1.0), 1e-9,
                  "Emissive materials contribute radiance at a hit.");

    Sampler firstSampler(12, 4, 99);
    Sampler secondSampler(12, 4, 99);
    const Vec3 firstPath = mirrorScene.trace(
        Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
        firstSampler, PathTraceSettings(8, 2));
    const Vec3 secondPath = mirrorScene.trace(
        Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
        secondSampler, PathTraceSettings(8, 2));
    expectVecNear(firstPath, secondPath, 0.0,
                  "Secondary-ray sampling is deterministic.");
}

void testAreaLightsAndEnvironment() {
    const Rectangle rectangle(
        Vec3(0.0, 0.0, -2.0), Vec3(0.0, 0.0, -1.0),
        Vec3(0.0, 1.0, 0.0), 4.0, 2.0,
        Material::diffuse(Vec3(), Vec3(5.0, 5.0, 5.0)));
    Sampler rectangleSampler(2, 3, 4);
    SurfaceSample rectangleSample;
    expect(rectangle.sampleSurface(
               rectangleSampler, rectangleSample),
           "Rectangular emitters support surface sampling.");
    expectNear(rectangleSample.areaPdf, 1.0 / 8.0, 1e-9,
               "Rectangle surface samples use uniform area density.");
    expect(std::abs(rectangleSample.point.X()) <= 2.0 &&
               std::abs(rectangleSample.point.Y()) <= 1.0,
           "Rectangle samples remain inside its bounds.");

    const Sphere sphere(
        Vec3(), 2.0,
        Material::diffuse(Vec3(), Vec3(1.0, 1.0, 1.0)));
    Sampler sphereSampler(5, 6, 7);
    SurfaceSample sphereSample;
    expect(sphere.sampleSurface(sphereSampler, sphereSample),
           "Spherical emitters support surface sampling.");
    expectNear(sphereSample.normal.getLength(), 1.0, 1e-9,
               "Sphere surface samples provide unit normals.");
    expectNear(
        sphereSample.point.distanceTo(Vec3()), 2.0, 1e-9,
        "Sphere samples lie on the sphere.");

    const Environment environment(
        Vec3(0.2, 0.1, 0.0), Vec3(0.0, 0.2, 0.8));
    expectVecNear(
        environment.evaluate(Vec3(0.0, -1.0, 0.0)),
        Vec3(0.2, 0.1, 0.0), 1e-9,
        "Environment horizon color appears below.");
    expectVecNear(
        environment.evaluate(Vec3(0.0, 1.0, 0.0)),
        Vec3(0.0, 0.2, 0.8), 1e-9,
        "Environment zenith color appears above.");
    Sampler environmentSampler(8, 9, 10);
    const EnvironmentSample environmentSample =
        environment.sample(environmentSampler);
    expectNear(environmentSample.direction.getLength(), 1.0, 1e-9,
               "Environment sampling returns unit directions.");
    expectNear(
        environmentSample.pdf, 1.0 / (4.0 * PI), 1e-9,
        "Uniform environment sampling has the expected PDF.");
    expectThrows([]() {
        const Environment invalid(
            Vec3(), Vec3(), -1.0);
        (void)invalid;
    }, "Environment lighting rejects negative intensity.");

    const std::string mapPath = "test-environment.ppm";
    {
        std::ofstream map(mapPath.c_str(), std::ios::binary);
        map << "P6\n2 1\n255\n";
        const unsigned char pixels[6] = {
            255, 0, 0, 0, 0, 255
        };
        map.write(
            reinterpret_cast<const char*>(pixels), sizeof(pixels));
    }
    Environment mapped;
    expect(mapped.loadPpm(mapPath, 2.0),
           "Environment loads a lat-long P6 PPM image.");
    const Vec3 mappedColor =
        mapped.evaluate(Vec3(0.0, 0.0, -1.0));
    expect(mappedColor.X() > 1.9 && mappedColor.Z() < 0.1,
           "Environment map lookup applies image color and intensity.");
    std::remove(mapPath.c_str());

    expectNear(Scene::powerHeuristic(0.5, 0.5), 0.5, 1e-9,
               "MIS power heuristic balances equal PDFs.");
    expect(Scene::powerHeuristic(1.0, 0.1) > 0.98,
           "MIS favors the strategy with the larger PDF.");

    Scene areaLightScene;
    areaLightScene.addShape(std::unique_ptr<Shape>(
        new Plane(
            Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -5.0),
            Material::diffuse(Vec3(0.8, 0.8, 0.8)))));
    areaLightScene.addShape(std::unique_ptr<Shape>(
        new Rectangle(
            Vec3(0.0, 2.0, -3.0), Vec3(0.0, -1.0, 0.0),
            Vec3(0.0, 0.0, -1.0), 2.0, 2.0,
            Material::diffuse(
                Vec3(), Vec3(10.0, 10.0, 10.0)))));
    Sampler areaSampler(1, 1, 123);
    const Vec3 areaLit = areaLightScene.trace(
        Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
        areaSampler, PathTraceSettings(1, 99));
    expect(areaLit.X() > 0.0 && areaLit.Y() > 0.0 &&
               areaLit.Z() > 0.0,
           "Emissive rectangles are sampled as direct area lights.");
}

void testAccelerationStructures() {
    const Aabb box(
        Vec3(-1.0, -1.0, -6.0),
        Vec3(1.0, 1.0, -4.0));
    expect(box.intersect(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               0.0, 100.0),
           "AABB slab traversal accepts a central ray.");
    expect(!box.intersect(
               Ray(Vec3(3.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0)),
               0.0, 100.0),
           "AABB slab traversal rejects a parallel outside ray.");
    const Aabb combined = Aabb::surrounding(
        box, Aabb(Vec3(2.0, -2.0, -7.0),
                  Vec3(3.0, 2.0, -3.0)));
    expectVecNear(
        combined.min(), Vec3(-1.0, -2.0, -7.0), 1e-9,
        "Surrounding AABB keeps component minima.");
    expectVecNear(
        combined.max(), Vec3(3.0, 2.0, -3.0), 1e-9,
        "Surrounding AABB keeps component maxima.");

    Sphere nearSphere(
        Vec3(0.0, 0.0, -4.0), 1.0,
        Material::diffuse(Vec3(1.0, 0.0, 0.0)));
    Sphere farSphere(
        Vec3(0.0, 0.0, -8.0), 1.0,
        Material::diffuse(Vec3(0.0, 1.0, 0.0)));
    std::vector<const Shape*> shapes;
    shapes.push_back(&farSphere);
    shapes.push_back(&nearSphere);
    Bvh bvh;
    bvh.build(shapes);
    HitRecord bvhHit;
    expect(bvh.intersect(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, bvhHit),
           "BVH traversal finds bounded geometry.");
    expect(bvhHit.shape == &nearSphere,
           "BVH traversal returns the nearest shape.");
    expect(bvh.nodeCount() > 0,
           "BVH construction creates traversal nodes.");

    Scene scene;
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(
            Vec3(0.0, 0.0, -4.0), 1.0,
            Material::diffuse(Vec3(1.0, 0.0, 0.0)))));
    scene.addShape(std::unique_ptr<Shape>(
        new Plane(
            Vec3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -10.0),
            Material::diffuse(Vec3(0.5, 0.5, 0.5)))));
    HitRecord acceleratedHit;
    expect(scene.findClosestHit(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, acceleratedHit),
           "Accelerated scene traversal finds a hit.");
    scene.setAccelerationEnabled(false);
    HitRecord bruteForceHit;
    expect(scene.findClosestHit(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, bruteForceHit),
           "Brute-force diagnostic traversal finds a hit.");
    expectNear(
        acceleratedHit.distance, bruteForceHit.distance, 1e-9,
        "BVH and brute-force traversal agree on hit distance.");
    expect(scene.boundedShapeCount() == 1,
           "Infinite planes remain outside the BVH.");

    expectThrows([]() {
        Scene scene;
        Camera camera(Vec3(), Vec3(), 1, 1, PI * 0.5);
        ImageWriter writer("unused.ppm");
        Renderer invalid(
            writer, scene, camera,
            RenderSettings(1, 0, 1, 1, 1, 1, 0));
        (void)invalid;
    }, "Renderers reject zero tile size.");
}

void testSceneCapabilityFeatures() {
    const Ray timedRay(
        Vec3(), Vec3(0.0, 0.0, -1.0), 0.75);
    expectNear(timedRay.time(), 0.75, 1e-9,
               "Rays preserve shutter time.");

    const Camera lensCamera(
        Vec3(), Vec3(), 100, 100, PI * 0.5,
        0.5, 10.0, 0.2, 0.8);
    Sampler lensSampler(4, 5, 6);
    const Ray lensRay =
        lensCamera.makeRay(50.0, 50.0, lensSampler);
    expect(lensRay.origin().distanceTo(Vec3()) > 0.0,
           "Thin-lens camera samples the aperture.");
    expect(lensRay.time() >= 0.2 && lensRay.time() <= 0.8,
           "Camera samples ray time inside the shutter interval.");

    const MovingSphere moving(
        Vec3(-1.0, 0.0, -5.0), Vec3(1.0, 0.0, -5.0),
        0.0, 1.0, 1.0, Material::diffuse(Vec3(1.0, 0.0, 0.0)));
    expectVecNear(moving.centerAt(0.5), Vec3(0.0, 0.0, -5.0), 1e-9,
                  "Moving sphere interpolates through shutter time.");
    HitRecord movingHit;
    expect(moving.intersect(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0), 0.5),
               1e-4, 100.0, movingHit),
           "Motion-blurred geometry intersects at ray time.");

    const CheckerTexture checker(
        Vec3(1.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0), 1.0);
    expect(checker.value(0.0, 0.0, Vec3(1.0, 1.0, 1.0)) !=
               checker.value(0.0, 0.0, Vec3(1.0, 1.0, -1.0)),
           "Checker texture varies procedurally in space.");
    const std::string texturePath = "test-texture.ppm";
    {
        std::ofstream texture(texturePath.c_str(), std::ios::binary);
        texture << "P6\n1 1\n255\n";
        const unsigned char pixel[3] = {32, 64, 128};
        texture.write(reinterpret_cast<const char*>(pixel), 3);
    }
    std::shared_ptr<ImageTexture> imageTexture(new ImageTexture());
    expect(imageTexture->loadPpm(texturePath),
           "Image texture loads P6 PPM data.");
    expectVecNear(
        imageTexture->value(0.5, 0.5, Vec3()),
        Vec3(32.0 / 255.0, 64.0 / 255.0, 128.0 / 255.0),
        1e-9, "Image texture returns UV-addressed color.");
    std::remove(texturePath.c_str());

    const Triangle triangle(
        Vertex(Vec3(-1.0, -1.0, -5.0), Vec3(0.0, 0.0, 1.0), 0.0, 0.0),
        Vertex(Vec3(1.0, -1.0, -5.0), Vec3(0.0, 0.0, 1.0), 1.0, 0.0),
        Vertex(Vec3(0.0, 1.0, -5.0), Vec3(0.0, 0.0, 1.0), 0.5, 1.0),
        Material::diffuse(Vec3(1.0, 1.0, 1.0)));
    HitRecord triangleHit;
    expect(triangle.intersect(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, triangleHit),
           "Triangle supports barycentric ray intersection.");
    expectNear(triangleHit.u, 0.5, 1e-9,
               "Triangle interpolates texture U coordinates.");
    expectNear(triangleHit.v, 0.5, 1e-9,
               "Triangle interpolates texture V coordinates.");

    const std::string objPath = "test-mesh.obj";
    {
        std::ofstream obj(objPath.c_str());
        obj << "v -1 -1 -5\nv 1 -1 -5\nv 1 1 -5\nv -1 1 -5\n";
        obj << "f 1 2 3 4\n";
    }
    ObjMesh mesh(objPath, Material::diffuse(Vec3(0.5, 0.5, 0.5)));
    expect(mesh.triangleCount() == 2,
           "OBJ loader triangulates polygon faces.");
    HitRecord meshHit;
    expect(mesh.intersect(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, meshHit),
           "Loaded OBJ mesh participates in intersections.");
    HitRecord bruteMeshHit;
    expect(mesh.intersectBruteForce(
               Ray(Vec3(), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, bruteMeshHit) &&
               meshHit.distance == bruteMeshHit.distance,
           "Mesh-local BVH agrees with brute-force triangles.");
    std::remove(objPath.c_str());

    Transform transformed(
        std::unique_ptr<Shape>(
            new Sphere(
                Vec3(), 1.0,
                Material::diffuse(Vec3(1.0, 1.0, 1.0)))),
        Vec3(2.0, 0.0, -5.0), Vec3(0.0, 0.3, 0.0),
        Vec3(2.0, 1.0, 1.0));
    HitRecord transformHit;
    expect(transformed.intersect(
               Ray(Vec3(2.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0)),
               1e-4, 100.0, transformHit),
           "Transformed instances support translation, rotation, and scale.");
    Aabb transformedBounds;
    expect(transformed.boundingBox(transformedBounds),
           "Transformed instances produce world-space bounds.");
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
        testDeterministicSampling();
        testMaterialsAndOptics();
        testPhysicallyBasedMaterials();
        testSecondaryRayTransport();
        testAreaLightsAndEnvironment();
        testAccelerationStructures();
        testSceneCapabilityFeatures();
        testPerformanceQuickWins();
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
