#include "AntBehavior.hpp"
Decision evaluateTarget(const Ant& ant,const Cell& current,const Cell& target){
    if(target.containsAnt()){
        const Ant& other=*target.getAnt();
        if(other.getColony()!=ant.getColony())return{Action::Attack,1};
        // Ally in the way. Without a way to pass each other, a dense colony
        // gridlocks: carriers wait for cells held by ants that cannot move.
        // A carrier never gets displaced.
        if(other.isCarryingSugar())return{Action::Stay,8};
        if(ant.isCarryingSugar()){
            // Carrier heading home: trade places with a non-carrying ally if that
            // cell is closer to the nest. Free closer cells (priority 4) are preferred.
            const int c=ant.getColony();
            if(target.getNestPheromones()[c]-current.getNestPheromones()[c]>0.001f)return{Action::Swap,5};
            return{Action::Stay,8};
        }
        // Searching ant with no free cell around: trade places with an ally, which
        // shuffles a jammed crowd. Only used when nothing better exists (priority 7).
        return{Action::Swap,7};
    }
    if(!ant.isCarryingSugar()&&target.containsSugar())return{Action::CollectSugar,2};
    if(ant.isCarryingSugar()&&target.containsNest()&&target.getNestColony()==ant.getColony())return{Action::ReturnSugar,3};
    if(target.containsNest())return{Action::Stay,8};
    const int c=ant.getColony();
    if(ant.isCarryingSugar()){
        // Return trip: climb the static nest gradient toward the own nest.
        const float nest=target.getNestPheromones()[c]-current.getNestPheromones()[c];
        if(nest>0.001f)return{Action::FollowNestPheromone,4};
        // No free cell gets closer (neighbours occupied or blocked by a foreign
        // nest): sidestep on a cell at the same distance instead of freezing.
        // Any strictly better cell always has priority 4 and wins over this.
        if(nest>=-0.001f)return{Action::Explore,6};
        return{Action::Stay,8};
    }
    // Searching ant: follow the food trail only UPHILL. Carrying ants lay a trail
    // that is strongest at the food, so a higher value means "closer to food".
    // Comparing with the current cell prevents walking back down the trail
    // toward the nest. Weak traces are ignored: the ant explores randomly.
    const float here=current.getSugarPheromones()[c];
    const float there=target.getSugarPheromones()[c];
    if(there>Config::MinFollowPheromone&&there>here+Config::PheromoneGradientEpsilon)return{Action::FollowSugarPheromone,5};
    return{Action::Explore,6};
}
const char* actionName(Action a){switch(a){case Action::Attack:return"Attack";case Action::CollectSugar:return"CollectSugar";case Action::ReturnSugar:return"ReturnSugar";case Action::FollowNestPheromone:return"FollowNestPheromone";case Action::FollowSugarTrail:return"FollowSugarTrail";case Action::FollowSugarPheromone:return"FollowSugarPheromone";case Action::Explore:return"Explore";case Action::Swap:return"Swap";case Action::Stay:return"Stay";}return"Unknown";}
