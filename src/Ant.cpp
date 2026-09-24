#include "Ant.hpp"
Ant::Ant(int id,int colony):colony(colony),id(id){}
void Ant::setCarryingSugar(bool v){carryingSugar=v;} void Ant::kill(){alive=false;} void Ant::setPosition(Coordinates p){position=p;}
void Ant::setPreviousPosition(Coordinates p){previousPosition=p;} void Ant::setLastAction(Action a){lastAction=a;} void Ant::incrementAge(){++age;} void Ant::recordFoodTrip(){++foodTrips;} void Ant::recordDistance(){++distanceTravelled;}
Coordinates Ant::getPosition()const{return position;} Coordinates Ant::getPreviousPosition()const{return previousPosition;} int Ant::getColony()const{return colony;} int Ant::getId()const{return id;} bool Ant::isCarryingSugar()const{return carryingSugar;} bool Ant::isAlive()const{return alive;} std::size_t Ant::getAge()const{return age;} std::size_t Ant::getFoodTrips()const{return foodTrips;} std::size_t Ant::getDistanceTravelled()const{return distanceTravelled;} Action Ant::getLastAction()const{return lastAction;}
void Ant::setTrailStrength(float s){trailStrength=s;} float Ant::getTrailStrength()const{return trailStrength;}
