#include "Renderer.h"
#include "SceneDescription/JsonSceneLoader.h"
#include "Writer/ImageWriter.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct CommandLineOptions {
    std::string outputPath;
    std::string sceneFile;
    std::string validateFile;
    std::string environmentMap;
    bool hasOutputPath = false;
    bool samples = false;
    bool preview = false;
    bool seed = false;
    bool workers = false;
    bool bounces = false;
    bool roulette = false;
    bool tileSize = false;
    bool statistics = false;
    bool acceleration = false;
    bool exposure = false;
    bool toneMapper = false;
    bool environmentIntensity = false;
    bool aperture = false;
    bool focusDistance = false;
    bool shutterOpen = false;
    bool shutterClose = false;
    bool cameraPosition = false;
    bool cameraRotation = false;
    bool fov = false;
    bool adaptiveMinSamples = false;
    bool adaptiveMaxSamples = false;
    bool adaptiveRelativeError = false;
    bool adaptiveAbsoluteError = false;
    bool adaptiveLuminanceFloor = false;
    bool adaptiveBatch = false;
    bool noAdaptive = false;
    bool noAutoExposure = false;
    bool targetLuminance = false;
    RenderSettings render;
    ImageOutputSettings output;
    double envIntensity = 1.0;
    Vec3 camPosition;
    Vec3 camRotation;
    double apertureValue = 0.0;
    double focusValue = 1.0;
    double shutterOpenValue = 0.0;
    double shutterCloseValue = 0.0;
    double fovValue = 0.0;
};

ToneMapper parseToneMapper(const std::string& value) {
    if (value == "none") return ToneMapper::None;
    if (value == "reinhard") return ToneMapper::Reinhard;
    if (value == "aces") return ToneMapper::Aces;
    throw std::invalid_argument(
        "Tone mapper must be none, reinhard, or aces.");
}

unsigned int parseUnsigned(
        const std::string& value, const std::string& optionName) {
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

double parseDouble(
        const std::string& value, const std::string& optionName) {
    std::size_t parsedCharacters = 0;
    const double parsed = std::stod(value, &parsedCharacters);
    if (parsedCharacters != value.size() || !std::isfinite(parsed)) {
        throw std::invalid_argument(
            optionName + " requires a finite number.");
    }
    return parsed;
}

Vec3 parseVec3(
        const std::string& value, const std::string& optionName) {
    const std::size_t first = value.find(',');
    const std::size_t second = value.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        value.find(',', second + 1) != std::string::npos) {
        throw std::invalid_argument(
            optionName + " requires three comma-separated numbers.");
    }
    return Vec3(
        parseDouble(value.substr(0, first), optionName),
        parseDouble(
            value.substr(first + 1, second - first - 1), optionName),
        parseDouble(value.substr(second + 1), optionName));
}

std::string executableDirectory(const char* executable) {
    const std::filesystem::path path =
        std::filesystem::absolute(executable);
    return path.has_parent_path()
        ? path.parent_path().string()
        : std::filesystem::current_path().string();
}

std::string bundledScene(
        const std::string& name, const char* executable) {
    const std::string filename =
        name == "demo" ? "demo.json" :
        name == "chessboard" ? "chessboard.json" : std::string();
    if (filename.empty()) {
        throw std::invalid_argument(
            "--scene must be demo or chessboard.");
    }
    const std::filesystem::path current =
        std::filesystem::current_path() / "scenes" / filename;
    if (std::filesystem::exists(current)) {
        return current.string();
    }
    const std::filesystem::path besideExecutable =
        std::filesystem::path(executableDirectory(executable))
            .parent_path() / "scenes" / filename;
    return besideExecutable.string();
}

