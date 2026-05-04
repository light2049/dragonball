#include "Core/Game.h"
#include "Core/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>

namespace db {

    // Simple file logging
    static void logMsg(const char* msg) {
        FILE* f = fopen("C:\\Users\\Lenovo\\game_log.txt", "a");
        if (f) {
            fprintf(f, "%s", msg);
            fclose(f);
        }
        // Also try writing to a simple path
        FILE* f2 = fopen("C:\\game_log.txt", "a");
        if (f2) {
            fprintf(f2, "%s", msg);
            fclose(f2);
        }
    }

    Game::Game() : window_(sf::VideoMode({800, 600}), "DragonBall - Phase 6") {
        window_.setVerticalSyncEnabled(true);
        window_.setFramerateLimit(60);
        try {
            const std::string CHAR_NAME = "Vegito_Blue_Kaioken";
            const std::string BASE_PATH   = "Data/Characters/" + CHAR_NAME + "/";
            const std::string SPRITE_DIR  = BASE_PATH + "Sprites/";
            const std::string AIR_FILE    = BASE_PATH + CHAR_NAME + ".air";
            const std::string CNS_FILE    = BASE_PATH + CHAR_NAME + ".cns";
            const std::string CMS_FILE    = BASE_PATH + "common1.cns";
            const std::string CMD_FILE    = BASE_PATH + CHAR_NAME + ".cmd";

            logMsg("Game constructor start\n");

            m_player = std::make_unique<Fighter>();
            m_player->loadAnimations(AIR_FILE, SPRITE_DIR, CHAR_NAME);
            m_player->loadStats(CNS_FILE);
            m_player->loadCommonStates(CMS_FILE);
            m_player->loadCommands(CMD_FILE);
            // 初始化完成后正式进入状态 0，确保状态属性被正确应用
            m_player->requestStateChange(0);
            logMsg("Player loaded\n");

            m_dummy = std::make_unique<Fighter>();
            m_dummy->setPosition(600.0f, 480.0f);
            m_dummy->loadAnimations(AIR_FILE, SPRITE_DIR, CHAR_NAME);
            m_dummy->loadStats(CNS_FILE);
            m_dummy->loadCommonStates(CMS_FILE);
            m_dummy->loadCommands(CMD_FILE);
            m_dummy->requestStateChange(0);
            logMsg("Dummy loaded\n");

            m_hudP1 = std::make_unique<HUD>();
            m_hudP1->setPosition(20.0f, 20.0f);
            m_hudP1->update(m_player->getCurrentLife(), m_player->getMaxLife());
            m_hudP1->updatePower(m_player->getPower(), m_player->getMaxPower());

            m_hudP2 = std::make_unique<HUD>();
            m_hudP2->setPosition(480.0f, 20.0f);
            m_hudP2->update(m_dummy->getCurrentLife(), m_dummy->getMaxLife());
            m_hudP2->updatePower(m_dummy->getPower(), m_dummy->getMaxPower());

            // 加载调试字体
            if (m_debugFont.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
                m_debugText = std::make_unique<sf::Text>(m_debugFont);
                m_debugText->setCharacterSize(14);
                m_debugText->setFillColor(sf::Color(0, 0, 0));
                m_debugText->setPosition({10.f, 540.f});
                m_debugReady = true;
            }

            logMsg("Game ready\n");

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

    void Game::processEvents() {
        while (const auto event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window_.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) window_.close();
                if (key->code == sf::Keyboard::Key::F1) {
                    Fighter::setShowDebug(!Fighter::getShowDebug());
                }
                // Event-based key tracking: prevents quick taps from being missed
                inputManager_.onKeyPressed(key->code);
            }
        }
    }

    void Game::update(float dt) {
        // 1. HitStop 冻结逻辑
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
            int newState = m_player->getCurrentStateNo();
            if (oldState != newState) {
                std::string msg = "State: " + std::to_string(oldState) + " -> " + std::to_string(newState) + "\n";
                logMsg(msg.c_str());
            }
        }

        // 3. P2 更新
        if (m_dummy) {
            sf::Vector2f playerPos = m_player->getPosition();
            m_dummy->update(dt, inputManagerP2_, playerPos);
        }

