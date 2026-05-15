#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

namespace db {
    class Dummy {
    public:
        Dummy(float x, float y);
        void setTexture(const sf::Texture& tex);
        void update(float dt);
        void takeDamage(int damage);
        void draw(sf::RenderWindow& window) const;

        sf::FloatRect getHurtBox() const;
        sf::Vector2f getPosition() const;
        int getCurrentLife() const { return m_currentLife; }
        int getMaxLife() const { return 10000; }
        bool isDead() const { return m_currentLife <= 0; }

    private:
        sf::Vector2f m_position;
        std::unique_ptr<sf::Sprite> m_sprite;
        int m_currentLife;
        sf::FloatRect m_hurtBox;
        float m_flashTimer = 0.0f;
    };
}