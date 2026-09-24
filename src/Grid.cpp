#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include "Grid.hpp"
#include "AntBehavior.hpp"

namespace {
sf::Color colonyColor(std::mt19937& rng){std::uniform_int_distribution<int>d(70,230);return sf::Color((sf::Uint8)d(rng),(sf::Uint8)d(rng),(sf::Uint8)d(rng));}
}
Grid::Grid(std::uint32_t s,SimulationParameters p):rng(s),seed(s),parameters(p){reset(s);}
void Grid::initializeCells(){for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x)cells[y][x]=Cell({x,y});}
void Grid::reset(std::uint32_t s){seed=s;rng.seed(s);ants.clear();nextAntId=0;step=0;history.clear();for(auto&c:colonies)c=Colony();initializeCells();initializeNests();initializeNestPheromones();initializeSugar();}
bool Grid::isInside(int x,int y)const{return x>=0&&x<Config::GridSize&&y>=0&&y<Config::GridSize;}
void Grid::initializeNests(){const int cx=Config::GridSize/2-1,cy=Config::GridSize/2-1;const double pi=std::acos(-1.0);for(int c=0;c<Config::ColonyCount;++c){int x=cx+(int)(Config::NestDistanceFromCenter*std::sin(2*pi*c/Config::ColonyCount));int y=cy+(int)(Config::NestDistanceFromCenter*std::cos(2*pi*c/Config::ColonyCount));nestPositions[c]={x,y};colonies[c]=Colony(c,nestPositions[c],colonyColor(rng));createNest(c,nestPositions[c]);initializeAnts(c,nestPositions[c]);}}
void Grid::createNest(int c,Coordinates p){for(int y=p.y;y<p.y+Config::NestSize;++y)for(int x=p.x;x<p.x+Config::NestSize;++x)if(isInside(x,y))cells[y][x].setNest(c);}
bool Grid::createAnt(int c,Coordinates p){if(!isInside(p.x,p.y)||cells[p.y][p.x].containsAnt()||cells[p.y][p.x].containsNest())return false;auto a=std::make_unique<Ant>(nextAntId++,c);a->setPosition(p);cells[p.y][p.x].placeAnt(*a);ants.push_back(std::move(a));return true;}
void Grid::initializeAnts(int c,Coordinates n){for(int y=n.y-1;y<n.y-1+Config::InitialAntArea;++y)for(int x=n.x-1;x<n.x-1+Config::InitialAntArea;++x)createAnt(c,{x,y});}
void Grid::initializeSugar(){placeInitialSugar();}
void Grid::placeInitialSugar(){std::uniform_int_distribution<int>d(0,Config::GridSize-1);int placed=0;while(placed<parameters.sugarSources){int x=d(rng),y=d(rng);if(cells[y][x].isEmpty()&&!cells[y][x].containsSugar()){cells[y][x].setSugar(parameters.sugarPerSource);++placed;}}}
void Grid::initializeNestPheromones(){for(int c=0;c<Config::ColonyCount;++c)for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x){int d=std::max(std::abs(x-nestPositions[c].x),std::abs(y-nestPositions[c].y));cells[y][x].setNestPheromone(c,std::max(0.f,1.f-(float)d/Config::GridSize));}}
std::vector<Cell*> Grid::getNeighbors(Cell& cell){std::vector<Cell*>n;n.reserve(8);for(auto p:getNeighborsCoordinates(cell.getPosition()))if(isInside(p.x,p.y))n.push_back(&cells[p.y][p.x]);std::shuffle(n.begin(),n.end(),rng);return n;}
bool Grid::hasReachedMaximumPopulation(int c)const{return getPopulation(c)>=static_cast<std::size_t>(parameters.maxAntsPerColony);}
std::size_t Grid::getPopulation(int c)const{std::size_t n=0;for(const auto&a:ants)if(a->isAlive()&&a->getColony()==c)++n;return n;}
bool Grid::findFreePositionNearNest(int c,Coordinates&r)const{Coordinates n=nestPositions[c];for(int radius=1;radius<5;++radius)for(int y=n.y-radius;y<=n.y+Config::NestSize-1+radius;++y)for(int x=n.x-radius;x<=n.x+Config::NestSize-1+radius;++x)if(isInside(x,y)&&cells[y][x].isEmpty()){r={x,y};return true;}return false;}
void Grid::reproduceColonies(){for(int c=0;c<Config::ColonyCount;++c){if(hasReachedMaximumPopulation(c)||colonies[c].getFood()<Config::ReproductionCost)continue;Coordinates p;if(findFreePositionNearNest(c,p)&&colonies[c].spendFood(Config::ReproductionCost)){if(createAnt(c,p))colonies[c].recordBirth();else colonies[c].addFood(Config::ReproductionCost);}}}
void Grid::moveAnt(Ant& a, Cell& target){
    if(!a.isAlive() || target.containsAnt() || target.containsNest()) return;

    Coordinates old = a.getPosition();
    if(!isInside(old.x, old.y) || cells[old.y][old.x].getAnt() != &a) return;

    cells[old.y][old.x].removeAnt();
    a.setPreviousPosition(old);
    target.placeAnt(a);
    a.setPosition(target.getPosition());
    a.recordDistance();

    layTrail(a, target);
}
// Only food-carrying ants deposit food pheromone, on the exact cell they enter.
// The strength decays at every step since the food, so the trail is strongest at
// the source: this is the gradient searching ants climb.
void Grid::layTrail(Ant& a, Cell& target){
    if(!a.isCarryingSugar()) return;
    const float strength=a.getTrailStrength()*Config::TrailStrengthDecay;
    a.setTrailStrength(strength);
    target.depositSugarPheromone(a.getColony(),strength);
}
// Two allied ants trade cells. Keeps cell<->ant pointers and positions consistent.
void Grid::swapAnts(Ant& a, Cell& target){
    Ant* other=target.getAnt();
    if(!other||other==&a||!a.isAlive()||!other->isAlive()||other->getColony()!=a.getColony()) return;
    const Coordinates pa=a.getPosition(), pb=target.getPosition();
    if(!isInside(pa.x,pa.y)||cells[pa.y][pa.x].getAnt()!=&a) return;
    cells[pa.y][pa.x].placeAnt(*other);
    target.placeAnt(a);
    a.setPreviousPosition(pa);      other->setPreviousPosition(pb);
    a.setPosition(pb);              other->setPosition(pa);
    a.recordDistance();             other->recordDistance();
    layTrail(a, target);
}
void Grid::updateAnt(Ant&a){if(!a.isAlive())return;Coordinates p=a.getPosition();if(!isInside(p.x,p.y))return;Cell&cur=cells[p.y][p.x];if(cur.getAnt()!=&a)return;a.incrementAge();auto neighbors=getNeighbors(cur);if(neighbors.empty())return;
    // Keep movement simple and local: neighbors are shuffled randomly, then
    // the first applicable priority rule is used. There is deliberately no
    // attraction toward the center, no edge penalty and no artificial
    // anti-backtracking score. This preserves the natural random walk.
    Decision best{Action::Stay,8};
    Cell* target=nullptr;
    float bestPheromone = -1.0f;
    const int colony = a.getColony();
    const float here = cur.getSugarPheromones()[colony];
    bool uphillExists = false;   // a higher pheromone cell exists, occupied or not
    bool sugarNearby = false;

    for(Cell* c : neighbors){
        Decision d = evaluateTarget(a, cur, *c);
        float pheromone = c->getSugarPheromones()[colony];

        if(pheromone > Config::MinFollowPheromone && pheromone > here + Config::PheromoneGradientEpsilon)
            uphillExists = true;
        if(c->containsSugar()) sugarNearby = true;

        // Among valid food-trail cells, take the strongest pheromone (steepest
        // uphill). Ties stay random thanks to the shuffled neighbour order.
        const bool betterFoodTrail =
            d.action == Action::FollowSugarPheromone &&
            best.action == Action::FollowSugarPheromone &&
            pheromone > bestPheromone;

        if(d.priority < best.priority || betterFoodTrail){
            best = d;
            target = c;
            bestPheromone = pheromone;
        }
    }
    if(!target||best.action==Action::Stay)return;

    // Dead end of a trail: a searching ant stands on a marked cell, nothing is
    // uphill and there is no food around (source exhausted or removed). Erase
    // this cell so the false trail is eaten from its tip instead of trapping
    // ants until it evaporates. Occupied uphill cells count as uphill, so a
    // congested but valid trail is never erased.
    if(!a.isCarryingSugar()&&best.action==Action::Explore&&here>0.f&&!uphillExists&&!sugarNearby)
        cur.clearSugarPheromone(colony);
    int c=a.getColony();a.setLastAction(best.action);
    switch(best.action){
      case Action::Attack:{Ant*v=target->getAnt();if(!v||v->getColony()==c)break;std::uniform_real_distribution<float>d(0.f,1.f);float attacker=1.f+0.15f*(float)std::min<std::size_t>(a.getAge(),100)/100.f;float defender=1.f+0.10f*(float)std::min<std::size_t>(v->getAge(),100)/100.f;float chance=std::clamp(parameters.combatChance*attacker/(attacker+defender)*2.f,0.05f,0.95f);if(d(rng)<chance){int vc=v->getColony();v->kill();target->removeAnt();colonies[c].recordKill();colonies[vc].recordDeath();}else{a.setLastAction(Action::Stay);}break;}
      case Action::CollectSugar:{
          a.setCarryingSugar(true);
          target->removeSugar(1);
          // The trail must be continuous from the source: the food cell holds
          // the maximum, the cell the ant is standing on holds one decay step
          // less, and the following cells keep decreasing toward the nest.
          const float first=Config::TrailInitialStrength*Config::TrailStrengthDecay;
          target->depositSugarPheromone(c, Config::TrailInitialStrength);
          cur.depositSugarPheromone(c, first);
          a.setTrailStrength(first);
          colonies[c].recordFoodCollected();
          break;}
      case Action::ReturnSugar:colonies[c].addFood(1);colonies[c].recordFoodDelivered();a.recordFoodTrip();a.setCarryingSugar(false);a.setTrailStrength(0.f);break;
      case Action::Swap:swapAnts(a,*target);break;
      case Action::FollowNestPheromone:case Action::FollowSugarTrail:case Action::FollowSugarPheromone:case Action::Explore:moveAnt(a,*target);break;case Action::Stay:break;}
}
void Grid::updatePheromones(){
    // Food pheromones only evaporate. There is deliberately no diffusion:
    // a pheromone can therefore only exist on cells actually visited by
    // a food-carrying ant.
    for(auto& r: cells)
        for(Cell& c: r)
            c.evaporatePheromones(parameters.pheromoneEvaporation);
}
void Grid::diffusePheromones(){using V=std::array<float,Config::ColonyCount>;std::array<std::array<V,Config::GridSize>,Config::GridSize> s{};for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x){int count=0;for(auto p:getNeighborsCoordinates({x,y}))if(isInside(p.x,p.y)){++count;for(int c=0;c<Config::ColonyCount;++c)s[y][x][c]+=cells[p.y][p.x].getSugarPheromones()[c];}if(count)for(int c=0;c<Config::ColonyCount;++c)s[y][x][c]/=count;}for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x)cells[y][x].diffusePheromones(s[y][x],parameters.pheromoneDiffusion);}
void Grid::cleanupDeadAnts(){for(const auto&a:ants)if(!a->isAlive()){auto p=a->getPosition();if(isInside(p.x,p.y)&&cells[p.y][p.x].getAnt()==a.get())cells[p.y][p.x].removeAnt();}ants.erase(std::remove_if(ants.begin(),ants.end(),[](const auto&a){return!a->isAlive();}),ants.end());}
void Grid::validateAntPositions()const{if(!Config::ValidateAntPositions)return;for(const auto&a:ants)if(a->isAlive()){auto p=a->getPosition();if(!isInside(p.x,p.y)||cells[p.y][p.x].getAnt()!=a.get())std::cerr<<"Warning: inconsistent ant "<<a->getId()<<"\n";}}
void Grid::updateTerritory(){/* Territory is derived at query time from the strongest nest pheromone. */}
std::size_t Grid::getTerritoryCount(int c)const{std::size_t count=0;for(const auto&r:cells)for(const Cell&cell:r){float best=-1;int owner=-1;for(int k=0;k<Config::ColonyCount;++k){float v=cell.getNestPheromones()[k];if(v>best){best=v;owner=k;}}if(owner==c)++count;}return count;}
void Grid::update(){
    updatePheromones();

    // Update only ants that existed at the beginning of this tick (newborn ants
    // wait for the next one), in a random order: a fixed order would always
    // give the same ants first pick of contested cells and of the trail.
    const std::size_t existingAnts = ants.size();
    std::vector<std::size_t> order(existingAnts);
    for(std::size_t i = 0; i < existingAnts; ++i) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);
    for(std::size_t i : order)
        updateAnt(*ants[i]);

    cleanupDeadAnts();
    reproduceColonies();
    validateAntPositions();
    ++step;
    recordHistory();
}
std::size_t Grid::getTotalAliveAnts()const{std::size_t n=0;for(const auto&a:ants)if(a->isAlive())++n;return n;}
bool Grid::addSugarAt(Coordinates p,int amount){if(!isInside(p.x,p.y)||cells[p.y][p.x].containsNest()||cells[p.y][p.x].containsAnt())return false;cells[p.y][p.x].setSugar(cells[p.y][p.x].getSugar()+std::max(1,amount));return true;}
bool Grid::removeSugarAt(Coordinates p,int amount){if(!isInside(p.x,p.y)||!cells[p.y][p.x].containsSugar())return false;cells[p.y][p.x].removeSugar(std::max(1,amount));return true;}
bool Grid::inspectCell(Coordinates p,std::string&out)const{if(!isInside(p.x,p.y))return false;const Cell&c=cells[p.y][p.x];std::ostringstream o;o<<"Cell ("<<p.x<<","<<p.y<<")\nSugar: "<<c.getSugar()<<"\nNest: "<<(c.containsNest()?std::to_string(c.getNestColony()):"none")<<"\n";if(c.containsAnt()){const Ant*a=c.getAnt();o<<"Ant #"<<a->getId()<<"\nColony: "<<a->getColony()<<"\nAction: "<<actionName(a->getLastAction())<<"\nAge: "<<a->getAge()<<"\nTrips: "<<a->getFoodTrips()<<"\nDistance: "<<a->getDistanceTravelled()<<"\nCarrying: "<<(a->isCarryingSugar()?"yes":"no")<<"\n";}float max=0;int owner=-1;for(int cidx=0;cidx<Config::ColonyCount;++cidx)if(c.getNestPheromones()[cidx]>max){max=c.getNestPheromones()[cidx];owner=cidx;}o<<"Territory: colony "<<owner<<"\n";out=o.str();return true;}
void Grid::recordHistory(){for(const Colony&c:colonies)history.push_back({step,c.getId(),getPopulation(c.getId()),c.getFood(),c.getBirths(),c.getDeaths(),c.getKills(),c.getFoodCollected(),c.getFoodDelivered(),getTerritoryCount(c.getId())});}
void Grid::exportStatsCsv(const std::string&f)const{std::ofstream file(f);if(!file){std::cerr<<"Could not open "<<f<<"\n";return;}file<<"step,colony,population,food,births,deaths,kills,food_collected,food_delivered,territory\n";for(const auto&r:history)file<<r.step<<','<<r.colony<<','<<r.population<<','<<r.food<<','<<r.births<<','<<r.deaths<<','<<r.kills<<','<<r.foodCollected<<','<<r.foodDelivered<<','<<r.territory<<'\n';}
void Grid::printConsole()const{for(const auto&r:cells){for(const Cell&c:r)std::cout<<(c.containsNest()?'N':c.containsAnt()?'A':c.containsSugar()?'S':'.');std::cout<<'\n';}}

std::vector<Grid::HistoryView> Grid::getHistory() const { std::vector<HistoryView> out; out.reserve(history.size()); for(const auto& r:history) out.push_back({r.step,r.colony,r.population,r.food,r.territory}); return out; }
