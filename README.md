# raytracer

A small C++11 ray tracer being evolved into a physically based path tracer.
Version 0.2 establishes a correctness-focused foundation with robust ray
intervals, hit records, camera orientation, linear color, sRGB output, portable
builds, and automated tests.

## Building

The project uses CMake 3.16 or newer and a C++11 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the demo renderer with an optional output path:

```sh
./build/raytracer output.ppm
```

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
