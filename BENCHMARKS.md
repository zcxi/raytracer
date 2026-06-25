# Performance Benchmarks

Benchmarks were run on June 24, 2026, on Windows with GCC 16.1.0 and a Release
build. Timings include final PPM output. Results vary with hardware and system
load, so use the commands below for comparisons on another machine.

## Standard scene

The v1.0 baseline was built from commit `a58dc78`. Both versions rendered the
same 640 by 640 scene with two samples, eight bounces, seed 1, and no previews.
The output SHA-256 was identical:
`CB1B66DFE9252377F8F89DE2AF69DDB15E7EFB235973ED7DC16B218C3CA64639`.

| Version | Workers | Time | Speedup |
| --- | ---: | ---: | ---: |
| v1.0 | 1 | 16.072 s | 1.00x |
| v1.1 | 1 | 1.452 s | 11.07x |
| v1.0 | automatic | 1.464 s | 1.00x |
| v1.1 | automatic | 0.327 s | 4.48x |

```sh
raytracer output.ppm --samples 2 --preview 0 --threads 1 --bounces 8 --seed 1
raytracer output.ppm --samples 2 --preview 0 --threads 0 --bounces 8 --seed 1
```

The v1.1 accelerated and `--no-bvh` renders also produced byte-identical files.

## Mesh-heavy benchmark

`raytracer_benchmark` creates a deterministic 5,000-triangle grid and traces
10,000 rays through both the mesh-local BVH and the diagnostic linear path.

| Mode | Time | Throughput | Hits |
| --- | ---: | ---: | ---: |
| Mesh BVH | 0.009270 s | 1.079 M rays/s | 10,000 |
| Linear triangles | 0.875480 s | 0.011 M rays/s | 10,000 |

Measured mesh-local BVH speedup: **94.44x**.

## Phase 8.1 / v1.2

These benchmarks were run on June 25, 2026, using GCC 16.1.0, a Release build,
and the same machine for both versions. The v1.1 baseline executable came from
commit `0de5ca6`. Times are averages of three standard-scene runs and five mesh
runs. Detailed counters were disabled for both standard-scene measurements.

The standard command rendered the 640 by 640 demo with eight samples, eight
bounces, seed 1, and no previews:

```sh
raytracer output.ppm --scene demo --samples 8 --preview 0 \
  --threads N --bounces 8 --seed 1
```

| Version | Workers | Average time | Throughput | Speedup |
| --- | ---: | ---: | ---: | ---: |
| v1.1 | 1 | 2.207 s | 1.48 M samples/s | 1.00x |
| v1.2 | 1 | 1.600 s | 2.05 M samples/s | **1.38x** |
| v1.1 | automatic | 1.245 s | 2.63 M samples/s | 1.00x |
| v1.2 | automatic | 1.118 s | 2.93 M samples/s | **1.11x** |

The matched baseline, optimized single-thread, and optimized multithread demo
renders happened to remain byte-identical with SHA-256
`413261934650944EB289CABF244746D569AEF3AC9B6ABE72BFFF915329F68D18`,
though exact identity is no longer an optimization requirement.

### Phase 8.1 mesh benchmark

| Version | Average BVH time | Throughput | Relative speed |
| --- | ---: | ---: | ---: |
| v1.1 | 0.008612 s | 1.16 M rays/s | 1.00x |
| v1.2 | 0.002326 s | 4.30 M rays/s | **3.70x** |

The mesh benchmark still reports 10,000 matching hits for accelerated and
linear traversal.

## Phase 10 / v1.4 — SIMD AABB Prototype

The standalone `SimdAabb` packet intersection was benchmarked on June 25, 2026
using GCC 16.1.0, a Release build with AVX enabled (`-mavx -mfma -O3`). The
benchmark pairs 1,000,000 rays into packets of 4 and tests slab intersection
against 250,000 random axis-aligned boxes.

| Mode | Time | Throughput | Hits |
| --- | ---: | ---: | ---: |
| Scalar (loop) | 0.006 s | 172 M rays/s | 4 |
| AVX packet | 0.006 s | 167 M rays/s | 4 |

Result agreement: **yes** — SIMD and scalar returned identical hit/miss decisions
for all 1,000,000 rays.

The SIMD packet traversal showed no meaningful throughput improvement (~0.97x).
The per-call cost of packing four lanes into `__m256d` registers dominates the
benefit of parallel slab evaluation. In a full BVH traversal the compiler's
auto-vectorization of the scalar loop also eliminates much of the gap.

**Feasibility gate not met.** The roadmap requires ≥1.25x end-to-end improvement
before integrating packet traversal into the production BVH. SIMD AABB support
remains as a prototype guarded by `RAYTRACER_SIMD` and excluded from the
production integrator. Further gains would require keeping ray data in SIMD
registers for an entire packet traversal, not rematerialising per AABB test.

## Phase 10 / v1.4 — Adaptive Sampling

Measured June 25, 2026 with GCC 16.1.0 in a Release build on `scenes/demo.json`.
Both renders used the same seed, disabled automatic exposure, and used a
32-sample maximum. Adaptive sampling used 8 minimum samples, batches of 4,
relative error 0.08, absolute error 0.01, and luminance floor 0.02.

| Mode | Time | Primary samples | Average spp | Relative speed |
| --- | ---: | ---: | ---: | ---: |
| Fixed 32 spp | 5.440 s | 13,107,200 | 32.0 | 1.00x |
| Adaptive 8–32 spp | 3.607 s | 8,907,864 | 21.7 | **1.51x** |

Adaptive sampling saved **32.0%** of primary samples. Compared with the fixed
32-spp tone-mapped PPM, its normalized byte-domain RMSE was **0.01624** and MAE
was **0.00567**. The result clears the roadmap's 1.5x representative-scene
speed target at low measured output error.

## Phase 10 / v1.4 — Light Influence Culling

The focused `scenes/milestone10-lights.json` render contains one useful
directional light plus point and spot lights deliberately rejected by range,
cone, or surface orientation.

| Counter | Count |
| --- | ---: |
| Considered analytic lights | 19,196 |
| Emitted shadow rays | 4,527 |
| Range rejects | 4,799 |
| Cone rejects | 4,799 |
| Back-facing rejects | 5,071 |

The pre-shadow checks rejected **14,669** light evaluations, avoiding shadow
rays for **76.4%** of considered lights in this stress scene. Directional
shadow rays use a finite conservative distance when every scene occluder is
bounded; scenes containing an infinite plane retain an infinite bound.

## Phase 11 / v1.5 — Realism Feature Smoke Benchmark

Measured June 25, 2026 with GCC 16.1.0 in a Release build using
`scenes/realism-showcase.json`. The scene contains two 64-segment lathed pieces,
a rounded-box board, clearcoat materials, texture-driven roughness and normals,
and an angular-radius directional light.

| Resolution | Adaptive range | Actual average spp | Time |
| --- | ---: | ---: | ---: |
| 480×320 | 8–24 spp | 8.9 spp | 0.166 s |

Adaptive sampling traced 1,372,924 primary samples and saved 62.8% of the
24-spp fixed budget. A one-sample 1280×720 chess-scene smoke render, including
16 lathed pawns and 64 rounded board squares, completed in 0.365 seconds and
produced all 203 expected scene objects without missing geometry.
