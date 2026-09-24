#pragma once
#include <SFML/Graphics.hpp>
#include "Grid.hpp"
class Renderer {
public:
    Renderer();
    void draw(sf::RenderWindow& window,const Grid& grid,bool showPheromones,bool showAnts,bool showSugar,bool showNests,bool showStats,bool showTerritory,bool showGraph,bool debugCell,bool showHelp,const std::string& debugText);
    Coordinates pixelToCell(sf::Vector2i pixel) const;
    bool isInsideWorld(sf::Vector2i pixel) const;
private:
    float cellSize=0.f; sf::Font font; sf::Text text; sf::VertexArray vertices{sf::Quads};
    sf::VertexArray territoryVertices{sf::Quads};
    sf::Color getCellColor(const Cell&,const Grid&,bool,bool,bool,bool)const;
    void updateVertices(const Grid&,bool,bool,bool,bool); void updateTerritory(const Grid&); void drawPanel(sf::RenderWindow&,const Grid&,bool,bool,bool); void updateText(const Grid&);
};
