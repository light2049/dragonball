#include "Core/Game.h"
#include "Core/ResourceManager.h"
#include "Utils/CnsParser.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace db {

    // Simple file logging
    static void logMsg(const char* msg) {
        FILE* f = fopen("C:\\Users\\Lenovo\\game_log.txt", "a");
        if (f) {
            fprintf(f, "%s", msg);
            fclose(f);
        }
        FILE* f2 = fopen("C:\\game_log.txt", "a");
        if (f2) {
            fprintf(f2, "%s", msg);
            fclose(f2);
        }
    }

    Game::Game() : window_(sf::VideoMode({960, 540}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close) {
        window_.setVerticalSyncEnabled(true);
        window_.setFramerateLimit(60);
        try {
            // 加载字体
            m_bitmapFont.load("Data/UI/fonts/flame_blocky_font/All_Characters");
            m_useBitmapFont = m_bitmapFont.hasLoaded();

            // 加载调试字体
            if (m_debugFont.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
                m_debugText = std::make_unique<sf::Text>(m_debugFont);
                m_debugText->setCharacterSize(14);
                m_debugText->setFillColor(sf::Color(255, 255, 200));
                m_debugText->setPosition({10.f, 540.f});
                m_debugReady = true;
            }

            // 发现角色和场景
            m_availableChars = discoverCharacters();
            m_availableStages = discoverStages();

            // 加载 UI 贴图
            loadUITextures();
            HUD::loadTextures();
            loadCharacterPortraits();
            loadStagePreviews();

            m_gameState = GameState::TITLE;
            updateViews({960, 540});
            logMsg("Game ready (TITLE screen)\n");

        } catch (const std::exception& e) {
            std::string err = "Error: ";
            err += e.what();
            err += "\n";
            logMsg(err.c_str());
        }
    }

    Game::~Game() { window_.close(); }

    void Game::spawnSpark(int animId, const sf::Vector2f& pos) {
        if (animId <= 0) animId = 1200;
        const Animation* anim = nullptr;
        if (m_player) anim = m_player->getAnimation(animId);
        if (!anim && m_dummy) anim = m_dummy->getAnimation(animId);
        if (!anim) return;

        Spark spark;
        spark.animPlayer.play(*anim);
        spark.position = pos;
        m_sparks.push_back(std::move(spark));
    }

    std::vector<Game::CharacterDef> Game::discoverCharacters() {
        std::vector<CharacterDef> chars;
        std::string charsDir = "Data/Characters/";
        try {
            for (const auto& entry : std::filesystem::directory_iterator(charsDir)) {
                if (!entry.is_directory()) continue;
                std::string dirName = entry.path().filename().string();
                std::string defPath = charsDir + dirName + "/" + dirName + ".def";

                std::string displayName = dirName;
                std::ifstream defFile(defPath);
                if (defFile.is_open()) {
                    std::string line;
                    while (std::getline(defFile, line)) {
                        // .def 中使用 displayname = "Name"
                        auto pos = line.find("displayname");
                        if (pos == std::string::npos) pos = line.find("displayname");
                        if (pos != std::string::npos) {
                            auto eq = line.find('=');
                            if (eq != std::string::npos) {
                                displayName = line.substr(eq + 1);
                                // 去掉首尾空格和引号
                                displayName.erase(0, displayName.find_first_not_of(" \t\""));
                                displayName.erase(displayName.find_last_not_of(" \t\"") + 1);
                            }
                            break;
                        }
                    }
                }
                chars.push_back({dirName, displayName});
                std::cout << "[Select] Found character: " << displayName << " (" << dirName << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Select] Error scanning characters: " << e.what() << std::endl;
        }
        if (chars.empty()) {
            std::cerr << "[Select] No characters found in " << charsDir << std::endl;
        }
        return chars;
    }

    std::vector<Game::StageDef> Game::discoverStages() {
        std::vector<StageDef> stages;
        std::string stagesDir = "Data/Stages/";
        try {
            for (const auto& entry : std::filesystem::directory_iterator(stagesDir)) {
                if (!entry.is_directory()) continue;
                std::string name = entry.path().filename().string();
                std::string bgPath = stagesDir + name + "/bg.png";
                StageDef sd;
                sd.name = name;
                sd.dirPath = stagesDir + name;
                // 加载战斗背景图 (大图, 800x600)
                if (!sd.background.loadFromFile(bgPath))
                    std::cerr << "[Stage] Failed to load background: " << bgPath << std::endl;
                stages.push_back(std::move(sd));
                std::cout << "[Stage] Found: " << name << std::endl;
            }
        } catch (...) {}
        if (stages.empty()) {
            // 默认场景
            StageDef sd;
            sd.name = "Arena";
            sd.dirPath = "";
            stages.push_back(std::move(sd));
        }
        return stages;
    }

    void Game::loadUITextures() {
        auto loadTex = [](sf::Texture& t, const std::string& path) {
            if (!t.loadFromFile(path)) {
                std::cerr << "[UI] Failed to load: " << path << std::endl;
            }
        };
        loadTex(m_texTitleBg,       "Data/UI/title/bg.png");
        loadTex(m_texSelectBg,      "Data/UI/select/bg.png");
        loadTex(m_texSelectArrowL,  "Data/UI/select/arrow_left_gold_40x60.png");
        loadTex(m_texSelectArrowR,  "Data/UI/select/arrow_right_gold_40x60.png");
        loadTex(m_texCursorGlow,    "Data/UI/select/cursor_glow.png");
        loadTex(m_texP1Tag,         "Data/UI/select/p1_tag.png");
        loadTex(m_texP2Tag,         "Data/UI/select/p2_tag.png");
        loadTex(m_texStageBg,       "Data/UI/stage_select/stage_select_bg.png");
        loadTex(m_texStageArrowL,   "Data/UI/stage_select/arrow_left_simple_40x60.png");
        loadTex(m_texStageArrowR,   "Data/UI/stage_select/arrow_right_simple_40x60.png");
        loadTex(m_texStageCursor,   "Data/UI/stage_select/cursor.png");
        std::cout << "[UI] Loaded textures.\n";
    }

    void Game::loadCharacterPortraits() {
        // 直接从 AIR 文件读取第一帧的精灵编号，加载对应的 PNG
        for (auto& cd : m_availableChars) {
            std::string airPath = "Data/Characters/" + cd.dirName + "/" + cd.dirName + ".air";
            std::ifstream airFile(airPath);
            if (!airFile.is_open()) continue;

            // 找到 [Begin Action 0]
            std::string line;
            bool found = false;
            while (std::getline(airFile, line)) {
                auto p = line.find("[Begin Action 0]");
                if (p != std::string::npos) { found = true; break; }
            }
            if (!found) continue;

            // 读取下一行非空非注释行，提取 group,image
            while (std::getline(airFile, line)) {
                // 去掉 Clsn 行
                if (line.find("Clsn") != std::string::npos || line.find("clsn") != std::string::npos) continue;
                // 去掉空行和注释
                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                if (trimmed.empty() || trimmed[0] == ';') continue;

                // 解析 "g,i, x,y, d" 格式
                std::stringstream ss(trimmed);
                std::string token;
                if (!std::getline(ss, token, ',')) continue;
                int group = std::stoi(token);
                if (!std::getline(ss, token, ',')) continue;
                int image = std::stoi(token);
                std::string pngPath = "Data/Characters/" + cd.dirName + "/Sprites/"
                    + cd.dirName + "_" + std::to_string(group) + "-" + std::to_string(image) + ".png";

                sf::Image img;
                if (img.loadFromFile(pngPath)) {
                    if (!cd.portraitLarge.loadFromImage(img) || !cd.portraitSmall.loadFromImage(img)) {
                        std::cerr << "[Portrait] Failed to create texture from image for " << cd.dirName << std::endl;
                    }
                }
                break; // only first frame
            }
        }
    }

    void Game::loadStagePreviews() {
        for (auto& sd : m_availableStages) {
            if (sd.background.getSize().x > 0) {
                sd.preview = sd.background;
            }
        }
    }

    void Game::initFight(int p1Choice, int p2Choice, int stageChoice) {
        auto loadChar = [&](const std::string& name, std::unique_ptr<Fighter>& fighter, float xPos) {
            fighter = std::make_unique<Fighter>();
            std::string base = "Data/Characters/" + name + "/";
            std::string prefix = name;
            fighter->loadAnimations(base + prefix + ".air", base + "Sprites/", prefix);
            fighter->loadStats(base + prefix + ".cns");
            fighter->loadCommonStates(base + "common1.cns");
            fighter->loadCommands(base + prefix + ".cmd");
            fighter->requestStateChange(0);
            float offsetY = static_cast<float>(fighter->getAnimationPlayer().getCurrentFrame().offset.y);
            fighter->setPosition(xPos, 480.0f - offsetY);
            fighter->setGroundLevel(480.0f - offsetY);
        };

        // 加载选中的场景背景
        if (stageChoice >= 0 && stageChoice < static_cast<int>(m_availableStages.size())) {
            m_stageBg = m_availableStages[stageChoice].background;
        }

        float centerX = static_cast<float>(m_windowSize.x) / 2.f;
        loadChar(m_availableChars[p1Choice].dirName, m_player, centerX - 200.f);
        loadChar(m_availableChars[p2Choice].dirName, m_dummy, centerX + 200.f);

        m_hudP1 = std::make_unique<HUD>();
        m_hudP1->setPosition(10.0f, 10.0f);
        m_hudP1->setFaceTexture(&m_availableChars[p1Choice].portraitSmall);
        m_hudP1->update(m_player->getCurrentLife(), m_player->getMaxLife());
        m_hudP1->updatePower(m_player->getPower(), m_player->getMaxPower());

        m_hudP2 = std::make_unique<HUD>();
        m_hudP2->setPosition(static_cast<float>(m_windowSize.x) - 10.0f, 10.0f);
        m_hudP2->setFlipped(true);
        m_hudP2->setFaceTexture(&m_availableChars[p2Choice].portraitSmall);
        m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
        m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());

        // 进入战斗时清除菜单残留的方向输入，防止角色开局自动移动
        inputManager_.reset();
        inputManagerP2_.reset();

        m_player->setRoundState(1);
        m_dummy->setRoundState(1);
        m_player->setRoundNo(1);
        m_dummy->setRoundNo(1);
        m_player->requestStateChange(5900);
        m_dummy->requestStateChange(5900);

        m_gameState = GameState::INTRO;
        m_selectPhase = 0;
        m_roundTimer = 0.f;
        std::cout << "[Select] Fight! " << m_availableChars[p1Choice].displayName
                  << " vs " << m_availableChars[p2Choice].displayName << std::endl;
    }

    void Game::run() {
        logMsg("Game run start\n");
        int frameCount = 0;
        while (window_.isOpen()) {
            float dt = clock_.restart().asSeconds();
            inputManager_.clearJustPressedLatch();
            inputManagerP2_.clearJustPressedLatch();
            processEvents();
            inputManager_.update();
            update(dt);
            render();

            frameCount++;
            if (frameCount % 60 == 0) {
                std::string msg = "Frame " + std::to_string(frameCount) + ", state=" + std::to_string(m_player ? m_player->getCurrentStateNo() : -1) + "\n";
                logMsg(msg.c_str());
            }
        }
    }

    void Game::updateViews(const sf::Vector2u& winSize) {
        m_windowSize = winSize;
        float winW = static_cast<float>(winSize.x);
        float winH = static_cast<float>(winSize.y);

        // UI View (1920x1080) — 铺满全窗口（拉伸适配）
        sf::View uiView(sf::FloatRect({0, 0}, {1920, 1080}));
        uiView.setViewport(sf::FloatRect({0, 0}, {1, 1}));
        m_uiView = uiView;

        // Game View — 铺满全窗口，坐标空间用窗口实际大小
        sf::View gameView(sf::FloatRect({0, 0}, {winW, winH}));
        gameView.setViewport(sf::FloatRect({0, 0}, {1, 1}));
        m_gameView = gameView;
    }

    void Game::processEvents() {
        while (const auto event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window_.close();
            if (const auto* size = event->getIf<sf::Event::Resized>()) {
                updateViews({size->size.x, size->size.y});
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    if (m_fullscreen) {
                        window_.create(sf::VideoMode({960, 540}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close);
                        window_.setVerticalSyncEnabled(true);
                        window_.setFramerateLimit(60);
                        m_fullscreen = false;
                        updateViews({960, 540});
                    } else {
                        window_.close();
                    }
                }
                if (key->code == sf::Keyboard::Key::F11) {
                    if (m_fullscreen) {
                        window_.create(sf::VideoMode({960, 540}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close);
                        window_.setVerticalSyncEnabled(true);
                        window_.setFramerateLimit(60);
                        m_fullscreen = false;
                        updateViews({960, 540});
                    } else {
                        window_.create(sf::VideoMode::getDesktopMode(), "DragonBall - Phase 6", sf::State::Fullscreen);
                        window_.setVerticalSyncEnabled(true);
                        window_.setFramerateLimit(60);
                        m_fullscreen = true;
                        updateViews(window_.getSize());
                    }
                }
                if (key->code == sf::Keyboard::Key::F1) {
                    Fighter::setShowDebug(!Fighter::getShowDebug());
                }
                if (key->code == sf::Keyboard::Key::F3) {
                    Fighter::setAnimDebug(!Fighter::getAnimDebug());
                    std::cout << "[AnimDebug] " << (Fighter::getAnimDebug() ? "ON" : "OFF") << std::endl;
                }
                // Event-based key tracking: prevents quick taps from being missed
                inputManager_.onKeyPressed(key->code);
            }
            if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
                inputManager_.onKeyReleased(key->code);
            }
        }
    }

    void Game::update(float dt) {
        if (m_gameState == GameState::TITLE) { updateTitle(dt); return; }
        if (m_gameState == GameState::SELECT) { updateSelect(dt); return; }
        if (m_gameState == GameState::STAGE_SELECT) { updateStageSelect(dt); return; }

        // 0. 开场动画 (检测进入循环后切到 191 → 0 → FIGHT)
        if (m_gameState == GameState::INTRO) {
            // 由 CNS State 190 自己控制: ChangeAnim(RoundState=1 时不冻结) → AnimTime=0 → ChangeState 0
            if (m_player && m_dummy &&
                m_player->getCurrentStateNo() == 0 &&
                m_dummy->getCurrentStateNo() == 0) {
                m_gameState = GameState::FIGHT;
                m_roundTimer = 0.f;
                std::cout << "[FIGHT] Round " << m_roundNumber << std::endl;
            }
            // 安全兜底: 15 秒强制退出
            m_roundTimer += dt;
            if (m_roundTimer > 15.f) {
                if (m_player) { m_player->requestStateChange(0); }
                if (m_dummy) { m_dummy->requestStateChange(0); }
                m_gameState = GameState::FIGHT;
            }
        }
        if (m_gameState == GameState::FIGHT) {
            m_roundTimer += dt;
        }

        // HitStop 冻结 (HitShakeOver 由 Fighter::update 基于状态计时器管理)
        if (m_hitStopTimer > 0.0f) {
            m_hitStopTimer -= dt;
            return;
        }

        // 1. SuperPause 检查 (从 Fighter 读取)
        if (m_player && m_player->isInSuperPause()) {
            m_superPauseTimer = m_player->getSuperPauseTime();
            m_superPauseDarken = m_player->getSuperPauseDarken();
            m_player->tickSuperPause();
        }
        if (m_superPauseTimer > 0) {
            m_superPauseTimer--;
            // 动画不更新 (MOVETIME=25, 暂停期间动画和状态都不推进)
            return;
        }
        if (m_superPauseTimer == 0) m_superPauseDarken = false;

        // 2. HitStop 冻结逻辑
        if (m_hitStopTimer > 0.0f) {
            m_hitStopTimer -= dt;
            return;
        }


        // JPx 锁存：按下后保持 15 帧可见
        if (inputManager_.justPressed('x')) m_debugJpxLatch = 15;
        else if (m_debugJpxLatch > 0) m_debugJpxLatch--;

        // 诊断：进入攻击/踢腿状态时记录所有输入状态
        {
            int ns = m_player ? m_player->getCurrentStateNo() : -1;
            static int lastLoggedState = -1;
            if (ns != lastLoggedState && (ns == 230 || ns == 240 || ns == 249 || ns == 200 || ns == 210 || ns == 220)) {
                lastLoggedState = ns;
                bool keyA  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
                bool keyK  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K);
                bool keyL  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
                bool mugenA = inputManager_.buttonA();
                bool mugenB = inputManager_.buttonB();
                bool cmdA  = inputManager_.isHeld("a");
                bool cmdB  = inputManager_.isHeld("b");
                bool justB = inputManager_.justPressed('b');
                bool cmdX  = inputManager_.isHeld("x");
                int dirVal = static_cast<int>(inputManager_.getDirection());
                logMsg(("Diag: state=" + std::to_string(ns)
                    + " keyA=" + std::to_string(keyA) + " keyK=" + std::to_string(keyK) + " keyL=" + std::to_string(keyL)
                    + " mugenA=" + std::to_string(mugenA) + " mugenB=" + std::to_string(mugenB)
                    + " cmdA=" + std::to_string(cmdA) + " cmdB=" + std::to_string(cmdB) + " justB=" + std::to_string(justB)
                    + " cmdX=" + std::to_string(cmdX) + " dir=" + std::to_string(dirVal) + "\n").c_str());
            }
        }

        // 2. P1 更新
        if (m_player) {
            sf::Vector2f dummyPos = m_dummy ? m_dummy->getPosition() : sf::Vector2f(-9999.f, 0.f);
            int oldState = m_player->getCurrentStateNo();
            m_player->update(dt, inputManager_, dummyPos);
            // 命中检测由 checkCombat() 统一处理 (在 P2 更新之后)
            int newState = m_player->getCurrentStateNo();
        }

        // 3. P2 更新
        if (m_dummy) {
            sf::Vector2f playerPos = m_player->getPosition();
            m_dummy->update(dt, inputManagerP2_, playerPos);
        }

        // 3. KO 检测
        if (m_gameState == GameState::FIGHT && m_player && m_dummy) {
            if (m_dummy->getCurrentLife() <= 0) {
                m_player->setMoveContact(false);
                m_dummy->requestStateChange(5150);
                m_player->requestStateChange(180);
                m_gameState = GameState::KO;
                m_koTimer = 2.5f;
                std::cout << "[KO] P2 KO'd!" << std::endl;
            }
            if (m_player->getCurrentLife() <= 0) {
                m_dummy->setMoveContact(false);
                m_player->requestStateChange(5150);
                m_dummy->requestStateChange(180);
                m_gameState = GameState::KO;
                m_koTimer = 2.5f;
                std::cout << "[KO] P1 KO'd!" << std::endl;
            }
        }
        if (m_gameState == GameState::KO) {
            m_koTimer -= dt;
            // KO 期间只更新动画, 冻结其他
            if (m_player && m_koTimer > 0.f && m_koTimer < 2.0f) {
                m_player->getAnimationPlayer().update(dt);
                m_dummy->getAnimationPlayer().update(dt);
            }
            if (m_koTimer <= 0.f) {
                // 判断谁赢了这回合
                bool p1KO = (m_player && m_player->getCurrentLife() <= 0);
                bool p2KO = (m_dummy && m_dummy->getCurrentLife() <= 0);
                if (p1KO) m_p2RoundsWon++;
                if (p2KO) m_p1RoundsWon++;
                if (m_p1RoundsWon >= 2 || m_p2RoundsWon >= 2) {
                    std::cout << "[MATCH] Player " << (m_p1RoundsWon >= 2 ? "1" : "2") << " wins!" << std::endl;
                    m_gameState = GameState::SELECT;
                    m_p1RoundsWon = 0;
                    m_p2RoundsWon = 0;
                    m_roundNumber = 1;
                    m_player.reset();
                    m_dummy.reset();
                    m_hudP1.reset();
                    m_hudP2.reset();
                    m_helpers.clear();
                    m_sparks.clear();
                    m_selectPhase = 0;
                } else {
                    resetRound();
                    m_roundNumber++;
                    std::cout << "[ROUND] Round " << m_roundNumber << std::endl;
                    if (m_player) { m_player->setRoundState(1); m_player->setRoundNo(m_roundNumber); m_player->requestStateChange(5900); }
                    if (m_dummy) { m_dummy->setRoundState(1); m_dummy->setRoundNo(m_roundNumber); m_dummy->requestStateChange(5900); }
                    m_gameState = GameState::INTRO;
                    m_roundTimer = 0.f;
                }
            }
        }

        // 3.25 读取并创建 Helper (飞行道具)
        if (m_player) {
            auto pending = m_player->drainPendingHelpers();
            for (auto& ph : pending) {
                // 去重: 同父对象同 ID 的 Helper 已存在则跳过
                bool exists = false;
                for (const auto& existing : m_helpers) {
                    if (existing.id == ph.id && !existing.done && existing.parent == m_player.get()) {
                        exists = true; break;
                    }
                }
                if (exists) continue;

                const Animation* anim = m_player->getAnimation(ph.animId);
                if (!anim) continue;
                HelperEntity he;
                he.id = ph.id;
                he.animPlayer.play(*anim);
                he.animPlayer.setFacingRight(ph.facingRight);
                he.facingRight = ph.facingRight;
                he.position = ph.position;
                he.velocity = ph.velocity;
                he.damage = ph.damage;
                he.sparkno = ph.sparkno;
                he.stateNo = ph.stateNo;        // Helper 状态号
                he.stateRegistry = &m_player->getStateRegistry();
                he.parent = m_player.get();
                he.parentStateno = ph.parentStateno;
                if (he.stateRegistry) {
                    const auto* sd = he.stateRegistry->getStateDef(he.stateNo);
                    if (sd) he.sprpriority = sd->sprpriority;
                }
                he.stateTime = -0.008f;
                m_helpers.push_back(std::move(he));
            }
        }
        if (m_dummy) {
            auto pending = m_dummy->drainPendingHelpers();
            for (auto& ph : pending) {
                bool exists = false;
                for (const auto& existing : m_helpers) {
                    if (existing.id == ph.id && !existing.done && existing.parent == m_dummy.get()) {
                        exists = true; break;
                    }
                }
                if (exists) continue;
                const Animation* anim = m_dummy->getAnimation(ph.animId);
                if (!anim) continue;
                HelperEntity he;
                he.id = ph.id;
                he.animPlayer.play(*anim);
                he.animPlayer.setFacingRight(ph.facingRight);
                he.facingRight = ph.facingRight;
                he.position = ph.position;
                he.velocity = ph.velocity;
                he.damage = ph.damage;
                he.sparkno = ph.sparkno;
                he.stateNo = ph.stateNo;
                he.stateRegistry = &m_dummy->getStateRegistry();
                he.parent = m_dummy.get();
                he.parentStateno = ph.parentStateno;
                he.stateTime = -0.008f;
                m_helpers.push_back(std::move(he));
            }
        }

        // 3.5 更新 HUD
        if (m_hudP1 && m_player) {
            m_hudP1->update(m_player->getCurrentLife(), m_player->getMaxLife());
            m_hudP1->updatePower(m_player->getPower(), m_player->getMaxPower());
        }
        if (m_hudP2 && m_dummy) {
            m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
            m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());
        }

        // 4. 战斗检测
        checkCombat();

        // 4.5 战斗检测后再次执行 P1 CNS (捕获 MOVECONTACT 触发的 Helper 等)
        if (m_player && m_player->isAttacking()) {
            m_player->executeCurrentStateCNS(nullptr, dt);
        }

        // 5. 物理碰撞 (推撞) + 场景边界
        handlePushCollision();
        if (m_player) clampFighterToStage(*m_player);
        if (m_dummy) clampFighterToStage(*m_dummy);

        // 6. 更新 Helper (飞行道具 + CNS 执行)
        for (auto& h : m_helpers) {
            h.animPlayer.update(dt);
            h.stateTime += dt;
            bool isFirstUpdate = h.firstUpdate;
            h.firstUpdate = false;  // 首次更新后清除标记
            h.drawOverrides = DrawOverrides(); // 重置每帧绘制覆盖

            // 先移动再执行 CNS (确保碰撞检测在最新位置)
            h.position.x += h.velocity.x * 60.f * dt;
            h.position.y += h.velocity.y * 60.f * dt;

            // 6.1 执行 Helper 的 CNS 状态 (关键控制器)
            if (const auto* stateDef = h.stateRegistry ? h.stateRegistry->getStateDef(h.stateNo) : nullptr) {
                int ticks = static_cast<int>(h.stateTime * 60.f);
                for (const auto& ctrl : stateDef->controllers) {
                    // VELSET: bypass parent TIME check, use helper own timer
                    if (ctrl->type == ControllerType::VELSET) {
                        const auto* vc = dynamic_cast<const VelSetController*>(ctrl.get());
                        if (vc && vc->hasX && h.stateTime < 0.02f) {
                            float dir = h.facingRight ? 1.f : -1.f;
                            h.velocity.x = dir * vc->valueX;
                        }
                    }
                    // 用父 Fighter 检查 trigger (Helper 自身没有 Fighter 状态)
                    if (!ctrl->checkTriggers(h.parent ? *h.parent : *(m_player.get()), nullptr, ticks)) continue;

                    // 手动执行关键控制器
                    switch (ctrl->type) {
                        case ControllerType::VELSET: {
                            const auto* vc = dynamic_cast<const VelSetController*>(ctrl.get());
                            if (vc && !vc->hasX && !vc->hasY) break;
                            // 简化: 直接用 rawValue 设置速度
                            if (vc->hasX && h.stateTime < 0.02f) {
                                float dir = h.facingRight ? 1.f : -1.f;
                                h.velocity.x = dir * vc->valueX;
                            }
                            break;
                        }
                        case ControllerType::POSADD: {
                            const auto* pc = dynamic_cast<const PosAddController*>(ctrl.get());
                            if (pc) {
                                // 用父 Fighter 求值表达式 (如 p2dist x)
                                Fighter* refFtr = h.parent ? h.parent : m_player.get();
                                float addX = pc->valueX;
                                float addY = pc->valueY;
                                if (!pc->valueXStr.empty()) addX = static_cast<float>(evaluateCNSExpression(pc->valueXStr, *refFtr));
                                if (!pc->valueYStr.empty()) addY = static_cast<float>(evaluateCNSExpression(pc->valueYStr, *refFtr));
                                h.position.x += addX * (h.facingRight ? 1.f : -1.f);
                                h.position.y += addY;
                            }
                            break;
                        }
                        case ControllerType::CHANGESTATE: {
                            const auto* cs = dynamic_cast<const ChangeStateController*>(ctrl.get());
                            if (cs && cs->value > 0) {
                                h.stateNo = cs->value;
                                h.stateTime = 0.f;
                            }
                            break;
                        }
                        case ControllerType::DESTROY_SELF: {
                            if (isFirstUpdate) break;
                            int selfTicks = static_cast<int>(h.stateTime * 60.f);
                            bool shouldDie = false;
                            for (const auto& tl : ctrl->triggers) {
                                for (const auto& c : tl.conditions) {
                                    if (c.type == CondType::TIME) {
                                        if (c.op == CondOp::EQ && selfTicks == c.rhsInt) shouldDie = true;
                                        if (c.op == CondOp::GTE && selfTicks >= c.rhsInt) shouldDie = true;
                                    }
                                    if (c.type == CondType::ANIMTIME && c.op == CondOp::EQ) {
                                        if (h.animPlayer.getAnimTime() == c.rhsInt) shouldDie = true;
                                    }
                                    if (c.type == CondType::PARENT_STATENO && c.op == CondOp::NEQ && h.parent) {
                                        // 使用帧开始时的父状态号 (避免同帧 ChangeState 的干扰)
                                        if (h.parent->getFrameStartState() != c.rhsInt) shouldDie = true;
                                    }
                                    if (c.type == CondType::PARENT_STATENO && c.op == CondOp::EQ && h.parent) {
                                        if (h.parent->getFrameStartState() == c.rhsInt) shouldDie = true;
                                    }
                                }
                            }
                            if (shouldDie) {
                                h.done = true;
                            }
                            break;
                        }
                        case ControllerType::BIND_TO_ROOT: {
                            const auto* bt = dynamic_cast<const BindToRootController*>(ctrl.get());
                            if (bt && h.parent) {
                                sf::Vector2f parPos = h.parent->getPosition();
                                float dir = h.parent->isFacingRight() ? 1.f : -1.f;
                                h.position.x = parPos.x + bt->m_posX * dir;
                                h.position.y = parPos.y + bt->m_posY;
                            }
                            break;
                        }
                        case ControllerType::HITDEF: {
                            // Helper 的 HitDef (碰撞检测基于最新位置)
                            if (h.hitCooldown <= 0 && m_player && m_dummy) {
                                const auto& hitDefs = h.stateRegistry->getHitDefs(h.stateNo);
                                if (!hitDefs.empty()) {
                                    for (const auto& hd : hitDefs) {
                                        Fighter* target = (h.parent == m_player.get()) ? m_dummy.get() : m_player.get();
                                        sf::FloatRect hurt = target->getActiveHurtbox();
                                        if (hurt.size.x > 0) {
                                            // 使用 Helper 当前动画帧的 clsn1 作为碰撞框
                                            sf::FloatRect hb = {{0, 0}, {0, 0}};
                                            const auto& frame = h.animPlayer.getCurrentFrame();
                                            if (!frame.clsn1.empty()) {
                                                // 计算所有 clsn1 的包围盒
                                                bool first = true;
                                                float minX = 0, maxX = 0, minY = 0, maxY = 0;
                                                float dir = h.facingRight ? 1.f : -1.f;
                                                for (const auto& clsn : frame.clsn1) {
                                                    sf::Vector2f p1 = {h.position.x + clsn.topLeft.x * dir, h.position.y + clsn.topLeft.y};
                                                    sf::Vector2f p2 = {h.position.x + clsn.bottomRight.x * dir, h.position.y + clsn.bottomRight.y};
                                                    float rx1 = std::min(p1.x, p2.x), ry1 = std::min(p1.y, p2.y);
                                                    float rx2 = std::max(p1.x, p2.x), ry2 = std::max(p1.y, p2.y);
                                                    if (first) {
                                                        minX = rx1; minY = ry1; maxX = rx2; maxY = ry2;
                                                        first = false;
                                                    } else {
                                                        minX = std::min(minX, rx1); minY = std::min(minY, ry1);
                                                        maxX = std::max(maxX, rx2); maxY = std::max(maxY, ry2);
                                                    }
                                                }
                                                hb = {{minX, minY}, {maxX - minX, maxY - minY}};
                                            } else {
                                                hb = {{h.position.x - 16.f, h.position.y - 16.f}, {32.f, 32.f}};
                                            }
                                            if (hb.findIntersection(hurt).has_value()) {
                                                auto hitPos = hb.findIntersection(hurt);
                                                target->takeDamage(hd.damage);

                                                // 设置 HitInfo (保留原始速度, 击飞在气功波结束后才触发)
                                                HitInfo hitInfo;
                                                hitInfo.damage = hd.damage;
                                                {
                                                    float hitDir = h.facingRight ? 1.f : -1.f;
                                                    hitInfo.groundVelocityX = hd.groundVelocityX * hitDir;
                                                    hitInfo.airVelocityX = hd.airVelocityX * hitDir;
                                                }
                                                hitInfo.airVelocityY = hd.airVelocityY;
                                                hitInfo.animtype = hd.animtype;
                                                hitInfo.groundHittime = hd.groundHittime > 0 ? hd.groundHittime : 15;
                                                hitInfo.fall = hd.fall;
                                                target->setHitInfo(hitInfo);

                                                // 每次命中重新进入受击状态 5000
                                                // → velset=0,0 冻结目标 + stateTimer 重置
                                                // → 目标原地硬直, 不击飞
                                                // → 气功波结束后不再重置, 计时器走到 HitShakeOver 才击飞
                                                std::string at = hd.animtype;
                                                std::transform(at.begin(), at.end(), at.begin(), ::tolower);
                                                int ts = 5000;
                                                if (at == "medium") ts = 5001;
                                                else if (at == "hard") ts = 5002;
                                                target->requestStateChange(ts);

                                                if (hitPos.has_value()) {
                                                    spawnSpark(hd.sparkno > 0 ? hd.sparkno : 1200, {
                                                        hitPos->position.x + hitPos->size.x / 2,
                                                        hitPos->position.y + hitPos->size.y / 2
                                                    });
                                                } else {
                                                    spawnSpark(hd.sparkno > 0 ? hd.sparkno : 1200, h.position);
                                                }
                                                if (h.parent) {
                                                    h.parent->setMoveContact(true);
                                                    h.parent->setMoveHit(true);
                                                }
                                                // 应用防御方暂停帧 (pausetime 第二值)
                                                int defenderPause = hd.guardPausetime;
                                                if (defenderPause > 0) {
                                                    m_hitStopTimer = defenderPause / 60.0f;
                                                    target->setHitShakeOver(false);
                                                }
                                                h.hasHit = true;
                                                h.hitCooldown = 2;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case ControllerType::ANGLEDRAW: {
                            const auto* ad = dynamic_cast<const AngleDrawController*>(ctrl.get());
                            if (ad) {
                                h.drawOverrides.scaleX = ad->m_scaleX;
                                h.drawOverrides.scaleY = ad->m_scaleY;
                            }
                            break;
                        }
                        case ControllerType::TRANS: {
                            const auto* tc = dynamic_cast<const TransController*>(ctrl.get());
                            if (tc) {
                                int alphaSrc = tc->m_alphaSrc;
                                // 表达式求值: 替换 time 为状态帧数
                                if (!tc->m_alphaSrcExpr.empty()) {
                                    std::string expr = tc->m_alphaSrcExpr;
                                    std::string timeStr = std::to_string(ticks);
                                    size_t pos = 0;
                                    while ((pos = expr.find("time", pos)) != std::string::npos) {
                                        expr.replace(pos, 4, timeStr);
                                        pos += timeStr.length();
                                    }
                                    Fighter& eFtr = h.parent ? *h.parent : *m_player;
                                    alphaSrc = evaluateCNSExpression(expr, eFtr);
                                }
                                int src = std::max(0, std::min(256, alphaSrc));
                                h.drawOverrides.alpha = static_cast<uint8_t>((src * 255) / 256);
                            }
                            break;
                        }
                        default: break;
                    }
                }
            }

            // 超出屏幕 → 移除 (lifetime 由 DestroySelf 控制, 不过期自动删除)
            h.lifetime--;
            if (h.hitCooldown > 0) h.hitCooldown--;
            else h.hasHit = false;
            if (h.lifetime <= 0 || h.position.x < -200.f || h.position.x > 1000.f) {
                h.done = true;
            }
        }
        m_helpers.erase(std::remove_if(m_helpers.begin(), m_helpers.end(),
            [](const HelperEntity& h) { return h.done; }), m_helpers.end());

        // 7. 更新火花动画
        for (auto& spark : m_sparks) {
            spark.animPlayer.update(dt);
            if (spark.animPlayer.hasJustLooped()) {
                spark.done = true;
            }
        }
        m_sparks.erase(std::remove_if(m_sparks.begin(), m_sparks.end(),
            [](const Spark& s) { return s.done; }), m_sparks.end());
    }

    void Game::updateTitle(float dt) {
        // 按任意键进入选人
        if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::Enter) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::Space) ||
            inputManager_.justPressed('x') || inputManager_.justPressed('y') ||
            inputManager_.justPressed('z') || inputManager_.justPressed('a') ||
            inputManager_.justPressed('b') || inputManager_.justPressed('c') ||
            inputManager_.justPressed('s') ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::A) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::D) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::W) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::S)) {
            m_gameState = GameState::SELECT;
            m_selectPhase = 0;
            m_p1Choice = 0;
            m_p2Choice = 0;
        }
    }

    void Game::updateSelect(float dt) {
        int n = static_cast<int>(m_availableChars.size());
        if (n == 0) return;

        // P1 选人 (A/D 切换, J 确认)
        if (m_selectPhase == 0) {
            int prev = m_p1Choice;
            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::A) ||
                inputManager_.isKeyJustPressed(sf::Keyboard::Key::Left)) {
                m_p1Choice = (m_p1Choice - 1 + n) % n;
            }
            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::D) ||
                inputManager_.isKeyJustPressed(sf::Keyboard::Key::Right)) {
                m_p1Choice = (m_p1Choice + 1) % n;
            }
            if (m_p1Choice != prev) m_selectAnimPos = 0.f;
            else m_selectAnimPos = std::min(m_selectAnimPos + dt * 8.f, 1.f);

            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J)) {
                m_selectPhase = 1;
                // P2 默认选 P1 没选的角色
                for (int i = 0; i < n; i++) {
                    if (i != m_p1Choice) { m_p2Choice = i; break; }
                }
                m_selectAnimPos = 0.f;
                std::cout << "[Select] P1 chose " << m_availableChars[m_p1Choice].displayName << std::endl;
            }
        }
        // P2 选人 (← → 切换, Enter 确认)
        else if (m_selectPhase == 1) {
            int prev = m_p2Choice;
            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::Left)) {
                do {
                    m_p2Choice = (m_p2Choice - 1 + n) % n;
                } while (m_p2Choice == m_p1Choice);
            }
            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::Right)) {
                do {
                    m_p2Choice = (m_p2Choice + 1) % n;
                } while (m_p2Choice == m_p1Choice);
            }
            if (m_p2Choice != prev) m_selectAnimPos = 0.f;
            else m_selectAnimPos = std::min(m_selectAnimPos + dt * 8.f, 1.f);

            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::Enter)) {
                m_selectPhase = 2;
                std::cout << "[Select] P2 chose " << m_availableChars[m_p2Choice].displayName << std::endl;
                m_gameState = GameState::STAGE_SELECT;
                m_stageChoice = 0;
                m_stageAnimPos = 1.f;
            }
        }
    }

    void Game::updateStageSelect(float dt) {
        int n = static_cast<int>(m_availableStages.size());
        if (n == 0) {
            m_gameState = GameState::SELECT;
            return;
        }
        int prev = m_stageChoice;
        if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::A) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::Left)) {
            m_stageChoice = (m_stageChoice - 1 + n) % n;
        }
        if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::D) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::Right)) {
            m_stageChoice = (m_stageChoice + 1) % n;
        }
        if (m_stageChoice != prev) m_stageAnimPos = 0.f;
        else m_stageAnimPos = std::min(m_stageAnimPos + dt * 8.f, 1.f);

        if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J) ||
            inputManager_.isKeyJustPressed(sf::Keyboard::Key::Enter)) {
            std::cout << "[Stage] Chose " << m_availableStages[m_stageChoice].name << std::endl;
            initFight(m_p1Choice, m_p2Choice, m_stageChoice);
        }
    }

    void Game::checkCombat() {
        if (!m_player || !m_dummy || m_dummy->isDead()) return;
        if (m_player->isHitConsumed()) {
            return;
        }
        if (m_player->getCurrentStateNo() == 600) {
            auto pos = m_player->getPosition();
            std::cout << "[CB600] consumed=" << m_player->isHitConsumed()
                      << " mh=" << m_player->hasMoveHit()
                      << " pos=(" << (int)pos.x << "," << (int)pos.y << ")"
                      << " vy=" << (int)m_player->getVelocityY()
                      << std::endl;
        }
        const auto& hitDefs = m_player->getCurrentHitDefs();
        static int emptyCount = 0;
        if (hitDefs.empty()) {
            if (++emptyCount % 60 == 1) {
                std::cout << "[checkCombat] no HitDefs state=" << m_player->getCurrentStateNo() << std::endl;
            }
            return;
        }
        // Debug: 打印 hitbox 位置
        static int cbCount = 0;
        if (++cbCount % 30 == 1) {
            sf::FloatRect hb = m_player->getActiveHitbox();
            sf::FloatRect hr = m_dummy->getActiveHurtbox();
            //std::cout << "[CB] hitbox=" << hb.position.x << "," << hb.position.y << " " << hb.size.x << "x" << hb.size.y
            //          << " hurtbox=" << hr.position.x << "," << hr.position.y << " " << hr.size.x << "x" << hr.size.y << std::endl;
        }

        const auto& hit = hitDefs[0];

        sf::FloatRect hitBox = m_player->getActiveHitbox();
        sf::FloatRect hurtBox = m_dummy->getActiveHurtbox();

        // 将攻击框偏移到物理移动前的位置 (防止 VelSet 冲过头导致判定落空)
        {
            sf::Vector2f delta = m_player->getPrePhysicsPos() - m_player->getPosition();
            hitBox.position.x += delta.x;
            hitBox.position.y += delta.y;
        }

        if (hitBox.size.x <= 0 || hitBox.size.y <= 0) {
            static int lastNoHitbox = -1;
            int cs = m_player->getCurrentStateNo();
            if (cs != lastNoHitbox) {
                logMsg(("No hitbox state=" + std::to_string(cs) + "\n").c_str());
                lastNoHitbox = cs;
            }
            return;
        }
        if (hurtBox.size.x <= 0 || hurtBox.size.y <= 0) return;

        auto intersection = hitBox.findIntersection(hurtBox);
        if (!intersection.has_value()) {
            static int lastMissState = -1;
            int cs = m_player->getCurrentStateNo();
            if (cs != lastMissState) {
                std::cout << "[CBmiss] state=" << cs
                          << " hitbox=(" << hitBox.position.x << "," << hitBox.position.y << " " << hitBox.size.x << "x" << hitBox.size.y << ")"
                          << " hurtbox=(" << hurtBox.position.x << "," << hurtBox.position.y << " " << hurtBox.size.x << "x" << hurtBox.size.y << ")"
                          << " playerPos=(" << m_player->getPosition().x << "," << m_player->getPosition().y << ")"
                          << " facing=" << (m_player->isFacingRight()?"R":"L")
                          << std::endl;
                lastMissState = cs;
            }
            return;
        }

        std::cout << "[checkCombat] HIT! state=" << m_player->getCurrentStateNo() << std::endl;
        m_player->setMoveContact(true);

        // 计算命中接触点 (碰撞矩形中心 + sparkxy 偏移)
        sf::Vector2f contactPoint(
            intersection->position.x + intersection->size.x / 2 + static_cast<float>(hit.sparkX),
            intersection->position.y + intersection->size.y / 2 + static_cast<float>(hit.sparkY)
        );
        spawnSpark(hit.sparkno, contactPoint);

        // 如果攻击方有 unguardable 标志, 防御无效
        bool isGuarding = m_dummy->isGuarding() && !(m_player->getAssertFlags() & 4);
        if (isGuarding) {
            m_player->setMoveGuarded(true);
        } else {
            m_player->setMoveHit(true);
        }
        m_player->setHitConsumed(true);

        std::string hitMsg = "HIT! state=" + std::to_string(m_player->getCurrentStateNo()) + "\n";
        logMsg(hitMsg.c_str());

        HitInfo info;
        info.damage = hit.damage;
        info.guardDamage = hit.guardDamage;
        info.groundHittime = (hit.groundHittime > 0) ? hit.groundHittime : 15;
        info.groundSlidetime = (hit.groundSlidetime > 0) ? hit.groundSlidetime : 15;
        info.airHittime = (hit.airHittime > 0) ? hit.airHittime : 12;
        // 速度方向按攻击方面向翻转 (负值=击退)
        {
            float hitDir = m_player->isFacingRight() ? 1.f : -1.f;
            info.groundVelocityX = hit.groundVelocityX * hitDir;
            info.airVelocityX = hit.airVelocityX * hitDir;
            info.airVelocityY = hit.airVelocityY;
        }
        info.airguardVelocityX = hit.airguardVelocityX;
        info.airguardVelocityY = hit.airguardVelocityY;
        info.animtype = hit.animtype.empty() ? "Light" : hit.animtype;
        info.p1stateno = hit.p1stateno;
        info.p2stateno = hit.p2stateno;
        info.fall = hit.fall;
        info.fallrecover = hit.fallRecover;
        info.fallyvel = hit.fallYVel;
        info.guarded = isGuarding;
        info.ctrltime = hit.groundHittime + 1;
        info.yaccel = 0;
        info.juggle = hit.juggle;
        info.hitid = hit.id;
        info.groundType = hit.groundType.empty() ? "High" : hit.groundType;
        info.airType = hit.airType.empty() ? "High" : hit.airType;
        info.envshakeTime = hit.envshakeTime;

        m_dummy->setHitInfo(info);

        if (isGuarding) {
            m_dummy->takeDamage(info.guardDamage);
            m_dummy->requestStateChange(150);
            if (hit.guardPausetime > 0) {
                m_hitStopDuration = hit.guardPausetime;
                m_hitStopTimer = hit.guardPausetime / 60.0f;
            }
        } else {
            m_dummy->takeDamage(info.damage);

            int targetState = 5000;
            {
                std::string type = info.animtype;
                if (type == "Light" || type == "light") targetState = 5000;
                else if (type == "Medium" || type == "medium") targetState = 5001;
                else targetState = 5002;
            }
            m_dummy->requestStateChange(targetState);

            // 击退速度由受击状态的 HitVelSet 控制器负责 (M.U.G.E.N 标准)
            // 不在 checkCombat 中设置

            // P1STATENO: 攻击方命中后强制跳转状态 (连段链)
            if (info.p1stateno > 0) {
                std::cout << "[Combo] p1stateno=" << info.p1stateno
                          << " playerState=" << m_player->getCurrentStateNo() << std::endl;
                m_player->requestStateChange(info.p1stateno);
            }

            m_dummy->setHitShakeOver(false);

            if (hit.pausetime > 0) {
                m_hitStopDuration = hit.pausetime;
                m_hitStopTimer = hit.pausetime / 60.0f;
            }
        }

        if (m_hudP2) m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
        if (m_hudP2) m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());
    }

    void Game::handlePushCollision() {
        if (!m_player || !m_dummy) return;

        // M.U.G.E.N 规范: 推撞只对非攻击/非受击状态生效
        if (m_player->getMoveType() != 0 || m_dummy->getMoveType() != 0) return;

        sf::FloatRect box1 = m_player->getPushBox();
        sf::FloatRect box2 = m_dummy->getPushBox();

        if (box1.findIntersection(box2).has_value()) {
            float center1 = box1.position.x + box1.size.x / 2;
            float center2 = box2.position.x + box2.size.x / 2;

            float overlap = (box1.size.x + box2.size.x) / 2 - std::abs(center2 - center1);
            if (overlap > 0) {
                float pushAmount = overlap + 1.0f;

                if (center1 < center2) {
                    m_player->addPositionX(-pushAmount);
                    m_dummy->addPositionX(pushAmount);
                } else {
                    m_player->addPositionX(pushAmount);
                    m_dummy->addPositionX(-pushAmount);
                }
            }
        }
    }

    void Game::clampFighterToStage(Fighter& fighter) {
        // M.U.G.E.N 标准舞台边界
        const float STAGE_LEFT = 0.f;
        const float STAGE_RIGHT = static_cast<float>(m_windowSize.x);
        const float STAGE_TOP = 0.f;
        sf::Vector2f pos = fighter.getPosition();
        sf::FloatRect pushBox = fighter.getPushBox();
        float halfWidth = pushBox.size.x * 0.5f;
        if (pos.x - halfWidth < STAGE_LEFT) pos.x = STAGE_LEFT + halfWidth;
        if (pos.x + halfWidth > STAGE_RIGHT) pos.x = STAGE_RIGHT - halfWidth;
        if (pos.y < STAGE_TOP) {
            pos.y = STAGE_TOP;
            if (fighter.getVelocityY() < 0.f) fighter.setVelocityY(0.f);
        }
        fighter.setPosition(pos.x, pos.y);
    }

    void Game::render() {
        // UI 画面 (TITLE/SELECT/STAGE_SELECT) — 使用 1920x1080 UI View 直接渲染
        if (m_gameState == GameState::TITLE) { renderTitle(); return; }
        if (m_gameState == GameState::SELECT) { renderSelect(); return; }
        if (m_gameState == GameState::STAGE_SELECT) { renderStageSelect(); return; }

        // 战斗场景 (FIGHT/INTRO/KO)
        // 画面震动 (EnvShake)
        sf::View shakeView = m_gameView;
        int totalShakeAmpl = 0;
        if (m_player && m_player->getShakeAmpl() > 0) totalShakeAmpl = m_player->getShakeAmpl();
        if (m_dummy && m_dummy->getShakeAmpl() > totalShakeAmpl) totalShakeAmpl = m_dummy->getShakeAmpl();
        if (totalShakeAmpl > 0) {
            float offsetX = static_cast<float>((std::rand() % (totalShakeAmpl * 2 + 1)) - totalShakeAmpl);
            float offsetY = static_cast<float>((std::rand() % (totalShakeAmpl * 2 + 1)) - totalShakeAmpl);
            shakeView.move({offsetX, offsetY});
        }
        window_.setView(shakeView);
        window_.clear(sf::Color(30, 30, 50));





































































































        // 战斗场景背景
        if (m_stageBg.getSize().x > 0 && (m_gameState == GameState::FIGHT || m_gameState == GameState::INTRO || m_gameState == GameState::KO)) {
            sf::Sprite stageSpr(m_stageBg);
            float sx = static_cast<float>(m_windowSize.x) / m_stageBg.getSize().x;
            float sy = static_cast<float>(m_windowSize.y) / m_stageBg.getSize().y;
            stageSpr.setScale({sx, sy});
            window_.draw(stageSpr);
        }

        // Helper 按 sprpriority 排序 (让低优先级的画在底层)
        std::vector<const HelperEntity*> sortedHelpers;
        sortedHelpers.reserve(m_helpers.size());
        for (const auto& h : m_helpers) sortedHelpers.push_back(&h);
        std::sort(sortedHelpers.begin(), sortedHelpers.end(),
            [](const HelperEntity* a, const HelperEntity* b) { return a->sprpriority < b->sprpriority; });
        for (const auto* h : sortedHelpers) {
            h->animPlayer.draw(window_, h->position, &h->drawOverrides);
        }
        for (const auto& spark : m_sparks) {
            spark.animPlayer.draw(window_, spark.position);
        }
        if (m_player) m_player->draw(window_);
        if (m_dummy) m_dummy->draw(window_);

        // 调试碰撞框 (始终在最上层)
        if (m_debugReady) {
            if (m_player) m_player->drawDebug(window_);
            if (m_dummy) m_dummy->drawDebug(window_);
        }

        // HUD 用游戏视图 (与战斗场景坐标一致)
        window_.setView(m_gameView);
        if (m_hudP2) m_hudP2->setPosition(static_cast<float>(m_windowSize.x) - 10.0f, 10.0f);
        if (m_hudP1) m_hudP1->draw(window_);
        if (m_hudP2) m_hudP2->draw(window_);

        // 计时器
        if (m_gameState == GameState::FIGHT) {
            int remaining = std::max(0, 99 - static_cast<int>(m_roundTimer));
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", remaining);
            m_bitmapFont.drawText(window_, buf, {m_windowSize.x / 2.f + 2.f, 30}, 28, sf::Color::White, {0.5f, 0.5f});
        }

        // 回合信息 (显示在计时器下方)
        if (m_player && (m_gameState == GameState::INTRO || m_gameState == GameState::FIGHT)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "ROUND %d", m_roundNumber);
            m_bitmapFont.drawText(window_, buf, {m_windowSize.x / 2.f, 58}, 24, sf::Color::White, {0.5f, 0.f});
        }

        // 调试信息：显示当前状态、动画ID、命令状态 (按下 F1 切换)
        if (m_debugReady && m_player && Fighter::getShowDebug()) {
            int stateNo = m_player->getCurrentStateNo();
            int animId = m_player->getCurrentAnimId();
            int animElem = m_player->getCurrentAnimElem();
            bool ctrl = m_player->hasControl();
            int stateType = m_player->getStateType();
            int moveType = m_player->getMoveType();
            int stateTime = m_player->getStateTime();

            char buf[256];
            // 行1: 状态和动画
            snprintf(buf, sizeof(buf),
                "State:%d Anim:%d Elem:%d Time:%d | Type:%d Move:%d | Ctrl:%d",
                stateNo, animId, animElem, stateTime, stateType, moveType, ctrl);
            m_debugText->setString(buf);
            window_.draw(*m_debugText);

            // 行2: 按钮按住状态 (H=hold)
            snprintf(buf, sizeof(buf),
                "Btn Hx:%d Hy:%d Hz:%d Ha:%d Hb:%d Hc:%d",
                inputManager_.buttonX() ? 1 : 0,
                inputManager_.buttonY() ? 1 : 0,
                inputManager_.buttonZ() ? 1 : 0,
                inputManager_.buttonA() ? 1 : 0,
                inputManager_.buttonB() ? 1 : 0,
                inputManager_.buttonC() ? 1 : 0);
            sf::Text btnText(m_debugFont);
            btnText.setCharacterSize(14);
            btnText.setFillColor(sf::Color(200, 200, 100));
            btnText.setPosition({10.f, 556.f});
            btnText.setString(buf);
            window_.draw(btnText);

            // 行3: 方向解析 + 刚按下(锁存)
            snprintf(buf, sizeof(buf),
                "Dir:%d fwd:%d back:%d up:%d down:%d | JPx:%d",
                static_cast<int>(inputManager_.getDirection()),
                inputManager_.isHeld("holdfwd") ? 1 : 0,
                inputManager_.isHeld("holdback") ? 1 : 0,
                inputManager_.isHeld("holdup") ? 1 : 0,
                inputManager_.isHeld("holddown") ? 1 : 0,
                m_debugJpxLatch);
            sf::Text dirText(m_debugFont);
            dirText.setCharacterSize(14);
            dirText.setFillColor(sf::Color(100, 200, 100));
            dirText.setPosition({10.f, 572.f});
            dirText.setString(buf);
            window_.draw(dirText);
        }

        // SuperPause 暗化效果
        if (m_superPauseDarken) {
            sf::RectangleShape dark({static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y)});
            dark.setFillColor({0, 0, 0, 120});  // 半透明黑
            window_.draw(dark);
        }

        if (m_player && m_dummy) {
            // KO 文字
            if (m_gameState == GameState::KO) {
                m_bitmapFont.drawText(window_, "K.O.!", {m_windowSize.x / 2.f, m_windowSize.y / 2.f}, 80, sf::Color::Red, {0.5f, 0.5f});
            }
        }

        window_.display();
    }

    void Game::resetRound() {
        if (!m_player || !m_dummy) return;
        m_player->setRoundState(1);
        m_dummy->setRoundState(1);
        m_player->setRoundNo(m_roundNumber);
        m_dummy->setRoundNo(m_roundNumber);
        m_player->requestStateChange(5900);
        m_dummy->requestStateChange(5900);
        m_player->resetLife();
        m_dummy->resetLife();
        m_player->setVelocityX(0.f);
        m_player->setVelocityY(0.f);
        m_dummy->setVelocityX(0.f);
        m_dummy->setVelocityY(0.f);
        float p1Y = 480.f - static_cast<float>(m_player->getAnimationPlayer().getCurrentFrame().offset.y);
        float p2Y = 480.f - static_cast<float>(m_dummy->getAnimationPlayer().getCurrentFrame().offset.y);
        float cx = static_cast<float>(m_windowSize.x) / 2.f;
        m_player->setPosition(cx - 200.f, p1Y);
        m_dummy->setPosition(cx + 200.f, p2Y);
        m_helpers.clear();
        m_sparks.clear();
        m_hitStopTimer = 0.f;
        m_superPauseTimer = 0;
        m_superPauseDarken = false;
    }

