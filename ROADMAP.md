# Ray Tracer Roadmap

The goal of this roadmap is to evolve the current direct-lighting ray caster into a
physically based path tracer. Each phase should leave the renderer working and
produce a visible, testable improvement.

## Phase 1: Correctness and Core Foundations

**Status: completed for v0.2 on June 24, 2026.**

Before adding advanced rendering features, make the current renderer numerically
correct, predictable, and easier to extend.

### Ray and camera correctness

- Calculate the aspect ratio using floating-point division.
- Build an orthonormal camera basis and apply the camera's orientation to primary
  rays.
- Introduce a `Ray` type containing an origin and normalized direction.
- Define a ray interval (`tMin`, `tMax`) so intersections can reject hits behind
  the origin or outside a shadow ray's valid range.

### Intersection correctness

- Replace heap-allocated intersection points with a `HitRecord` returned through
  a value or output parameter.
- Store hit distance, point, surface normal, front/back-face state, and material
  reference in `HitRecord`.
- Rewrite sphere intersection using the quadratic equation and select the nearest
  valid positive root.
- Fix plane intersection so it works from either side and rejects parallel rays.
- Offset secondary and shadow ray origins by a small epsilon to prevent
  self-intersection.
- Remove exact floating-point vector equality from intersection logic.

### Math and shading correctness

- Fix `Vec3::operator/` so `vector / scalar` divides each component by the scalar.
- Clamp diffuse response with `max(0, normal.dot(lightDirection))`.
- Include light color in direct-light calculations.
- Use linear floating-point RGB values, preferably in the range `0.0–1.0`, during
  all lighting calculations.
- Clamp negative color values before image conversion.
- Apply linear-to-sRGB gamma conversion when writing the final image.
- Make the output path configurable instead of using an absolute machine-specific
  path.

### Ownership and execution safety

- Replace owning raw pointers with values or smart pointers.
- Clear renderer work queues before starting a new render.
- Handle `hardware_concurrency()` returning zero.
- Remove the framebuffer-wide write mutex by assigning exclusive pixels or tiles
  to workers.

### Verification

- Add unit tests for vector arithmetic, sphere roots, inside-sphere rays, plane
  intersections, ray intervals, and normal orientation.
- Render small deterministic reference scenes for camera orientation, inverse
  square falloff, shadows, and gamma correction.
- Run with AddressSanitizer or Visual Studio diagnostics and confirm there are no
  leaks or invalid accesses.

**Completion target:** the existing scene renders without distortion, leaks,
self-shadow artifacts, negative lighting, or camera-direction errors.

### Delivered in v0.2

- Added portable CMake library, renderer, and test targets.
- Added normalized rays, bounded intersections, and value-based hit records.
- Corrected camera orientation, aspect ratio, sphere roots, two-sided planes,
  normal orientation, shadow-ray offsets, Lambertian response, light color, and
  inverse-square point-light behavior.
- Corrected vector division and quaternion normalization, inversion, and rotation.
- Replaced per-intersection allocations and scene ownership raw pointers.
- Switched lighting to linear RGB and added safe sRGB PPM encoding.
- Replaced shared ray/framebuffer locks with atomic pixel scheduling.
- Added deterministic tests for math, camera projection, intersections, direct
  lighting, shadows, background color, invalid inputs, and image encoding.
- Added optional AddressSanitizer/UBSan build instrumentation. The portable MinGW
  package used for the Windows verification build did not include those runtime
  libraries; strict compiler warnings and Cppcheck passed cleanly.

## Phase 2: Sampling and Image Quality

**Status: completed for v0.3 on June 24, 2026.**

- Cast multiple jittered samples through each pixel.
- Average samples in linear color space.
- Use a deterministic, independently seeded random-number generator per worker.
- Add configurable samples per pixel.
- Add progressive accumulation and periodic image output.
- Add exposure control and a simple Reinhard or ACES tone mapper.

**Visible result:** smooth silhouettes, reduced aliasing, and preserved highlight
detail.

### Delivered in v0.3

- Added configurable jittered samples per pixel with linear-space accumulation.
- Added deterministic per-pixel/per-sample random streams independent of thread
  scheduling.
