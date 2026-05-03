# Math Art Cards

Generative / mathematical animation cards for RestScreen. Each runs as a `RestCard` subclass using an LVGL canvas. Cards show for the standard 60s interval; animations run via `tick()`.

---

## Shared design rules

- **LVGL canvas** (`lv_canvas_create`) with a raw pixel buffer — direct pixel writes, no LVGL shapes
- **Incremental rendering** — compute a budget of pixels/cells per `tick()` call (~2–5ms budget), never block the loop
- **Random seed on `show()`** — each visit looks different; color palette + initial params randomized
- **Circular clip** — pixels outside `(x-120)²+(y-120)² > 120²` stay black; respects the round display
- **`isVisible()`** — always true (pure generative, no AppState dependency)

---

## Option A — Julia Set ← start here

**What it is:** Parametric companion to Mandelbrot. For each pixel `(x,y)` mapped to complex plane, iterate `z → z²+c` until escape or max iterations. `c` is a complex constant — changing it morphs the shape completely.

**Why it fits:** Infinite detail, mathematically rigorous, visually hypnotic. Hits all requirements.

**"Starts center, fills border" mechanic:** Render pixels in **outward spiral order** from center. Interior pixels (in-set) appear first; escape contours expand outward ring by ring. Looks like the fractal is crystallizing from the middle.

**Render strategy:**
- ~2000 px/tick at 100 iterations max → full 240×240 in ~30 ticks (~150ms at 5ms/tick)
- After full render: slowly drift `c` along a curve → fractal morphs in real time
- Or: zoom into a point on the boundary (most interesting region)

**Randomness:**
- `c` picked randomly on unit circle (or from a curated set of beautiful values)
- Color palette: random HSV band, or pick 3 anchor colors and interpolate by escape count

**Animation modes (pick one per visit):**
1. `CRYSTALLIZE` — spiral render from center, then static
2. `MORPH` — render then slowly rotate `c` (shape changes continuously)
3. `ZOOM` — fix `c`, slowly zoom into boundary point

**Known good `c` values:**
```
c = -0.7 + 0.27i       // classic dendrite
c = -0.4 + 0.6i        // spirals
c = 0.285 + 0.01i      // near-Mandelbrot structure
c = -0.8 + 0.156i      // sea horse valley
c =  0.45 + 0.1428i    // douady rabbit
```

**Complexity:** Medium. ~150 lines. Canvas buffer = 240×240×2 bytes = ~115KB (fits in PSRAM).

---

## Option B — Conway's Game of Life

**What it is:** Cellular automaton. Grid of on/off cells. Each tick: cell survives if 2–3 neighbors, is born if exactly 3, otherwise dies.

**Why it fits:** Escher-like emergent complexity — three rules produce gliders, oscillators, chaos. Pattern recognition in the noise is addictive.

**"Starts center, fills border" mechanic:** Seed a small random cluster (e.g. 20×20 cells) at center. Life spreads outward, fills screen over 50–100 generations. Some runs die out (re-seed); others explode. Natural variation.

**Grid options:**
| Cell size | Grid | Buffer |
|---|---|---|
| 3×3 px | 80×80 = 6400 cells | ~800 bytes (bitarray) |
| 4×4 px | 60×60 = 3600 cells | ~450 bytes |

**Color ideas:**
- Binary (white on black) — clean, classic
- **Age coloring** — cells colored by how many generations they've survived (young=bright, old=dim). Beautiful trails.
- Random palette per run — born cells get a random hue, fade on death

**Animation:** One generation per N ticks (tune for speed — ~5–10 fps feels right). Dead-simple to implement.

**Special touches:**
- Detect stable/dead state → re-seed from center with new random pattern
- Optionally seed with a known pattern (glider gun, R-pentomino) for guaranteed spread

**Complexity:** Low. ~100 lines. Simplest to implement correctly.

---

## Option C — Plasma / Diamond-Square

**What it is:** Recursive midpoint displacement. Divide screen into 4, assign random midpoint color, recurse. Produces smooth flowing color fields.

**"Starts center, fills border":** Natural — center cell is first, subdivision expands outward.

**Animation:** Slow continuous color shift (add time offset to color function). Or re-render with new seed every N seconds.

**Complexity:** Very low. ~60 lines. Runs in <50ms total (no incremental needed).

**Vibe:** More "screensaver" than math-art. Less depth than Julia or GoL.

---

## Option D — Hilbert Curve

**What it is:** Space-filling curve. Order 1 = small square. Order N = visits every cell in an NxN grid exactly once. At order 7 it fills 128×128, touching every corner.

**"Starts center, fills border":** Animate by drawing curve segments one by one. Path snakes from center toward edges. At full draw, the entire circle is covered.

**Color:** Gradient along path length (rainbow, or two-color fade). Or color by recursion depth.

**Complexity:** Low. ~80 lines. Pure vector, no canvas buffer needed (use `lv_canvas_draw_line`).

