#pragma once

#include <cstddef>

namespace Config {

// ============================================================
// WORLD
// ============================================================

constexpr int GridSize = 150;

constexpr int ColonyCount = 10;

constexpr int SugarSourceCount = 150;

constexpr int SugarPerSource = 100;


// ============================================================
// COLONIES & ANTS
// ============================================================

constexpr int ReproductionCost = 10;

constexpr int MaxAntsPerColony = 500;

constexpr int InitialAntArea = 4;

constexpr int NestSize = 2;

constexpr int NestDistanceFromCenter = GridSize / 2 - 5;


// ============================================================
// FOOD PHEROMONES
// ============================================================

// Initial pheromone value on a newly created food source.
constexpr float InitialSugarPheromone = 1.0f;

// Trail strength written on the food source itself when an ant picks up food.
// A carrying ant then lays a trail whose strength is multiplied by
// TrailStrengthDecay at every step, so the value increases toward the food.
constexpr float TrailInitialStrength = 1.0f;
constexpr float TrailStrengthDecay = 0.97f;

// A searching ant only follows a neighbouring cell whose pheromone is above
// this value AND higher than the value on its own cell (uphill = toward food).
// The renderer uses the same threshold, so what is drawn is what ants can follow.
constexpr float MinFollowPheromone = 0.02f;
constexpr float PheromoneGradientEpsilon = 1e-4f;

// Pheromone is multiplied by this value every simulation step.
//
// 0.999 = very slow (half-life ~23 s at 30 steps/s)
// 0.995 = slow (half-life ~4.6 s)  <- default
// 0.98  = fast (half-life ~1 s): a trail vanishes before a second ant can use it
constexpr float PheromoneEvaporation = 0.995f;

// Food pheromones never diffuse to neighbouring cells.
// They only exist on cells actually visited by ants.
constexpr float PheromoneDiffusion = 0.0f;

constexpr float MaxPheromone = 1.0f;


// ============================================================
// SIMULATION
// ============================================================

constexpr unsigned int FrameRate = 60;

// Number of simulation updates performed per second.
constexpr double SimulationStepsPerSecond = 30.0;

// Safety limit preventing too many simulation updates
// from being processed during a single rendered frame.
constexpr int MaxSimulationStepsPerFrame = 8;


// ============================================================
// WINDOW & RENDERING
// ============================================================

constexpr int WindowWidth = 1100;

constexpr int WindowHeight = 760;

// Size of the simulation grid in pixels.
constexpr int WorldPixelSize = 720;

constexpr float CellGap = 0.0f;


// ============================================================
// COMBAT
// ============================================================

constexpr float CombatBaseChance = 0.72f;


// ============================================================
// DEBUG
// ============================================================

// Useful while debugging the ant/cell synchronization.
// Set to false later if you want maximum performance.
constexpr bool ValidateAntPositions = true;

}