- Added pass-based progressive rendering and configurable periodic output.
- Added exposure control in photographic stops.
- Added selectable Reinhard and ACES tone mapping before sRGB conversion.
- Added command-line controls for samples, previews, exposure, tone mapping,
  random seed, and worker count.
- Added focused tests for deterministic jitter, exposure, tone mapping, and
  invalid sampling configuration.

## Phase 3: Materials and Secondary Rays

**Status: completed for v0.4 on June 24, 2026.**

- Introduce a `Material` interface or tagged material model.
- Support diffuse/Lambertian reflection using cosine-weighted hemisphere sampling.
- Support perfect mirrors.
- Support dielectric transmission using Snell's law.
- Handle total internal reflection.
- Use Schlick's approximation for Fresnel reflectance.
- Replace the fixed recursion limit with configurable bounce depth.
- Add Russian roulette path termination after several bounces.

**Visible result:** reflections, glass, indirect illumination, color bleeding, and
non-black shadow regions.

### Delivered in v0.4

- Added tagged diffuse, perfect-mirror, and dielectric materials with optional
  emission and tint.
- Added iterative multi-bounce path transport with linear throughput tracking.
- Added cosine-weighted hemisphere sampling for diffuse global illumination.
- Added exact mirror reflection and Snell-law dielectric refraction.
- Added Schlick Fresnel selection and total internal reflection.
- Added direct point-light sampling at every diffuse bounce, allowing secondary
  surfaces to receive lighting with less noise.
- Added configurable maximum path depth and Russian roulette start depth.
- Extended deterministic sampling to all secondary-ray decisions.
- Added CLI controls for bounce depth and Russian roulette.
- Added optics, material-validation, emissive, mirror-transport, and secondary
  sampling tests.

## Phase 4: Path-Traced Lighting

**Status: completed for v0.5 on June 24, 2026.**

- Treat emissive materials as light sources.
- Add rectangular and spherical area lights.
- Sample points on light surfaces for soft shadows.
- Combine direct-light sampling with BSDF sampling.
- Use multiple importance sampling to reduce noise.
- Support an environment color, gradient sky, and eventually HDR environment maps.

**Visible result:** soft shadows, natural ambient lighting, and substantially less
noise around small or bright lights.

### Delivered in v0.5

- Added automatic light registration for sampleable emissive geometry.
- Added uniform surface sampling and area PDFs for spheres and finite rectangles.
- Added rectangular and spherical area lights with naturally soft shadows.
- Added one-light direct sampling at every diffuse bounce.
- Added power-heuristic multiple importance sampling between light and BSDF
  strategies, including complementary weights when BSDF paths hit emitters.
- Added uniform environment sampling with matching MIS weights.
- Added configurable gradient environment lighting.
- Added optional lat-long P6 PPM environment maps and intensity control.
- Added area-light, surface-PDF, environment, and MIS tests.

## Phase 5: Physically Based Materials

**Status: completed for v0.6 on June 24, 2026.**

- Add base color, roughness, metallic, transmission, emission, and index of
  refraction parameters.
- Implement a Cook-Torrance microfacet BRDF.
- Use GGX normal distribution, Smith geometry masking, and Schlick Fresnel.
- Add importance sampling for the GGX distribution.
- Enforce approximate energy conservation between diffuse and specular lobes.

**Visible result:** convincing plastics, rough and polished metals, glossy paint,
and more realistic highlights.

### Delivered in v0.6

- Added unified base color, roughness, metallic, transmission, emission, and IOR
  material parameters.
- Added Cook-Torrance microfacet BRDF evaluation.
- Added GGX normal distribution and importance sampling.
- Added Smith geometry masking and vector Schlick Fresnel.
- Added energy-aware diffuse/specular lobe mixing for dielectrics and metals.
- Added mixed diffuse/GGX PDFs for direct-light MIS and secondary transport.
- Added deterministic principled transmission sampling.
- Added PBR parameter, Fresnel, GGX, Smith, evaluation, and sampling tests.

## Phase 6: Performance and Scalability

