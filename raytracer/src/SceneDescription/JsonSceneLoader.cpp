#include "JsonSceneLoader.h"

#include "../Materials/Material.h"
#include "../Scene/Environment.h"
#include "../Scene/Light/DirectionalSource.h"
#include "../Scene/Light/PointSource.h"
#include "../Scene/Light/SpotSource.h"
#include "../Shapes/Cube.h"
#include "../Shapes/Lathe.h"
#include "../Shapes/MovingSphere.h"
#include "../Shapes/ObjMesh.h"
#include "../Shapes/Plane.h"
#include "../Shapes/Pyramid.h"
#include "../Shapes/Rectangle.h"
#include "../Shapes/RectangularPrism.h"
#include "../Shapes/RoundedBox.h"
#include "../Shapes/Sphere.h"
#include "../Shapes/Transform.h"
#include "../Shapes/Triangle.h"
#include "../Textures/Texture.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using Json = nlohmann::json;
namespace fs = std::filesystem;

[[noreturn]] void fail(
        const fs::path& scenePath, const std::string& field,
        const std::string& message) {
    throw std::runtime_error(
        scenePath.string() + ": " + field + ": " + message);
}

Json readJson(const fs::path& path) {
    std::ifstream input(path);
    if (!input) {
        fail(path, "$", "failed to open scene file");
    }
    try {
        return Json::parse(input);
    } catch (const Json::parse_error& error) {
        fail(path, "$", std::string("invalid JSON: ") + error.what());
    }
}

void mergeNamed(Json& destination, const Json& source,
                const char* key, const fs::path& path) {
    if (!source.contains(key)) {
        return;
    }
    if (!source.at(key).is_object()) {
        fail(path, key, "must be an object");
    }
    if (!destination.contains(key)) {
        destination[key] = Json::object();
    }
    for (const auto& [name, value] : source.at(key).items()) {
        if (destination[key].contains(name)) {
            fail(path, std::string(key) + "." + name,
                 "duplicate name across includes");
        }
        destination[key][name] = value;
    }
}

void mergeArray(Json& destination, const Json& source,
                const char* key, const fs::path& path) {
    if (!source.contains(key)) {
        return;
    }
    if (!source.at(key).is_array()) {
        fail(path, key, "must be an array");
    }
    if (!destination.contains(key)) {
        destination[key] = Json::array();
    }
    for (const Json& value : source.at(key)) {
        destination[key].push_back(value);
    }
}

void makeAssetPathsAbsolute(Json& value, const fs::path& sourcePath) {
    if (value.is_array()) {
        for (Json& element : value) {
            makeAssetPathsAbsolute(element, sourcePath);
        }
        return;
    }
    if (!value.is_object()) {
        return;
    }

    const bool assetResource =
        value.contains("type") && value.at("type").is_string() &&
        (value.at("type").get<std::string>() == "image" ||
         value.at("type").get<std::string>() == "obj");
    if (assetResource && value.contains("path") &&
        value.at("path").is_string()) {
        fs::path assetPath(value.at("path").get<std::string>());
        if (!assetPath.is_absolute()) {
            value["path"] =
                (sourcePath.parent_path() / assetPath)
                    .lexically_normal().string();
        }
    }
    for (auto& [name, child] : value.items()) {
        if (name != "includes") {
            makeAssetPathsAbsolute(child, sourcePath);
        }
    }
}

