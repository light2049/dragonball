#include "UI/HUD.h"
#include <algorithm>
#include <iostream>

namespace db {

    sf::Texture HUD::s_texLifeBg;
    sf::Texture HUD::s_texLifeFill;
    sf::Texture HUD::s_texPowerBg;
    sf::Texture HUD::s_texPowerFill;
    sf::Texture HUD::s_texTimer;

    void HUD::loadTextures() {
        auto load = [](sf::Texture& t, const std::string& path) {
            if (!t.loadFromFile(path))
                std::cerr << "[HUD] Failed: " << path << std::endl;
        };
        load(s_texLifeBg,   "Data/UI/hud/lifebar_bg.png");
        load(s_texLifeFill, "Data/UI/hud/lifebar_fill.png");
        load(s_texPowerBg,  "Data/UI/hud/powerbar_bg.png");
        load(s_texPowerFill,"Data/UI/hud/powerbar_fill.png");
        load(s_texTimer,    "Data/UI/hud/time_bg.png");

        if (s_texLifeBg.getSize().x > 0)
            std::cout << "[HUD] Lifebar texture: " << s_texLifeBg.getSize().x << "x" << s_texLifeBg.getSize().y << "\n";
    }

    HUD::HUD() {
        if (s_texLifeBg.getSize().x > 0) {
            m_lifebarWidth = static_cast<float>(s_texLifeBg.getSize().x);
            m_lifebarHeight = static_cast<float>(s_texLifeBg.getSize().y);
        }
        if (s_texPowerBg.getSize().x > 0) {
            m_powerbarWidth = static_cast<float>(s_texPowerBg.getSize().x);
            m_powerbarHeight = static_cast<float>(s_texPowerBg.getSize().y);
        }
    }

    void HUD::setPosition(float x, float y) { m_x = x; m_y = y; }

    void HUD::update(int currentLife, int maxLife) {
        m_currentRatio = (maxLife > 0) ? std::min(1.f, static_cast<float>(currentLife) / maxLife) : 0.f;
    }

    void HUD::updatePower(int currentPower, int maxPower) {
        m_powerRatio = (maxPower > 0) ? std::min(1.f, static_cast<float>(currentPower) / maxPower) : 0.f;
    }

    void HUD::draw(sf::RenderWindow& window) {
        float dir = m_flipped ? -1.f : 1.f;
        const float inset = 2.f;
        const float gap = 2.f;

        if (s_texLifeFill.getSize().x > 0 && m_currentRatio > 0.f) {
            sf::Sprite spr(s_texLifeFill);
            int maxW = static_cast<int>(s_texLifeFill.getSize().x) - static_cast<int>(inset * 2);
            int fillW = static_cast<int>(maxW * m_currentRatio);
            spr.setTextureRect(sf::IntRect({0, 0}, {fillW, static_cast<int>(s_texLifeFill.getSize().y)}));
            spr.setScale({dir, 1.f});
            float fx = m_flipped ? m_x - inset : m_x + inset;
            spr.setPosition({fx, m_y + inset});
            window.draw(spr);
        }

        if (s_texLifeBg.getSize().x > 0) {
            sf::Sprite spr(s_texLifeBg);
            spr.setScale({dir, 1.f});
            spr.setPosition({m_x, m_y});
            window.draw(spr);
        }

        if (s_texPowerFill.getSize().x > 0 && m_powerRatio > 0.f) {
            sf::Sprite spr(s_texPowerFill);
            int maxW = static_cast<int>(s_texPowerFill.getSize().x) - static_cast<int>(inset * 2);
            int pw = static_cast<int>(maxW * m_powerRatio);
            spr.setTextureRect(sf::IntRect({0, 0}, {pw, static_cast<int>(s_texPowerFill.getSize().y)}));
            spr.setScale({dir, 1.f});
            float fx = m_flipped ? m_x - inset : m_x + inset;
            spr.setPosition({fx, m_y + m_lifebarHeight + gap + inset});
            window.draw(spr);
        }

        if (s_texPowerBg.getSize().x > 0) {
            sf::Sprite spr(s_texPowerBg);
            spr.setScale({dir, 1.f});
            spr.setPosition({m_x, m_y + m_lifebarHeight + gap});
            window.draw(spr);
        }

        if (m_faceTex && m_faceTex->getSize().x > 0) {
            sf::Sprite face(*m_faceTex);
            float faceSize = 40.f;
            float scale = faceSize / m_faceTex->getSize().y;
            face.setScale({dir * scale, scale});
            face.setPosition({m_flipped ? m_x + m_lifebarWidth + 4.f : m_x - faceSize - 4.f, m_y});
            window.draw(face);
        }

        if (s_texTimer.getSize().x > 0) {
            sf::Sprite timer(s_texTimer);
            float centerX = window.getView().getSize().x / 2.f;
            timer.setPosition({centerX - s_texTimer.getSize().x / 2.f * std::abs(timer.getScale().x), m_y});
            window.draw(timer);
        }
    }

}