#pragma once
#include "Characters/Fighter.h"
#include "Core/InputManager.h"
#include "UI/HUD.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

namespace db {

    struct Spark {
        AnimationPlayer animPlayer;
        sf::Vector2f position;
        float timer = 0.f;
        bool done = false;
    };

    // ✅ 飞行道具 / 特效实体 (简化版 Helper)
    struct HelperEntity {
        int id = 0;
        AnimationPlayer animPlayer;
        sf::Vector2f position;
        sf::Vector2f velocity;  // px/tick
        bool facingRight = true;
        int lifetime = 180;      // frames remaining
        bool done = false;
    };

    class Game {
    public:
        Game();
        ~Game();
        void run();

    private:
        void processEvents();
        void update(float dt);
        void render();
        void checkCombat();
        void handlePushCollision();
        void spawnSpark(int animId, const sf::Vector2f& pos);

        sf::RenderWindow window_;
        sf::Clock clock_;
        InputManager inputManager_;
        InputManager inputManagerP2_;
        sf::Font m_debugFont;
        std::unique_ptr<sf::Text> m_debugText;
        bool m_debugReady = false;
        int m_debugJpxLatch = 0;

        std::unique_ptr<Fighter> m_player;
        std::unique_ptr<Fighter> m_dummy;

        std::unique_ptr<HUD> m_hudP1;
        std::unique_ptr<HUD> m_hudP2;

        float m_hitStopTimer = 0.0f;
        int m_hitStopDuration = 0;

        std::vector<Spark> m_sparks;
        std::vector<HelperEntity> m_helpers;
    };

} // namespace db
