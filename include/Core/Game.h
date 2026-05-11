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
        int hitCooldown = 0;
        int sprpriority = 0;   // 绘制层级
        // CNS 执行
        int stateNo = 0;
        float stateTime = 0.f;
        class StateRegistry* stateRegistry = nullptr;
        class Fighter* parent = nullptr;
        int parentStateno = 0;  // 创建时的父状态, 父状态改变后销毁
        // 执行过 HitDef 避免一帧多次判定
        bool hitDefFired = false;
        bool firstUpdate = true;  // 创建后的首次更新 (跳过 DestroySelf)
        // 每帧绘制覆盖 (由 AngleDraw / Trans 控制器设置)
        DrawOverrides drawOverrides;
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
        void clampFighterToStage(Fighter& fighter);
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

        // 角色定义
        struct CharacterDef {
            std::string dirName;
            std::string displayName;
        };
        std::vector<CharacterDef> discoverCharacters();
        void initFight(int p1Choice, int p2Choice);

        // 回合系统
        enum class GameState { SELECT, INTRO, FIGHT, KO };
        GameState m_gameState = GameState::SELECT;
        int m_roundNumber = 1;
        int m_p1RoundsWon = 0;
        int m_p2RoundsWon = 0;
        float m_koTimer = 0.f;
        float m_roundTimer = 0.f;

        // 选人界面
        std::vector<CharacterDef> m_availableChars;
        int m_p1Choice = 0;
        int m_p2Choice = 0;
        int m_selectPhase = 0;  // 0=P1选, 1=P2选, 2=准备开始
        sf::View m_uiView{sf::FloatRect({0, 0}, {1920, 1080})};

        std::vector<Spark> m_sparks;
        std::vector<HelperEntity> m_helpers;
    };

} // namespace db
