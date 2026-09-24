#include <algorithm>
#include "Cell.hpp"
#include "Ant.hpp"
Cell::Cell(Coordinates p):position(p){}
void Cell::placeAnt(Ant& a){ant=&a;} void Cell::removeAnt(){ant=nullptr;}
void Cell::setSugar(int a){sugar=std::max(0,a);} void Cell::removeSugar(int a){sugar=std::max(0,sugar-a);} void Cell::setNest(int c){nestColony=c;nestPheromones[c]=1.f;}
void Cell::addSugarPheromone(int c,float a){sugarPheromones[c]=std::min(Config::MaxPheromone,sugarPheromones[c]+a);}
// Deposit = keep the strongest value, never accumulate. Accumulating saturates
// every cell of a busy trail at MaxPheromone and destroys the gradient that
// searching ants need in order to know which way the food is.
void Cell::depositSugarPheromone(int c,float s){sugarPheromones[c]=std::min(Config::MaxPheromone,std::max(sugarPheromones[c],s));}
void Cell::clearSugarPheromone(int c){sugarPheromones[c]=0.f;}
void Cell::evaporatePheromones(float evaporation){for(float&v:sugarPheromones)v*=evaporation;}
void Cell::diffusePheromones(const std::array<float,Config::ColonyCount>& s,float diffusion){for(int c=0;c<Config::ColonyCount;++c)sugarPheromones[c]=std::min(Config::MaxPheromone,sugarPheromones[c]*(1.f-diffusion)+s[c]*diffusion);}
void Cell::setNestPheromone(int c,float v){nestPheromones[c]=v;}
bool Cell::containsAnt()const{return ant!=nullptr;} bool Cell::containsSugar()const{return sugar>0;} bool Cell::containsNest()const{return nestColony>=0;} bool Cell::isEmpty()const{return !containsAnt()&&!containsNest();}
Coordinates Cell::getPosition()const{return position;} Ant* Cell::getAnt()const{return ant;} int Cell::getSugar()const{return sugar;} int Cell::getNestColony()const{return nestColony;}
const std::array<float,Config::ColonyCount>& Cell::getSugarPheromones()const{return sugarPheromones;} const std::array<float,Config::ColonyCount>& Cell::getNestPheromones()const{return nestPheromones;}
