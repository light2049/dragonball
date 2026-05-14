#pragma once
#include "Characters/Fighter.h"
#include "Core/InputManager.h"
#include "UI/HUD.h"
#include "UI/BitmapFont.h"
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

    struct HelperEntity {
        int id = 0;
        AnimationPlayer animPlayer;
        sf::Vector2f position;
        sf::Vector2f velocity;
        bool facingRight = true;
        int lifetime = 180;
        bool done = false;
        int damage = 20;
        int sparkno = 1200;
        bool hasHit = false;
        int hitCooldown = 0;
        int sprpriority = 0;
        int stateNo = 0;
        float stateTime = 0.f;
        class StateRegistry* stateRegistry = nullptr;
        class Fighter* parent = nullptr;
        int parentStateno = 0;
        bool hitDefFired = false;
        bool firstUpdate = true;
        DrawOverrides drawOverrides;
    };

    enum class GameMode { VS_PLAYER, VS_AI };
    enum class MenuChoice { SINGLE, VS, EXIT };

    class Game {
    public:
        Game();
        ~Game();
        void run();

    private:
        void processEvents();
        void update(float dt);
        void render();
        void updateTitle(float dt);
        void updateSelect(float dt);
        void updateStageSelect(float dt);
        void renderTitle();
        void renderSelect();
        void renderStageSelect();
        void checkCombat();
        void checkCombatBetween(Fighter& attacker, Fighter& defender);
        void handlePushCollision();
        void clampFighterToStage(Fighter& fighter);
        void spawnSpark(int animId, const sf::Vector2f& pos);
        void resetRound();
        void updateViews(const sf::Vector2u& winSize);
        void loadUITextures();
        void loadCharacterPortraits();
        void loadStagePreviews();

        sf::RenderWindow window_;
        sf::Clock clock_;
        InputManager inputManager_;
        InputManager inputManagerP2_{KeyMapping::p2()};
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
        int m_superPauseTimer = 0;
        bool m_superPauseDarken = false;

        BitmapFont m_bitmapFont;
        bool m_useBitmapFont = false;

        struct CharacterDef {
            std::string dirName;
            std::string displayName;
            sf::Texture portraitSmall;
            sf::Texture portraitLarge;
        };
        struct StageDef {
            std::string name;
            std::string dirPath;
            sf::Texture preview;
            sf::Texture background;
        };

        std::vector<CharacterDef> discoverCharacters();
        std::vector<StageDef> discoverStages();
        void initFight(int p1Choice, int p2Choice, int stageChoice);

        enum class GameState { TITLE, SELECT, STAGE_SELECT, INTRO, FIGHT, KO };
        GameState m_gameState = GameState::TITLE;
        int m_roundNumber = 1;
        int m_p1RoundsWon = 0;
        int m_p2RoundsWon = 0;
        float m_koTimer = 0.f;
        float m_roundTimer = 0.f;

        std::vector<CharacterDef> m_availableChars;
        int m_p1Choice = 0;
        int m_p2Choice = 0;
        int m_selectPhase = 0;
        float m_selectAnimPos = 0.f;
        float m_selectOffsetY = 150.f;
        float m_selectSpacing = 710.f;

        std::vector<StageDef> m_availableStages;
        int m_stageChoice = 0;
        float m_stageAnimPos = 0.f;

        GameMode m_gameMode = GameMode::VS_PLAYER;
        int m_menuPhase = 0;
        MenuChoice m_menuChoice = MenuChoice::SINGLE;
        float m_menuOffsetY = 695.f;

        sf::Texture m_texTitleBg;
        sf::Texture m_texSelectBg;
        sf::Texture m_texSelectArrowL;
        sf::Texture m_texSelectArrowR;
        sf::Texture m_texCursorGlow;
        sf::Texture m_texP1Tag;
        sf::Texture m_texP2Tag;
        sf::Texture m_texStageBg;
        sf::Texture m_texStageArrowL;
        sf::Texture m_texStageArrowR;
        sf::Texture m_texStageCursor;
        sf::Texture m_texMenuArrow;

        sf::Texture m_stageBg;

        sf::View m_uiView{sf::FloatRect({0, 0}, {1920, 1080})};
        sf::View m_gameView{sf::FloatRect({0, 0}, {800, 600})};
        sf::Vector2u m_windowSize{960, 540};
        bool m_fullscreen = false;

        std::vector<Spark> m_sparks;
        std::vector<HelperEntity> m_helpers;
    };

}