Json loadWithIncludes(
        const fs::path& inputPath,
        std::unordered_set<std::string>& activePaths) {
    const fs::path path = fs::weakly_canonical(inputPath);
    const std::string key = path.generic_string();
    if (!activePaths.insert(key).second) {
        fail(path, "includes", "include cycle detected");
    }
    Json document = readJson(path);
    if (!document.is_object()) {
        fail(path, "$", "top-level value must be an object");
    }
    makeAssetPathsAbsolute(document, path);

    Json merged = Json::object();
    if (document.contains("includes")) {
        if (!document.at("includes").is_array()) {
            fail(path, "includes", "must be an array");
        }
        for (std::size_t index = 0;
             index < document.at("includes").size(); ++index) {
            const Json& include = document.at("includes").at(index);
            if (!include.is_string()) {
                fail(path, "includes[" + std::to_string(index) + "]",
                     "must be a string path");
            }
            const fs::path includePath =
                path.parent_path() / include.get<std::string>();
            const Json child = loadWithIncludes(includePath, activePaths);
            mergeNamed(merged, child, "textures", path);
            mergeNamed(merged, child, "materials", path);
            mergeNamed(merged, child, "prototypes", path);
            mergeArray(merged, child, "lights", path);
            mergeArray(merged, child, "objects", path);
        }
    }

    for (const char* name : {
             "version", "render", "camera", "environment"}) {
        if (document.contains(name)) {
            merged[name] = document.at(name);
        }
    }
    mergeNamed(merged, document, "textures", path);
    mergeNamed(merged, document, "materials", path);
    mergeNamed(merged, document, "prototypes", path);
    mergeArray(merged, document, "lights", path);
    mergeArray(merged, document, "objects", path);
    activePaths.erase(key);
    return merged;
}

const Json& required(
        const Json& object, const char* key,
        const fs::path& path, std::string_view context) {
    if (!object.contains(key)) {
        fail(path, std::string(context) + "." + key,
             "required field is missing");
    }
    return object.at(key);
}

double number(
        const Json& value, const fs::path& path,
        const std::string& field) {
    if (!value.is_number()) {
        fail(path, field, "must be a number but received " +
             std::string(value.type_name()));
    }
    const double result = value.get<double>();
    if (!std::isfinite(result)) {
        fail(path, field, "must be finite but received " +
             std::to_string(result));
    }
    return result;
}

double optionalNumber(
        const Json& object, const char* key, double fallback,
        const fs::path& path, const std::string& context) {
    return object.contains(key)
        ? number(object.at(key), path, context + "." + key)
        : fallback;
}

unsigned int unsignedNumber(
        const Json& value, const fs::path& path,
        const std::string& field, bool allowZero = true) {
    const double parsed = number(value, path, field);
    if (parsed < 0.0 || std::floor(parsed) != parsed ||
        parsed > std::numeric_limits<unsigned int>::max() ||
        (!allowZero && parsed == 0.0)) {
        fail(path, field,
             allowZero
                 ? "must be a non-negative integer but received " +
                       std::to_string(parsed)
                 : "must be a positive integer but received " +
                       std::to_string(parsed));
    }
    return static_cast<unsigned int>(parsed);
}

std::string stringValue(
        const Json& value, const fs::path& path,
        const std::string& field) {
    if (!value.is_string()) {
        fail(path, field, "must be a string but received " +
             std::string(value.type_name()));
    }
    return value.get<std::string>();
}

Vec3 vec3(
        const Json& value, const fs::path& path,
        const std::string& field) {
    if (!value.is_array()) {
        fail(path, field, "must be an array of three numbers but received " +
             std::string(value.type_name()));
    }
    if (value.size() != 3) {
        fail(path, field, "must be an array of three numbers but has " +
             std::to_string(value.size()) + " elements");
    }
    return Vec3(
        number(value.at(0), path, field + "[0]"),
        number(value.at(1), path, field + "[1]"),
        number(value.at(2), path, field + "[2]"));
}

Vec3 optionalVec3(
        const Json& object, const char* key, const Vec3& fallback,
        const fs::path& path, const std::string& context) {
    return object.contains(key)
        ? vec3(object.at(key), path, context + "." + key)
        : fallback;
}

bool optionalBool(
        const Json& object, const char* key, bool fallback,
        const fs::path& path, const std::string& context) {
    if (!object.contains(key)) {
        return fallback;
    }
    if (!object.at(key).is_boolean()) {
        fail(path, context + "." + key, "must be a boolean");
    }
    return object.at(key).get<bool>();
}

ToneMapper parseToneMapper(
        const std::string& value, const fs::path& path,
        const std::string& field) {
    if (value == "none") return ToneMapper::None;
    if (value == "reinhard") return ToneMapper::Reinhard;
    if (value == "aces") return ToneMapper::Aces;
    fail(path, field, "must be none, reinhard, or aces");
}

fs::path resolvePath(
        const fs::path& scenePath, const std::string& value) {
    const fs::path path(value);
    return path.is_absolute()
        ? path
        : (scenePath.parent_path() / path).lexically_normal();
}

