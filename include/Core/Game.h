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
        int lifetime = 180;
        bool done = false;
        int damage = 20;
        int sparkno = 1200;
        bool hasHit = false;
        int sprpriority = 0;   // 绘制层级
        // CNS 执行
        int stateNo = 0;
        float stateTime = 0.f;
        class StateRegistry* stateRegistry = nullptr;
        class Fighter* parent = nullptr;
        int parentStateno = 0;  // 创建时的父状态, 父状态改变后销毁
        // 执行过 HitDef 避免一帧多次判定
        bool hitDefFired = false;
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
        void resetRound();

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

        // SuperPause 状态 (由 CNS SuperPause 控制器触发)
        int m_superPauseTimer = 0;
        bool m_superPauseDarken = false;

        // 回合系统
        enum class GameState { INTRO, FIGHT, KO };
        GameState m_gameState = GameState::FIGHT;
        int m_roundNumber = 1;
        int m_p1RoundsWon = 0;
        int m_p2RoundsWon = 0;
        float m_koTimer = 0.f;
        float m_roundTimer = 0.f;

        std::vector<Spark> m_sparks;
        std::vector<HelperEntity> m_helpers;
    };

} // namespace db