void printUsage() {
    std::cout
        << "Usage: raytracer [output] [options]\n"
        << "  --scene-file FILE Load a versioned JSON scene\n"
        << "  --validate-scene FILE  Validate without rendering\n"
        << "  --scene NAME      Compatibility alias: demo or chessboard\n"
        << "  --samples N       Override samples per pixel\n"
        << "  --preview N       Override preview interval\n"
        << "  --seed N          Override deterministic seed\n"
        << "  --threads N       Worker count; 0 uses hardware concurrency\n"
        << "  --bounces N       Override maximum path depth\n"
        << "  --rr-start N      Override Russian roulette start\n"
        << "  --tile-size N     Override square tile size\n"
        << "  --exposure EV     Override exposure in stops\n"
        << "  --tone-map NAME   none, reinhard, or aces\n"
        << "  --env-map PATH    Override environment map\n"
        << "  --env-intensity N Override environment intensity\n"
        << "  --no-bvh          Disable BVH for diagnostics\n"
        << "  --stats           Collect detailed tracing counters\n"
        << "  --aperture N      Override lens aperture diameter\n"
        << "  --focus N         Override focus distance\n"
        << "  --shutter-open N  Override shutter start time\n"
        << "  --shutter-close N Override shutter end time\n"
        << "  --cam-pos X,Y,Z   Override camera position\n"
        << "  --cam-rot P,Y,R   Override camera Euler rotation\n"
        << "  --fov N           Override vertical field of view\n"
        << "  --adaptive        Enable adaptive sampling\n"
        << "  --no-adaptive     Disable adaptive sampling from scene defaults\n"
        << "  --min-samples N   Min samples per pixel for adaptive\n"
        << "  --max-samples N   Max samples per pixel for adaptive\n"
        << "  --adaptive-batch N  Samples between convergence checks\n"
        << "  --relative-error N  Relative error threshold (default 0.05)\n"
        << "  --absolute-error N  Absolute error threshold (default 0.005)\n"
        << "  --luminance-floor N Luminance floor for relative error\n"
        << "  --denoise         Enable a-trous denoising\n"
        << "  --no-auto-exposure  Disable automatic exposure\n"
        << "  --target-luminance N Target middle-grey luminance (default 0.18)\n";
}