**Vibe:** Geometric, meditative. Very different from Julia/GoL. Good contrast card.

---

## Option E — L-System / Fractal Tree

**What it is:** String rewriting system. `F → F[+F][-F]` repeated N times produces branching trees, ferns, dragon curves.

**"Starts center, fills border":** Trunk starts at center-bottom, branches fill upward and outward.

**Animation:** Grow depth level 1→N over card lifetime. Or slowly sway branches (perturb angles with sin wave).

**Color:** Branch color by depth (trunk dark, tips bright). Or seasonal palette.

**Complexity:** Low-medium. ~100 lines. String buffer can get large at high depth — cap at depth 6–7.

**Vibe:** Organic, botanical. Escher-adjacent (he loved plant/symmetry).

---

---

## Option F — Langton's Ant

**What it is:** One or more "ants" on a grid. Each step: on a white cell → turn right, flip to black, move forward; on a black cell → turn left, flip to white, move forward. Single ant produces 10k steps of chaos then suddenly locks into a repeating diagonal "highway" forever. Multiple ants with different rule sets collide and create wild emergent patterns.

**Why it fits:** Same cellular automaton spirit as GoL, but deterministic emergence — the highway "reveal" is a satisfying payoff. Very different character.

**Memory:** Grid = 1 bit/cell → uint8 grid at COLS×ROWS = same as GoL. Ants = tiny struct array (pos + direction). Trivial.

**Render strategy:** One step per tick (or N steps per tick for speed). No incremental render needed — grid is always fully drawn each frame.

**Randomness:** Ant start position + initial heading randomized. Optional: multi-ant with 2–4 ants using different rule variants (LLRR, LR, RLL, etc.).

**Controls:** Encoder = speed, Button = reseed new random start.

**Complexity:** Very low. ~80 lines. Easiest next card after GoL.

---

## Option G — Particle Life

**What it is:** N particles of K species. Each species-pair has an attraction/repulsion coefficient (randomly chosen per run). Particles apply forces to all others within a radius, update velocity + position. Emergent flocking, crystallization, orbiting, predator-prey dynamics — all from random force tables.

**Why it fits:** Completely different visual character from Julia and GoL — fluid, alive, looks like a living ecosystem. Highest visual impact of all options.

**Memory:** N=150 particles × (x, y, vx, vy, species) as floats → ~3KB. Pixel buffer still needed for rendering (115KB). Total ~118KB.

**Render strategy:** Each frame: clear pixBuf, draw N colored dots (5×5 px each), invalidate canvas. O(N²) force calc at N=150 = 22,500 pairs/frame — fast enough at 5ms budget.

**Randomness:** Force table (K×K matrix) fully randomized on reseed. Species colors randomized. K=4–6 species feels right.

**Controls:** Encoder = simulation speed, Button = randomize force table (new "ecosystem").

**Complexity:** Medium. ~120 lines. Most visually impressive.

---

## Option H — Reaction-Diffusion (Gray-Scott)

**What it is:** Two chemical species (U, V) diffuse across a grid and react. Produces coral, leopard spots, labyrinths, solitons depending on feed/kill parameters.

**Why it fits:** Stunning organic patterns. Very different from all others.

**Memory blocker:** Float per cell × 2 species × full grid = 240×240×4×2 ≈ 460KB. **No PSRAM on M5Dial — not feasible at full res.** Reduced to 80×80 = ~51KB float grid — works but looks coarse. Marginal.

**Status:** Parked unless PSRAM becomes available or a coarse-grid aesthetic is acceptable.

---

## Implementation plan

### Phase 1 — Julia Set standalone test
- Temporary `setup()`/`loop()` sketch outside card system
- Iterate on: color mapping, render speed, spiral order, `c` selection
- No card boilerplate until visuals are right

### Phase 2 — Wrap as `FractalCard`
- Implement `RestCard` interface
- Canvas buffer in PSRAM (`ps_malloc`)
- `show()`: pick random mode + `c` + palette, start render
- `tick()`: advance render budget or animate
- `hide()`: stop animation, free nothing (buffer reused)

### Phase 3 — Conway's Game of Life as `LifeCard`
- Same structure, much simpler render
- Two buffers (current + next gen), double-buffered swap

### Phase 4 — Others if desired
- Add remaining options as separate cards, each ~1 day of work
- All share canvas infrastructure from Phase 2

---

## Notes / constraints

- PSRAM available on M5Dial (ESP32-S3). Canvas buffer (115KB) must use `ps_malloc`, not stack/heap.
- `lv_canvas_set_px` is slow per-call. Prefer writing directly to the pixel buffer, then `lv_obj_invalidate(canvas)`.
- `tick()` runs every loop (~5ms budget). Overrunning causes display jank. Measure with `millis()` and stop early.
- Circular clip: precompute a lookup table of valid x ranges per row at `init()` time — faster than per-pixel sqrt.
