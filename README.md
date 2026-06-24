# raytracer

A small C++11 path tracer. Version 0.6 adds physically based materials with
Cook-Torrance GGX reflection, Smith masking, Schlick Fresnel, metallic/roughness
parameters, transmission, and importance-sampled BSDF lobes.

## Building

The project uses CMake 3.16 or newer and a C++11 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the demo renderer with an optional output path and image-quality settings:

```sh
./build/raytracer output.ppm --samples 128 --bounces 8 \
  --rr-start 4 --exposure 0.5 --tone-map aces --preview 16 --seed 1 \
  --env-map studio.ppm --env-intensity 1.5
```

Available tone mappers are `none`, `reinhard`, and `aces`. Exposure is expressed
in photographic stops. A preview interval of zero disables intermediate output.
Sampling is deterministic: the same scene, sample count, and seed produce the
same image regardless of worker scheduling.

Materials can be created with `Material::principled(baseColor, roughness,
metallic, transmission, ior)`. Legacy `diffuse`, `mirror`, and `dielectric`
factories remain available. Principled surfaces combine an energy-aware diffuse
lobe with Cook-Torrance GGX specular reflection and importance sampling.
Emissive spheres and rectangles are automatically registered as area lights.
Diffuse light and BSDF samples are combined with the power heuristic. A gradient
environment is built in, and lat-long P6 PPM images can be loaded as environment
maps.

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

- `raytracer/src/Math` — vectors, quaternions, and rays
- `raytracer/src/Materials` — diffuse, mirror, dielectric, and emissive
  material parameters
- `raytracer/src/Scene` — camera, scene traversal, and lights
- `raytracer/src/Shapes` — spheres, planes, cubes, rectangular prisms,
  square pyramids, and intersection records
- `raytracer/src/Writer` — linear RGB to sRGB PPM output
- `tests` — deterministic correctness tests

See [ROADMAP.md](ROADMAP.md) for the path-tracing roadmap.

## Sample trace

![header image](/raytracer/output.png)
