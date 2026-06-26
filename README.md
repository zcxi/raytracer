# raytracer

A small C++26 path tracer. Version 1.5 adds rounded and lathed geometry,
Radiance HDR environment maps with importance sampling, PBR texture channels,
clearcoat materials, and soft directional sunlight.

Adaptive sampling is enabled by default in the bundled demo and chessboard
scene files. Use `--no-adaptive` for a fixed-sample render. Other scene files
can enable it with `adaptiveSampling`, or it can be enabled with `--adaptive`.
Adaptive sampling is configured with
`--min-samples`, `--max-samples`, `--adaptive-batch`, `--relative-error`,
`--absolute-error`, and `--luminance-floor`. Render summaries report actual
samples traced, average samples per pixel, convergence, and samples saved.

## Building

The project uses CMake 3.25 or newer and a C++26 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the demo renderer with an optional output path and image-quality settings:

```sh
./build/raytracer output.ppm --samples 128 --bounces 8 \
  --rr-start 4 --exposure 0.5 --tone-map aces --preview 16 --seed 1 \
  --env-map studio.ppm --env-intensity 1.5 --tile-size 16
```

Render from a versioned JSON scene file:

```sh
./build/raytracer --scene-file scenes/demo.json --samples 64 \
  --exposure 0.2 --tone-map reinhard
```

Render the complete framed chessboard scene with all 32 pieces:

```sh
./build/raytracer chess.png --scene chessboard --samples 128 \
  --bounces 6 --aperture 0.08 --exposure 0.3
```

Render the compact realism feature showcase:

```sh
./build/raytracer realism.png --scene-file scenes/realism-showcase.json
```

Lens and shutter controls are available through `--aperture`, `--focus`,
`--shutter-open`, and `--shutter-close`.

Camera tuning can be done without editing C++:

```sh
./build/raytracer --scene-file scenes/chessboard.json --print-camera

./build/raytracer camera-preview.png --scene-file scenes/chessboard.json \
  --cam-pos 8.5,6.8,4.0 --cam-rot -0.32,-0.38,0 \
  --focus 21 --samples 1 --no-adaptive --preview-scale 0.35

./build/raytracer --scene-file scenes/chessboard.json \
  --cam-pos 8.5,6.8,4.0 --cam-rot -0.32,-0.38,0 \
  --focus 21 --save-camera scenes/chessboard.camera.json
```

`--preview-scale` only affects the rendered preview resolution. It does not
modify saved scene dimensions, so the same tuned camera can be reused for a
full-resolution final render.

Available tone mappers are `none`, `reinhard`, and `aces`. Exposure is expressed
in photographic stops. A preview interval of zero disables intermediate output.
Sampling is deterministic: the same scene, sample count, and seed produce the
same image regardless of worker scheduling.

Materials can be created with `Material::principled(baseColor, roughness,
metallic, transmission, ior)`. Legacy `diffuse`, `mirror`, and `dielectric`
factories remain available. Principled surfaces combine an energy-aware diffuse
lobe with Cook-Torrance GGX specular reflection, optional clearcoat, and
importance sampling. JSON materials support `baseColorTexture`,
`roughnessTexture`, `metallicTexture`, and `normalTexture`.
Emissive spheres and rectangles are automatically registered as area lights.
Diffuse light and BSDF samples are combined with the power heuristic. A gradient
environment is built in, and lat-long P6 PPM images can be loaded as environment
maps. Radiance `.hdr` maps retain values above one and use luminance-weighted
importance sampling.

Finite geometry is automatically placed in a BVH; infinite planes are tested
separately. Use `--no-bvh` to compare against brute-force traversal. Each render
reports total time, throughput, and BVH size. Add `--stats` when detailed rays,
shadow rays, AABB tests, primitive tests, node visits, and path depth are needed.
Keeping counters off avoids instrumentation in normal renders.

When previews are disabled, all samples for a tile are rendered together for
better cache locality and fewer worker barriers. Release builds use relaxed
floating-point optimization by default; configure with
`-DRAYTRACER_FAST_MATH=OFF` for strict IEEE behavior.

Build and run the reproducible mesh benchmark with:

```sh
cmake --build build-release
./build-release/raytracer_benchmark
```

See [BENCHMARKS.md](BENCHMARKS.md) for the v1.0 through v1.2 results and
commands used.

## JSON scene format (v1)

Scenes can be loaded from versioned JSON files with `--scene-file`. The
format supports textures, materials, prototypes, instances, groups, analytic
lights, emissive geometry, and a gradient or mapped environment.

CLI values override equivalents from the JSON file. Validate a file without
rendering:

```sh
./build/raytracer --validate-scene scenes/demo.json
```

Relative asset paths (OBJ meshes, PPM textures, environment maps) are
resolved from the file that declares them, including resources declared by an
included scene file. Include cycles and duplicate named resources are rejected.
The bundled `demo.json` and `chessboard.json` in `scenes/` are the reference implementations. See
[ROADMAP.md](ROADMAP.md) (Phase 9) for the full JSON schema and field
reference.

Directional lights support optional soft sunlight with
`"angularRadius"` in radians. A value of `0` keeps a hard-edged sun;
small values such as `0.012` add natural penumbra to directional shadows.

On multi-configuration generators such as Visual Studio, add
`--config Release` to the build command and run the executable from the
configuration subdirectory.

Sanitizer instrumentation is available on GCC/Clang toolchains that ship the
required runtimes:

```sh
cmake -S . -B build-sanitize -DRAYTRACER_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

## Source layout

- `raytracer/src/Acceleration` — AABBs and iterative BVH traversal
- `raytracer/src/Textures` — solid, checker, and PPM image textures

- `raytracer/src/Math` — vectors, quaternions, and rays
- `raytracer/src/Materials` — diffuse, mirror, dielectric, and emissive
  material parameters
- `raytracer/src/Scene` — camera, scene traversal, and lights
- `raytracer/src/Shapes` — spheres, planes, cubes, rectangular prisms,
  square pyramids, and intersection records
- `raytracer/src/Writer` — linear RGB to sRGB PPM output
- `raytracer/src/SceneDescription` — versioned JSON scene loader
- `tests` — deterministic correctness tests

See [ROADMAP.md](ROADMAP.md) for the path-tracing roadmap.

## Sample trace

![header image](/raytracer/output.png)