struct Loader {
    fs::path path;
    fs::path directory;
    Json document;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, Material> materials;
    std::unordered_map<std::string, Json> prototypes;
    SceneLoadSummary summary;

    explicit Loader(const fs::path& inputPath)
        : path(fs::weakly_canonical(inputPath)),
          directory(path.parent_path()),
          document(),
          textures(),
          materials(),
          prototypes(),
          summary() {
        std::unordered_set<std::string> activePaths;
        document = loadWithIncludes(path, activePaths);
    }

    void validateVersion() const {
        const Json& versionValue = required(
            document, "version", path, "$");
        const unsigned int version = unsignedNumber(
            versionValue, path, "version", false);
        if (version != 1) {
            fail(path, "version", "unsupported scene version " +
                 std::to_string(version));
        }
    }

    void loadTextures() {
        if (!document.contains("textures")) return;
        const Json& values = document.at("textures");
        if (!values.is_object()) {
            fail(path, "textures", "must be an object");
        }
        for (const auto& [name, value] : values.items()) {
            const std::string context = "textures." + name;
            if (!value.is_object()) fail(path, context, "must be an object");
            const std::string type = stringValue(
                required(value, "type", path, context),
                path, context + ".type");
            std::shared_ptr<Texture> texture;
            if (type == "solid") {
                texture = std::make_shared<SolidTexture>(vec3(
                    required(value, "color", path, context),
                    path, context + ".color"));
            } else if (type == "checker") {
                texture = std::make_shared<CheckerTexture>(
                    vec3(required(value, "first", path, context),
                         path, context + ".first"),
                    vec3(required(value, "second", path, context),
                         path, context + ".second"),
                    number(required(value, "scale", path, context),
                           path, context + ".scale"));
            } else if (type == "image") {
                const fs::path imagePath = resolvePath(
                    path, stringValue(
                        required(value, "path", path, context),
                        path, context + ".path"));
                std::shared_ptr<ImageTexture> image =
                    std::make_shared<ImageTexture>();
                if (!image->loadPpm(imagePath.string())) {
                    fail(path, context + ".path",
                         "failed to load PPM texture " +
                         imagePath.string());
                }
                texture = image;
            } else {
                fail(path, context + ".type",
                     "unknown texture type \"" + type + "\"");
            }
            textures.emplace(name, std::move(texture));
        }
        summary.textures = textures.size();
    }

    void loadMaterials() {
        if (!document.contains("materials")) return;
        const Json& values = document.at("materials");
        if (!values.is_object()) {
            fail(path, "materials", "must be an object");
        }
        for (const auto& [name, value] : values.items()) {
            const std::string context = "materials." + name;
            if (!value.is_object()) fail(path, context, "must be an object");
            const std::string type = stringValue(
                required(value, "type", path, context),
                path, context + ".type");
            const Vec3 emission = optionalVec3(
                value, "emission", Vec3(), path, context);
            Material material;
            if (type == "diffuse") {
                material = Material::diffuse(vec3(
                    required(value, "albedo", path, context),
                    path, context + ".albedo"), emission);
            } else if (type == "mirror") {
                material = Material::mirror(optionalVec3(
                    value, "tint", Vec3(1.0, 1.0, 1.0),
                    path, context), emission);
            } else if (type == "dielectric") {
                material = Material::dielectric(
                    optionalNumber(value, "ior", 1.5, path, context),
                    optionalVec3(
                        value, "tint", Vec3(1.0, 1.0, 1.0),
                        path, context),
                    emission);
            } else if (type == "principled") {
                material = Material::principled(
                    vec3(required(value, "baseColor", path, context),
                         path, context + ".baseColor"),
                    optionalNumber(value, "roughness", 0.5, path, context),
                    optionalNumber(value, "metallic", 0.0, path, context),
                    optionalNumber(value, "transmission", 0.0, path, context),
                    optionalNumber(value, "ior", 1.5, path, context),
                    emission,
                    optionalNumber(value, "clearcoat", 0.0, path, context),
                    optionalNumber(
                        value, "clearcoatRoughness", 0.1, path, context));
            } else {
                fail(path, context + ".type",
                     "unknown material type \"" + type + "\"");
            }
            if (value.contains("texture")) {
                const std::string textureName = stringValue(
                    value.at("texture"), path, context + ".texture");
                const auto found = textures.find(textureName);
                if (found == textures.end()) {
                    fail(path, context + ".texture",
                         "references unknown texture \"" +
                         textureName + "\"");
                }
                material = material.withTexture(found->second);
            }
            auto textureByName = [&](const char* key)
                    -> std::shared_ptr<Texture> {
                if (!value.contains(key)) return std::shared_ptr<Texture>();
                const std::string textureName = stringValue(
                    value.at(key), path, context + "." + key);
                const auto found = textures.find(textureName);
                if (found == textures.end()) {
                    fail(path, context + "." + key,
                         "references unknown texture \"" +
                         textureName + "\"");
                }
                return found->second;
            };
            if (value.contains("baseColorTexture")) {
                material = material.withTexture(
                    textureByName("baseColorTexture"));
            }
            if (value.contains("roughnessTexture")) {
                material = material.withRoughnessTexture(
                    textureByName("roughnessTexture"));
            }
            if (value.contains("metallicTexture")) {
                material = material.withMetallicTexture(
                    textureByName("metallicTexture"));
            }
            if (value.contains("normalTexture")) {
                material = material.withNormalTexture(
                    textureByName("normalTexture"),
                    optionalNumber(
                        value, "normalStrength", 1.0, path, context));
            }
            materials.emplace(name, material);
        }
        summary.materials = materials.size();
    }

