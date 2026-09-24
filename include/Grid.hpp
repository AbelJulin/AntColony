#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "Ant.hpp"
#include "Cell.hpp"
#include "Colony.hpp"
#include "Config.hpp"

struct SimulationParameters {
    int sugarSources = Config::SugarSourceCount;
    int sugarPerSource = Config::SugarPerSource;
    int maxAntsPerColony = Config::MaxAntsPerColony;
    float pheromoneEvaporation = Config::PheromoneEvaporation;
    float pheromoneDiffusion = Config::PheromoneDiffusion;
    float combatChance = Config::CombatBaseChance;
};

class Grid {
public:
    explicit Grid(std::uint32_t seed = std::random_device{}(), SimulationParameters parameters = {});
    void update();
    void reset(std::uint32_t seed);
    bool addSugarAt(Coordinates p, int amount);
    bool removeSugarAt(Coordinates p, int amount);
    const auto& getCells() const { return cells; }
    const auto& getAnts() const { return ants; }
    const auto& getColonies() const { return colonies; }
    std::size_t getStep() const { return step; }
    std::size_t getTotalAliveAnts() const;
    std::size_t getPopulation(int colony) const;
    std::size_t getTerritoryCount(int colony) const;
    void exportStatsCsv(const std::string& filename) const;
    void printConsole() const;
    bool inspectCell(Coordinates p, std::string& out) const;
    std::uint32_t getSeed() const { return seed; }
    const SimulationParameters& getParameters() const { return parameters; }
    struct HistoryView { std::size_t step; int colony; std::size_t population; int food; std::size_t territory; };
    std::vector<HistoryView> getHistory() const;

private:
    std::array<std::array<Cell, Config::GridSize>, Config::GridSize> cells;
    std::vector<std::unique_ptr<Ant>> ants;
    std::array<Coordinates, Config::ColonyCount> nestPositions{};
    std::array<Colony, Config::ColonyCount> colonies;
    std::mt19937 rng;
    std::uint32_t seed;
    std::size_t step = 0;
    int nextAntId = 0;
    SimulationParameters parameters;
    struct HistoryRow {
        std::size_t step;
        int colony;
        std::size_t population;
        int food;
        std::size_t births, deaths, kills, foodCollected, foodDelivered, territory;
    };
    std::vector<HistoryRow> history;

    void initializeCells();
    void initializeNests();
    void initializeNestPheromones();
    void initializeSugar();
    void initializeAnts(int colony, Coordinates nest);
    void createNest(int colony, Coordinates position);
    bool createAnt(int colony, Coordinates position);
    void reproduceColonies();
    void updateAnt(Ant& ant);
    void moveAnt(Ant& ant, Cell& target);
    void swapAnts(Ant& ant, Cell& target);
    void layTrail(Ant& ant, Cell& target);
    void updatePheromones();
    void diffusePheromones();
    void cleanupDeadAnts();
    std::vector<Cell*> getNeighbors(Cell& cell);
    bool isInside(int x, int y) const;
    bool findFreePositionNearNest(int colony, Coordinates& result) const;
    bool hasReachedMaximumPopulation(int colony) const;
    void placeInitialSugar();
    void recordHistory();
    void validateAntPositions() const;
    void updateTerritory();
};