**Status: completed for v0.7 on June 24, 2026.**

- Add axis-aligned bounding boxes.
- Build a bounding volume hierarchy for scene geometry.
- Traverse the BVH iteratively or with shallow recursion.
- Render image tiles instead of maintaining shared ray queues.
- Benchmark intersection throughput and render time per sample.
- Add release-build settings and profile before applying low-level optimizations.

**Completion target:** scenes with many objects and hundreds of samples per pixel
remain practical to render.

### Delivered in v0.7

- Added robust slab-tested axis-aligned bounding boxes.
- Added bounds for spheres, rectangles, pyramids, cubes, and rectangular prisms.
- Added median-split BVH construction along each node's longest axis.
- Added iterative closest-hit and early-out occlusion traversal.
- Kept infinite planes in a correct fallback path outside the BVH.
- Added a brute-force `--no-bvh` diagnostic and benchmarking mode.
- Replaced per-pixel atomic scheduling with configurable square render tiles.
- Added pass timing, total render time, sample throughput, and BVH node metrics.
- Added AABB, BVH nearest-hit, brute-force agreement, unbounded fallback, and
  tile validation tests.

## Phase 7: Camera, Geometry, and Scene Features

**Status: completed for v1.0 on June 24, 2026.**

- Add thin-lens depth of field with aperture and focus-distance controls.
- Add motion blur by sampling ray time.
- Add triangles and triangle meshes.
- Load common mesh formats such as OBJ or glTF.
- Add texture coordinates, image textures, normal maps, and procedural textures.
- Add transforms so geometry can be translated, rotated, and scaled independently.

**Visible result:** richer scenes with photographic focus, detailed assets, and
textured surfaces.

### Delivered in v1.0

- Added thin-lens camera sampling with configurable aperture and focus distance.
- Added shutter intervals, ray time, and motion-blurred moving spheres.
- Added barycentric triangle intersections with interpolated normals and UVs.
- Added OBJ loading with `v`, `vt`, `vn`, polygon fan triangulation, and BVH
  participation.
- Added transformed shape instances with translation, Euler rotation,
  non-uniform scale, inverse-transpose normals, and transformed bounds.
- Added sphere, rectangle, and triangle UV coordinates.
- Added solid, procedural checker, and P6 PPM image textures.
- Added texture-aware material evaluation inside the PBR integrator.
- Added tests for lens sampling, shutter time, motion, UV interpolation, texture
  loading, OBJ triangulation, transformed intersections, and bounds.

## Phase 8: Performance Quick Wins

**Status: completed for v1.1 on June 24, 2026.**

This milestone focuses on high-impact changes with relatively low implementation
risk. Every optimization must preserve deterministic output and be supported by
before/after benchmarks.

### Required work

1. Replace `Vec3`'s dynamic `std::vector<double>` storage with three direct
   numeric members.
2. Stop rebuilding the scene BVH after every `addShape()` call. Add a scene
   `finalize()` step that builds acceleration structures once after scene setup.
3. Build a mesh-local BVH inside `ObjMesh` instead of testing every triangle.
4. Keep render worker threads alive for the full render instead of creating and
   joining them once per sample pass.
5. Avoid copying the full framebuffer for progressive output. Normalize and
   tone-map directly from the accumulation buffer using the completed sample
   count.
6. Add lightweight counters for rays, AABB tests, primitive tests, BVH nodes,
   shadow rays, and average path depth.

### Stretch work

- Traverse the nearer BVH child first to improve pruning.
- Add variance tracking as groundwork for adaptive sampling.
- Store mesh vertices and triangles in more cache-friendly contiguous arrays.

### Acceptance criteria

- Debug and Release tests remain green.
- Identical seeds produce byte-identical images before and after optimization.
- BVH and brute-force diagnostic renders remain byte-identical.
- No per-vector heap allocation remains in rendering hot paths.
- No worker thread is created inside an individual sample pass.
- The standard benchmark scene renders at least 2x faster than the v1.0 baseline.
- A mesh-heavy benchmark shows a substantially larger speedup than the simple
  primitive scene.