    const Material& material(
            const Json& object, const std::string& context) const {
        const std::string name = stringValue(
            required(object, "material", path, context),
            path, context + ".material");
        const auto found = materials.find(name);
        if (found == materials.end()) {
            fail(path, context + ".material",
                 "references unknown material \"" + name + "\"");
        }
        return found->second;
    }

    Vertex vertex(
            const Json& value, const Vec3& offset,
            const std::string& context) const {
        if (!value.is_object()) fail(path, context, "must be an object");
        return Vertex(
            vec3(required(value, "position", path, context),
                 path, context + ".position") + offset,
            optionalVec3(value, "normal", Vec3(), path, context),
            optionalNumber(value, "u", 0.0, path, context),
            optionalNumber(value, "v", 0.0, path, context));
    }

    std::unique_ptr<Shape> shape(
            const Json& object, const Vec3& offset,
            const std::string& context) const {
        const std::string type = stringValue(
            required(object, "type", path, context),
            path, context + ".type");
        const Material& selectedMaterial = material(object, context);
        const bool hasTransform = object.contains("transform");
        const Vec3 geometryOffset =
            hasTransform ? Vec3() : offset;
        std::unique_ptr<Shape> result;
        if (type == "sphere") {
            result.reset(new Sphere(
                vec3(required(object, "center", path, context),
                     path, context + ".center") + geometryOffset,
                number(required(object, "radius", path, context),
                       path, context + ".radius"),
                selectedMaterial));
        } else if (type == "movingSphere") {
            result.reset(new MovingSphere(
                vec3(required(object, "startCenter", path, context),
                     path, context + ".startCenter") + geometryOffset,
                vec3(required(object, "endCenter", path, context),
                     path, context + ".endCenter") + geometryOffset,
                number(required(object, "startTime", path, context),
                       path, context + ".startTime"),
                number(required(object, "endTime", path, context),
                       path, context + ".endTime"),
                number(required(object, "radius", path, context),
                       path, context + ".radius"),
                selectedMaterial));
        } else if (type == "plane") {
            result.reset(new Plane(
                vec3(required(object, "normal", path, context),
                     path, context + ".normal"),
                vec3(required(object, "point", path, context),
                     path, context + ".point") + geometryOffset,
                selectedMaterial));
        } else if (type == "rectangle") {
            result.reset(new Rectangle(
                vec3(required(object, "center", path, context),
                     path, context + ".center") + geometryOffset,
                vec3(required(object, "normal", path, context),
                     path, context + ".normal"),
                vec3(required(object, "up", path, context),
                     path, context + ".up"),
                number(required(object, "width", path, context),
                       path, context + ".width"),
                number(required(object, "height", path, context),
                       path, context + ".height"),
                selectedMaterial));
        } else if (type == "cube") {
            result.reset(new Cube(
                vec3(required(object, "center", path, context),
                     path, context + ".center") + geometryOffset,
                number(required(object, "size", path, context),
                       path, context + ".size"),
                selectedMaterial));
        } else if (type == "rectangularPrism") {
            result.reset(new RectangularPrism(
                vec3(required(object, "center", path, context),
                     path, context + ".center") + geometryOffset,
                vec3(required(object, "dimensions", path, context),
                     path, context + ".dimensions"),
                selectedMaterial));
        } else if (type == "roundedBox") {
            result.reset(new RoundedBox(
                vec3(required(object, "center", path, context),
                     path, context + ".center") + geometryOffset,
                vec3(required(object, "dimensions", path, context),
                     path, context + ".dimensions"),
                number(required(object, "radius", path, context),
                       path, context + ".radius"),
                selectedMaterial));
        } else if (type == "lathe") {
            const Json& profileValue =
                required(object, "profile", path, context);
            if (!profileValue.is_array() || profileValue.size() < 2) {
                fail(path, context + ".profile",
                     "must be an array with at least two [radius,height] points");
            }
            std::vector<Lathe::ProfilePoint> profile;
            profile.reserve(profileValue.size());
            for (std::size_t index = 0;
                 index < profileValue.size(); ++index) {
                const Json& point = profileValue.at(index);
                if (!point.is_array() || point.size() != 2) {
                    fail(path, context + ".profile[" +
                         std::to_string(index) + "]",
                         "must be [radius,height]");
                }
                profile.emplace_back(
                    number(point.at(0), path, context + ".profile[" +
                           std::to_string(index) + "][0]"),
                    number(point.at(1), path, context + ".profile[" +
                           std::to_string(index) + "][1]"));
            }
            result.reset(new Lathe(
                profile,
                object.contains("segments")
                    ? unsignedNumber(
                          object.at("segments"), path,
                          context + ".segments", false)
                    : 48,
                selectedMaterial));
        } else if (type == "pyramid") {
            result.reset(new Pyramid(
                vec3(required(object, "baseCenter", path, context),
                     path, context + ".baseCenter") + geometryOffset,
                number(required(object, "width", path, context),
                       path, context + ".width"),
                number(required(object, "depth", path, context),
                       path, context + ".depth"),
                number(required(object, "height", path, context),
                       path, context + ".height"),
                selectedMaterial));
        } else if (type == "triangle") {
            result.reset(new Triangle(
                vertex(required(object, "first", path, context),
                       geometryOffset, context + ".first"),
                vertex(required(object, "second", path, context),
                       geometryOffset, context + ".second"),
                vertex(required(object, "third", path, context),
                       geometryOffset, context + ".third"),
                selectedMaterial));
        } else if (type == "obj") {
            const fs::path meshPath = resolvePath(
                path, stringValue(
                    required(object, "path", path, context),
                    path, context + ".path"));
            result.reset(new ObjMesh(meshPath.string(), selectedMaterial));
        } else {
            fail(path, context + ".type",
                 "unknown shape type \"" + type + "\"");
        }

        if (hasTransform || type == "obj" || type == "lathe") {
            const Json transform = object.value(
                "transform", Json::object());
            if (!transform.is_object()) {
                fail(path, context + ".transform", "must be an object");
            }
            const Vec3 translation = optionalVec3(
                transform, "translation", Vec3(), path,
                context + ".transform") + offset;
            const Vec3 rotation = optionalVec3(
                transform, "rotation", Vec3(), path,
                context + ".transform");
            const Vec3 scale = optionalVec3(
                transform, "scale", Vec3(1.0, 1.0, 1.0),
                path, context + ".transform");
            result.reset(new Transform(
                std::move(result), translation, rotation, scale));
        }
        return result;
    }