void Game::renderTitle() {
    window_.clear(sf::Color::Black);
    window_.setView(m_uiView);
    sf::Sprite bg(m_texTitleBg);
    if (m_texTitleBg.getSize().x > 0) {
        float sx = 1920.f / m_texTitleBg.getSize().x;
        float sy = 1080.f / m_texTitleBg.getSize().y;
        bg.setScale({sx, sy});
        window_.draw(bg);
    }
    m_bitmapFont.drawText(window_, "PRESS ENTER TO START", {960.f, 980.f}, 64, sf::Color::White, {0.5f, 0.f});
    window_.display();
}

void Game::renderSelect() {
    window_.clear(sf::Color::Black);
    window_.setView(m_uiView);
    sf::Sprite bg(m_texSelectBg);
    if (m_texSelectBg.getSize().x > 0) {
        bg.setScale({1920.f / m_texSelectBg.getSize().x, 1080.f / m_texSelectBg.getSize().y});
        window_.draw(bg);
    }

    m_bitmapFont.drawText(window_, "SELECT YOUR FIGHTER", {960, 60}, 72, sf::Color::White, {0.5f, 0.f});

    int n = static_cast<int>(m_availableChars.size());
    float centerX = 960.f, centerY = 500.f + m_selectOffsetY;
    float sideW = 140.f, sideH = 240.f;
    float largeW = sideW + (320.f - sideW) * m_selectAnimPos;
    float largeH = sideH + (500.f - sideH) * m_selectAnimPos;
    float spacing = m_selectSpacing;

    auto drawPortrait = [&](int idx, float offsetX, float maxW, float maxH) {
        if (idx < 0 || idx >= n) return;
        const auto& cd = m_availableChars[idx];
        const auto& tex = cd.portraitLarge;
        if (tex.getSize().x == 0) return;
        float scale = std::min(maxW / tex.getSize().x, maxH / tex.getSize().y);
        float sprW = tex.getSize().x * scale;
        float sprH = tex.getSize().y * scale;
        float xPos = centerX + offsetX - sprW / 2;
        float yPos = centerY - sprH / 2;

        sf::Sprite spr(tex);
        spr.setScale({scale, scale});
        spr.setPosition({xPos, yPos});
        window_.draw(spr);

        m_bitmapFont.drawText(window_, cd.displayName, {centerX + offsetX, yPos + sprH + 40}, 28, sf::Color::White, {0.5f, 0.f});
    };

    int centerIdx = (m_selectPhase == 0) ? m_p1Choice : m_p2Choice;
    int leftIdx = (centerIdx - 1 + n) % n;
    int rightIdx = (centerIdx + 1) % n;

    drawPortrait(leftIdx, -spacing, sideW, sideH);
    drawPortrait(rightIdx, spacing, sideW, sideH);
    drawPortrait(centerIdx, 0, largeW, largeH);

    // 发光框 — 放大包裹当前角色 (预计算尺寸供箭头使用)
    float gScale = 1.f, glowW = 0.f, glowH = 0.f;
    float pad = 60.f;
    if (m_texCursorGlow.getSize().x > 0) {
        glowW = static_cast<float>(m_texCursorGlow.getSize().x);
        glowH = static_cast<float>(m_texCursorGlow.getSize().y);
        gScale = std::max((largeW + pad) / glowW, (largeH + pad) / glowH);
        sf::Sprite glow(m_texCursorGlow);
        glow.setScale({gScale, gScale});
        glow.setPosition({centerX - glowW * gScale / 2, centerY - glowH * gScale / 2});
        window_.draw(glow);
    }

    // P1/P2 标签 — 放在角色框正上方
    float tagScale = 2.f;
    float tagY = centerY - glowH * gScale / 2 - m_texP1Tag.getSize().y * tagScale - 8.f;
    if (m_texP1Tag.getSize().x > 0 && m_selectPhase == 0) {
        sf::Sprite tag(m_texP1Tag);
        tag.setScale({tagScale, tagScale});
        tag.setPosition({centerX - m_texP1Tag.getSize().x * tagScale / 2, tagY});
        window_.draw(tag);
    }
    if (m_texP2Tag.getSize().x > 0 && m_selectPhase >= 1) {
        sf::Sprite tag(m_texP2Tag);
        tag.setScale({tagScale, tagScale});
        tag.setPosition({centerX - m_texP2Tag.getSize().x * tagScale / 2, tagY});
        window_.draw(tag);
    }

    // 箭头 — 放在选中框两侧
    float frameHalfW = glowW * gScale / 2;
    if (m_texSelectArrowL.getSize().x > 0 && glowW > 0) {
        sf::Sprite arrow(m_texSelectArrowL);
        arrow.setPosition({centerX - frameHalfW - m_texSelectArrowL.getSize().x - 4, centerY - m_texSelectArrowL.getSize().y / 2});
        window_.draw(arrow);
    }
    if (m_texSelectArrowR.getSize().x > 0 && glowW > 0) {
        sf::Sprite arrow(m_texSelectArrowR);
        arrow.setPosition({centerX + frameHalfW + 4, centerY - m_texSelectArrowR.getSize().y / 2});
        window_.draw(arrow);
    }

    // 底部提示
    if (m_selectPhase == 0) {
        m_bitmapFont.drawText(window_, "PLAYER 1 - A/D SELECT  J CONFIRM", {960, 980}, 28, sf::Color{180,180,180}, {0.5f, 0.f});
    } else {
        m_bitmapFont.drawText(window_, "PLAYER 2 - LEFT/RIGHT SELECT  ENTER CONFIRM", {960, 980}, 28, sf::Color{180,180,180}, {0.5f, 0.f});
    }
    window_.display();
}

