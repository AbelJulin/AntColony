#include "Colony.hpp"
Colony::Colony(int i,Coordinates p,sf::Color c):id(i),nestPosition(p),color(c){}
int Colony::getId()const{return id;} int Colony::getFood()const{return food;} Coordinates Colony::getNestPosition()const{return nestPosition;} sf::Color Colony::getColor()const{return color;}
void Colony::addFood(int a){food+=a;} bool Colony::spendFood(int a){if(a>food)return false;food-=a;return true;}
void Colony::recordBirth(){++births;}void Colony::recordDeath(){++deaths;}void Colony::recordKill(){++kills;}void Colony::recordFoodCollected(){++foodCollected;}void Colony::recordFoodDelivered(){++foodDelivered;}
std::size_t Colony::getBirths()const{return births;}std::size_t Colony::getDeaths()const{return deaths;}std::size_t Colony::getKills()const{return kills;}std::size_t Colony::getFoodCollected()const{return foodCollected;}std::size_t Colony::getFoodDelivered()const{return foodDelivered;}
