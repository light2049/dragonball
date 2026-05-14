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

            m_bitmapFont.load("Data/UI/fonts/flame_blocky_font/All_Characters");
            m_useBitmapFont = m_bitmapFont.hasLoaded();

            if (m_debugFont.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
                m_debugText = std::make_unique<sf::Text>(m_debugFont);
                m_debugText->setCharacterSize(14);
                m_debugText->setFillColor(sf::Color(255, 255, 200));
                m_debugText->setPosition({10.f, 540.f});
                m_debugReady = true;
            }

            m_availableChars = discoverCharacters();
            m_availableStages = discoverStages();

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

                        auto pos = line.find("displayname");
                        if (pos == std::string::npos) pos = line.find("displayname");
                        if (pos != std::string::npos) {
                            auto eq = line.find('=');
                            if (eq != std::string::npos) {
                                displayName = line.substr(eq + 1);

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

                (void)sd.background.loadFromFile(bgPath);
                stages.push_back(std::move(sd));
                std::cout << "[Stage] Found: " << name << std::endl;
            }
        } catch (...) {}
        if (stages.empty()) {

            StageDef sd;
            sd.name = "Arena";
            sd.dirPath = "";
            stages.push_back(std::move(sd));
        }
        return stages;
    }

    void Game::loadUITextures() {
        auto loadTex = [](sf::Texture& t, const std::string& path) {
            (void)t.loadFromFile(path);
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

        for (auto& cd : m_availableChars) {
            std::string airPath = "Data/Characters/" + cd.dirName + "/" + cd.dirName + ".air";
            std::ifstream airFile(airPath);
            if (!airFile.is_open()) continue;

            std::string line;
            bool found = false;
            while (std::getline(airFile, line)) {
                auto p = line.find("[Begin Action 0]");
                if (p != std::string::npos) { found = true; break; }
            }
            if (!found) continue;

            while (std::getline(airFile, line)) {

                if (line.find("Clsn") != std::string::npos || line.find("clsn") != std::string::npos) continue;

                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                if (trimmed.empty() || trimmed[0] == ';') continue;

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
                    (void)cd.portraitLarge.loadFromImage(img);
                    (void)cd.portraitSmall.loadFromImage(img);
                }
                break;
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
            inputManagerP2_.update();
            update(dt);
            render();

            frameCount++;
            if (frameCount % 60 == 0) {
                std::string msg = "Frame " + std::to_string(frameCount) + ", state=" + std::to_string(m_player ? m_player->getCurrentStateNo() : -1) + "\n";
                logMsg(msg.c_str());
            }
        }
    }

    void Game::updateViews(const sf::Vector2u& /*winSize*/) {
        // UI view: fixed 1920x1080 logical coordinate space
        sf::View uiView(sf::FloatRect({0, 0}, {1920, 1080}));
        uiView.setViewport(sf::FloatRect({0, 0}, {1, 1}));
        m_uiView = uiView;

        // Game view: fixed 960x540 logical — SFML auto-scales to fill the window
        // This ensures fullscreen mode scales up the original content proportionally
        m_gameView = sf::View(sf::FloatRect({0, 0}, {960, 540}));
        m_gameView.setViewport(sf::FloatRect({0, 0}, {1, 1}));

        m_windowSize = {960, 540};
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

                auto routeKey = [&](sf::Keyboard::Key k) {
                    if (k == sf::Keyboard::Key::Numpad1 || k == sf::Keyboard::Key::Numpad2 ||
                        k == sf::Keyboard::Key::Numpad3 || k == sf::Keyboard::Key::Numpad4 ||
                        k == sf::Keyboard::Key::Numpad5 || k == sf::Keyboard::Key::Numpad6 ||
                        k == sf::Keyboard::Key::Numpad0 ||
                        k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::Down ||
                        k == sf::Keyboard::Key::Left || k == sf::Keyboard::Key::Right) {
                        inputManagerP2_.onKeyPressed(k);
                    } else if (k == sf::Keyboard::Key::Enter) {
                        inputManager_.onKeyPressed(k);
                        inputManagerP2_.onKeyPressed(k);
                    } else {
                        inputManager_.onKeyPressed(k);
                    }
                };
                routeKey(key->code);
            }
            if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
                auto routeKey = [&](sf::Keyboard::Key k) {
                    if (k == sf::Keyboard::Key::Numpad1 || k == sf::Keyboard::Key::Numpad2 ||
                        k == sf::Keyboard::Key::Numpad3 || k == sf::Keyboard::Key::Numpad4 ||
                        k == sf::Keyboard::Key::Numpad5 || k == sf::Keyboard::Key::Numpad6 ||
                        k == sf::Keyboard::Key::Numpad0 ||
                        k == sf::Keyboard::Key::Up || k == sf::Keyboard::Key::Down ||
                        k == sf::Keyboard::Key::Left || k == sf::Keyboard::Key::Right) {
                        inputManagerP2_.onKeyReleased(k);
                    } else if (k == sf::Keyboard::Key::Enter) {
                        inputManager_.onKeyReleased(k);
                        inputManagerP2_.onKeyReleased(k);
                    } else {
                        inputManager_.onKeyReleased(k);
                    }
                };
                routeKey(key->code);
            }
        }
    }

    void Game::update(float dt) {
        if (m_gameState == GameState::TITLE) { updateTitle(dt); return; }
        if (m_gameState == GameState::SELECT) { updateSelect(dt); return; }
        if (m_gameState == GameState::STAGE_SELECT) { updateStageSelect(dt); return; }

        if (m_gameState == GameState::INTRO) {

            if (m_player && m_dummy &&
                m_player->getCurrentStateNo() == 0 &&
                m_dummy->getCurrentStateNo() == 0) {
                m_gameState = GameState::FIGHT;
                m_roundTimer = 0.f;
                std::cout << "[FIGHT] Round " << m_roundNumber << std::endl;
            }

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

        if (m_hitStopTimer > 0.0f) {
            m_hitStopTimer -= dt;
            return;
        }

        // Fighter updates — run BEFORE superpause check so animation advances during pause
        if (m_player) {
            sf::Vector2f dummyPos = m_dummy ? m_dummy->getPosition() : sf::Vector2f(-9999.f, 0.f);
            m_player->update(dt, inputManager_, dummyPos);
        }

        if (m_dummy) {
            sf::Vector2f playerPos = m_player ? m_player->getPosition() : sf::Vector2f(-9999.f, 0.f);
            m_dummy->update(dt, inputManagerP2_, playerPos);
        }

        // SuperPause — return early after fighter updates so only combat/post is skipped
        if (m_player && m_player->isInSuperPause()) {
            m_superPauseTimer = m_player->getSuperPauseTime();
            m_superPauseDarken = m_player->getSuperPauseDarken();
            m_player->tickSuperPause();
        }
        if (m_superPauseTimer > 0) {
            m_superPauseTimer--;

            return;
        }
        if (m_superPauseTimer == 0) m_superPauseDarken = false;

        if (m_hitStopTimer > 0.0f) {
            m_hitStopTimer -= dt;
            return;
        }

        if (inputManager_.justPressed('x')) m_debugJpxLatch = 15;
        else if (m_debugJpxLatch > 0) m_debugJpxLatch--;

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

            if (m_player && m_koTimer > 0.f && m_koTimer < 2.0f) {
                m_player->getAnimationPlayer().update(dt);
                m_dummy->getAnimationPlayer().update(dt);
            }
            if (m_koTimer <= 0.f) {

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

        if (m_player) {
            auto pending = m_player->drainPendingHelpers();
            for (auto& ph : pending) {

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
                he.stateNo = ph.stateNo;
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

        if (m_hudP1 && m_player) {
            m_hudP1->update(m_player->getCurrentLife(), m_player->getMaxLife());
            m_hudP1->updatePower(m_player->getPower(), m_player->getMaxPower());
        }
        if (m_hudP2 && m_dummy) {
            m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
            m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());
        }

        checkCombat();

        if (m_player && m_player->isAttacking()) {
            m_player->executeCurrentStateCNS(nullptr, dt);
        }

        handlePushCollision();
        if (m_player) clampFighterToStage(*m_player);
        if (m_dummy) clampFighterToStage(*m_dummy);

        for (auto& h : m_helpers) {
            h.animPlayer.update(dt);
            h.stateTime += dt;
            bool isFirstUpdate = h.firstUpdate;
            h.firstUpdate = false;
            h.drawOverrides = DrawOverrides();

            h.position.x += h.velocity.x * 60.f * dt;
            h.position.y += h.velocity.y * 60.f * dt;

            if (const auto* stateDef = h.stateRegistry ? h.stateRegistry->getStateDef(h.stateNo) : nullptr) {
                int ticks = static_cast<int>(h.stateTime * 60.f);
                for (const auto& ctrl : stateDef->controllers) {

                    if (ctrl->type == ControllerType::VELSET) {
                        const auto* vc = dynamic_cast<const VelSetController*>(ctrl.get());
                        if (vc && vc->hasX && h.stateTime < 0.02f) {
                            float dir = h.facingRight ? 1.f : -1.f;
                            h.velocity.x = dir * vc->valueX;
                        }
                    }

                    if (!ctrl->checkTriggers(h.parent ? *h.parent : *(m_player.get()), nullptr, ticks)) continue;

                    switch (ctrl->type) {
                        case ControllerType::VELSET: {
                            const auto* vc = dynamic_cast<const VelSetController*>(ctrl.get());
                            if (vc && !vc->hasX && !vc->hasY) break;

                            if (vc->hasX && h.stateTime < 0.02f) {
                                float dir = h.facingRight ? 1.f : -1.f;
                                h.velocity.x = dir * vc->valueX;
                            }
                            break;
                        }
                        case ControllerType::POSADD: {
                            const auto* pc = dynamic_cast<const PosAddController*>(ctrl.get());
                            if (pc) {

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

                            if (h.hitCooldown <= 0 && m_player && m_dummy) {
                                const auto& hitDefs = h.stateRegistry->getHitDefs(h.stateNo);
                                if (!hitDefs.empty()) {
                                    for (const auto& hd : hitDefs) {
                                        Fighter* target = (h.parent == m_player.get()) ? m_dummy.get() : m_player.get();
                                        sf::FloatRect hurt = target->getActiveHurtbox();
                                        if (hurt.size.x > 0) {

                                            sf::FloatRect hb = {{0, 0}, {0, 0}};
                                            const auto& frame = h.animPlayer.getCurrentFrame();
                                            if (!frame.clsn1.empty()) {

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
                                if (tc->m_transType == "add" || tc->m_transType == "add1" || tc->m_transType == "addalpha")
                                    h.drawOverrides.useAdditiveBlend = true;
                            }
                            break;
                        }
                        default: break;
                    }
                }
            }

            h.lifetime--;
            if (h.hitCooldown > 0) h.hitCooldown--;
            else h.hasHit = false;
            if (h.lifetime <= 0 || h.position.x < -200.f || h.position.x > 1000.f) {
                h.done = true;
            }
        }
        m_helpers.erase(std::remove_if(m_helpers.begin(), m_helpers.end(),
            [](const HelperEntity& h) { return h.done; }), m_helpers.end());

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

            if (inputManager_.isKeyJustPressed(sf::Keyboard::Key::J) ||
                inputManager_.isKeyJustPressed(sf::Keyboard::Key::Enter)) {
                m_selectPhase = 1;

                for (int i = 0; i < n; i++) {
                    if (i != m_p1Choice) { m_p2Choice = i; break; }
                }
                m_selectAnimPos = 0.f;
                std::cout << "[Select] P1 chose " << m_availableChars[m_p1Choice].displayName << std::endl;
            }
        }

        else if (m_selectPhase == 1) {
            int prev = m_p2Choice;
            if (inputManagerP2_.isKeyJustPressed(sf::Keyboard::Key::Left) ||
                inputManagerP2_.isKeyJustPressed(sf::Keyboard::Key::Right)) {
                if (inputManagerP2_.isKeyJustPressed(sf::Keyboard::Key::Left)) {
                    do {
                        m_p2Choice = (m_p2Choice - 1 + n) % n;
                    } while (m_p2Choice == m_p1Choice);
                } else {
                    do {
                        m_p2Choice = (m_p2Choice + 1) % n;
                    } while (m_p2Choice == m_p1Choice);
                }
            }
            if (m_p2Choice != prev) m_selectAnimPos = 0.f;
            else m_selectAnimPos = std::min(m_selectAnimPos + dt * 8.f, 1.f);

            if (inputManagerP2_.isKeyJustPressed(sf::Keyboard::Key::Numpad0) ||
                inputManagerP2_.isKeyJustPressed(sf::Keyboard::Key::Enter)) {
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
        const auto& hitDefs = m_player->getCurrentHitDefs();
        if (hitDefs.empty()) {
            return;
        }

        const auto& hit = hitDefs[0];

        sf::FloatRect hitBox = m_player->getActiveHitbox();
        sf::FloatRect hurtBox = m_dummy->getActiveHurtbox();

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

        sf::Vector2f contactPoint(
            intersection->position.x + intersection->size.x / 2 + static_cast<float>(hit.sparkX),
            intersection->position.y + intersection->size.y / 2 + static_cast<float>(hit.sparkY)
        );
        spawnSpark(hit.sparkno, contactPoint);

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
            if (info.p2stateno > 0 && m_dummy->hasState(info.p2stateno)) {
                targetState = info.p2stateno;
            } else {
                std::string type = info.animtype;
                if (type == "Light" || type == "light") targetState = 5000;
                else if (type == "Medium" || type == "medium") targetState = 5001;
                else if (type == "Hard" || type == "hard") targetState = 5002;
                else targetState = 5000;
                if (!m_dummy->hasState(targetState)) targetState = 5000;
            }
            m_dummy->requestStateChange(targetState);

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

        if (m_gameState == GameState::TITLE) { renderTitle(); return; }
        if (m_gameState == GameState::SELECT) { renderSelect(); return; }
        if (m_gameState == GameState::STAGE_SELECT) { renderStageSelect(); return; }

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

        if (m_stageBg.getSize().x > 0 && (m_gameState == GameState::FIGHT || m_gameState == GameState::INTRO || m_gameState == GameState::KO)) {
            sf::Sprite stageSpr(m_stageBg);
            float sx = static_cast<float>(m_windowSize.x) / m_stageBg.getSize().x;
            float sy = static_cast<float>(m_windowSize.y) / m_stageBg.getSize().y;
            stageSpr.setScale({sx, sy});
            window_.draw(stageSpr);
        }

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

        if (m_debugReady) {
            if (m_player) m_player->drawDebug(window_);
            if (m_dummy) m_dummy->drawDebug(window_);
        }

        window_.setView(m_gameView);
        if (m_hudP2) m_hudP2->setPosition(static_cast<float>(m_windowSize.x) - 10.0f, 10.0f);
        if (m_hudP1) m_hudP1->draw(window_);
        if (m_hudP2) m_hudP2->draw(window_);

        if (m_gameState == GameState::FIGHT) {
            int remaining = std::max(0, 99 - static_cast<int>(m_roundTimer));
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", remaining);
            m_bitmapFont.drawText(window_, buf, {m_windowSize.x / 2.f + 2.f, 30}, 28, sf::Color::White, {0.5f, 0.5f});
        }

        if (m_player && (m_gameState == GameState::INTRO || m_gameState == GameState::FIGHT)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "ROUND %d", m_roundNumber);
            m_bitmapFont.drawText(window_, buf, {m_windowSize.x / 2.f, 58}, 24, sf::Color::White, {0.5f, 0.f});
        }

        if (m_debugReady && m_player && Fighter::getShowDebug()) {
            int stateNo = m_player->getCurrentStateNo();
            int animId = m_player->getCurrentAnimId();
            int animElem = m_player->getCurrentAnimElem();
            bool ctrl = m_player->hasControl();
            int stateType = m_player->getStateType();
            int moveType = m_player->getMoveType();
            int stateTime = m_player->getStateTime();

            char buf[256];

            snprintf(buf, sizeof(buf),
                "State:%d Anim:%d Elem:%d Time:%d | Type:%d Move:%d | Ctrl:%d",
                stateNo, animId, animElem, stateTime, stateType, moveType, ctrl);
            m_debugText->setString(buf);
            window_.draw(*m_debugText);

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

        if (m_superPauseDarken) {
            sf::RectangleShape dark({static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y)});
            dark.setFillColor({0, 0, 0, 120});
            window_.draw(dark);
        }

        if (m_player && m_dummy) {

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
}