CommandLineOptions parseArguments(
        int argc, char* argv[]) {
    CommandLineOptions options;
    std::string sceneAlias = "demo";
    int index = 1;
    if (index < argc && argv[index][0] != '-') {
        options.outputPath = argv[index++];
        options.hasOutputPath = true;
    }
    while (index < argc) {
        const std::string argument = argv[index++];
        if (argument == "--help") {
            printUsage();
            std::exit(0);
        }
        if (argument == "--no-bvh") {
            options.acceleration = true;
            continue;
        }
        if (argument == "--stats") {
            options.statistics = true;
            continue;
        }
        if (argument == "--adaptive") {
            options.render.adaptiveSampling = true;
            continue;
        }
        if (argument == "--no-adaptive") {
            options.noAdaptive = true;
            continue;
        }
        if (argument == "--denoise") {
            options.render.denoise.enabled = true;
            continue;
        }
        if (argument == "--no-auto-exposure") {
            options.noAutoExposure = true;
            continue;
        }
        if (index >= argc) {
            throw std::invalid_argument(
                "Missing value for option " + argument + ".");
        }
        const std::string value = argv[index++];
        if (argument == "--scene-file") {
            options.sceneFile = value;
        } else if (argument == "--validate-scene") {
            options.validateFile = value;
        } else if (argument == "--scene") {
            sceneAlias = value;
        } else if (argument == "--samples") {
            options.render.samplesPerPixel =
                parseUnsigned(value, argument);
            options.samples = true;
        } else if (argument == "--preview") {
            options.render.previewInterval =
                parseUnsigned(value, argument);
            options.preview = true;
        } else if (argument == "--seed") {
            options.render.randomSeed = parseSeed(value);
            options.seed = true;
        } else if (argument == "--threads") {
            options.render.workerCount = parseUnsigned(value, argument);
            options.workers = true;
        } else if (argument == "--bounces") {
            options.render.maxBounces = parseUnsigned(value, argument);
            options.bounces = true;
        } else if (argument == "--rr-start") {
            options.render.russianRouletteStart =
                parseUnsigned(value, argument);
            options.roulette = true;
        } else if (argument == "--tile-size") {
            options.render.tileSize = parseUnsigned(value, argument);
            options.tileSize = true;
        } else if (argument == "--exposure") {
            options.output.exposure = parseDouble(value, argument);
            options.exposure = true;
        } else if (argument == "--tone-map") {
            options.output.toneMapper = parseToneMapper(value);
            options.toneMapper = true;
        } else if (argument == "--env-map") {
            options.environmentMap = value;
        } else if (argument == "--env-intensity") {
            options.envIntensity = parseDouble(value, argument);
            options.environmentIntensity = true;
        } else if (argument == "--aperture") {
            options.apertureValue = parseDouble(value, argument);
            options.aperture = true;
        } else if (argument == "--focus") {
            options.focusValue = parseDouble(value, argument);
            options.focusDistance = true;
        } else if (argument == "--shutter-open") {
            options.shutterOpenValue = parseDouble(value, argument);
            options.shutterOpen = true;
        } else if (argument == "--shutter-close") {
            options.shutterCloseValue = parseDouble(value, argument);
            options.shutterClose = true;
        } else if (argument == "--cam-pos") {
            options.camPosition = parseVec3(value, argument);
            options.cameraPosition = true;
        } else if (argument == "--cam-rot") {
            options.camRotation = parseVec3(value, argument);
            options.cameraRotation = true;
        } else if (argument == "--fov") {
            options.fovValue = parseDouble(value, argument);
            options.fov = true;
        } else if (argument == "--min-samples") {
            options.render.adaptiveMinSamples =
                parseUnsigned(value, argument);
            options.adaptiveMinSamples = true;
        } else if (argument == "--max-samples") {
            options.render.adaptiveMaxSamples =
                parseUnsigned(value, argument);
            options.adaptiveMaxSamples = true;
        } else if (argument == "--adaptive-batch") {
            options.render.adaptiveBatch =
                parseUnsigned(value, argument);
            options.adaptiveBatch = true;
        } else if (argument == "--relative-error") {
            options.render.adaptiveRelativeError =
                parseDouble(value, argument);
            options.adaptiveRelativeError = true;
        } else if (argument == "--absolute-error") {
            options.render.adaptiveAbsoluteError =
                parseDouble(value, argument);
            options.adaptiveAbsoluteError = true;
        } else if (argument == "--luminance-floor") {
            options.render.adaptiveLuminanceFloor =
                parseDouble(value, argument);
            options.adaptiveLuminanceFloor = true;
        } else if (argument == "--target-luminance") {
            options.output.targetLuminance = parseDouble(value, argument);
            options.targetLuminance = true;
        } else {
            throw std::invalid_argument("Unknown option " + argument + ".");
        }
    }
    if (options.sceneFile.empty() && options.validateFile.empty()) {
        options.sceneFile = bundledScene(sceneAlias, argv[0]);
    }
    return options;
}

