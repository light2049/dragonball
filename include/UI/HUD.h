#pragma once
#include <SFML/Graphics.hpp>

namespace db {
    class HUD {
    public:
        HUD();
        void setPosition(float x, float y);
        void setFlipped(bool flipped) { m_flipped = flipped; }
        void update(int currentLife, int maxLife);
        void updatePower(int currentPower, int maxPower);
        void setFaceTexture(const sf::Texture* tex) { m_faceTex = tex; }
        void draw(sf::RenderWindow& window);

        static void loadTextures();

    private:
        const sf::Texture* m_faceTex = nullptr;
        float m_lifebarWidth = 400.f, m_lifebarHeight = 40.f;
        float m_powerbarWidth = 300.f, m_powerbarHeight = 16.f;
        float m_currentRatio = 1.f, m_powerRatio = 0.f;
        bool m_flipped = false;
        float m_x = 0.f, m_y = 0.f;

        static sf::Texture s_texLifeBg;
        static sf::Texture s_texLifeFill;
        static sf::Texture s_texPowerBg;
        static sf::Texture s_texPowerFill;
        static sf::Texture s_texTimer;
    };
} // namespace db