#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include "Scene/Scene.h"
#include "Shapes/Sphere.h"
#include "Writer/ImageWriter.h"
#include "Scene/Light/PointSource.h"
#include "Renderer.h"
#include "Shapes/Cube.h"
#include "Shapes/Plane.h"
#include "Shapes/Pyramid.h"
#include "Shapes/Rectangle.h"
#include "Shapes/RectangularPrism.h"
#include "Shapes/MovingSphere.h"
#include "Shapes/ObjMesh.h"
#include "Shapes/Transform.h"
#include "Textures/Texture.h"

namespace {

struct CommandLineOptions {
    std::string outputPath;
    RenderSettings renderSettings;
    ImageOutputSettings outputSettings;
    std::string environmentMap;
    double environmentIntensity;
    bool accelerationEnabled;
    double aperture;
    double focusDistance;
    double shutterOpen;
    double shutterClose;
    std::string scene;

    CommandLineOptions()
        : outputPath("output.ppm"),
          renderSettings(32, 8, 1, 0, 8, 4),
          outputSettings(0.0, ToneMapper::Aces),
          environmentMap(),
          environmentIntensity(1.0),
          accelerationEnabled(true),
          aperture(0.35),
          focusDistance(58.0),
          shutterOpen(0.0),
          shutterClose(1.0),
          scene("demo") {
    }
};

ToneMapper parseToneMapper(const std::string& value) {
    if (value == "none") {
        return ToneMapper::None;
    }
    if (value == "reinhard") {
        return ToneMapper::Reinhard;
    }
    if (value == "aces") {
        return ToneMapper::Aces;
    }
    throw std::invalid_argument(
        "Tone mapper must be none, reinhard, or aces.");
}

unsigned int parseUnsigned(const std::string& value,
                           const std::string& optionName) {
    if (value.empty() || value[0] == '-') {
        throw std::invalid_argument(
            optionName + " requires a non-negative integer.");
    }
    std::size_t parsedCharacters = 0;
    const unsigned long parsed = std::stoul(value, &parsedCharacters);
    if (parsedCharacters != value.size() ||
        parsed > std::numeric_limits<unsigned int>::max()) {
        throw std::invalid_argument(
            optionName + " requires a valid unsigned integer.");
    }
    return static_cast<unsigned int>(parsed);
}

std::uint64_t parseSeed(const std::string& value) {
    if (value.empty() || value[0] == '-') {
        throw std::invalid_argument(
            "--seed requires a non-negative integer.");
    }
    std::size_t parsedCharacters = 0;
    const std::uint64_t parsed = std::stoull(value, &parsedCharacters);
    if (parsedCharacters != value.size()) {
        throw std::invalid_argument(
            "--seed requires a valid unsigned integer.");
    }
    return parsed;
}

void printUsage() {
    std::cout
        << "Usage: raytracer [output.ppm] [options]\n"
        << "  --samples N       Samples per pixel (default: 16)\n"
        << "  --exposure EV     Exposure in stops (default: 0)\n"
        << "  --tone-map NAME   none, reinhard, or aces (default: aces)\n"
        << "  --preview N       Rewrite output every N samples (default: 4)\n"
        << "  --seed N          Deterministic sampling seed (default: 1)\n"
        << "  --threads N       Worker count; 0 uses hardware concurrency\n"
        << "  --bounces N       Maximum path depth (default: 8)\n"
        << "  --rr-start N      Russian roulette start bounce (default: 4)\n"
        << "  --tile-size N     Square render tile size (default: 16)\n"
        << "  --env-map PATH    Lat-long P6 PPM environment map\n"
        << "  --env-intensity N Environment multiplier (default: 1)\n"
        << "  --no-bvh          Disable BVH for diagnostics/benchmarking\n"
        << "  --aperture N      Lens aperture diameter (default: 0.35)\n"
        << "  --focus N         Focus distance (default: 58)\n"
        << "  --shutter-open N  Shutter start time (default: 0)\n"
        << "  --shutter-close N Shutter end time (default: 1)\n"
        << "  --scene NAME      Scene to render: demo, chessboard (default: demo)\n";
}

CommandLineOptions parseArguments(int argc, char* argv[]) {
    CommandLineOptions options;
    int index = 1;
    if (index < argc && argv[index][0] != '-') {
        options.outputPath = argv[index++];
    }

    while (index < argc) {
        const std::string argument = argv[index++];
        if (argument == "--help") {
            printUsage();
            std::exit(0);
        }
        if (argument == "--no-bvh") {
            options.accelerationEnabled = false;
            continue;
        }
        if (index >= argc) {
            throw std::invalid_argument(
                "Missing value for option " + argument + ".");
        }
        const std::string value = argv[index++];
        if (argument == "--samples") {
            options.renderSettings.samplesPerPixel =
                parseUnsigned(value, argument);
        } else if (argument == "--exposure") {
            options.outputSettings.exposure = std::stod(value);
            if (!std::isfinite(options.outputSettings.exposure)) {
                throw std::invalid_argument(
                    "--exposure requires a finite number.");
            }
        } else if (argument == "--tone-map") {
            options.outputSettings.toneMapper = parseToneMapper(value);
        } else if (argument == "--preview") {
            options.renderSettings.previewInterval =
                parseUnsigned(value, argument);
        } else if (argument == "--seed") {
            options.renderSettings.randomSeed = parseSeed(value);
        } else if (argument == "--threads") {
            options.renderSettings.workerCount =
                parseUnsigned(value, argument);
        } else if (argument == "--bounces") {
            options.renderSettings.maxBounces =
                parseUnsigned(value, argument);
        } else if (argument == "--rr-start") {
            options.renderSettings.russianRouletteStart =
                parseUnsigned(value, argument);
        } else if (argument == "--tile-size") {
            options.renderSettings.tileSize =
                parseUnsigned(value, argument);
        } else if (argument == "--env-map") {
            options.environmentMap = value;
        } else if (argument == "--env-intensity") {
            options.environmentIntensity = std::stod(value);
            if (!std::isfinite(options.environmentIntensity) ||
                options.environmentIntensity < 0.0) {
                throw std::invalid_argument(
                    "--env-intensity requires a finite non-negative number.");
            }
        } else if (argument == "--aperture") {
            options.aperture = std::stod(value);
        } else if (argument == "--focus") {
            options.focusDistance = std::stod(value);
        } else if (argument == "--shutter-open") {
            options.shutterOpen = std::stod(value);
        } else if (argument == "--shutter-close") {
            options.shutterClose = std::stod(value);
        } else if (argument == "--scene") {
            options.scene = value;
        } else {
            throw std::invalid_argument("Unknown option " + argument + ".");
        }
    }
    if (options.renderSettings.samplesPerPixel == 0) {
        throw std::invalid_argument("Samples per pixel must be positive.");
    }
    if (options.renderSettings.maxBounces == 0) {
        throw std::invalid_argument("Maximum bounces must be positive.");
    }
    if (options.renderSettings.tileSize == 0) {
        throw std::invalid_argument("Tile size must be positive.");
    }
    if (options.aperture < 0.0 || options.focusDistance <= 0.0 ||
        options.shutterClose < options.shutterOpen) {
        throw std::invalid_argument("Invalid camera lens or shutter options.");
    }
    return options;
}

void demo1(const CommandLineOptions& options) {
    const double pi = 3.14159265358979323846;
    Scene scene;
    Environment environment(
        Vec3(0.08, 0.06, 0.05),
        Vec3(0.12, 0.2, 0.42),
        options.environmentIntensity);
    if (!options.environmentMap.empty() &&
        !environment.loadPpm(
            options.environmentMap, options.environmentIntensity)) {
        throw std::runtime_error(
            "Failed to load environment map " +
            options.environmentMap + ".");
    }
    scene.setEnvironment(environment);
    scene.setAccelerationEnabled(options.accelerationEnabled);
    const std::shared_ptr<Texture> checker(
        new CheckerTexture(
            Vec3(0.12, 0.04, 0.03),
            Vec3(0.55, 0.22, 0.12), 0.35));

    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(-7, 7, -58), 4.0,
                   Material::principled(
                       Vec3(0.92, 0.95, 1.0),
                       0.08, 1.0, 0.0))));
    scene.addShape(std::unique_ptr<Shape>(
        new RectangularPrism(
            Vec3(7, 7, -58), Vec3(10, 6, 5),
            Material::principled(
                Vec3(0.08, 0.65, 0.18),
                0.38, 0.0, 0.0))));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(-7, -7, -58), 4,
                   Material::principled(
                       Vec3(0.98, 0.99, 1.0),
                       0.03, 0.0, 1.0, 1.5))));
    scene.addShape(std::unique_ptr<Shape>(
        new Pyramid(Vec3(7, -11, -59), 10, 7, 10,
                    Material::principled(
                        Vec3(1.0, 0.58, 0.08),
                        0.22, 0.9, 0.0))));
    scene.addShape(std::unique_ptr<Shape>(
        new MovingSphere(
            Vec3(-1.5, -1.0, -52), Vec3(1.5, -1.0, -52),
            0.0, 1.0, 1.2,
            Material::principled(
                Vec3(0.85, 0.15, 0.08), 0.3, 0.0, 0.0))));
    scene.addShape(std::unique_ptr<Shape>(
        new Transform(
            std::unique_ptr<Shape>(
                new ObjMesh(
                    "assets/demo.obj",
                    Material::principled(
                        Vec3(0.2, 0.45, 0.95), 0.25, 0.35, 0.0))),
            Vec3(0, 1, -55), Vec3(0.2, 0.5, 0.15),
            Vec3(1.5, 1.5, 1.5))));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(0, 14, -59), 2.0,
                   Material::diffuse(
                       Vec3(0.0, 0.0, 0.0),
                       Vec3(10.0, 7.0, 3.0)))));
    scene.addShape(std::unique_ptr<Shape>(
        new Rectangle(
            Vec3(0, -17, -54), Vec3(0, 1, 0),
            Vec3(0, 0, -1), 18, 7,
            Material::diffuse(
                Vec3(0.0, 0.0, 0.0),
                Vec3(4.0, 5.0, 8.0)))));
    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 0, -1), Vec3(0, 0, -64),
                  Material::principled(
                      Vec3(0.5, 0.16, 0.12),
                      0.82, 0.0, 0.0).withTexture(checker))));
    scene.finalize();

    Camera camera(Vec3(0, 0.1, 0), Vec3(),
                  640, 640, pi / 6,
                  options.aperture, options.focusDistance,
                  options.shutterOpen, options.shutterClose);
    ImageWriter imageWriter(options.outputPath, options.outputSettings);
    Renderer render(
        imageWriter, scene, camera, options.renderSettings);
    render.render();
}