    void addObject(
            Scene& scene, const Json& object, const Vec3& offset,
            const std::string& context, unsigned int depth = 0,
            const std::string& materialOverride = std::string()) {
        if (depth > 64) {
            fail(path, context, "prototype/group nesting is too deep");
        }
        if (!object.is_object()) fail(path, context, "must be an object");
        const std::string type = stringValue(
            required(object, "type", path, context),
            path, context + ".type");
        if (type == "group") {
            const Vec3 groupOffset = offset + optionalVec3(
                object, "translation", Vec3(), path, context);
            const Json& children = required(
                object, "children", path, context);
            if (!children.is_array()) {
                fail(path, context + ".children", "must be an array");
            }
            for (std::size_t index = 0; index < children.size(); ++index) {
                addObject(
                    scene, children.at(index), groupOffset,
                    context + ".children[" + std::to_string(index) + "]",
                    depth + 1, materialOverride);
            }
            return;
        }
        if (type == "instance") {
            const std::string prototypeName = stringValue(
                required(object, "prototype", path, context),
                path, context + ".prototype");
            const auto found = prototypes.find(prototypeName);
            if (found == prototypes.end()) {
                fail(path, context + ".prototype",
                     "references unknown prototype \"" +
                     prototypeName + "\"");
            }
            addObject(
                scene, found->second,
                offset + optionalVec3(
                    object, "translation", Vec3(), path, context),
                context + "<" + prototypeName + ">", depth + 1,
                object.contains("material")
                    ? stringValue(
                          object.at("material"), path,
                          context + ".material")
                    : materialOverride);
            return;
        }
        Json resolvedObject = object;
        if (!materialOverride.empty()) {
            resolvedObject["material"] = materialOverride;
        }
        scene.addShape(shape(resolvedObject, offset, context));
        ++summary.objects;
    }