- Benchmark results and tracing counters are printed or saved in a reproducible
  format.

### Delivered in v1.1

- Replaced heap-backed `Vec3` values with three inline doubles.
- Deferred scene BVH construction until `finalize()` and retained safe lazy
  finalization for direct scene queries.
- Added a mesh-local BVH for OBJ triangles and a diagnostic linear traversal.
- Kept worker threads alive for the complete render instead of recreating them
  for every sample pass.
- Wrote progressive images directly from the accumulation buffer without a
  normalized framebuffer copy.
- Added per-worker counters for rays, shadow rays, AABB tests, primitive tests,
  BVH node visits, paths, and average path depth.
- Removed per-ray heap allocation from BVH traversal.
- Fixed a C++11 evaluation-order bug that could corrupt recursive BVH child
  indices after vector reallocation.
- Added deterministic regression coverage for deferred finalization, recursive
  BVH construction, mesh acceleration, counters, and byte-identical progressive
  output.
- Added a reproducible benchmark executable and recorded standard-scene and
  mesh-heavy results in `BENCHMARKS.md`.

## Phase 8.1: Relaxed Performance Tuning

**Status: completed for v1.2 on June 25, 2026.**

This follow-up removes byte-identical output as a hard optimization constraint.
Changes must still preserve valid intersections and stable visual output, but
small floating-point differences are acceptable when they improve throughput.

### Delivered in v1.2

- Made detailed tracing counters opt-in through `--stats`, removing their
  instrumentation cost from normal renders.
- Batched all samples for each tile when previews are disabled, reducing worker
  synchronization and improving accumulation-buffer locality.
- Added optional Release fast-math optimization, enabled by default and
  configurable with `RAYTRACER_FAST_MATH`.
- Normalized rays with one square root instead of calculating their length
  twice.
- Cached ray origins, reciprocal directions, signs, and parallel-axis state once
  per BVH query without increasing the size of every path ray.
- Replaced median-only BVH construction with 12-bin surface-area-heuristic
  splitting and cached primitive bounds.
- Traversed nearer BVH children first and skipped queued nodes beyond the
  closest known hit.
- Added regression coverage for SAH surface-area calculations, opt-in counters,
  and batched-versus-progressive accumulation.
- Benchmarked the new renderer against commit `0de5ca6`; see `BENCHMARKS.md`.

### Acceptance criteria

- Debug and Release correctness tests pass.
- BVH and brute-force intersections agree within numerical tolerance.
- Reference renders contain no NaNs, missing geometry, or visible regressions.
- Performance is measured with counters disabled.
- Standard-scene and mesh-heavy benchmarks improve over the v1.1 baseline.

## Phase 9: JSON Scene Description Format

**Target release: v1.3.**

**Status: completed for v1.3 on June 25, 2026.**

Replace hardcoded scene construction in `main.cpp` with versioned JSON scene
files. A scene file should describe the camera, renderer defaults, environment,
textures, materials, lights, geometry, transforms, and external assets without
requiring the renderer to be recompiled.

### Goals

- Load a complete scene with `--scene-file path/to/scene.json`.
- Keep rendering and scene construction separate from command-line parsing.
- Migrate the existing demo and chessboard scenes to JSON without losing
  features or visual quality.
- Produce useful validation errors containing the JSON field and object name.
- Resolve relative asset paths from the directory containing the scene file.
- Use a versioned schema so future scene features can remain backward
  compatible.

### Proposed v1 document structure