        // 3.25 读取并创建 Helper (飞行道具)
        if (m_player) {
            auto pending = m_player->drainPendingHelpers();
            for (auto& ph : pending) {
                const Animation* anim = m_player->getAnimation(ph.animId);
                if (!anim) continue;
                HelperEntity he;
                he.id = ph.id;
                he.animPlayer.play(*anim);
                he.animPlayer.setFacingRight(ph.facingRight);
                he.position = ph.position;
                he.velocity = ph.velocity;
                m_helpers.push_back(std::move(he));
            }
        }
        if (m_dummy) {
            auto pending = m_dummy->drainPendingHelpers();
            for (auto& ph : pending) {
                const Animation* anim = m_dummy->getAnimation(ph.animId);
                if (!anim) continue;
                HelperEntity he;
                he.id = ph.id;
                he.animPlayer.play(*anim);
                he.animPlayer.setFacingRight(ph.facingRight);
                he.position = ph.position;
                he.velocity = ph.velocity;
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

        // 5. 物理碰撞 (推撞)
        handlePushCollision();

        // 6. 更新 Helper (飞行道具)
        for (auto& h : m_helpers) {
            h.position.x += h.velocity.x * 60.f * dt;
            h.position.y += h.velocity.y * 60.f * dt;
            h.animPlayer.update(dt);
            h.lifetime--;
            // 超出屏幕或生命期结束 → 移除
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
        if (!m_player->isAttacking()) return;
        if (m_player->hasMoveContact()) return;

        const auto& hitDefs = m_player->getCurrentHitDefs();
        if (hitDefs.empty()) {
            static int lastLoggedState = -1;
            int currentState = m_player->getCurrentStateNo();
            if (currentState != lastLoggedState) {
                std::string msg = "No HitDefs in state " + std::to_string(currentState) + "\n";
                logMsg(msg.c_str());
                lastLoggedState = currentState;
            }
            return;
        }

        const auto& hit = hitDefs[0];

        sf::FloatRect hitBox = m_player->getActiveHitbox();
        sf::FloatRect hurtBox = m_dummy->getActiveHurtbox();

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
        if (!intersection.has_value()) return;

        m_player->setMoveContact(true);

        // 计算命中接触点 (碰撞矩形中心 + sparkxy 偏移)
        sf::Vector2f contactPoint(
            intersection->position.x + intersection->size.x / 2 + static_cast<float>(hit.sparkX),
            intersection->position.y + intersection->size.y / 2 + static_cast<float>(hit.sparkY)
        );
        spawnSpark(hit.sparkno, contactPoint);

        bool isGuarding = m_dummy->isGuarding();
        if (isGuarding) {
            m_player->setMoveGuarded(true);
        } else {
            m_player->setMoveHit(true);
        }

        std::string hitMsg = "HIT! state=" + std::to_string(m_player->getCurrentStateNo()) + "\n";
        logMsg(hitMsg.c_str());

        HitInfo info;
        info.damage = hit.damage;
        info.guardDamage = hit.guardDamage;
        info.groundHittime = (hit.groundHittime > 0) ? hit.groundHittime : 15;
        info.groundSlidetime = (hit.groundSlidetime > 0) ? hit.groundSlidetime : 15;
        info.airHittime = (hit.airHittime > 0) ? hit.airHittime : 12;
        info.groundVelocityX = hit.groundVelocityX;
        info.airVelocityX = hit.airVelocityX;
        info.airVelocityY = hit.airVelocityY;
        info.airguardVelocityX = hit.airguardVelocityX;
        info.airguardVelocityY = hit.airguardVelocityY;
        info.animtype = hit.animtype.empty() ? "Light" : hit.animtype;
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

            float knockback = info.groundVelocityX * 60.f;
            if (m_player->isFacingRight()) {
                m_dummy->setVelocityX(knockback);
            } else {
                m_dummy->setVelocityX(-knockback);
            }

            int targetState = 5000;
            if (info.p2stateno > 0) {
                targetState = info.p2stateno;
            } else {
                std::string type = info.animtype;
                if (type == "Light" || type == "light") targetState = 5000;
                else if (type == "Medium" || type == "medium") targetState = 5001;
                else targetState = 5002;
            }
            m_dummy->requestStateChange(targetState);

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

        sf::FloatRect box1 = m_player->getPushBox();
        sf::FloatRect box2 = m_dummy->getPushBox();

        if (box1.findIntersection(box2).has_value()) {
            float center1 = box1.position.x + box1.size.x / 2;
            float center2 = box2.position.x + box2.size.x / 2;

            float overlap = (box1.size.x + box2.size.x) / 2 - std::abs(center2 - center1);
            if (overlap > 0) {
                float pushAmount = overlap / 2.0f + 1.0f;

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
        window_.setView(shakeView);

        window_.clear(sf::Color(200, 200, 200));

        if (m_player) m_player->draw(window_);
        if (m_dummy) m_dummy->draw(window_);

        // 绘制 Helper (飞行道具)
        for (const auto& h : m_helpers) {
            h.animPlayer.draw(window_, h.position);
        }

        // 绘制火花效果
        for (const auto& spark : m_sparks) {
            spark.animPlayer.draw(window_, spark.position);
        }

        // HUD 用默认视图 (不震动)
        window_.setView(window_.getDefaultView());
        if (m_hudP1) m_hudP1->draw(window_);
        if (m_hudP2) m_hudP2->draw(window_);

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

        window_.display();
    }

} // namespace db