void chessboard(const CommandLineOptions& options) {
    const double pi = 3.14159265358979323846;
    const double boardCenterZ = -28.0;
    Scene scene;
    Environment environment(
        Vec3(0.02, 0.02, 0.04),
        Vec3(0.0, 0.0, 0.0),
        options.environmentIntensity);
    if (!options.environmentMap.empty() &&
        !environment.loadPpm(
            options.environmentMap, options.environmentIntensity)) {
        throw std::runtime_error(
            "Failed to load environment map " +
            options.environmentMap + ".");
    }
    scene.setEnvironment(environment);
    scene.setAccelerationEnabled(options.accelerationEnabled);

    const std::shared_ptr<Texture> checker(
        new CheckerTexture(
            Vec3(0.92, 0.91, 0.88),
            Vec3(0.08, 0.08, 0.12), pi * 0.5));

    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 1, 0), Vec3(0, -0.5, boardCenterZ),
                  Material::principled(
                      Vec3(0.5, 0.5, 0.5),
                      0.65, 0.0, 0.0).withTexture(checker))));

    const double pieceY = -0.5;
    auto piece = [&](double x, double z, double radius, double height,
                     const Vec3& color, double metallic = 0.0) {
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, pieceY + radius, z), radius,
                       Material::principled(
                           color, 0.35, metallic, 0.0))));
        (void)height;
    };

    piece(-6, boardCenterZ - 6.5, 0.45, 0.0, Vec3(0.95, 0.93, 0.85));
    piece(-2, boardCenterZ - 6.5, 0.40, 0.0, Vec3(0.95, 0.93, 0.85));
    piece( 2, boardCenterZ - 6.5, 0.38, 0.0, Vec3(0.95, 0.93, 0.85));
    piece( 6, boardCenterZ - 6.5, 0.42, 0.0, Vec3(0.95, 0.93, 0.85));
    piece(-6, boardCenterZ - 2.5, 0.35, 0.0, Vec3(0.95, 0.93, 0.85));
    piece(-2, boardCenterZ - 2.5, 0.35, 0.0, Vec3(0.95, 0.93, 0.85));
    piece( 2, boardCenterZ - 2.5, 0.35, 0.0, Vec3(0.95, 0.93, 0.85));
    piece( 6, boardCenterZ - 2.5, 0.35, 0.0, Vec3(0.95, 0.93, 0.85));

    piece(-6, boardCenterZ + 2.5, 0.35, 0.0, Vec3(0.04, 0.04, 0.08));
    piece(-2, boardCenterZ + 2.5, 0.35, 0.0, Vec3(0.04, 0.04, 0.08));
    piece( 2, boardCenterZ + 2.5, 0.35, 0.0, Vec3(0.04, 0.04, 0.08));
    piece( 6, boardCenterZ + 2.5, 0.35, 0.0, Vec3(0.04, 0.04, 0.08));
    piece(-6, boardCenterZ + 6.5, 0.45, 0.0, Vec3(0.04, 0.04, 0.08));
    piece(-2, boardCenterZ + 6.5, 0.40, 0.0, Vec3(0.04, 0.04, 0.08));
    piece( 2, boardCenterZ + 6.5, 0.38, 0.0, Vec3(0.04, 0.04, 0.08));
    piece( 6, boardCenterZ + 6.5, 0.42, 0.0, Vec3(0.04, 0.04, 0.08));

    scene.addShape(std::unique_ptr<Shape>(
        new Rectangle(
            Vec3(0, 8, boardCenterZ), Vec3(0, -1, 0),
            Vec3(0, 0, -1), 14, 3,
            Material::diffuse(
                Vec3(0.0, 0.0, 0.0),
                Vec3(12.0, 11.0, 9.0)))));

    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(8, 3, boardCenterZ + 8), 2.0,
                   Material::principled(
                       Vec3(0.92, 0.95, 1.0),
                       0.08, 1.0, 0.0))));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(-8, 2.2, boardCenterZ + 7), 1.8,
                   Material::principled(
                       Vec3(0.98, 0.99, 1.0),
                       0.03, 0.0, 1.0, 1.5))));

    scene.finalize();

    Camera camera(Vec3(5, 3.5, 0), Vec3(-0.14, 0.17, 0.03),
                  800, 600, pi / 5,
                  options.aperture, options.focusDistance,
                  options.shutterOpen, options.shutterClose);
    ImageWriter imageWriter(options.outputPath, options.outputSettings);
    Renderer render(
        imageWriter, scene, camera, options.renderSettings);
    render.render();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const CommandLineOptions options = parseArguments(argc, argv);
        if (options.scene == "chessboard") {
            chessboard(options);
        } else {
            demo1(options);
        }
        std::cout << "Wrote " << options.outputPath << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        printUsage();
        return 1;
    }
}