```json
{
  "version": 1,
  "render": {
    "width": 800,
    "height": 600,
    "samples": 128,
    "maxBounces": 8,
    "russianRouletteStart": 4,
    "exposure": 0.3,
    "toneMapper": "aces"
  },
  "camera": {
    "position": [9.5, 7.2, 3.5],
    "rotation": [-0.29, -0.42, 0.0],
    "verticalFov": 0.785398,
    "aperture": 0.08,
    "focusDistance": 24.0,
    "shutter": [0.0, 1.0]
  },
  "environment": {
    "bottom": [0.035, 0.025, 0.02],
    "top": [0.16, 0.20, 0.32],
    "intensity": 1.0
  },
  "textures": {
    "board-checker": {
      "type": "checker",
      "first": [0.78, 0.58, 0.31],
      "second": [0.19, 0.055, 0.025],
      "scale": 1.0
    }
  },
  "materials": {
    "ivory": {
      "type": "principled",
      "baseColor": [0.92, 0.84, 0.66],
      "roughness": 0.18,
      "metallic": 0.0,
      "transmission": 0.0,
      "ior": 1.5
    }
  },
  "lights": [
    {
      "type": "point",
      "position": [-5.5, 9.5, -16.0],
      "color": [1.0, 0.78, 0.55],
      "intensity": 8500.0
    }
  ],
  "objects": [
    {
      "name": "white-king",
      "type": "sphere",
      "center": [0.5, 1.5, -14.5],
      "radius": 0.18,
      "material": "ivory",
      "transform": {
        "translation": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0]
      }
    }
  ]
}
```

### Supported v1 resources

- Textures: solid color, procedural checker, and PPM image texture.
- Materials: diffuse, mirror, dielectric, and principled PBR.
- Shapes: sphere, moving sphere, plane, rectangle, cube, rectangular prism,
  pyramid, triangle, and OBJ mesh.
- Transforms: translation, Euler rotation in radians, and non-uniform scale.
- Lights: point lights and emissive sampleable geometry.
- Environment: solid/gradient sky and optional lat-long environment map.
- Camera: image dimensions, transform, field of view, aperture, focus distance,
  and shutter interval.
- Render defaults: samples, bounce limits, seed, tile size, preview interval,
  exposure, tone mapper, output path, and statistics.

### Implementation plan

#### Step 1: JSON dependency and parsing foundation

- Add a maintained JSON library through CMake, preferably `nlohmann/json`.
- Create `SceneDescription/JsonHelpers` for required fields, optional fields,
  vector/color parsing, numeric ranges, enum parsing, and contextual errors.
- Require a top-level integer `version`; reject unsupported versions before
  loading resources.
- Add parser-only tests for valid JSON, malformed JSON, missing fields, invalid
  vector lengths, unknown enum values, and non-finite numbers.

#### Step 2: Separate loaded configuration from runtime objects

- Introduce a `LoadedScene` result containing `Scene`, `Camera`,
  `RenderSettings`, and `ImageOutputSettings`.
- Add a `SceneLoader` interface with a JSON implementation so parsing is not
  coupled to `main.cpp`.
- Move reusable scene-construction helpers out of `main.cpp`.
- Preserve command-line overrides: explicit CLI values should override values
  supplied by the JSON file.

#### Step 3: Resource registries

- Parse textures first into a named
  `unordered_map<string, shared_ptr<Texture>>`.
- Parse materials second and resolve optional texture references by name.
- Reject duplicate names and unresolved references with messages such as
  `materials.board.texture references unknown texture "oak"`.
- Keep shared textures alive through material ownership.

#### Step 4: Geometry and transform factories

- Implement one factory per shape type with strict required/optional fields.
- Resolve each object's material name and construct the corresponding `Shape`.
- Wrap geometry in `Transform` only when a transform block is present.
- Support OBJ paths relative to the scene file and report asset-loading failures
  with both the object name and resolved path.
- Add shape-factory tests comparing JSON-created intersections and bounds with
  directly constructed C++ shapes.

#### Step 5: Camera, lighting, and environment

- Parse point lights and add them through `Scene::addLight`.
- Allow emissive geometry to continue registering itself automatically.
- Parse gradient colors, intensity, and optional environment-map path.
- Build the camera only after validating dimensions, lens values, field of
  view, and shutter interval.

#### Step 6: CLI integration

- Add `--scene-file FILE` as the primary scene-loading option.
- Keep `--scene demo` and `--scene chessboard` temporarily as compatibility
  aliases that resolve to bundled JSON files.
- Define precedence as: engine defaults, then JSON values, then explicit CLI
  overrides.
- Add `--validate-scene FILE` to parse and validate without rendering.
- Print a concise load summary with object, light, material, texture, and BVH
  counts.

