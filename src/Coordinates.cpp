#include "Coordinates.hpp"
std::array<Coordinates,8> getNeighborsCoordinates(Coordinates c){return {Coordinates(c.x-1,c.y),Coordinates(c.x+1,c.y),Coordinates(c.x,c.y-1),Coordinates(c.x,c.y+1),Coordinates(c.x+1,c.y+1),Coordinates(c.x-1,c.y-1),Coordinates(c.x+1,c.y-1),Coordinates(c.x-1,c.y+1)};}
