#pragma once
#include <cstddef>
#include <SFML/Graphics/Color.hpp>
#include "Coordinates.hpp"
class Colony {
public:
    Colony()=default; Colony(int id,Coordinates nestPosition,sf::Color color);
    int getId() const; int getFood() const; Coordinates getNestPosition() const; sf::Color getColor() const;
    void addFood(int amount); bool spendFood(int amount);
    void recordBirth(); void recordDeath(); void recordKill(); void recordFoodCollected(); void recordFoodDelivered();
    std::size_t getBirths()const; std::size_t getDeaths()const; std::size_t getKills()const; std::size_t getFoodCollected()const; std::size_t getFoodDelivered()const;
private:
    int id=-1,food=0; Coordinates nestPosition; sf::Color color; std::size_t births=0,deaths=0,kills=0,foodCollected=0,foodDelivered=0;
};