#### Step 7: Migrate bundled scenes

- Create `scenes/demo.json` and `scenes/chessboard.json`.
- Remove the large hardcoded `demo1()` and `chessboard()` construction blocks
  after their JSON replacements are verified.
- Keep generated/repeated chess-piece geometry manageable through one of:
  object prototypes plus instances, reusable groups, or a small `include`
  mechanism. Choose one feature for schema v1 rather than duplicating hundreds
  of object definitions.
- Resolve included files relative to their parent and detect include cycles.

#### Step 8: Validation and regression coverage

- Test every supported texture, material, shape, light, environment, and camera
  field.
- Test duplicate names, missing references, unsupported versions, malformed
  transforms, missing files, include cycles, and invalid numeric ranges.
- Render small deterministic JSON scenes and compare them with equivalent C++
  scene construction within a numerical or perceptual tolerance.
- Run AddressSanitizer/UBSan where available and verify failed loads release all
  partially created resources.

### Acceptance criteria

- `raytracer output.png --scene-file scenes/chessboard.json` renders the complete
  chess scene without scene-specific C++ construction code.
- Bundled demo and chessboard scenes are represented entirely by JSON and
  external assets.
- All currently supported camera, environment, material, texture, light, shape,
  transform, and render settings can be represented.
- Invalid files fail before rendering with the scene path and precise JSON field
  in the error.
- Relative OBJ, texture, environment-map, and included-scene paths work from any
  current working directory.
- CLI overrides are documented and covered by tests.
- Loading occurs once before rendering and has no measurable effect on tracing
  throughput.

### Deferred beyond v1

- JSON Schema generation and editor autocomplete.
- glTF scene import.
- Animation tracks and keyframes.
- Arbitrary user-defined procedural geometry or material graphs.
- GPU-specific scene packing.

## Phase 10: Adaptive Sampling and Lighting Efficiency

**Target release: v1.4.**

**Status: completed for v1.4 on June 25, 2026.**

Reduce wasted samples and shadow work while expanding practical lighting
controls. This milestone includes only features that have a credible path to
better image quality per unit of render time. Optimizations must be benchmarked
independently and retain a scalar fallback.

### Scope decisions

- Implement adaptive sampling using measured variance, not pixel darkness alone.
  Dark pixels may receive more samples only when their relative noise estimate
  remains high.
- Implement directional and spot lights because they add useful scene controls
  and fit the existing `LightSource` abstraction.
- Implement finite-light influence culling for point and spot lights before
  creating shadow rays.
- Do not describe directional sunlight as range- or frustum-culled: it has
  infinite reach. Directional shadow rays may instead use the finite scene
  bounds to choose a safe maximum distance.
- Prototype SIMD BVH packet traversal only for coherent ray batches. Ship it
  only if end-to-end benchmarks improve; keep scalar traversal for divergent
  secondary rays and unsupported standard libraries.

## Workstream A: Adaptive sampling

### Design

- Track each pixel's sample count, running mean radiance, and luminance variance
  using Welford's numerically stable online algorithm.
- Begin testing convergence only after a configurable minimum sample count.
- Estimate error using standard error or a confidence interval:
  `sqrt(variance / sampleCount)`.
- Normalize error by a luminance floor so dark pixels are judged by relative
  noise without dividing by values close to zero.
- Mark a pixel converged when both absolute and relative error thresholds are
  satisfied.
- Continue sampling unconverged pixels until they converge or reach the maximum
  samples per pixel.
- Evaluate convergence in small batches, such as every 4 or 8 samples, rather
  than after every sample.

### Renderer changes

- Replace the single global completed-sample count with per-pixel sample counts.
- Add accumulation records containing RGB sum, luminance mean, luminance `M2`,
  sample count, and convergence state.
- Change tile work generation so workers skip converged pixels and retire tiles
  when all pixels inside them converge.
- Update image writing to divide each pixel by its own completed sample count.
- Preserve the existing fixed-sample path when adaptive sampling is disabled.

### Configuration

- Add render settings and JSON fields:
  - `adaptiveSampling`
  - `minSamples`
  - `maxSamples`
  - `sampleBatch`
  - `relativeError`
  - `absoluteError`
  - `luminanceFloor`