    LoadedScene load() {
        validateVersion();
        loadTextures();
        loadMaterials();
        if (document.contains("prototypes")) {
            if (!document.at("prototypes").is_object()) {
                fail(path, "prototypes", "must be an object");
            }
            for (const auto& [name, value] :
                 document.at("prototypes").items()) {
                prototypes.emplace(name, value);
            }
        }

        RenderSettings renderSettings;
        ImageOutputSettings outputSettings;
        unsigned int width = 640;
        unsigned int height = 640;
        std::string outputPath = "output.ppm";
        if (document.contains("render")) {
            const Json& render = document.at("render");
            if (!render.is_object()) fail(path, "render", "must be an object");
            if (render.contains("width")) width = unsignedNumber(
                render.at("width"), path, "render.width", false);
            if (render.contains("height")) height = unsignedNumber(
                render.at("height"), path, "render.height", false);
            if (render.contains("samples")) {
                renderSettings.samplesPerPixel = unsignedNumber(
                    render.at("samples"), path, "render.samples", false);
            }
            if (render.contains("previewInterval")) {
                renderSettings.previewInterval = unsignedNumber(
                    render.at("previewInterval"), path,
                    "render.previewInterval");
            }
            if (render.contains("seed")) {
                renderSettings.randomSeed = unsignedNumber(
                    render.at("seed"), path, "render.seed");
            }
            if (render.contains("workers")) {
                renderSettings.workerCount = unsignedNumber(
                    render.at("workers"), path, "render.workers");
            }
            if (render.contains("maxBounces")) {
                renderSettings.maxBounces = unsignedNumber(
                    render.at("maxBounces"), path,
                    "render.maxBounces", false);
            }
            if (render.contains("russianRouletteStart")) {
                renderSettings.russianRouletteStart = unsignedNumber(
                    render.at("russianRouletteStart"), path,
                    "render.russianRouletteStart");
            }
            if (render.contains("tileSize")) {
                renderSettings.tileSize = unsignedNumber(
                    render.at("tileSize"), path,
                    "render.tileSize", false);
            }
            renderSettings.collectStats = optionalBool(
                render, "statistics", false, path, "render");
            renderSettings.adaptiveSampling = optionalBool(
                render, "adaptiveSampling", false, path, "render");
            if (render.contains("adaptiveMinSamples")) {
                renderSettings.adaptiveMinSamples = unsignedNumber(
                    render.at("adaptiveMinSamples"), path,
                    "render.adaptiveMinSamples", false);
            }
            if (render.contains("adaptiveMaxSamples")) {
                renderSettings.adaptiveMaxSamples = unsignedNumber(
                    render.at("adaptiveMaxSamples"), path,
                    "render.adaptiveMaxSamples", false);
            }
            if (render.contains("adaptiveBatch")) {
                renderSettings.adaptiveBatch = unsignedNumber(
                    render.at("adaptiveBatch"), path,
                    "render.adaptiveBatch", false);
            }
            renderSettings.adaptiveRelativeError = optionalNumber(
                render, "adaptiveRelativeError", 0.05, path, "render");
            renderSettings.adaptiveAbsoluteError = optionalNumber(
                render, "adaptiveAbsoluteError", 0.005, path, "render");
            renderSettings.adaptiveLuminanceFloor = optionalNumber(
                render, "adaptiveLuminanceFloor", 0.01, path, "render");
            renderSettings.denoise.enabled = optionalBool(
                render, "denoise", false, path, "render");
            if (render.contains("denoiseIterations")) {
                renderSettings.denoise.iterations = static_cast<int>(
                    unsignedNumber(
                        render.at("denoiseIterations"), path,
                        "render.denoiseIterations", false));
            }
            renderSettings.denoise.colorPhi = optionalNumber(
                render, "denoiseColorPhi", 0.5, path, "render");
            renderSettings.denoise.normalPhi = optionalNumber(
                render, "denoiseNormalPhi", 0.2, path, "render");
            renderSettings.denoise.positionPhi = optionalNumber(
                render, "denoisePositionPhi", 1.0, path, "render");
            outputSettings.exposure = optionalNumber(
                render, "exposure", 0.0, path, "render");
            outputSettings.autoExposure = optionalBool(
                render, "autoExposure", true, path, "render");
            outputSettings.targetLuminance = optionalNumber(
                render, "targetLuminance", 0.18, path, "render");
            if (render.contains("toneMapper")) {
                outputSettings.toneMapper = parseToneMapper(
                    stringValue(render.at("toneMapper"), path,
                                "render.toneMapper"),
                    path, "render.toneMapper");
            }
            if (render.contains("output")) {
                outputPath = resolvePath(
                    path, stringValue(render.at("output"), path,
                                      "render.output")).string();
            }
            if (renderSettings.adaptiveSampling) {
                if (renderSettings.adaptiveMinSamples >
                    renderSettings.adaptiveMaxSamples) {
                    fail(path, "render.adaptiveMinSamples",
                         "must be less than or equal to adaptiveMaxSamples");
                }
                if (renderSettings.adaptiveRelativeError < 0.0) {
                    fail(path, "render.adaptiveRelativeError",
                         "must be non-negative");
                }
                if (renderSettings.adaptiveAbsoluteError < 0.0) {
                    fail(path, "render.adaptiveAbsoluteError",
                         "must be non-negative");
                }
                if (renderSettings.adaptiveLuminanceFloor <= 0.0) {
                    fail(path, "render.adaptiveLuminanceFloor",
                         "must be positive");
                }
            }
        }

        Environment environment;
        if (document.contains("environment")) {
            const Json& value = document.at("environment");
            if (!value.is_object()) {
                fail(path, "environment", "must be an object");
            }
            const Vec3 bottom = optionalVec3(
                value, "bottom", Vec3(), path, "environment");
            const Vec3 top = optionalVec3(
                value, "top", bottom, path, "environment");
            const double intensity = optionalNumber(
                value, "intensity", 1.0, path, "environment");
            environment = Environment(bottom, top, intensity);
            if (value.contains("map")) {
                const fs::path mapPath = resolvePath(
                    path, stringValue(value.at("map"), path,
                                      "environment.map"));
                const std::string extension = mapPath.extension().string();
                const bool loaded =
                    extension == ".hdr" || extension == ".HDR"
                    ? environment.loadHdr(mapPath.string(), intensity)
                    : environment.loadPpm(mapPath.string(), intensity);
                if (!loaded) {
                    fail(path, "environment.map",
                         "failed to load environment map " +
                         mapPath.string());
                }
            }
        }

        std::unique_ptr<Scene> scene(new Scene());
        scene->setEnvironment(environment);
        if (document.contains("lights")) {
            const Json& lights = document.at("lights");
            if (!lights.is_array()) {
                fail(path, "lights", "must be an array");
            }
            for (std::size_t index = 0; index < lights.size(); ++index) {
                const Json& light = lights.at(index);
                const std::string context =
                    "lights[" + std::to_string(index) + "]";
                if (!light.is_object()) fail(path, context, "must be an object");
                const std::string type = stringValue(
                    required(light, "type", path, context),
                    path, context + ".type");
                if (type == "point") {
                    scene->addLight(std::unique_ptr<LightSource>(
                        new PointSource(
                            vec3(required(light, "position", path, context),
                                 path, context + ".position"),
                            vec3(required(light, "color", path, context),
                                 path, context + ".color"),
                            number(required(light, "intensity", path, context),
                                   path, context + ".intensity"),
                            optionalNumber(
                                light, "range",
                                std::numeric_limits<double>::infinity(),
                                path, context))));
                } else if (type == "directional") {
                    scene->addLight(std::unique_ptr<LightSource>(
                        new DirectionalSource(
                            vec3(required(light, "direction", path, context),
                                 path, context + ".direction").normalize(),
                            vec3(required(light, "color", path, context),
                                 path, context + ".color"),
                            number(required(light, "irradiance", path, context),
                                   path, context + ".irradiance"),
                            optionalNumber(
                                light, "angularRadius", 0.0,
                                path, context))));
                } else if (type == "spot") {
                    scene->addLight(std::unique_ptr<LightSource>(
                        new SpotSource(
                            vec3(required(light, "position", path, context),
                                 path, context + ".position"),
                            vec3(required(light, "direction", path, context),
                                 path, context + ".direction"),
                            vec3(required(light, "color", path, context),
                                 path, context + ".color"),
                            number(required(light, "intensity", path, context),
                                   path, context + ".intensity"),
                            number(required(light, "range", path, context),
                                   path, context + ".range"),
                            number(required(light, "innerAngle", path, context),
                                   path, context + ".innerAngle"),
                            number(required(light, "outerAngle", path, context),
                                   path, context + ".outerAngle"))));
                } else {
                    fail(path, context + ".type",
                         "unknown light type \"" + type + "\"");
                }
                ++summary.lights;
            }
        }
        if (document.contains("objects")) {
            const Json& objects = document.at("objects");
            if (!objects.is_array()) {
                fail(path, "objects", "must be an array");
            }
            for (std::size_t index = 0; index < objects.size(); ++index) {
                const Json& object = objects.at(index);
                std::string context =
                    "objects[" + std::to_string(index) + "]";
                if (object.is_object() && object.contains("name") &&
                    object.at("name").is_string()) {
                    context += "(" + object.at("name").get<std::string>() + ")";
                }
                addObject(*scene, object, Vec3(), context);
            }
        }
        scene->finalize();

        const Json& cameraValue = required(
            document, "camera", path, "$");
        if (!cameraValue.is_object()) fail(path, "camera", "must be an object");
        const Vec3 position = vec3(
            required(cameraValue, "position", path, "camera"),
            path, "camera.position");
        const Vec3 rotation = vec3(
            required(cameraValue, "rotation", path, "camera"),
            path, "camera.rotation");
        const double fov = number(
            required(cameraValue, "verticalFov", path, "camera"),
            path, "camera.verticalFov");
        const double aperture = optionalNumber(
            cameraValue, "aperture", 0.0, path, "camera");
        const double focusDistance = optionalNumber(
            cameraValue, "focusDistance", 1.0, path, "camera");
        Vec3 shutter(0.0, 0.0, 0.0);
        if (cameraValue.contains("shutter")) {
            const Json& value = cameraValue.at("shutter");
            if (!value.is_array() || value.size() != 2) {
                fail(path, "camera.shutter",
                     "must be an array of two numbers");
            }
            shutter = Vec3(
                number(value.at(0), path, "camera.shutter[0]"),
                number(value.at(1), path, "camera.shutter[1]"), 0.0);
        }

        LoadedScene result;
        result.scene = std::move(scene);
        result.cameraState = CameraState(
            position, rotation, static_cast<int>(width),
            static_cast<int>(height), fov, aperture, focusDistance,
            shutter.X(), shutter.Y());
        result.camera = result.cameraState.makeCamera();
        result.renderSettings = renderSettings;
        result.outputSettings = outputSettings;
        result.outputPath = outputPath;
        result.summary = summary;
        return result;
    }
};

} // namespace

LoadedScene JsonSceneLoader::load(const std::string& path) {
    return Loader(fs::path(path)).load();
}

void JsonSceneLoader::validate(const std::string& path) {
    (void)load(path);
}
