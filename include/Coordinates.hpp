#pragma once
#include <array>
struct Coordinates { int x=0; int y=0; Coordinates()=default; Coordinates(int xv,int yv):x(xv),y(yv){} };
std::array<Coordinates,8> getNeighborsCoordinates(Coordinates position);
inline bool operator==(Coordinates a, Coordinates b){return a.x==b.x&&a.y==b.y;}
inline bool operator!=(Coordinates a, Coordinates b){return !(a==b);}
