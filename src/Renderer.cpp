#include <algorithm>
#include <sstream>
#include "Renderer.hpp"
Renderer::Renderer():cellSize((float)Config::WorldPixelSize/Config::GridSize),vertices(sf::Quads,Config::GridSize*Config::GridSize*4),territoryVertices(sf::Quads,Config::GridSize*Config::GridSize*4){font.loadFromFile("assets/arial.ttf")||font.loadFromFile("arial.ttf");text.setFont(font);text.setCharacterSize(14);text.setFillColor(sf::Color::White);text.setOutlineColor(sf::Color::Black);text.setOutlineThickness(1.f);}
sf::Color Renderer::getCellColor(const Cell& cell, const Grid& grid, bool pher, bool ants, bool sugar, bool nests) const {
    // Pheromone rendering is deliberately limited to FOOD pheromones.
    // Nest pheromones are never rendered.
    if (ants && cell.containsAnt())
        return grid.getColonies()[cell.getAnt()->getColony()].getColor();

    if (sugar && cell.containsSugar())
        return sf::Color(245, 210, 80);

    if (nests && cell.containsNest())
        return sf::Color(139, 69, 19);

    if (!pher)
        return sf::Color(12, 12, 18);

    int foodOwner = -1;
    float strongestFood = 0.f;

    for (int c = 0; c < Config::ColonyCount; ++c) {
        if (cell.getSugarPheromones()[c] > strongestFood) {
            strongestFood = cell.getSugarPheromones()[c];
            foodOwner = c;
        }
    }

    // No pheromone = normal background.
    // A stronger pheromone concentration produces a brighter version
    // of the colony colour, giving a simple visual gradient.
    // Only draw what ants can actually follow (same threshold as the behaviour).
    if (foodOwner < 0 || strongestFood <= Config::MinFollowPheromone)
        return sf::Color(12, 12, 18);

    sf::Color food = grid.getColonies()[foodOwner].getColor();
    // Keep faint trails readable: the raw value can be very small far from the
    // food, which would otherwise render as almost black.
    float intensity = 0.3f + 0.7f * std::clamp(strongestFood, 0.f, 1.f);

    return sf::Color(
        static_cast<sf::Uint8>(food.r * intensity),
        static_cast<sf::Uint8>(food.g * intensity),
        static_cast<sf::Uint8>(food.b * intensity)
    );
}
void Renderer::updateVertices(const Grid&g,bool p,bool a,bool s,bool n){for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x){std::size_t i=(std::size_t)(y*Config::GridSize+x)*4;float l=x*cellSize,t=y*cellSize,r=l+cellSize,b=t+cellSize;auto c=getCellColor(g.getCells()[y][x],g,p,a,s,n);for(int k=0;k<4;++k){vertices[i+k].color=c;}vertices[i].position={l,t};vertices[i+1].position={r,t};vertices[i+2].position={r,b};vertices[i+3].position={l,b};}}
void Renderer::updateTerritory(const Grid&g){for(int y=0;y<Config::GridSize;++y)for(int x=0;x<Config::GridSize;++x){int owner=0;float best=-1;for(int c=0;c<Config::ColonyCount;++c)if(g.getCells()[y][x].getNestPheromones()[c]>best){best=g.getCells()[y][x].getNestPheromones()[c];owner=c;}std::size_t i=(std::size_t)(y*Config::GridSize+x)*4;sf::Color c=g.getColonies()[owner].getColor();c.a=38;float l=x*cellSize,t=y*cellSize,r=l+cellSize,b=t+cellSize;vertices[i].color=sf::Color::Transparent;for(int k=0;k<4;++k){territoryVertices[i+k].color=c;}territoryVertices[i].position={l,t};territoryVertices[i+1].position={r,t};territoryVertices[i+2].position={r,b};territoryVertices[i+3].position={l,b};}}
void Renderer::updateText(const Grid&g){std::ostringstream o;o<<"Step "<<g.getStep()<<" | Ants "<<g.getTotalAliveAnts()<<" | Seed "<<g.getSeed()<<"\n";for(const Colony&c:g.getColonies())o<<"C"<<c.getId()<<"  ants="<<g.getPopulation(c.getId())<<" food="<<c.getFood()<<" territory="<<g.getTerritoryCount(c.getId())<<" births="<<c.getBirths()<<" deaths="<<c.getDeaths()<<" kills="<<c.getKills()<<"\n";text.setString(o.str());}
void Renderer::drawPanel(sf::RenderWindow&w,const Grid&g,bool stats,bool graph,bool help){sf::RectangleShape panel({(float)(Config::WindowWidth-Config::WorldPixelSize-20),(float)Config::WindowHeight-20});panel.setPosition(Config::WorldPixelSize+10,10);panel.setFillColor(sf::Color(18,18,25,240));w.draw(panel);if(stats){updateText(g);text.setPosition(Config::WorldPixelSize+20,20);w.draw(text);}if(help){
 sf::Text title= text; title.setString("CONTROLS"); title.setCharacterSize(18); title.setPosition(Config::WorldPixelSize+20,390); w.draw(title);
 sf::Text controls=text; controls.setCharacterSize(13); controls.setPosition(Config::WorldPixelSize+20,420);
 controls.setString("SPACE   Pause / Resume\nR       Restart\n+ / -   Simulation speed\nP       Pheromones\nA       Ants\nS       Sugar\nN       Nests\nG       Population graph\nI       Statistics\nD       Cell debug\nE       Export CSV\nH       Show / hide help\nESC     Quit\n\nMOUSE\nLeft    Add sugar / paint\nRight   Remove sugar / erase"); w.draw(controls);
}if(graph){const auto h=g.getHistory();if(!h.empty()){float gx=Config::WorldPixelSize+25,gy=500,gw=350,gh=210;sf::RectangleShape bg({gw,gh});bg.setPosition(gx,gy);bg.setFillColor(sf::Color(8,8,12,230));w.draw(bg);std::size_t maxStep=h.back().step;std::size_t maxPop=1;for(const auto&r:h)maxPop=std::max(maxPop,r.population);for(int c=0;c<Config::ColonyCount;++c){sf::VertexArray line(sf::LineStrip);for(const auto&r:h)if(r.colony==c){float x=gx+(maxStep?gw*(float)r.step/(float)maxStep:0);float y=gy+gh-gh*(float)r.population/(float)maxPop;sf::Vertex v({x,y},g.getColonies()[c].getColor());line.append(v);}w.draw(line);}}}}
void Renderer::draw(sf::RenderWindow&w,const Grid&g,bool p,bool a,bool s,bool n,bool stats,bool territory,bool graph,bool debug,bool help,const std::string&debugText){updateVertices(g,p,a,s,n);w.draw(vertices);drawPanel(w,g,stats,graph,help);if(debug&&!debugText.empty()){sf::RectangleShape box({330,150});box.setPosition(10,Config::WorldPixelSize-160);box.setFillColor(sf::Color(10,10,15,230));w.draw(box);text.setString(debugText);text.setPosition(18,Config::WorldPixelSize-152);w.draw(text);}}
Coordinates Renderer::pixelToCell(sf::Vector2i p)const{return{p.x/(int)cellSize,p.y/(int)cellSize};}bool Renderer::isInsideWorld(sf::Vector2i p)const{return p.x>=0&&p.y>=0&&p.x<Config::WorldPixelSize&&p.y<Config::WorldPixelSize;}
