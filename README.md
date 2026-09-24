# Ant Colony Simulation

A C++17 / SFML agent-based simulation of eight competing ant colonies on a bounded
50x50 grid. Ants forage for sugar, lay and follow pheromone trails, fight ants from
other colonies, and turn the food they bring home into new ants.

The simulation is deterministic: a given seed always produces the same run.

## Table of contents

1. [Requirements](#1-requirements)
2. [Build and run](#2-build-and-run)
3. [Command line and configuration](#3-command-line-and-configuration)
4. [Controls](#4-controls)
5. [Architecture overview](#5-architecture-overview)
6. [World model](#6-world-model)
7. [Simulation loop](#7-simulation-loop)
8. [Ant behaviour](#8-ant-behaviour)
9. [Pheromone system](#9-pheromone-system)
10. [Crowding: swapping places](#10-crowding-swapping-places)
11. [Colonies, combat and reproduction](#11-colonies-combat-and-reproduction)
12. [Rendering and user interface](#12-rendering-and-user-interface)
13. [Statistics, export and analysis](#13-statistics-export-and-analysis)
14. [Configuration constants](#14-configuration-constants)
15. [Design rationale and tuning](#15-design-rationale-and-tuning)
16. [Testing without a window](#16-testing-without-a-window)
17. [Known limitations and unused code](#17-known-limitations-and-unused-code)
18. [Project layout](#18-project-layout)

---

## 1. Requirements

- A C++17 compiler (g++ or clang++).
- SFML **2.x**: graphics, window and system modules. SFML 3 is not supported
  (the code uses `sf::Quads`, `sf::Uint8`, ...).
- Optional, for the analysis script: Python 3 with `pandas` and `matplotlib`.

## 2. Build and run

```bash
make                       # macOS / Homebrew: expects SFML in /usr/local/opt/sfml@2
make SFML_PREFIX=/usr      # Linux with libsfml-dev installed
./ant_colony --seed 42
```

Run from the project root: the font is loaded from `assets/arial.ttf` (falling back
to `arial.ttf` in the current directory).

| Target | Effect |
|---|---|
| `make` | Optimised build (`-O2`, `-Wall -Wextra -Wpedantic`) |
| `make debug` | `-g -O0` |
| `make asan` | `-g -O0` with AddressSanitizer and UndefinedBehaviorSanitizer |
| `make clean` | Removes the binary, objects, dependency files and `simulation_stats.csv` |

The Makefile generates header dependency files (`-MMD -MP`), so editing a header
rebuilds every source file that includes it. Run `make clean` once when upgrading
from an older copy that still contains stale `.o` files.

## 3. Command line and configuration

Options can be given on the command line or as `key=value` lines in a config file
(`./ant_colony --config experiment.cfg`). Lines starting with `#` are comments.
Options are applied in order, so the last occurrence wins: put `--config` first if
you want command-line options to override it. Both `--option value` and
`--option=value` are accepted.

| CLI option | Config key | Default | Meaning |
|---|---|---|---|
| `--seed N` | `seed` | random | Seed of the random generator |
| `--sugar N` | `sugar_sources` | 50 | Number of sugar sources placed at start |
| `--sugar-per-source N` | `sugar_per_source` | 100 | Sugar units in each source |
| `--evaporation X` | `evaporation` | 0.995 | Food pheromone multiplier applied every tick |
| `--diffusion X` | `diffusion` | 0.0 | Parsed and stored, but **currently has no effect** (see section 17) |
| `--combat X` | `combat` | 0.72 | Base chance of winning a fight |
| `--max-ants N` | `max_ants` | 500 | Population cap per colony |
| `--help` | | | Print usage |

`experiment.cfg` is an example:

```
seed=42
sugar_sources=50
sugar_per_source=100
evaporation=0.995
diffusion=0.0
combat=0.72
max_ants=500
```

The seed drives everything random: colony colours, sugar placement, neighbour
shuffling, update order and combat outcomes. `R` restarts with the same seed.

## 4. Controls

| Input | Action |
|---|---|
| `Space` | Pause / resume |
| `R` | Restart with the current seed |
| `+` / `-` | Simulation speed x1.25 / /1.25 (1 to 480 steps per second) |
| `P` | Toggle food pheromone display |
| `A` | Toggle ants |
| `S` | Toggle sugar |
| `N` | Toggle nests |
| `G` | Toggle population graph |
| `I` | Toggle statistics panel |
| `D` | Toggle cell inspection (shows the cell under the mouse) |
| `H` | Toggle the on-screen help panel |
| `E` | Export `simulation_stats.csv` |
| Left mouse | Add sugar (click or drag) |
| Right mouse | Remove sugar (click or drag) |
| `Esc` | Quit |

Mouse sugar edits add or remove `Config::SugarPerSource` units (100) per action.
Sugar cannot be added on a nest or on a cell occupied by an ant.

## 5. Architecture overview

| Class / file | Responsibility |
|---|---|
| `Ant` | State of one ant: colony, position, previous position, carrying flag, alive flag, age, food trips, distance travelled, last action, current trail strength |
| `Cell` | State of one grid cell: occupant pointer, sugar amount, nest owner, per-colony food pheromone and nest pheromone |
| `Colony` | Colony identity, colour, nest position, food stock and counters (births, deaths, kills, collected, delivered) |
| `Grid` | Owns cells, ants, colonies and the random generator. Contains the update loop, movement, swapping, pheromone update, combat, reproduction, history and CSV export |
| `AntBehavior` | Pure decision rules: `evaluateTarget(ant, current, target)` returns an action and a priority |
| `Renderer` | Draws the world with one `sf::VertexArray` of quads, plus the side panel, graph and inspection box |
| `Simulation` | Window, fixed-timestep loop, keyboard and mouse handling |
| `Coordinates` | `(x, y)` pair and the eight-neighbour helper |
| `Config.hpp` | All compile-time constants |
| `main.cpp` | Command line and config file parsing |

`Grid` is the only class that mutates the simulation. `AntBehavior` never modifies
anything: it only inspects cells, which keeps the decision rules easy to test and
change.

### Data ownership and invariants

- `Grid` owns ants through `std::vector<std::unique_ptr<Ant>>`, so an ant's address
  is stable. Cells hold a **non-owning** `Ant*`.
- Invariant: for every living ant `a`, `cells[a.y][a.x].getAnt() == &a`, and a cell
  never points to a dead ant. Every code path that moves ants (`moveAnt`,
  `swapAnts`, combat, `cleanupDeadAnts`) preserves it.
- With `Config::ValidateAntPositions = true`, `validateAntPositions()` checks the
  invariant once per tick and prints `Warning: inconsistent ant <id>` to `stderr`
  if it is ever broken.
- Ants never leave the grid (no wrap-around) and never enter a nest cell, whatever
  the colony. Delivering sugar happens from an adjacent cell.

## 6. World model

### Grid

`Config::GridSize = 50`, so 2500 cells, addressed as `cells[y][x]`. Neighbourhood is
the eight surrounding cells (Moore neighbourhood), clipped at the borders.

### Nests

There are `ColonyCount = 8` colonies. Colony `c` has a `NestSize x NestSize` (2x2)
nest whose top-left cell is placed on a circle around the grid centre:

```
x = 24 + (int)(20 * sin(2*pi*c / 8))
y = 24 + (int)(20 * cos(2*pi*c / 8))          (radius = GridSize/2 - 5 = 20)
```

The nest cells are marked with the colony index. Nest cells can never hold ants or
sugar.

### Initial ants

Each colony starts with the free cells of a 4x4 square starting one cell up and left
of its nest (`InitialAntArea = 4`). The 2x2 nest occupies four of the sixteen cells,
so each colony starts with **12 ants** (96 in total).

### Initial sugar

`sugar_sources` distinct cells are drawn uniformly at random among cells with no
ant, no nest and no sugar. Each receives `sugar_per_source` units. Sugar does not
regenerate.

### Nest pheromone field

For every colony `c` and every cell, the static nest pheromone is

```
nest[c](x, y) = max(0, 1 - d / GridSize)
d = max(|x - nestX|, |y - nestY|)          (Chebyshev distance to the nest's top-left cell)
```

Chebyshev distance matches the eight-way movement, so every step that reduces `d`
lowers it by exactly one and diagonal moves are not penalised. Values range from 1
next to the nest down to 0.02 in the farthest corner, so a strictly closer
neighbour always has a strictly higher value (difference of at least 0.02).

This field is computed once and never changes. Territory statistics are derived from
it: a cell belongs to the colony whose nest field is highest there (ties go to the
lowest colony index).

## 7. Simulation loop

### Real time versus simulation time

`Simulation::run()` uses a fixed timestep accumulator. Each rendered frame it adds
the elapsed time and runs as many `grid.update()` calls as fit, at
`simulationStepsPerSecond` (default 30), capped at
`MaxSimulationStepsPerFrame = 8` per frame to avoid a spiral of death. With a 60 fps
limit this puts a ceiling of about 480 steps per second, which matches the `+` limit.

### One tick (`Grid::update`)

```
1. updatePheromones()      evaporate every food pheromone by the evaporation factor
2. snapshot ant count      ants created later this tick do not act yet
3. shuffle update order    fresh random permutation of the ants
4. for each ant: updateAnt()
5. cleanupDeadAnts()       clear dead ants from cells, erase them from the vector
6. reproduceColonies()     at most one birth per colony
7. validateAntPositions()  debug consistency check
8. ++step, recordHistory() one statistics row per colony
```

Why a random update order: with a fixed order the same ants would always win
contested cells and be first onto fresh trails. Newborns wait for the next tick.

### One ant (`Grid::updateAnt`)

1. Skip if dead, outside the grid, or if its cell does not point back to it.
2. Increase its age.
3. Collect the (up to eight) neighbouring cells in random order.
4. For each neighbour, call `evaluateTarget` and keep the decision with the lowest
   priority number. Among several decisions of the same priority the first in the
   shuffled order wins, which makes ties random. The one exception is the food
   trail rule, where the strongest pheromone wins (section 9).
5. While scanning, also note whether an uphill pheromone cell exists (occupied or
   not) and whether any neighbour holds sugar. These two flags drive dead-end
   cleanup.
6. If nothing applicable was found, or the best decision is `Stay`, do nothing.
7. Apply dead-end cleanup if it qualifies (section 9).
8. Execute the chosen action.

## 8. Ant behaviour

### Priority table

`evaluateTarget(ant, current, target)` classifies one neighbouring cell. A lower
number means higher priority. `Stay` carries priority 8 and is never selected as a
move; it means "this cell is unusable".

| Priority | Action | Condition on the target cell | Effect |
|---|---|---|---|
| 1 | `Attack` | Holds an ant of another colony | Probabilistic fight (section 11) |
| 2 | `CollectSugar` | Ant not carrying, cell has sugar | Take 1 unit, start a trail |
| 3 | `ReturnSugar` | Ant carrying, cell belongs to its own nest | Deliver 1 unit |
| 4 | `FollowNestPheromone` | Ant carrying, free cell with higher nest value | Move |
| 5 | `FollowSugarPheromone` | Ant not carrying, free cell above the threshold **and** higher than the current cell | Move |
| 5 | `Swap` | Ant carrying, non-carrying ally on a cell with higher nest value | Trade places |
| 6 | `Explore` | Ant not carrying, free cell, no trail to follow. For a carrier: free cell at the same nest distance (sidestep) | Move |
| 7 | `Swap` | Ant not carrying, non-carrying ally on the cell | Trade places |
| 8 | `Stay` | Anything else (foreign nest, ally that is carrying, worse cell for a carrier, ...) | Nothing |

Evaluation order inside `evaluateTarget`:

1. Cell holds an ant: other colony gives `Attack`; ally that is carrying gives
   `Stay`; ally not carrying gives `Swap` (priority 5 or 7 as above) or `Stay` for a
   carrier that would not get closer.
2. Not carrying and sugar present: `CollectSugar`.
3. Carrying and own nest: `ReturnSugar`.
4. Any other nest cell: `Stay`.
5. Carrier: closer cell gives `FollowNestPheromone`; same-distance cell gives
   `Explore`; otherwise `Stay`.
6. Searching ant: `FollowSugarPheromone` if uphill on the food trail, otherwise
   `Explore`.

### What each action does

- **Attack**: see section 11. The attacker never moves into the victim's cell.
- **CollectSugar**: sets the ant to carrying, removes 1 sugar unit from the cell,
  writes the initial trail (section 9), increments the colony's collected counter.
  The ant does not move: the sugar cell stays free of ants.
- **ReturnSugar**: adds 1 food to the colony, increments the delivered counter and
  the ant's trip counter, clears the carrying flag, resets its trail strength.
- **FollowNestPheromone / FollowSugarPheromone / Explore**: `moveAnt` moves the ant
  one cell, updates the previous position and the distance travelled, and, for a
  carrier, lays a trail.
- **Swap**: `swapAnts` exchanges two allied ants (section 10).

An ant carries at most one unit, and only ants that carry food deposit pheromone.
Exploring ants leave no trace, so the food field never becomes a map-wide noise
signal.

### Movement philosophy

Movement is deliberately simple: shuffled neighbours, first best priority wins.
There is no attraction to the centre, no repulsion from edges, no scoring function
and no memory. Exploration is a pure random walk over free cells.

## 9. Pheromone system

Two pheromones exist for each colony. Ants only read the fields of their own colony.

| | Nest pheromone | Food pheromone |
|---|---|---|
| Purpose | Lead carriers home | Lead searchers to food |
| Written by | Initialisation only | Ants carrying food |
| Dynamics | Static | Evaporates every tick |
| Read by | Carrying ants | Searching ants |
| Rendered | Never | Yes (`P`) |
| Diffusion | none | none (see section 17) |

### Food trail, step by step

Consider one ant picking up sugar at cell `S` while standing on cell `X`:

1. `S` receives strength `TrailInitialStrength = 1.0`.
2. `X` (the ant's own cell) receives `1.0 * 0.97 = 0.97`, and the ant's trail
   strength becomes `0.97`. Marking `X` closes the gap between the sugar and the
   first cell the ant will enter.
3. At each move the ant multiplies its trail strength by `TrailStrengthDecay = 0.97`
   and writes the result on the cell it enters.
4. On delivery the strength is reset to 0.

The result is a one-cell-wide trail whose value is highest at the food and decreases
along the return path. A trail of 20 steps ends at about `0.97^20 = 0.54`; 40 steps
gives about 0.30.

### Deposit rule: maximum, not sum

`Cell::depositSugarPheromone` sets the cell to `max(current, deposit)`, clamped to
`MaxPheromone = 1.0`. Adding instead would saturate every cell of a busy trail at 1.0
and destroy the gradient. With the maximum, a busy trail keeps its slope, and when
several ants reach a cell from different distances the strongest (shortest) one
defines its value.

### Following the trail

A searching ant treats a neighbour as followable when both hold:

```
target > MinFollowPheromone (0.02)
target > current + PheromoneGradientEpsilon (1e-4)
```

The second condition makes the ant climb only **uphill**, toward the food, and stops
it from walking back down toward the nest. Among followable neighbours it picks the
strongest; ties are random. If none qualifies it explores.

The renderer draws a cell as pheromone only above the same `0.02` threshold, so what
is displayed is what ants can actually follow.

### Evaporation

Every tick every food pheromone is multiplied by `PheromoneEvaporation` (default
0.995), a half-life of about 138 ticks (about 4.6 s at 30 steps per second). The
factor is multiplicative on all cells, so the relative slope of a trail is preserved
while it fades.

| Factor | Half-life (ticks) | Comment |
|---|---|---|
| 0.98 | 34 | Trails vanish before a second ant can use them |
| 0.99 | 69 | Short-lived trails |
| 0.995 | 138 | Default |
| 0.999 | 693 | Very persistent, slow to forget depleted sources |

### Dead-end cleanup

When a source is exhausted or removed, the trail leads to a cell with no food. A
searching ant standing on a marked cell erases that cell's food pheromone when all
of the following hold:

- its chosen action is `Explore` (no trail to follow),
- its current cell has a positive pheromone value,
- no neighbour is uphill, **counting occupied neighbours**,
- no neighbour holds sugar.

Counting occupied neighbours as uphill matters: a congested but valid trail is never
erased just because the next cell happens to be occupied. Repeated visits eat an
obsolete trail from its tip, so ants stop bouncing at the end of a dead trail instead
of waiting for evaporation.

### Carriers and the nest gradient

Carriers move to any free neighbour with a higher nest value (random among equals).
Because of the Chebyshev field, a return path is as long as the Chebyshev distance to
the nest. If no free closer cell exists they sidestep to a free cell at the same
distance instead of freezing, or swap with a non-carrying ally on a closer cell
(section 10).

## 10. Crowding: swapping places

Without a way to pass each other, a dense colony gridlocks around its nest. Cells
next to the nest fill up with searching ants that cannot move away because the cells
behind them are full of carriers waiting to come in, and the whole jam freezes
permanently.

Allied ants therefore trade places (`Grid::swapAnts`):

- A **carrier** may swap with a **non-carrying** ally standing on a cell that is
  closer to the nest. Free closer cells (priority 4) are preferred to swaps
  (priority 5), and sidesteps (priority 6) come after.
- A **searching** ant may swap with a non-carrying ally only as a last resort
  (priority 7), which shuffles a jammed crowd without disturbing free movement.
- A carrier is never displaced.

`swapAnts` exchanges the two cell pointers and the two positions, updates the
previous positions and the distance counters, and lets a carrier lay its trail on the
cell it enters, so the cell to ant invariant always holds.

## 11. Colonies, combat and reproduction

### Combat

When the best decision is `Attack`, with `age` capped at 100 ticks:

```
attackerPower = 1 + 0.15 * min(attackerAge, 100) / 100
defenderPower = 1 + 0.10 * min(defenderAge, 100) / 100
chance = clamp( combat * attackerPower / (attackerPower + defenderPower) * 2 , 0.05 , 0.95 )
```

With the default `combat = 0.72` and equal ages the chance is 72 %. A random number
decides the outcome:

- **Win**: the victim is killed and removed from its cell, the attacker's colony
  records a kill and the victim's colony a death. The attacker stays where it is.
- **Loss**: nothing happens (the attack is recorded as `Stay`); the defender does not
  counter-attack during the attacker's turn but will act on its own turn.

Because the chance is clamped, `--combat 0` still leaves a 5 % chance of winning.
Killed ants stay in the vector until `cleanupDeadAnts` at the end of the tick, and
are skipped when updated.

### Food and reproduction

Delivered food goes into the colony's stock. At the end of each tick and for each
colony, if population is below the cap and food is at least `ReproductionCost = 10`,
the simulation looks for a free cell (no ant, no nest) in square rings of radius 1
to 4 around the nest, spends 10 food and creates one ant there. If creation fails the
food is refunded. A colony therefore gains at most one ant per tick. Newborns start
acting on the next tick.

Population is counted by scanning living ants, which is cheap at this scale
(a few thousand ants at most).

### Colony extinction

A colony with zero ants cannot recover: nobody brings food, so no births happen.
Since sugar does not regenerate, the default world eventually runs out of sugar and
combat slowly thins the colonies (section 17).

## 12. Rendering and user interface

### Window and timing

The window is 1100x760. The world occupies a 720x720 pixel square, so a cell is
14.4 pixels. The right side holds the information panel. Frame rate is limited to 60.

### World drawing

One `sf::VertexArray` of `GridSize * GridSize` quads is rewritten every frame. The
colour of a cell is chosen in this order:

1. **Ant** (if ants are shown): the colour of its colony.
2. **Sugar** (if shown): `(245, 210, 80)`.
3. **Nest** (if shown): `(139, 69, 19)`.
4. **Food pheromone** (if pheromones are shown and the value exceeds 0.02): the
   colour of the colony with the strongest value, scaled by `0.3 + 0.7 * value` so
   that faint trails stay readable.
5. **Background**: `(12, 12, 18)`.

Nest pheromones and territories are never drawn.

### Side panel

- **Statistics** (`I`): step, seed, total ants, and per colony the population, food,
  territory, births, deaths and kills.
- **Graph** (`G`): a line per colony showing population over time, scaled to the
  maximum population and the current step.
- **Help** (`H`): list of controls.
- **Inspection** (`D`): a box showing the hovered cell: coordinates, sugar, nest
  owner, and if an ant is present its id, colony, last action, age, trips, distance
  and carrying flag, plus the territory owner.

## 13. Statistics, export and analysis

`Grid::recordHistory` appends one row per colony every tick. Pressing `E` writes
`simulation_stats.csv`:

```
step,colony,population,food,births,deaths,kills,food_collected,food_delivered,territory
```

| Column | Meaning |
|---|---|
| `step` | Tick number |
| `colony` | Colony index (0 to 7) |
| `population` | Living ants |
| `food` | Food currently in the colony's stock |
| `births`, `deaths`, `kills` | Cumulative counters |
| `food_collected`, `food_delivered` | Cumulative units picked up and delivered |
| `territory` | Cells whose highest nest pheromone belongs to this colony |

```bash
python3 analysis/analyze.py simulation_stats.csv
```

The script plots population, stored food and territory per colony and prints the
final state. For reproducible experiments fix the seed and change a single parameter
at a time.

## 14. Configuration constants

All in `include/Config.hpp`.

| Constant | Value | Meaning |
|---|---|---|
| `GridSize` | 50 | Grid width and height |
| `ColonyCount` | 8 | Number of colonies |
| `SugarSourceCount` | 50 | Default number of sugar sources |
| `SugarPerSource` | 100 | Default sugar per source; also the amount added or removed by a mouse click |
| `ReproductionCost` | 10 | Food needed for one new ant |
| `MaxAntsPerColony` | 500 | Default population cap |
| `InitialAntArea` | 4 | Side of the starting square of ants |
| `NestSize` | 2 | Side of a nest |
| `NestDistanceFromCenter` | 20 | Radius of the circle of nests |
| `TrailInitialStrength` | 1.0 | Value written on the food cell at pickup |
| `TrailStrengthDecay` | 0.97 | Trail strength multiplier per step of a carrier |
| `MinFollowPheromone` | 0.02 | Minimum value a searcher will follow and the renderer will draw |
| `PheromoneGradientEpsilon` | 1e-4 | Minimum margin for "uphill" |
| `PheromoneEvaporation` | 0.995 | Food pheromone multiplier per tick |
| `PheromoneDiffusion` | 0.0 | Diffusion parameter (unused, section 17) |
| `MaxPheromone` | 1.0 | Upper bound of a pheromone value |
| `FrameRate` | 60 | Frame limit |
| `SimulationStepsPerSecond` | 30 | Default tick rate |
| `MaxSimulationStepsPerFrame` | 8 | Ticks allowed per frame |
| `WindowWidth`, `WindowHeight` | 1100, 760 | Window size |
| `WorldPixelSize` | 720 | Size of the map in pixels |
| `CombatBaseChance` | 0.72 | Default combat parameter |
| `ValidateAntPositions` | true | Enable the per-tick consistency check |

Changing `GridSize`, `ColonyCount` or `WorldPixelSize` needs a rebuild. Nest
placement assumes the nests fit on the circle without overlapping, which holds for the
defaults.

## 15. Design rationale and tuning

| Problem in a naive model | Choice made here |
|---|---|
| Constant deposits give trails with no direction, so searchers walk toward the nest as often as toward the food | Deposit strength decays along the return path, and searchers only climb uphill |
| Additive deposits saturate | Deposit keeps the maximum |
| A gap between the sugar and the first marked cell lets followers stop one cell short | The sugar cell and the pickup cell are both marked |
| Evaporation of 0.98 wipes a trail in about a second | Default 0.995 |
| Old trails trap ants after a source is exhausted | Dead-end cleanup |
| Carriers freeze when closer cells are occupied | Sidestep and swap |
| Dense colonies gridlock around the nest | Ant swapping (section 10) |
| Fixed update order favours early ants | Random update order every tick |

Tuning hints:

- **Sharper or weaker gradient**: change `TrailStrengthDecay`. Lower values give a
  steeper slope but shorter reach: a trail shorter than about `ln(0.02)/ln(decay)`
  steps (about 128 at 0.97) stays followable at its far end.
- **Longer memory**: raise the evaporation factor toward 0.999. Trails then persist
  after a source is gone; dead-end cleanup limits the harm.
- **Faster growth or collapse**: `ReproductionCost`, `MaxAntsPerColony`,
  `SugarPerSource`.
- **More aggression**: raise `--combat`; remember the 5 to 95 percent clamp.

## 16. Testing without a window

The simulation core does not need a window. `Grid` and its dependencies only use
`sf::Color` (in `Colony`), so a headless program can link against SFML's graphics and
system libraries and drive `Grid` directly:

```cpp
#include <iostream>
#include "Grid.hpp"

int main() {
    SimulationParameters p;            // defaults from Config.hpp
    p.sugarPerSource = 5000;
    Grid grid(42, p);                  // fixed seed
    for (int i = 0; i < 20000; ++i) grid.update();

    std::size_t delivered = 0;
    for (const auto& c : grid.getColonies()) delivered += c.getFoodDelivered();
    std::cout << "ants=" << grid.getTotalAliveAnts()
              << " delivered=" << delivered << "\n";
}
```

```bash
g++ -std=c++17 -O2 -Iinclude test.cpp src/Grid.cpp src/Cell.cpp src/Ant.cpp \
    src/AntBehavior.cpp src/Colony.cpp src/Coordinates.cpp \
    -o test -lsfml-graphics -lsfml-system
```

Useful checks: run the same seed twice and compare (determinism), run with
`make asan`, watch `stderr` for `inconsistent ant` warnings, and count ants that have
not moved for many ticks to detect gridlock.

## 17. Known limitations and unused code

Behaviour:

- **Sugar does not regenerate.** With working trails the default world (50 sources of
  100 units) is exhausted after roughly a thousand ticks. Births then stop and
  combat slowly reduces the colonies. Use `--sugar-per-source`, or paint sugar with
  the mouse, to prolong a run.
- **Exploration is a pure random walk.** Searchers have no directional persistence.
  This is intentional, but it is the main limit on how quickly new sources are found.
- **`--diffusion` has no effect.** `Grid::diffusePheromones()` and
  `Cell::diffusePheromones()` are implemented but never called from `Grid::update()`.
  Diffusion is effectively off.
- **Mouse to cell conversion** in `Renderer::pixelToCell` divides by `(int)cellSize`
  (14) instead of the real 14.4, so clicks drift toward the right and bottom edges.
  Out-of-range results are rejected by the bounds check, but cells near the far
  edges are hard to hit precisely.
- **History grows without bound**: one row per colony per tick is kept for the graph
  and CSV export, and recording computes territory each tick. Very long runs use
  growing memory and time.
- The `--combat 0` case still leaves a 5 % win chance (clamp).

Unused code left in place:

- `Action::FollowSugarTrail` (enum value with no producer).
- `Ant::previousPosition` is stored and updated but not read by any rule.
- `Config::InitialSugarPheromone` and `Config::CellGap`.
- `Renderer::updateTerritory` (territory overlay is disabled) and
  `Grid::updateTerritory` (empty).
- The `territory` parameter of `Renderer::draw`.

## 18. Project layout

```
Makefile
experiment.cfg          Example configuration
README.md
analysis/
  analyze.py            Plots the exported CSV
assets/
  arial.ttf             Font used by the UI
include/
  Ant.hpp  AntBehavior.hpp  Cell.hpp  Colony.hpp  Config.hpp
  Coordinates.hpp  Grid.hpp  Renderer.hpp  Simulation.hpp
src/
  Ant.cpp  AntBehavior.cpp  Cell.cpp  Colony.cpp  Coordinates.cpp
  Grid.cpp  Renderer.cpp  Simulation.cpp  main.cpp
```
