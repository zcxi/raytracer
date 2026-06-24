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

- Add axis-aligned bounding boxes.
- Build a bounding volume hierarchy for scene geometry.
- Traverse the BVH iteratively or with shallow recursion.
- Render image tiles instead of maintaining shared ray queues.
- Benchmark intersection throughput and render time per sample.
- Add release-build settings and profile before applying low-level optimizations.

**Completion target:** scenes with many objects and hundreds of samples per pixel
remain practical to render.

## Phase 7: Camera, Geometry, and Scene Features

- Add thin-lens depth of field with aperture and focus-distance controls.
- Add motion blur by sampling ray time.
- Add triangles and triangle meshes.
- Load common mesh formats such as OBJ or glTF.
- Add texture coordinates, image textures, normal maps, and procedural textures.
- Add transforms so geometry can be translated, rotated, and scaled independently.

**Visible result:** richer scenes with photographic focus, detailed assets, and
textured surfaces.

### Phase 8: Quality of life improvements
- Add a table to set objects on
- Add output format in png or jpeg
- Explore the use of Gpu Accelerated tracing

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

Phase 5 completed June 24, 2026. Phase 6 acceleration remains.

### v1.0 — Scene-Capable Renderer

Complete the selected Phase 7 features, document the scene format, and provide a
small reference-scene suite.
