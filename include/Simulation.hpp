#pragma once
#include <cstdint>
#include <string>
#include <SFML/Graphics.hpp>
#include "Grid.hpp"
#include "Renderer.hpp"

class Simulation {
public:
    Simulation(Grid& grid, std::uint32_t seed);
    void run();

private:
    sf::RenderWindow window;
    Grid& grid;
    Renderer renderer;
    bool paused = false;
    bool showPheromones = true;
    bool showAnts = true;
    bool showSugar = true;
    bool showNests = true;
    bool showStats = true;
    bool showTerritory = false;
    bool showGraph = false;
    bool showHelp = false;
    bool debugCell = false;
    std::string debugText;
    double simulationStepsPerSecond = Config::SimulationStepsPerSecond;
    std::uint32_t seed;

    void handleEvents();
    void handleMouse(const sf::Event& event);
    void restart();
};
