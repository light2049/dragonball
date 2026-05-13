#include "Characters/Dummy.h"
#include <iostream>

namespace db {
    Dummy::Dummy(float x, float y) : m_position(x, y), m_currentLife(10000) {
    }

    void Dummy::setTexture(const sf::Texture& tex) {
        m_sprite = std::make_unique<sf::Sprite>(tex);
        m_sprite->setPosition(m_position);
    }

    void Dummy::update(float dt) {
        m_flashTimer -= dt;
        if (m_flashTimer <= 0.0f) {
            if (m_sprite) m_sprite->setColor(sf::Color::White);
        }

        if (m_sprite) {

            m_hurtBox.position = {m_position.x - 25.0f, m_position.y - 100.0f};
            m_hurtBox.size = {50.0f, 100.0f};
        }
    }

    void Dummy::takeDamage(int damage) {
        if (isDead()) return;
        m_currentLife -= damage;
        if (m_currentLife < 0) m_currentLife = 0;

        if (m_sprite) m_sprite->setColor(sf::Color(255, 50, 50));
        m_flashTimer = 0.2f;

        std::cout << "[Dummy] HP: " << m_currentLife << "\n";
    }

    void Dummy::draw(sf::RenderWindow& window) const {
        if (!m_sprite) return;

        sf::Sprite drawSprite(*m_sprite);
        float height = drawSprite.getLocalBounds().size.y;

        drawSprite.setPosition({m_position.x, m_position.y - height});

        window.draw(drawSprite);
    }

    sf::FloatRect Dummy::getHurtBox() const { return m_hurtBox; }
    sf::Vector2f Dummy::getPosition() const { return m_position; }
}