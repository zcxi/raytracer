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
    double camX, camY, camZ;
    double camPitch, camYaw, camRoll;
    double fov;

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
          scene("demo"),
          camX(-1), camY(-1), camZ(-1),
          camPitch(-1), camYaw(-1), camRoll(-1),
          fov(-1.0) {
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

Vec3 parseVec3(const std::string& value, const std::string& optionName) {
    std::size_t first = value.find(',');
    std::size_t second = value.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        value.find(',', second + 1) != std::string::npos) {
        throw std::invalid_argument(
            optionName + " requires three comma-separated numbers.");
    }
    return Vec3(
        std::stod(value.substr(0, first)),
        std::stod(value.substr(first + 1, second - first - 1)),
        std::stod(value.substr(second + 1)));
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
        << "  --scene NAME      Scene: demo, chessboard (default: demo)\n"
        << "  --cam-pos X,Y,Z   Camera position (e.g. 8,3,-5)\n"
        << "  --cam-rot P,Y,R   Camera rotation pitch,yaw,roll in radians\n"
        << "  --fov N           Vertical field of view in radians\n";
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
        } else if (argument == "--cam-pos") {
            Vec3 pos = parseVec3(value, argument);
            options.camX = pos.X();
            options.camY = pos.Y();
            options.camZ = pos.Z();
        } else if (argument == "--cam-rot") {
            Vec3 rot = parseVec3(value, argument);
            options.camPitch = rot.X();
            options.camYaw = rot.Y();
            options.camRoll = rot.Z();
        } else if (argument == "--fov") {
            options.fov = std::stod(value);
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

    Vec3 camPos(16, 2, 16);
    Vec3 camRot(0.06, -0.30, 0.0);
    if (options.camX >= 0) {
        camPos = Vec3(options.camX, options.camY, options.camZ);
    }
    if (options.camPitch >= 0) {
        camRot = Vec3(options.camPitch, options.camYaw, options.camRoll);
    }
    double fov = pi / 4;
    if (options.fov > 0) {
        fov = options.fov;
    }
    Camera camera(camPos, camRot,
                  640, 640, fov,
                  options.aperture, options.focusDistance,
                  options.shutterOpen, options.shutterClose);
    ImageWriter imageWriter(options.outputPath, options.outputSettings);
    Renderer render(
        imageWriter, scene, camera, options.renderSettings);
    render.render();
}

void chessboard(const CommandLineOptions& options) {
    const double pi = 3.14159265358979323846;
    const double boardZ = -18.0;
    const double boardY = 1.51;
    Scene scene;
    Environment environment(
        Vec3(0.06, 0.05, 0.06),
        Vec3(0.18, 0.22, 0.35),
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

    const Vec3 woodColor(0.25, 0.15, 0.08);
    scene.addShape(std::unique_ptr<Shape>(
        new RectangularPrism(
            Vec3(0, 0.0, boardZ), Vec3(4.6, 1.5, 4.6),
            Material::principled(woodColor, 0.55, 0.0, 0.0))));
    const Vec3 legColor(0.18, 0.10, 0.05);
    auto leg = [&](double lx, double lz) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(lx, -2.5, lz), Vec3(0.3, 1.0, 0.3),
                Material::principled(legColor, 0.6, 0.0, 0.0))));
    };
    leg(-3.8, boardZ - 3.8);
    leg( 3.8, boardZ - 3.8);
    leg(-3.8, boardZ + 3.8);
    leg( 3.8, boardZ + 3.8);

    const std::shared_ptr<Texture> checker(
        new CheckerTexture(
            Vec3(0.96, 0.94, 0.88),
            Vec3(0.06, 0.06, 0.10), pi * 0.5));

    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 1, 0), Vec3(0, boardY, boardZ),
                  Material::diffuse(Vec3(0.5, 0.5, 0.5))
                      .withTexture(checker))));

    const double py = boardY;
    const Vec3 whitePc(0.96, 0.94, 0.88);
    const Vec3 blackPc(0.06, 0.06, 0.10);
    auto pieceMat = [](const Vec3& c, double rough = 0.25) {
        return Material::principled(c, rough, 0.0, 0.0);
    };

    auto pawn = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, py + 0.35, z), 0.22, pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 0.12, z), Vec3(0.16, 0.12, 0.16),
                pieceMat(col, 0.4))));
    };
    auto rook = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 0.5, z), Vec3(0.20, 0.5, 0.20),
                pieceMat(col))));
    };
    auto knight = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x + 0.08, py + 0.42, z), Vec3(0.22, 0.42, 0.16),
                pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x + 0.15, py + 0.68, z), 0.16,
                       pieceMat(col))));
    };
    auto bishop = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 0.48, z), Vec3(0.18, 0.48, 0.18),
                pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, py + 0.80, z), 0.17, pieceMat(col))));
    };
    auto queen = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 0.55, z), Vec3(0.22, 0.55, 0.22),
                pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, py + 0.90, z), 0.20, pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, py + 1.15, z), 0.10, pieceMat(col))));
    };
    auto king = [&](double x, double z, const Vec3& col) {
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 0.58, z), Vec3(0.22, 0.58, 0.22),
                pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new Sphere(Vec3(x, py + 0.93, z), 0.18, pieceMat(col))));
        scene.addShape(std::unique_ptr<Shape>(
            new RectangularPrism(
                Vec3(x, py + 1.12, z), Vec3(0.06, 0.10, 0.22),
                pieceMat(col, 0.3))));
    };

    auto placeBackRow = [&](double z, const Vec3& col) {
        rook(-3.5, z, col);
        knight(-2.5, z, col);
        bishop(-1.5, z, col);
        queen(-0.5, z, col);
        king(0.5, z, col);
        bishop(1.5, z, col);
        knight(2.5, z, col);
        rook(3.5, z, col);
    };
    auto placePawns = [&](double z, const Vec3& col) {
        for (int i = 0; i < 8; ++i)
            pawn(-3.5 + i, z, col);
    };

    placeBackRow(boardZ - 3.5, blackPc);
    placePawns(boardZ - 2.5, blackPc);
    placePawns(boardZ + 2.5, whitePc);
    placeBackRow(boardZ + 3.5, whitePc);

    scene.addShape(std::unique_ptr<Shape>(
        new Rectangle(
            Vec3(0, 9, boardZ + 2), Vec3(0, -1, 0),
            Vec3(0, 0, -1), 8, 3,
            Material::diffuse(
                Vec3(0.0, 0.0, 0.0),
                Vec3(14.0, 12.0, 9.0)))));
    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 0, -1), Vec3(0, 0, boardZ + 8),
                  Material::principled(
                      Vec3(0.55, 0.50, 0.45),
                      0.88, 0.0, 0.0))));

    scene.finalize();

    Vec3 camPos(8, 5, 3);
    Vec3 camRot(-0.22, 0.22, 0.0);
    if (options.camX >= 0) {
        camPos = Vec3(options.camX, options.camY, options.camZ);
    }
    if (options.camPitch >= 0) {
        camRot = Vec3(options.camPitch, options.camYaw, options.camRoll);
    }
    double fov = pi / 4;
    if (options.fov > 0) {
        fov = options.fov;
    }
    Camera camera(camPos, camRot,
                  800, 600, fov,
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
