#pragma once
#include <SFML/Graphics.hpp>

namespace db {
    class HUD {
    public:
        HUD();
        void setPosition(float x, float y);
        void update(int currentLife, int maxLife);
        void updatePower(int currentPower, int maxPower);
        void draw(sf::RenderWindow& window);

    private:
        sf::RectangleShape m_bgBar;
        sf::RectangleShape m_fgBar;
        sf::RectangleShape m_powerBgBar;
        sf::RectangleShape m_powerFgBar;
    };
}