- Add CLI overrides such as `--adaptive`, `--min-samples`,
  `--max-samples`, and `--noise-threshold`.
- Print average samples per pixel, converged-pixel percentage, and samples saved.

### Validation

- Unit-test Welford updates with known sample sequences.
- Verify constant-color pixels converge at the minimum sample count.
- Verify deliberately noisy pixels continue toward the maximum.
- Ensure zero-luminance and firefly samples do not produce NaNs or premature
  convergence.
- Compare adaptive renders with fixed high-sample references using RMSE and a
  perceptual metric.

### Acceptance criteria

- Smooth regions use substantially fewer samples than high-variance edges,
  glossy reflections, depth-of-field transitions, and dark indirect-light
  regions.
- Representative scenes render at least 1.5x faster at comparable measured
  error, or achieve lower error in the same render time.
- No pixel receives fewer than `minSamples` or more than `maxSamples`.
- Disabling adaptive sampling preserves the existing fixed-sample behavior.

## Workstream B: Directional and spot lights

### Light sampling interface

- Replace the point-light-specific renderer path with a common light sample
  result containing direction, radiance, maximum shadow distance, and validity.
- Add a virtual method such as
  `bool sampleIncident(const Vec3& point, LightSample& sample) const`.
- Keep point, spot, and directional light evaluation outside the BSDF so all
  light types share visibility and shading code.
- Remove assumptions that every analytic light has a finite position.

### Directional light

- Store a normalized direction toward the light, color, and irradiance.
- Return constant incoming radiance independent of the hit-point position.
- Use an infinite shadow distance when the scene has unbounded geometry.
- When all shadow-casting geometry is bounded, intersect the shadow ray with the
  scene's aggregate bounds and use the exit distance as a safe finite limit.
- Expose JSON fields `type: "directional"`, `direction`, `color`, and
  `irradiance`.

### Spot light

- Store position, normalized forward direction, intensity, maximum range, inner
  cone angle, and outer cone angle.
- Apply inverse-square attenuation inside the range.
- Use smooth interpolation between the inner and outer cones to avoid a hard
  lighting edge.
- Reject points behind the light, outside its range, or outside its outer cone
  before any visibility query.
- Expose JSON fields `type: "spot"`, `position`, `direction`, `color`,
  `intensity`, `range`, `innerAngle`, and `outerAngle`.

### Validation

- Test directional-light invariance with hit-point distance.
- Test spot-light center, penumbra, outer-cone, behind-light, and range cases.
- Test invalid zero directions, negative intensity/range, and reversed cone
  angles.
- Render focused references for sunlight shadows and a spot-lit object.

## Workstream C: Light-influence and shadow-ray culling

This replaces the proposed "sunlight frustum culling" with checks that are
geometrically valid for each light type.

### Required culling

- Reject every analytic light when `normal dot lightDirection <= 0`.
- Reject point and spot lights outside an optional finite influence radius.
- Reject spot lights outside the outer cone before BSDF evaluation or shadow-ray
  construction.
- Reject lights whose attenuated radiance is below a configurable conservative
  threshold only if the threshold is disabled by default and tested for bias.
- Set point/spot shadow-ray `tMax` to distance minus epsilon.
- Bound directional shadow rays by aggregate scene bounds only when all relevant
  occluders are bounded; otherwise retain infinity.

### Optional spatial light culling

- If scenes gain hundreds of finite lights, add a light BVH or clustered
  world-space grid keyed by influence bounds.
- Do not implement a light acceleration structure for the current small-light
  scenes unless profiling shows analytic-light iteration is material.

### Counters

- Add opt-in statistics for considered lights, range rejects, cone rejects,
  back-facing rejects, emitted shadow rays, and occluded shadow rays.
- Use the counters to prove that culling reduces shadow work without changing
  uncullable light contributions.

## Workstream D: SIMD BVH packet traversal

### Feasibility gate

- Detect usable C++26 `<simd>` support at configure time.
- Compile a scalar fallback on compilers or standard libraries without
  `std::simd`.
