#include "UI/HUD.h"

namespace db {
    HUD::HUD() {
        m_bgBar.setSize({300.0f, 20.0f});
        m_bgBar.setFillColor(sf::Color(50, 50, 50));

        m_fgBar.setSize({300.0f, 20.0f});
        m_fgBar.setFillColor(sf::Color(230, 50, 50));

        m_powerBgBar.setSize({300.0f, 10.0f});
        m_powerBgBar.setFillColor(sf::Color(80, 80, 80));

        m_powerFgBar.setSize({0.0f, 10.0f});
        m_powerFgBar.setFillColor(sf::Color(0, 200, 255));
    }

    void HUD::setPosition(float x, float y) {
        m_bgBar.setPosition({x, y});
        m_fgBar.setPosition({x, y});
        m_powerBgBar.setPosition({x, y + 24.0f});
        m_powerFgBar.setPosition({x, y + 24.0f});
    }

    void HUD::update(int currentLife, int maxLife) {
        float ratio = static_cast<float>(currentLife) / maxLife;
        if (ratio < 0.0f) ratio = 0.0f;
        m_fgBar.setSize({300.0f * ratio, 20.0f});
    }

    void HUD::updatePower(int currentPower, int maxPower) {
        float ratio = static_cast<float>(currentPower) / maxPower;
        if (ratio < 0.0f) ratio = 0.0f;
        m_powerFgBar.setSize({300.0f * ratio, 10.0f});
    }

    void HUD::draw(sf::RenderWindow& window) {
        window.draw(m_bgBar);
        window.draw(m_fgBar);
        window.draw(m_powerBgBar);
        window.draw(m_powerFgBar);
    }
}