void Game::renderStageSelect() {
    window_.clear(sf::Color::Black);
    window_.setView(m_uiView);
    sf::Sprite bg(m_texStageBg);
    if (m_texStageBg.getSize().x > 0) {
        bg.setScale({1920.f / m_texStageBg.getSize().x, 1080.f / m_texStageBg.getSize().y});
        window_.draw(bg);
    }

    m_bitmapFont.drawText(window_, "SELECT STAGE", {960, 60}, 72, sf::Color::White, {0.5f, 0.f});

    int n = static_cast<int>(m_availableStages.size());
    float centerX = 960.f, centerY = 500.f;
    float sideW = 200.f, sideH = 130.f;
    float largeW = sideW + (560.f - sideW) * m_stageAnimPos;
    float largeH = sideH + (380.f - sideH) * m_stageAnimPos;
    float spacing = 420.f;
    float framePadding = 24.f;

    int centerIdx = m_stageChoice;
    int leftIdx = (centerIdx - 1 + n) % n;
    int rightIdx = (centerIdx + 1) % n;

    auto drawPreview = [&](int idx, float offsetX, float w, float h) {
        if (idx < 0 || idx >= n) return;
        const auto& tex = m_availableStages[idx].preview;
        if (tex.getSize().x == 0) return;
        float scale = std::min(w / tex.getSize().x, h / tex.getSize().y);
        float sw = tex.getSize().x * scale, sh = tex.getSize().y * scale;
        sf::Sprite spr(tex);
        spr.setScale({scale, scale});
        spr.setPosition({centerX + offsetX - sw / 2, centerY - sh / 2});
        window_.draw(spr);
        m_bitmapFont.drawText(window_, m_availableStages[idx].name, {centerX + offsetX, centerY + h / 2 + 10}, 24, sf::Color::White, {0.5f, 0.f});
    };

    drawPreview(leftIdx, -spacing, sideW, sideH);
    drawPreview(rightIdx, spacing, sideW, sideH);
    drawPreview(centerIdx, 0, largeW, largeH);

    // 两侧预览图的选择框
    auto drawFrame = [&](float offsetX, float pw, float ph) {
        if (m_texStageCursor.getSize().x == 0) return;
        sf::Sprite cursor(m_texStageCursor);
        float pad = 10.f;
        float cs = std::max((pw + pad) / m_texStageCursor.getSize().x,
                            (ph + pad) / m_texStageCursor.getSize().y);
        cursor.setScale({cs, cs});
        cursor.setPosition({centerX + offsetX - m_texStageCursor.getSize().x * cs / 2,
                           centerY - m_texStageCursor.getSize().y * cs / 2});
        window_.draw(cursor);
    };
    drawFrame(-spacing, sideW, sideH);
    drawFrame(spacing, sideW, sideH);

    // 中间选中框
    if (m_texStageCursor.getSize().x > 0) {
        sf::Sprite cursor(m_texStageCursor);
        float cursorScale = std::max((largeW + framePadding) / m_texStageCursor.getSize().x,
                                      (largeH + framePadding) / m_texStageCursor.getSize().y);
        cursor.setScale({cursorScale, cursorScale});
        float cw = m_texStageCursor.getSize().x * cursorScale;
        float ch = m_texStageCursor.getSize().y * cursorScale;
        cursor.setPosition({centerX - cw / 2, centerY - ch / 2});
        window_.draw(cursor);
    }

    // 箭头放在选中框两侧
    if (m_texStageArrowL.getSize().x > 0) {
        sf::Sprite arrow(m_texStageArrowL);
        float frameHalfW = (largeW + framePadding) / 2;
        arrow.setPosition({centerX - frameHalfW - m_texStageArrowL.getSize().x - 2, centerY - m_texStageArrowL.getSize().y / 2});
        window_.draw(arrow);
    }
    if (m_texStageArrowR.getSize().x > 0) {
        sf::Sprite arrow(m_texStageArrowR);
        float frameHalfW = (largeW + framePadding) / 2;
        arrow.setPosition({centerX + frameHalfW + 2, centerY - m_texStageArrowR.getSize().y / 2});
        window_.draw(arrow);
    }

    m_bitmapFont.drawText(window_, "LEFT/RIGHT SELECT  ENTER CONFIRM", {960, 980}, 28, sf::Color{180,180,180}, {0.5f, 0.f});
    window_.display();
}
} // namespace db