- Benchmark a standalone packet traversal prototype before changing the
  production integrator.
- Require a meaningful end-to-end gain, not merely a faster synthetic AABB loop.

### Packet design

- Start with packets of 4 or 8 coherent primary rays from adjacent pixels in a
  tile.
- Store packet origins, reciprocal directions, active-lane masks, and closest
  distances in structure-of-arrays form.
- Test one BVH node against all active lanes using `std::simd`.
- Maintain lane masks per child and compact or split packets only when needed.
- Fall back to scalar traversal when too few lanes remain active.
- Keep secondary, reflection, refraction, and diffuse bounce rays scalar until
  profiling demonstrates useful coherence.

### Integration plan

1. Add an isolated `SimdAabb` benchmark comparing scalar and packet slab tests.
2. Add `Bvh::intersectPacket()` for closest-hit primary rays only.
3. Generate neighboring primary rays in packet-width groups inside each tile.
4. Verify packet and scalar hit identity within numerical tolerance.
5. Benchmark simple primitives, dense meshes, depth of field, and motion blur.
6. Enable packet traversal only for workloads where it wins; expose a diagnostic
   switch to force scalar or SIMD mode.

### SIMD acceptance criteria

- Scalar traversal remains the default fallback and passes all existing tests.
- Packet traversal returns the same hit/miss decisions and nearest distances
  within tolerance.
- Coherent primary-ray benchmarks improve by at least 1.25x end to end.
- Depth-of-field or highly divergent workloads do not regress by more than 5%;
  otherwise they automatically use scalar traversal.
- If the feasibility gate is not met, document the prototype results and defer
  production SIMD integration rather than shipping extra complexity.

## Milestone 10 implementation order

1. Add adaptive accumulation records and fixed-path compatibility tests.
2. Implement batched convergence checks and adaptive tile scheduling.
3. Generalize `LightSource` around incident-light samples.
4. Add directional and spot lights with unit/reference tests.
5. Add finite-light range/cone/back-face culling and counters.
6. Add aggregate scene bounds for directional shadow limits where valid.
7. Benchmark adaptive sampling and light culling independently.
8. Prototype `std::simd` AABB packets behind a build feature check.
9. Integrate packet primary traversal only if the feasibility criteria pass.
10. Add Phase 10 fields to the JSON scene format after Phase 9 lands.

## Milestone 10 acceptance criteria

- Adaptive sampling and all new light types are configurable through JSON.
- Fixed and adaptive render modes both remain available.
- Directional and spot lights integrate with direct lighting, shadows, and
  tracing statistics.
- Point/spot influence culling measurably reduces unnecessary shadow rays.
- Benchmarks report image error, average samples per pixel, shadow rays saved,
  and render-time changes.
- SIMD support is optional, benchmark-gated, and never required for correctness.

## Phase 11: Future quality-of-life and platform work

- Explore GPU-accelerated tracing.
- Add scene editor tooling and live reload.
- Add additional output formats and metadata controls.

## Suggested Release Milestones

### v0.2 — Correct Renderer

Completed June 24, 2026.

### v0.3 — Clean Images

Completed June 24, 2026.

### v0.4 — First Path Tracer

Completed June 24, 2026.

### v0.5 — Realistic Lighting

Completed June 24, 2026.

### v0.6 — Physically Based Renderer

Phase 5 completed June 24, 2026.

### v0.7 — Accelerated Renderer

Phase 6 completed June 24, 2026.

### v1.0 — Scene-Capable Renderer

Completed June 24, 2026.

### v1.1 — Performance Quick Wins

Completed June 24, 2026.

### v1.2 — Relaxed Performance Tuning

Completed June 25, 2026.

### v1.3 — JSON Scene Descriptions

Complete Phase 9 with versioned JSON loading, resource registries, scene
validation, CLI overrides, and JSON migrations of the demo and chessboard.

### v1.4 — Adaptive Sampling and Lighting Efficiency

Complete Phase 10 with variance-guided adaptive sampling, directional and spot
lights, finite-light influence culling, and benchmark-gated SIMD primary-ray
traversal.
