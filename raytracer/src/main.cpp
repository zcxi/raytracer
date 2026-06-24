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
#include "Shapes/RectangularPrism.h"

namespace {

struct CommandLineOptions {
    std::string outputPath;
    RenderSettings renderSettings;
    ImageOutputSettings outputSettings;

    CommandLineOptions()
        : outputPath("output.ppm"),
          renderSettings(16, 4, 1, 0),
          outputSettings(0.0, ToneMapper::Aces) {
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
        << "  --threads N       Worker count; 0 uses hardware concurrency\n";
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
        } else {
            throw std::invalid_argument("Unknown option " + argument + ".");
        }
    }
    if (options.renderSettings.samplesPerPixel == 0) {
        throw std::invalid_argument("Samples per pixel must be positive.");
    }
    return options;
}

void demo1(const CommandLineOptions& options) {
    const double pi = 3.14159265358979323846;
    Scene scene(Vec3(0.02, 0.03, 0.05));

    scene.addShape(std::unique_ptr<Shape>(
        new Cube(Vec3(-7, 7, -58), 7.0,
                 Vec3(0.15, 0.65, 1.0), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new RectangularPrism(
            Vec3(7, 7, -58), Vec3(10, 6, 5),
            Vec3(0.2, 1.0, 0.35), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(-7, -7, -58), 4,
                   Vec3(1.0, 0.2, 0.1), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Pyramid(Vec3(7, -11, -59), 10, 7, 10,
                    Vec3(1.0, 0.75, 0.1), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 0, -1), Vec3(0, 0, -64),
                  Vec3(1.0, 0.0, 1.0), Vec3(), 0.0, 1.0)));

    scene.addLight(std::unique_ptr<LightSource>(
        new PointSource(Vec3(40, 40, 10), Vec3(1.0, 1.0, 1.0), 150000)));

    Camera camera(Vec3(0, 0.1, 0), Vec3(),
                  640, 640, pi / 6);
    ImageWriter imageWriter(options.outputPath, options.outputSettings);
    Renderer render(
        imageWriter, scene, camera, options.renderSettings);
    render.render();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const CommandLineOptions options = parseArguments(argc, argv);
        demo1(options);
        std::cout << "Wrote " << options.outputPath << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        printUsage();
        return 1;
    }
}
