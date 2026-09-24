#pragma once
#include <cstddef>
#include "Coordinates.hpp"

enum class Action { Attack, CollectSugar, ReturnSugar, FollowNestPheromone, FollowSugarTrail, FollowSugarPheromone, Explore, Swap, Stay };

class Ant {
public:
    Ant(int id, int colony);
    void setCarryingSugar(bool carrying);
    void kill();
    void setPosition(Coordinates position);
    void setPreviousPosition(Coordinates position);
    void setLastAction(Action action);
    void incrementAge();
    void recordFoodTrip();
    void recordDistance();
    void setTrailStrength(float strength);
    Coordinates getPosition() const;
    Coordinates getPreviousPosition() const;
    int getColony() const;
    int getId() const;
    bool isCarryingSugar() const;
    bool isAlive() const;
    std::size_t getAge() const;
    std::size_t getFoodTrips() const;
    std::size_t getDistanceTravelled() const;
    float getTrailStrength() const;
    Action getLastAction() const;
private:
    int colony;
    int id;
    Coordinates position;
    Coordinates previousPosition{-1000, -1000};
    bool carryingSugar = false;
    bool alive = true;
    std::size_t age = 0;
    std::size_t foodTrips = 0;
    std::size_t distanceTravelled = 0;
    // Intensity of the food trail this ant is currently laying. It starts at the
    // maximum on the food source and decays with every step, so the trail gets
    // stronger the closer it is to the food (this is what gives it a direction).
    float trailStrength = 0.f;
    Action lastAction = Action::Stay;
};