void applyOverrides(
        LoadedScene& loaded, const CommandLineOptions& options) {
    if (options.samples) {
        loaded.renderSettings.samplesPerPixel =
            options.render.samplesPerPixel;
    }
    if (options.preview) {
        loaded.renderSettings.previewInterval =
            options.render.previewInterval;
    }
    if (options.seed) {
        loaded.renderSettings.randomSeed = options.render.randomSeed;
    }
    if (options.workers) {
        loaded.renderSettings.workerCount = options.render.workerCount;
    }
    if (options.bounces) {
        loaded.renderSettings.maxBounces = options.render.maxBounces;
    }
    if (options.roulette) {
        loaded.renderSettings.russianRouletteStart =
            options.render.russianRouletteStart;
    }
    if (options.tileSize) {
        loaded.renderSettings.tileSize = options.render.tileSize;
    }
    if (options.statistics) {
        loaded.renderSettings.collectStats = true;
    }
    if (options.noAdaptive) {
        loaded.renderSettings.adaptiveSampling = false;
    } else if (options.render.adaptiveSampling) {
        loaded.renderSettings.adaptiveSampling = true;
    }
    if (options.adaptiveMinSamples) {
        loaded.renderSettings.adaptiveMinSamples =
            options.render.adaptiveMinSamples;
    }
    if (options.adaptiveMaxSamples) {
        loaded.renderSettings.adaptiveMaxSamples =
            options.render.adaptiveMaxSamples;
    }
    if (options.adaptiveBatch) {
        loaded.renderSettings.adaptiveBatch =
            options.render.adaptiveBatch;
    }
    if (options.adaptiveRelativeError) {
        loaded.renderSettings.adaptiveRelativeError =
            options.render.adaptiveRelativeError;
    }
    if (options.adaptiveAbsoluteError) {
        loaded.renderSettings.adaptiveAbsoluteError =
            options.render.adaptiveAbsoluteError;
    }
    if (options.adaptiveLuminanceFloor) {
        loaded.renderSettings.adaptiveLuminanceFloor =
            options.render.adaptiveLuminanceFloor;
    }
    if (options.render.denoise.enabled) {
        loaded.renderSettings.denoise.enabled = true;
    }
    if (options.exposure) {
        loaded.outputSettings.exposure = options.output.exposure;
    }
    if (options.noAutoExposure) {
        loaded.outputSettings.autoExposure = false;
    }
    if (options.targetLuminance) {
        loaded.outputSettings.targetLuminance =
            options.output.targetLuminance;
    }
    if (options.toneMapper) {
        loaded.outputSettings.toneMapper = options.output.toneMapper;
    }
    if (options.hasOutputPath) {
        loaded.outputPath = options.outputPath;
    }
    loaded.scene->setAccelerationEnabled(!options.acceleration);
    if (options.environmentIntensity) {
        loaded.scene->setEnvironmentIntensity(options.envIntensity);
    }
    if (!options.environmentMap.empty()) {
        const double intensity = options.environmentIntensity
            ? options.envIntensity : 1.0;
        if (!loaded.scene->loadEnvironmentMap(
                options.environmentMap, intensity)) {
            throw std::runtime_error(
                "Failed to load environment map " +
                options.environmentMap + ".");
        }
    }

    const Camera& camera = *loaded.camera;
    loaded.camera.reset(new Camera(
        options.cameraPosition
            ? options.camPosition : camera.getPosition(),
        options.cameraRotation
            ? options.camRotation : camera.getRotation(),
        camera.getImageWidth(), camera.getImageHeight(),
        options.fov ? options.fovValue : camera.getFov(),
        options.aperture ? options.apertureValue : camera.getAperture(),
        options.focusDistance
            ? options.focusValue : camera.getFocusDistance(),
        options.shutterOpen
            ? options.shutterOpenValue : camera.getShutterOpen(),
        options.shutterClose
            ? options.shutterCloseValue : camera.getShutterClose()));
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const CommandLineOptions options = parseArguments(argc, argv);
        if (!options.validateFile.empty()) {
            JsonSceneLoader::validate(options.validateFile);
            std::cout << "Scene is valid: "
                      << options.validateFile << std::endl;
            return 0;
        }

        LoadedScene loaded = JsonSceneLoader::load(options.sceneFile);
        applyOverrides(loaded, options);
        std::cout << "Loaded " << options.sceneFile
                  << ": " << loaded.summary.objects << " objects, "
                  << loaded.summary.lights << " analytic lights, "
                  << loaded.summary.materials << " materials, "
                  << loaded.summary.textures << " textures, "
                  << loaded.scene->bvhNodeCount() << " BVH nodes."
                  << std::endl;

        ImageWriter writer(loaded.outputPath, loaded.outputSettings);
        Renderer renderer(
            writer, *loaded.scene, *loaded.camera,
            loaded.renderSettings);
        renderer.render();
        std::cout << "Wrote " << loaded.outputPath << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        printUsage();
        return 1;
    }
}
