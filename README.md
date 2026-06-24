# raytracer

A small C++11 ray tracer being evolved into a physically based path tracer.
Version 0.3 adds deterministic multisampling, progressive output, exposure
control, and HDR tone mapping on top of the correctness-focused v0.2 foundation.

## Building

The project uses CMake 3.16 or newer and a C++11 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the demo renderer with an optional output path and image-quality settings:

```sh
./build/raytracer output.ppm --samples 64 --exposure 0.5 \
  --tone-map aces --preview 8 --seed 1
```

Available tone mappers are `none`, `reinhard`, and `aces`. Exposure is expressed
in photographic stops. A preview interval of zero disables intermediate output.
Sampling is deterministic: the same scene, sample count, and seed produce the
same image regardless of worker scheduling.

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
- `raytracer/src/Scene` — camera, scene traversal, and lights
- `raytracer/src/Shapes` — spheres, planes, cubes, rectangular prisms,
  square pyramids, and intersection records
- `raytracer/src/Writer` — linear RGB to sRGB PPM output
- `tests` — deterministic correctness tests

See [ROADMAP.md](ROADMAP.md) for the path-tracing roadmap.

## Sample trace

![header image](/raytracer/output.png)
