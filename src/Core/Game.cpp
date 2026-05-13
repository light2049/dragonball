#include "Core/Game.h"
#include "Core/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>

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

    Game::Game() : window_(sf::VideoMode({800, 600}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close) {
        window_.setVerticalSyncEnabled(true);
        window_.setFramerateLimit(60);
        try {
            m_availableChars = discoverCharacters();

            // 加载调试字体
            if (m_debugFont.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
                m_debugText = std::make_unique<sf::Text>(m_debugFont);
                m_debugText->setCharacterSize(14);
                m_debugText->setFillColor(sf::Color(0, 0, 0));
                m_debugText->setPosition({10.f, 540.f});
                m_debugReady = true;
            }

            logMsg("Game ready (SELECT screen)\n");

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

    void Game::initFight(int p1Choice, int p2Choice) {
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

        loadChar(m_availableChars[p1Choice].dirName, m_player, 200.f);
        loadChar(m_availableChars[p2Choice].dirName, m_dummy, 600.f);

        m_hudP1 = std::make_unique<HUD>();
        m_hudP1->setPosition(20.0f, 20.0f);
        m_hudP1->update(m_player->getCurrentLife(), m_player->getMaxLife());
        m_hudP1->updatePower(m_player->getPower(), m_player->getMaxPower());

        m_hudP2 = std::make_unique<HUD>();
        m_hudP2->setPosition(1600.0f, 20.0f);
        m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
        m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());

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
            inputManager_.clearJustPressedLatch();  // Clear previous frame's latch
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

        // UI View (1920x1080) — 保持比例填满窗口
        float uiScaleW = winW / 1920.f;
        float uiScaleH = winH / 1080.f;
        float uiScale = std::min(uiScaleW, uiScaleH);
        sf::View uiView(sf::FloatRect({0, 0}, {1920, 1080}));
        uiView.setViewport(sf::FloatRect({{(1.f - uiScale / uiScaleW) / 2.f, (1.f - uiScale / uiScaleH) / 2.f}, {uiScale / uiScaleW, uiScale / uiScaleH}}));
        m_uiView = uiView;

        // Game View (800x600) — letterbox
        float gameScaleW = winW / 800.f;
        float gameScaleH = winH / 600.f;
        float gameScale = std::min(gameScaleW, gameScaleH);
        sf::View gameView(sf::FloatRect({0, 0}, {800, 600}));
        gameView.setViewport(sf::FloatRect({{(1.f - gameScale / gameScaleW) / 2.f, (1.f - gameScale / gameScaleH) / 2.f}, {gameScale / gameScaleW, gameScale / gameScaleH}}));
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
                        window_.create(sf::VideoMode({800, 600}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close);
                        window_.setVerticalSyncEnabled(true);
                        window_.setFramerateLimit(60);
                        m_fullscreen = false;
                        updateViews({800, 600});
                    } else {
                        window_.close();
                    }
                }
                if (key->code == sf::Keyboard::Key::F11) {
                    if (m_fullscreen) {
                        window_.create(sf::VideoMode({800, 600}), "DragonBall - Phase 6", sf::Style::Resize | sf::Style::Close);
                        window_.setVerticalSyncEnabled(true);
                        window_.setFramerateLimit(60);
                        m_fullscreen = false;
                        updateViews({800, 600});
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
        }
    }

    void Game::update(float dt) {
        // 0. 选人界面
        if (m_gameState == GameState::SELECT) {
            int cols = 2;
            int rows = (static_cast<int>(m_availableChars.size()) + cols - 1) / cols;

            // P1 控制 (WASD + J 确认)
            if (m_selectPhase == 0) {
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::W)) {
                    m_p1Choice = (m_p1Choice - cols + static_cast<int>(m_availableChars.size())) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::S)) {
                    m_p1Choice = (m_p1Choice + cols) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::A)) {
                    m_p1Choice = (m_p1Choice - 1 + static_cast<int>(m_availableChars.size())) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::D)) {
                    m_p1Choice = (m_p1Choice + 1) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J)) {
                    m_selectPhase = 1;
                    m_p2Choice = (m_p1Choice + 1) % m_availableChars.size();
                    std::cout << "[Select] P1 chose " << m_availableChars[m_p1Choice].displayName << std::endl;
                }
            }

            // P2 控制 (IJKL + Enter 确认)
            if (m_selectPhase >= 1) {
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::I)) {
                    m_p2Choice = (m_p2Choice - cols + static_cast<int>(m_availableChars.size())) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::K)) {
                    m_p2Choice = (m_p2Choice + cols) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J)) {
                    m_p2Choice = (m_p2Choice - 1 + static_cast<int>(m_availableChars.size())) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::L)) {
                    m_p2Choice = (m_p2Choice + 1) % m_availableChars.size();
                }
                if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::Enter)) {
                    m_selectPhase = 2;
                    std::cout << "[Select] P2 chose " << m_availableChars[m_p2Choice].displayName << std::endl;
                    initFight(m_p1Choice, m_p2Choice);
                }
            }
            return;
        }

        // 0. 开场动画 (检测进入循环后切到 191 → 0 → FIGHT)
        if (m_gameState == GameState::INTRO) {
            // 由 CNS State 190 自己控制: ChangeAnim(RoundState=1 时不冻结) → AnimTime=0 → ChangeState 0
            if (m_player && m_dummy &&
                m_player->getCurrentStateNo() == 0 &&
                m_dummy->getCurrentStateNo() == 0) {
                m_gameState = GameState::FIGHT;
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
        if (m_gameState == GameState::FIGHT && m_roundTimer > 0.f) {
            m_roundTimer -= dt;
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
        const float STAGE_RIGHT = 800.f;  // 匹配窗口宽度
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
        // 画面震动 (EnvShake)
        sf::View shakeView = window_.getDefaultView();
        int totalShakeAmpl = 0;
        if (m_player && m_player->getShakeAmpl() > 0) totalShakeAmpl = m_player->getShakeAmpl();
        if (m_dummy && m_dummy->getShakeAmpl() > totalShakeAmpl) totalShakeAmpl = m_dummy->getShakeAmpl();
        if (totalShakeAmpl > 0) {
            float offsetX = static_cast<float>((std::rand() % (totalShakeAmpl * 2 + 1)) - totalShakeAmpl);
            float offsetY = static_cast<float>((std::rand() % (totalShakeAmpl * 2 + 1)) - totalShakeAmpl);
            shakeView.move({offsetX, offsetY});
        }
        // M-eM-+M-^EM-eM-^BM-^BM-hM-^UM-^LM-gM-^MM-^AM-gM-^]M-^M M-gM-^@M-^AM-eM-^HM-6M-fM-^@M-^AM-hM-^PM-^AM-hM-^LM-^B (800x600 letterbox)
        if (m_gameState != GameState::SELECT) {
            window_.setView(m_gameView);
        } else {
            window_.setView(m_uiView);
        }
        window_.clear(sf::Color(30, 30, 50));

        // M-iM-^@M-^IM-dM-:M-:M-gM-^UM-^LM-iM-^]M-"
        if (m_gameState == GameState::SELECT) {
            window_.setView(m_uiView);
            // 背景
            sf::RectangleShape bg({1920, 1080});
            bg.setFillColor(sf::Color(20, 20, 40));
            window_.draw(bg);

            // 标题
            sf::Text title(m_debugFont);
            title.setString("SELECT YOUR FIGHTER");
            title.setCharacterSize(48);
            title.setFillColor(sf::Color::White);
            title.setPosition({960, 60});
            auto tb = title.getLocalBounds();
            title.setOrigin({tb.size.x / 2, 0});
            window_.draw(title);

            // 角色网格
            int n = static_cast<int>(m_availableChars.size());
            int cols = 2;
            int rows = (n + cols - 1) / cols;
            float cellW = 350.f, cellH = 200.f;
            float gridW = cols * cellW, gridH = rows * cellH;
            float startX = (1920 - gridW) / 2 + cellW / 2;
            float startY = (1080 - gridH) / 2 + cellH / 2;

            for (int i = 0; i < n; i++) {
                int row = i / cols, col = i % cols;
                float cx = startX + col * cellW;
                float cy = startY + row * cellH;

                sf::RectangleShape card({cellW - 20, cellH - 20});
                card.setOrigin({(cellW - 20) / 2, (cellH - 20) / 2});
                card.setPosition({cx, cy});

                // 选中高亮
                bool p1Sel = (i == m_p1Choice);
                bool p2Sel = (i == m_p2Choice && m_selectPhase >= 1);
                if (p1Sel && p2Sel) {
                    card.setFillColor(sf::Color(180, 80, 80));
                    card.setOutlineThickness(4);
                    card.setOutlineColor(sf::Color::White);
                } else if (p1Sel) {
                    card.setFillColor(sf::Color(60, 60, 180));
                    card.setOutlineThickness(3);
                    card.setOutlineColor(sf::Color(100, 150, 255));
                } else if (p2Sel) {
                    card.setFillColor(sf::Color(180, 60, 60));
                    card.setOutlineThickness(3);
                    card.setOutlineColor(sf::Color(255, 150, 100));
                } else {
                    card.setFillColor(sf::Color(50, 50, 80));
                    card.setOutlineThickness(1);
                    card.setOutlineColor(sf::Color(100, 100, 120));
                }
                window_.draw(card);

                // 角色名
                sf::Text charName(m_debugFont);
                charName.setString(m_availableChars[i].displayName);
                charName.setCharacterSize(28);
                charName.setFillColor(sf::Color::White);
                charName.setPosition({cx, cy + 10});
                auto nb = charName.getLocalBounds();
                charName.setOrigin({nb.size.x / 2, 0});
                window_.draw(charName);
            }

            // 提示文字
            sf::Text hint(m_debugFont);
            if (m_selectPhase == 0) {
                hint.setString("P1: WASD move, J confirm");
            } else {
                hint.setString("P2: IJKL move, Enter confirm");
            }
            hint.setCharacterSize(20);
            hint.setFillColor(sf::Color(180, 180, 200));
            hint.setPosition({960, 900});
            tb = hint.getLocalBounds();
            hint.setOrigin({tb.size.x / 2, 0});
            window_.draw(hint);

            // 选择状态
            sf::Text status(m_debugFont);
            status.setCharacterSize(20);
            status.setPosition({960, 950});
            if (m_selectPhase == 0) {
                status.setString("Waiting for P1...");
                status.setFillColor(sf::Color(100, 150, 255));
            } else if (m_selectPhase == 1) {
                status.setString("P1: " + m_availableChars[m_p1Choice].displayName + " | Waiting for P2...");
                status.setFillColor(sf::Color(255, 150, 100));
            }
            tb = status.getLocalBounds();
            status.setOrigin({tb.size.x / 2, 0});
            window_.draw(status);

            window_.display();
            return;
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

        // HUD 用默认视图 (不震动)
        window_.setView(window_.getDefaultView());
        if (m_hudP1) m_hudP1->draw(window_);
        if (m_hudP2) m_hudP2->draw(window_);

        // 回合/比赛信息
        if (m_player) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Round %d", m_roundNumber);
            sf::Text roundText(m_debugFont);
            roundText.setString(buf);
            roundText.setCharacterSize(24);
            roundText.setFillColor(sf::Color::White);
            sf::FloatRect tb2 = roundText.getLocalBounds();
            roundText.setOrigin({tb2.position.x + tb2.size.x/2.f, tb2.position.y + tb2.size.y/2.f});
            roundText.setPosition({400.f, 60.f});
            window_.draw(roundText);
        }

        // 调试信息：显示当前状态、动画ID、命令状态
        if (m_debugReady && m_player) {
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
            sf::RectangleShape dark({800.f, 600.f});
            dark.setFillColor({0, 0, 0, 120});  // 半透明黑
            window_.draw(dark);
        }

        if (m_player && m_dummy) {
            // KO 文字
            if (m_gameState == GameState::KO) {
                sf::Text koText(m_debugFont);
                koText.setString("K.O.!");
                koText.setCharacterSize(60);
                koText.setFillColor(sf::Color::Red);
                sf::FloatRect tb = koText.getLocalBounds();
                koText.setOrigin({tb.position.x + tb.size.x/2.f, tb.position.y + tb.size.y/2.f});
                koText.setPosition({400.f, 200.f});
                window_.draw(koText);
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
        float p1Y = 480.f - static_cast<float>(m_player->getAnimationPlayer().getCurrentFrame().offset.y);
        float p2Y = 480.f - static_cast<float>(m_dummy->getAnimationPlayer().getCurrentFrame().offset.y);
        m_player->setPosition(200.f, p1Y);
        m_dummy->setPosition(600.f, p2Y);
        m_helpers.clear();
        m_sparks.clear();
        m_hitStopTimer = 0.f;
        m_superPauseTimer = 0;
        m_superPauseDarken = false;
    }

} // namespace db
