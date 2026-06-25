# Partial Re-rendering Design

## Motivation

Interactive scenes such as a chess game should not require restarting the
entire render after every move. Most scene geometry, camera visibility, and
pixel history remain unchanged.

A partial re-render system should preserve valid accumulation data, identify
the image regions affected by a scene edit, and concentrate new samples in
those regions.

Moving one object can affect more than its visible silhouette. It can also
change:

- Cast shadows
- Reflections and transmission
- Indirect illumination
- Previously occluded geometry
- Depth-of-field and motion-blur coverage

The implementation should therefore use conservative invalidation initially,
then become more precise as additional rendering metadata is introduced.

## Core architecture

### Mutable scene objects

Every scene object should have a stable identifier and mutable transform:

```cpp
scene.updateTransform("white-knight-g1", newTransform);
```

Updating an object should:

1. Save its old world-space bounds.
2. Apply the new transform.
3. Calculate its new world-space bounds.
4. Mark the acceleration structure and affected image tiles dirty.

For BVH maintenance, begin with refitting ancestor bounds. Perform a complete
rebuild only when refitting produces a sufficiently degraded hierarchy.

Chess moves usually alter one piece, or two pieces during a capture or
castling, making refitting much cheaper than rebuilding the scene BVH.

### Persistent accumulation

Do not clear the renderer's entire accumulation buffer between scene updates.
Preserve per-pixel:

- Radiance sum
- Sample count
- Luminance mean and variance
- Convergence state
- Albedo
- Surface normal
- World-space hit position
- First-hit object ID
- Depth

Only records invalidated by a scene edit should be reset immediately.

### Dirty tile mask

Use the renderer's existing tile grid as the first invalidation granularity.
For each moved object:

1. Project its old world-space bounds into screen space.
2. Project its new world-space bounds into screen space.
3. Take the union of both projected regions.
4. Expand the region for antialiasing and lens effects.
5. Mark intersecting tiles dirty.

The old bounds cover newly revealed geometry, while the new bounds cover the
piece at its destination.

An initial implementation should use 16-by-16 tiles. Pixel-level invalidation
can be added later if profiling shows that tile granularity wastes meaningful
work.

### Shadow invalidation

A moved piece can alter shadows well outside its screen-space silhouette.

The simplest conservative chess-specific approach is to invalidate the
screen-space bounds of the board whenever a piece moves. This still preserves
the background and surrounding environment while preventing visibly stale
piece shadows.

A more precise implementation can calculate a light-space affected region:

- Directional lights project old and new object bounds along the light
  direction.
- Point and spot lights form conservative volumes from the light position
  through the moved bounds.
- Receiving surfaces intersected by these volumes contribute additional dirty
  screen-space tiles.

### Selective accumulation reset

Dirty records restart sampling:

```cpp
for (const Pixel& pixel : dirtyPixels) {
    accumulation[pixel].reset();
}
```

Unchanged records retain their accumulated samples and variance estimates.
The adaptive scheduler should prioritize:

1. Pixels with completely invalid history.
2. Tiles bordering invalid regions.
3. Existing high-variance pixels.
4. Stable pixels receiving occasional refresh samples.

## Indirect illumination

Perfectly identifying every pixel affected by a global-illumination change is
expensive. A moved piece may slightly affect most of the image through indirect
light paths.

A practical hybrid strategy is:

- Fully reset directly affected pixels.
- Preserve but decay history elsewhere.
- Spend most samples on dirty tiles.
- Reserve a small sampling budget for the rest of the image.

For example:

```cpp
record.rgbSum *= 0.95;
record.sampleCount =
    static_cast<unsigned int>(record.sampleCount * 0.95);
record.converged = false;
```

A reasonable initial budget is 90 percent for dirty pixels and 10 percent for
background refresh. This gradually corrects subtle global-lighting changes
without pausing for a complete frame render.

Separating direct and indirect illumination into different accumulation
buffers would later allow direct-light changes to be reset aggressively while
retaining more indirect history.

## Temporal reprojection

A more advanced renderer can reuse history after camera movement as well as
object movement.

Store world position, depth, normal, material or object ID, and radiance for
each pixel. For the next frame:

1. Reproject previous surface samples through the current camera.
2. Compare object ID, depth, and normal.
3. Accept matching history.
4. Reject moved surfaces, disocclusions, and mismatched geometry.
5. Fill newly visible regions with fresh samples.

History clamping should prevent stale bright samples from producing trails or
ghosting.

## Proposed implementation phases

### Phase A: Practical chess updates

- Add stable object IDs.
- Add mutable object transforms.
- Preserve accumulation between renders.
- Add BVH refitting for moved objects.
- Project old and new bounds into screen space.
- Add a dirty tile mask.
- Reset and adaptively sample dirty tiles.
- Conservatively invalidate the visible board region for shadows.
- Support captures, castling, promotion, and en passant as multi-object edits.

This phase provides the best performance improvement for the least
implementation complexity.

### Phase B: Precise lighting invalidation

- Calculate light-space dirty regions.
- Track first-hit object IDs and depth per tile or pixel.
- Separate direct and indirect accumulation.
- Reset direct-light history precisely.
- Decay indirect-light history globally.
- Add counters for reused pixels, invalidated pixels, and refresh samples.

### Phase C: General temporal rendering

- Reproject previous-frame history.
- Add depth, normal, and object-ID rejection.
- Detect disocclusions.
- Clamp temporal history.
- Refresh stable regions continuously.
- Apply the denoiser after each interactive update.
- Support camera movement and animated scenes.

## Suggested APIs

```cpp
struct SceneEdit {
    ObjectId object;
    Transform previousTransform;
    Transform currentTransform;
};

struct InvalidationResult {
    TileMask dirtyTiles;
    std::size_t invalidatedPixels;
};

InvalidationResult Scene::applyEdits(
    const std::vector<SceneEdit>& edits);

void Renderer::resume(
    const TileMask& dirtyTiles,
    const PartialRenderSettings& settings);
```

`PartialRenderSettings` should include:

- Dirty-region sample budget
- Background refresh percentage
- History decay factor
- Tile expansion margin
- Whether to invalidate the full board shadow region
- Maximum time budget per interactive update

## Verification and benchmarks

Correctness tests should cover:

- A normal move
- A capture
- Castling
- Promotion
- En passant
- Moving a piece into and out of another piece's shadow
- Revealing previously hidden board geometry
- Reflections containing moved pieces
- Deterministic results after the image fully reconverges

Benchmarks should report:

- BVH refit versus rebuild time
- Percentage of pixels retaining history
- Number of invalidated tiles
- Time to first usable updated image
- Time to reconvergence
- RMSE against a clean full-frame render
- Ghosting or stale-shadow failures

The primary success metric for chess should be the latency from submitting a
move to displaying a visually correct updated board, followed by convergence
to a result matching a clean full render within the chosen error tolerance.
