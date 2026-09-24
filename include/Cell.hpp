#pragma once
#include <array>
#include "Coordinates.hpp"
#include "Config.hpp"
class Ant;
class Cell {
public:
    Cell()=default; explicit Cell(Coordinates position);
    void placeAnt(Ant& ant); void removeAnt();
    void setSugar(int amount); void removeSugar(int amount); void setNest(int colony);
    void addSugarPheromone(int colony,float amount);
    void depositSugarPheromone(int colony,float strength);
    void clearSugarPheromone(int colony);
    void evaporatePheromones(float evaporation);
    void diffusePheromones(const std::array<float,Config::ColonyCount>& sugarValues,float diffusion);
    void setNestPheromone(int colony,float value);
    bool containsAnt() const; bool containsSugar() const; bool containsNest() const; bool isEmpty() const;
    Coordinates getPosition() const; Ant* getAnt() const; int getSugar() const; int getNestColony() const;
    const std::array<float,Config::ColonyCount>& getSugarPheromones() const;
    const std::array<float,Config::ColonyCount>& getNestPheromones() const;
private:
    Coordinates position; Ant* ant=nullptr; int sugar=0; int nestColony=-1;
    std::array<float,Config::ColonyCount> sugarPheromones{};
    std::array<float,Config::ColonyCount> nestPheromones{};
};
