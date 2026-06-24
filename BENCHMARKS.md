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
