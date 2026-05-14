#include "Characters/Fighter.h"
#include "Core/InputManager.h"
#include "Utils/CnsParser.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    void logMsg(const char* msg) {
        FILE* f = fopen("D:\\DragonBall\\build\\fighter_log.txt", "a");
        if (f) {
            fprintf(f, "%s", msg);
            fclose(f);
        }
    }
}

namespace db {
    bool Fighter::m_showDebug = false;
    bool Fighter::m_showAnimDebug = false;

    Fighter::Fighter() : m_position(200.f, 480.f), m_velocity(0.f, 0.f), m_isGrounded(true) {
        m_currentStateNo = 0;
        m_previousStateNo = -1;
        m_stateTimer = 0.0f;
        m_stateType = 0;
        m_moveType = 0;
        m_physicsType = 0;
        m_isHitOver = true;
        m_isHitShakeOver = true;
        m_canRecover = false;
    }

    void Fighter::loadAnimations(const std::string& airPath, const std::string& basePath, const std::string& prefix) {
        m_animations = AirParser::parse(airPath, basePath, prefix, m_sffDb);
        std::cout << "[Fighter] Loaded " << m_animations.size() << " animations." << std::endl;
        switchAnimation(0);
    }

    void Fighter::loadStats(const std::string& cnsPath) {
        auto stats = CnsParser::loadStats(cnsPath);
        m_maxLife = stats.maxLife;
        m_currentLife = stats.maxLife;
        m_maxPower = stats.maxPower;
        m_currentPower = 0;

        m_pushBack = static_cast<float>(stats.groundBack);
        m_pushFront = static_cast<float>(stats.groundFront);
        m_pushHeight = static_cast<float>(stats.height);

        m_velData = stats.velocity;
        m_moveData = stats.movement;

        std::cout << "[Fighter] Stats Loaded -> Life: " << m_maxLife
                  << ", Gravity: " << (m_moveData.yaccel * 3600.f)
                  << ", JumpY: " << (m_velData.jumpNeuY * 60.f) << std::endl;

        m_stateRegistry.loadCNS(cnsPath);
    }

    void Fighter::loadCommonStates(const std::string& commonCnsPath) {
        m_stateRegistry.loadCNS(commonCnsPath);
    }

    void Fighter::loadCommands(const std::string& cmdPath) {
        m_cmdParser.load(cmdPath);

        m_stateRegistry.loadCNS(cmdPath);
    }

    void Fighter::takeDamage(int damage) {
        if (isDead()) return;
        m_currentLife -= damage;
        if (m_currentLife < 0) m_currentLife = 0;
        m_hitFlashTimer = 6;

        m_explods.erase(std::remove_if(m_explods.begin(), m_explods.end(),
            [](const ExplodInstance& e) { return e.removeOnGetHit; }), m_explods.end());
    }
    int Fighter::getCurrentLife() const { return m_currentLife; }
    int Fighter::getMaxLife() const { return m_maxLife; }
    bool Fighter::isDead() const { return m_currentLife <= 0; }

    float Fighter::getConstValue(const std::string& path) const {

        if (path == "velocity.walk.fwd.x")       return m_velData.walkFwd;
        if (path == "velocity.walk.back.x")      return m_velData.walkBack;
        if (path == "velocity.jump.y")           return m_velData.jumpNeuY;
        if (path == "velocity.jump.neu.x")       return m_velData.jumpNeuX;
        if (path == "velocity.jump.neu.y")       return m_velData.jumpNeuY;
        if (path == "velocity.jump.fwd.x")       return m_velData.jumpFwd;
        if (path == "velocity.jump.back.x")      return m_velData.jumpBack;
        if (path == "velocity.run.fwd.x")        return m_velData.runFwdX;
        if (path == "velocity.run.fwd.y")        return m_velData.runFwdY;
        if (path == "velocity.run.back.x")       return m_velData.runBackX;
        if (path == "velocity.run.back.y")       return m_velData.runBackY;
        if (path == "velocity.runjump.fwd.x")    return m_velData.runjumpFwd;
        if (path == "velocity.runjump.back.x")   return m_velData.runjumpBack;
        if (path == "velocity.airjump.neu.x")    return m_velData.airjumpNeuX;
        if (path == "velocity.airjump.neu.y")    return m_velData.airjumpNeuY;
        if (path == "velocity.airjump.fwd.x")    return m_velData.airjumpFwd;
        if (path == "velocity.airjump.back.x")   return m_velData.airjumpBack;
        if (path == "movement.yaccel")           return m_moveData.yaccel;
        if (path == "movement.stand.friction")   return m_moveData.standFriction;
        if (path == "movement.crouch.friction")  return m_moveData.crouchFriction;
        if (path == "movement.airjump.num")      return static_cast<float>(m_moveData.airjumpNum);
        if (path == "movement.airjump.height")   return static_cast<float>(m_moveData.airjumpHeight);
        if (path == "movement.stand.friction.threshold")  return 0.5f;
        if (path == "movement.air.gethit.groundlevel") {

            return GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
        }
        if (path == "movement.crouch.friction.threshold") return 0.5f;
        return 0.0f;
    }

    void Fighter::setVelocityX(float x) { m_velocity.x = x; }
    void Fighter::setVelocityY(float y) { m_velocity.y = y; }
    float Fighter::getVelocityX() const { return m_velocity.x; }
    float Fighter::getVelocityY() const { return m_velocity.y; }

    void Fighter::addPositionX(float x) { m_position.x += x; }
    void Fighter::addPositionY(float y) { m_position.y += y; }
    sf::Vector2f Fighter::getPosition() const { return m_position; }
    int Fighter::getCurrentAnimFrameIndex() const {
        return static_cast<int>(m_animationPlayer.getCurrentFrameIndex());
    }

    float Fighter::getFeetY() const {
        return m_position.y + static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
    }
    bool Fighter::isGrounded() const { return m_isGrounded; }
    bool Fighter::isFacingRight() const { return m_animationPlayer.isFacingRight(); }

    void Fighter::performJump() {
        m_velocity.y = m_velData.jumpNeuY * 60.f;
    }
    void Fighter::setFacingRight(bool right) { m_animationPlayer.setFacingRight(right); }

    bool Fighter::isAttacking() const { return m_isAttacking; }
    void Fighter::setAttacking(bool attacking) {
        m_isAttacking = attacking;
        if(attacking) m_hasHitCurrentAttack = false;
    }
    bool Fighter::hasHitThisAttack() const { return m_hasHitCurrentAttack; }
    void Fighter::markAttackAsHit() { m_hasHitCurrentAttack = true; }

    void Fighter::switchAnimation(int animId) {
        if (m_animations.contains(animId)) {
            m_animationPlayer.play(m_animations.at(animId));
        } else {

        }
    }

    void Fighter::requestStateChange(int stateNo) {

        bool isEngineState = (stateNo == 0 || stateNo == 10 || stateNo == 11 ||
                              stateNo == 12 || stateNo == 20 || stateNo == 40 ||
                              stateNo == 50 || stateNo == 52 ||
                              stateNo == 100 || stateNo == 105 || stateNo == 106);

        bool hasCnsDef = m_stateRegistry.hasState(stateNo);
        bool hasAnim = m_animations.contains(stateNo);

        if (!isEngineState && !hasCnsDef && !hasAnim) {

            return;
        }

        m_previousStateNo = m_currentStateNo;
        m_currentStateNo = stateNo;
        m_stateTimer = -0.008f;
        m_hasHitCurrentAttack = false;
        m_lastAnimElem = 0;

        if (m_showAnimDebug) {
            std::cout << "[StateChange] " << m_previousStateNo << " -> " << stateNo
                      << " anim=" << m_animationPlayer.getCurrentAnimId()
                      << " timer=0" << std::endl;
        }

        if (stateNo == 0) {
            clearExplods();
            m_shakeTime = 0;
            m_shakeAmpl = 0;
            m_afterImageActive = false;
            m_afterImageGhosts.clear();
            m_velocity.x = 0;
            m_velocity.y = 0;
        }

        int animBefore = m_animationPlayer.getCurrentAnimId();
        if (hasCnsDef) {
            m_stateRegistry.applyStateAttributes(stateNo, *this);
        } else {

            if (stateNo == 0) {

                setMoveType(0);
                setControl(true);
            } else if (stateNo > 0 && stateNo < 200) {

                setMoveType(0);
                setControl(true);
            } else if (stateNo >= 200 && stateNo < 1000) {

                setMoveType(1);
                setControl(false);
            } else if (stateNo >= 5000 && stateNo < 6000) {

                setMoveType(2);
                setControl(false);
            }
        }

        if (m_animationPlayer.getCurrentAnimId() == animBefore) {
            auto it = m_animations.find(stateNo);
            if (it != m_animations.end()) {
                switchAnimation(stateNo);
            }
        }

        m_hitConsumed = false;

        if (m_moveType == 0) {
            m_hasMoveContact = false;
            m_hasMoveHit = false;
            m_hasMoveGuarded = false;
            m_isAttacking = false;
        } else if (m_moveType == 1) {
            m_hasMoveContact = false;
            m_hasMoveHit = false;
            m_hasMoveGuarded = false;
            m_isAttacking = true;
        }

        float dx = m_opponentPos.x - m_position.x;
        if (dx > 0) m_animationPlayer.setFacingRight(true);
        else if (dx < 0) m_animationPlayer.setFacingRight(false);
    }

    const std::vector<HitDef>& Fighter::getCurrentHitDefs() const {
        const auto& cur = m_stateRegistry.getHitDefs(m_currentStateNo);
        if (!cur.empty()) return cur;

        return m_lastAttackHitDefs;
    }

    void Fighter::update(float dt, InputManager& inputMgr, const sf::Vector2f& opponentPos) {
        m_frameStartState = m_currentStateNo;
        if (isDead() && m_currentStateNo != 5150 && m_currentStateNo != 180 && m_currentStateNo != 5100) return;
        m_opponentPos = opponentPos;
        dt = std::min(dt, 0.05f);
        m_stateTimer += dt;

        if (m_currentStateNo == 600) {
            static int diagF = 0;
            if (++diagF % 60 == 1) {
                FILE* f = fopen("C:\\Users\\Lenovo\\game_log.txt", "a");
                if (f) {
                    fprintf(f, "[DiagFlags] hit=%d contact=%d guarded=%d\n",
                        m_hasMoveHit ? 1 : 0, m_hasMoveContact ? 1 : 0, m_hasMoveGuarded ? 1 : 0);
                    fclose(f);
                }
            }
        }

        m_prePhysicsPos = m_position;

        float gravity = 0.f;
        float friction = 1.f;
        bool applyGroundCollision = true;

        switch (m_physicsType) {
            case 0: gravity = m_moveData.yaccel * 3600.f; friction = m_moveData.standFriction; break;
            case 1: gravity = m_moveData.yaccel * 3600.f; friction = m_moveData.crouchFriction; break;
            case 2: gravity = m_moveData.yaccel * 3600.f; friction = 1.f; break;
            case 3: default: gravity = 0.f; friction = 1.f; applyGroundCollision = false; break;
        }

        m_velocity.y += gravity * dt;
        m_position += m_velocity * dt;

        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);

            if (m_stateType != 2) {
                m_velocity.y = 0;
            }
            if (!m_isGrounded && m_physicsType == 2) {
                requestStateChange(52);
            }
            m_isGrounded = true;
        } else if (applyGroundCollision) {
            m_isGrounded = false;
        } else {
            m_isGrounded = false;
        }

        if (!applyGroundCollision && friction == 1.f && m_isGrounded) {
            m_velocity.x *= 0.88f;
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }
        if (m_isGrounded && friction < 1.f) {
            m_velocity.x *= std::pow(friction, dt * 60.f);
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }

        if (m_currentStateNo == 600) {
            static int diagCounter600 = 0;
            if (++diagCounter600 % 120 == 1) {
                FILE* f = fopen("C:\\Users\\Lenovo\\game_log.txt", "a");
                if (f) {
                    fprintf(f, "[Diag600] pos=(%.0f,%.0f) vel=(%.0f,%.0f) dt=%.4f gounded=%d grav=%.0f\n",
                        m_position.x, m_position.y, m_velocity.x, m_velocity.y, dt, m_isGrounded ? 1 : 0, m_moveData.yaccel * 3600.f);
                    fclose(f);
                }
            }
        }

        m_drawOverrides = DrawOverrides();
        m_assertFlags = 0;
        if (m_palFXTime > 0) m_palFXTime--;

        if (m_hitFlashTimer > 0) {
            m_drawOverrides.hitFlash = static_cast<uint8_t>((m_hitFlashTimer * 255) / 6);
            m_hitFlashTimer--;
        }

        inputMgr.clearCommandResults();
        m_cmdParser.evaluate(inputMgr, isFacingRight());
        for (const auto& [name, def] : m_cmdParser.getCommands()) {
            inputMgr.setCommandResult(name, m_cmdParser.isActive(name));
        }

        int stateBeforeMinusOne = m_currentStateNo;
        m_stateRegistry.executeState(-1, *this, &inputMgr, dt);

        if (stateBeforeMinusOne != m_currentStateNo) {
            m_cmdParser.resetBuffers();
        }
        m_frameStartState = m_currentStateNo;

        bool inputActive = m_hasControl;
        float distToOpponent = std::abs(opponentPos.x - m_position.x);
        bool inGuardRange = isInGuardDist();

        DirInput rawDir = inputMgr.getDirection();
        bool fwd = (rawDir == DirInput::F || rawDir == DirInput::UF || rawDir == DirInput::DF);
        bool back = (rawDir == DirInput::B || rawDir == DirInput::UB || rawDir == DirInput::DB);
        bool up = (rawDir == DirInput::U || rawDir == DirInput::UF || rawDir == DirInput::UB);
        bool down = (rawDir == DirInput::D || rawDir == DirInput::DF || rawDir == DirInput::DB);
        bool charge = inputMgr.isHeld("hold_s");

        if (m_currentStateNo == 0 && inputActive) {

            if (up) { requestStateChange(40); }
            else if (down) { requestStateChange(10); }
            else if (fwd || back) {
                bool holdingBack = (isFacingRight() && back) || (!isFacingRight() && fwd);
                if (holdingBack && inGuardRange) {
                    requestStateChange(120);
                } else {

                    setFacingRight(fwd);
                    requestStateChange(20);
                }
            }
            if (charge && m_currentPower < m_maxPower) {
                requestStateChange(195);
            }
        }
        else if (m_currentStateNo == 20) {

            if (up) { requestStateChange(40); }
            else if (down) { requestStateChange(10); }
            else if (fwd || back) {
                float dirSign = isFacingRight() ? 1.f : -1.f;
                bool charFwd = (isFacingRight() && fwd) || (!isFacingRight() && back);
                if (charFwd) {
                    m_velocity.x = dirSign * m_velData.walkFwd * 60.f;
                    if (m_animationPlayer.getCurrentAnimId() != 20) switchAnimation(20);
                } else {
                    m_velocity.x = dirSign * m_velData.walkBack * 60.f;
                    if (m_animationPlayer.getCurrentAnimId() != 21) switchAnimation(21);
                }
            } else {
                requestStateChange(0);
            }
        }
        else if (m_currentStateNo == 40) {

            if (fwd) m_lastJumpDir = 1;
            else if (back) m_lastJumpDir = -1;
            else m_lastJumpDir = 0;

            if (m_stateTimer >= 0.05f) {

                bool wasRunning = (m_previousStateNo == 100 || m_previousStateNo == 106);
                if (wasRunning) {

                    float vx = (m_lastJumpDir >= 0) ? m_velData.runFwdX * 60.f : m_velData.runBackX * 60.f;
                    setVelocityX(vx);
                    setVelocityY(m_velData.runFwdY * 60.f);
                } else {

                    if (m_lastJumpDir == 1) m_velocity.x += m_velData.jumpFwd * 60.f;
                    else if (m_lastJumpDir == -1) m_velocity.x += m_velData.jumpBack * 60.f;

                    float maxVx = std::max(std::abs(m_velData.walkFwd), std::abs(m_velData.jumpFwd)) * 60.f;
                    if (std::abs(m_velocity.x) > maxVx) m_velocity.x = (m_velocity.x > 0 ? 1 : -1) * maxVx;
                    setVelocityY(m_velData.jumpNeuY * 60.f);
                }
                requestStateChange(50);
            }
        }
        else if (m_currentStateNo == 51 || m_currentStateNo == 50) {

            float airAccel = m_velData.walkFwd * 20.f;
            if (fwd) { m_velocity.x += airAccel * dt * 60.f; }
            if (back) { m_velocity.x -= airAccel * dt * 60.f; }

            float airMax = std::abs(m_velData.jumpFwd) * 60.f;
            if (m_velocity.x > airMax) m_velocity.x = airMax;
            if (m_velocity.x < -airMax) m_velocity.x = -airMax;
        }
        if (m_currentStateNo == 51) {

        }
        else if (m_currentStateNo == 50) {

            if (m_velocity.y >= 0.f && m_stateTimer > 0.05f) {
                requestStateChange(51);
            }

            if (m_velocity.y > -480.f) {
                int curAnim = m_animationPlayer.getCurrentAnimId();
                if (curAnim >= 41 && curAnim <= 43) {
                    int fallAnim = curAnim + 3;
                    if (m_animations.contains(fallAnim)) switchAnimation(fallAnim);
                }
            }
        }
        else if (m_currentStateNo == 52) {

            if (m_stateTimer > 0.1f) requestStateChange(0);
        }
        else if (m_currentStateNo == 11 && inputActive) {

            if (!down) { requestStateChange(12); }

            else if (back && inGuardRange) { requestStateChange(120); }
        }

        else if (m_currentStateNo == 100 && up) {
            requestStateChange(106);
        }

        else if (m_currentStateNo == 45) {

        }

        else if (m_currentStateNo == 106) {
            if (m_stateTimer >= 0.05f) requestStateChange(50);
        }

        else if (m_currentStateNo == 132) {
            if (!back) {
                requestStateChange(140);
            } else if (m_isGrounded) {
                requestStateChange(52);
            }
        }
        else if (m_currentStateNo == 10 && inputActive) {

            if (!down) { requestStateChange(0); }
        }

        else if (m_currentStateNo == 120) {
            if (!back) {
                requestStateChange(140);
            } else if (m_animationPlayer.hasJustLooped()) {
                if (m_stateType == 2) {
                    requestStateChange(132);
                } else if (down) {
                    requestStateChange(131);
                } else {
                    requestStateChange(130);
                }
                m_animationPlayer.clearLoopFlag();
            }
        }

        else if (m_currentStateNo == 131) {
            if (!down) { requestStateChange(130); }
        }

        if (m_currentStateNo == 5110) {
            int lieTicks = static_cast<int>(m_stateTimer * 60.f);
            int getupTime = std::max(m_hitInfo.groundHittime, m_hitInfo.groundSlidetime) + 20;
            if (lieTicks >= getupTime) {
                requestStateChange(5120);
            }
        }

        if (m_currentStateNo == 5150 && m_animationPlayer.hasJustLooped()) {
            if (m_stateRegistry.hasState(170)) requestStateChange(170);
            m_animationPlayer.clearLoopFlag();
        }

        m_animationPlayer.tickAnimTime();

        if (m_currentStateNo != 20) {
            m_stateRegistry.executeState(m_currentStateNo, *this, &inputMgr, dt);
        }

        if (m_currentStateNo == 195 || m_currentStateNo == 196 || m_currentStateNo == 0) {
            static int lastDbgState = -1;
            if (m_currentStateNo != lastDbgState) {
                lastDbgState = m_currentStateNo;
                bool hs = inputMgr.isHeld("hold_s");
                bool ctrl = hasControl();
            }
        }

        m_stateRegistry.executeState(-2, *this, &inputMgr, dt);

        m_stateRegistry.executeState(-3, *this, &inputMgr, dt);

        {
            const auto& hd = m_stateRegistry.getHitDefs(m_currentStateNo);
            if (!hd.empty()) m_lastAttackHitDefs = hd;
        }

        if (m_moveType == 2) {
            int stateTicks = static_cast<int>(m_stateTimer * 60.f);
            if (stateTicks >= m_hitInfo.groundHittime) {
                m_isHitOver = true;
            }

            if (!m_isHitShakeOver && stateTicks >= m_hitInfo.groundHittime) {
                m_isHitShakeOver = true;
            }
        }

        if (m_currentStateNo != 0 && !m_stateRegistry.hasState(m_currentStateNo)) {
            bool shouldReturn = false;
            if (m_animations.contains(m_currentStateNo)) {

                if (m_animationPlayer.hasJustLooped()) {
                    m_animationPlayer.clearLoopFlag();
                    shouldReturn = true;
                }

                int stateTicks = static_cast<int>(m_stateTimer * 60.f);
                if (stateTicks > 60) {
                    shouldReturn = true;
                }
            } else {

                shouldReturn = true;
            }
            if (shouldReturn) {
                requestStateChange(0);
            }
        }

        if (m_currentStateNo != 0 && m_stateRegistry.hasState(m_currentStateNo)) {
            int stateTicks = static_cast<int>(m_stateTimer * 60.f);
            if (stateTicks > 600) {
                requestStateChange(0);
            }
        }

        m_hasMoveHit = false;
        m_hasMoveContact = false;

        m_animationPlayer.update(dt);

        {
            int curElem = m_animationPlayer.getCurrentAnimElem();
            if (curElem != m_lastAnimElem) {
                m_lastAnimElem = curElem;
                m_hitConsumed = false;
            }
        }

        if (m_showAnimDebug && m_currentStateNo != 0) {
            static int lastState = -1, lastAnim = -1, lastElem = -1;
            int curState = m_currentStateNo;
            int curAnim = m_animationPlayer.getCurrentAnimId();
            int curElem = m_animationPlayer.getCurrentAnimElem();
            int totalFrames = m_animationPlayer.getTotalFrames();
            const auto& frame = m_animationPlayer.getCurrentFrame();
            bool hasClsn1 = !frame.clsn1.empty();
            bool texOk = m_animationPlayer.getSpriteSize().x > 0;

            if (curState != lastState || curAnim != lastAnim || curElem != lastElem) {
                lastState = curState; lastAnim = curAnim; lastElem = curElem;
                int stateTicks = static_cast<int>(m_stateTimer * 60.f);
                std::cout << "[AnimTrace] state=" << curState
                          << " anim=" << curAnim
                          << " elem=" << curElem << "/" << totalFrames
                          << " tick=" << stateTicks
                          << " vel=(" << static_cast<int>(m_velocity.x) << "," << static_cast<int>(m_velocity.y) << ")"
                          << " hitdef=" << (hasClsn1 ? 1 : 0)
                          << " " << (texOk ? "OK" : "MISS")
                          << " tex=" << frame.texturePath
                          << std::endl;
            }
        }

        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
        }

        sf::Vector2f fighterPos = m_position;
        for (auto& explod : m_explods) {
            explod.animPlayer.update(dt);

            if (explod.bindtime != 0) {
                if (explod.bindtime > 0) explod.bindtime--;

                float dir = isFacingRight() ? 1.f : -1.f;
                explod.currentPos = {
                    fighterPos.x + explod.pos.x * dir,
                    fighterPos.y + explod.pos.y
                };

                bool parentFacing = isFacingRight();
                explod.animPlayer.setFacingRight(explod.facing == 1 ? parentFacing : !parentFacing);
            } else {

                explod.currentPos.x += explod.vel.x;
                explod.currentPos.y += explod.vel.y;
            }

            if (explod.removetime == -2) {

                if (explod.animPlayer.hasJustLooped()) {
                    explod.done = true;
                }
            } else if (explod.removetime > 0) {
                explod.removetime--;
                if (explod.removetime == 0) explod.done = true;
            }
        }
        m_explods.erase(std::remove_if(m_explods.begin(), m_explods.end(),
            [](const ExplodInstance& e) { return e.done; }), m_explods.end());

        if (m_shakeTime > 0) {
            m_shakeTime--;
            if (m_shakeTime <= 0) m_shakeAmpl = 0;
        }

        if (m_afterImageActive) {
            m_afterImageTime--;
            if (m_afterImageTime <= 0) {
                m_afterImageActive = false;
                m_afterImageGhosts.clear();
            } else {
                m_afterImageCaptureTimer++;
                if (m_afterImageCaptureTimer >= m_afterImageTimegap) {
                    m_afterImageCaptureTimer = 0;

                    AfterImageGhost ghost;
                    auto cloned = m_animationPlayer.cloneSprite();
                    if (cloned.has_value()) {
                        ghost.sprite = std::make_unique<sf::Sprite>(std::move(cloned.value()));
                    }
                    ghost.position = m_position;

                    const AnimFrame& frame = m_animationPlayer.getCurrentFrame();
                    ghost.frameOffset = sf::Vector2f{
                        static_cast<float>(frame.offset.x),
                        static_cast<float>(frame.offset.y)
                    };
                    if (ghost.sprite) {
                        ghost.spriteHeight = ghost.sprite->getLocalBounds().size.y;
                    }

                    m_afterImageGhosts.push_back(std::move(ghost));
                    while (static_cast<int>(m_afterImageGhosts.size()) > m_afterImageLength) {
                        m_afterImageGhosts.erase(m_afterImageGhosts.begin());
                    }
                }
            }
        }
    }

    void Fighter::update(float dt, const SimpleInputState& input, const sf::Vector2f& opponentPos) {
        if (isDead() && m_currentStateNo != 5150 && m_currentStateNo != 180 && m_currentStateNo != 5100) return;
        m_opponentPos = opponentPos;
        dt = std::min(dt, 0.05f);
        m_stateTimer += dt;

        float gravity = 0.f;
        float friction = 1.f;
        bool applyGroundCollision = true;

        switch (m_physicsType) {
            case 0: gravity = m_moveData.yaccel * 3600.f; friction = m_moveData.standFriction; break;
            case 1: gravity = m_moveData.yaccel * 3600.f; friction = m_moveData.crouchFriction; break;
            case 2: gravity = m_moveData.yaccel * 3600.f; friction = 1.f; break;
            case 3: default: gravity = 0.f; friction = 1.f; applyGroundCollision = false; break;
        }

        m_velocity.y += gravity * dt;
        m_position += m_velocity * dt;

        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);

            if (m_stateType != 2) {
                m_velocity.y = 0;
            }
            if (!m_isGrounded && m_physicsType == 2) {
                requestStateChange(52);
            }
            m_isGrounded = true;
        } else if (applyGroundCollision) {
            m_isGrounded = false;
        } else {
            m_isGrounded = false;
        }

        if (!applyGroundCollision && friction == 1.f && m_isGrounded) {
            m_velocity.x *= 0.88f;
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }
        if (m_isGrounded && friction < 1.f) {
            m_velocity.x *= std::pow(friction, dt * 60.f);
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }

        if (m_moveType == 2) {
            if (static_cast<int>(m_stateTimer * 60.f) >= m_hitInfo.groundHittime) {
                m_isHitOver = true;
            }
        }

        m_animationPlayer.update(dt);
    }

    void Fighter::draw(sf::RenderWindow& window) const {

        if (m_afterImageActive && !m_afterImageGhosts.empty()) {
            int count = static_cast<int>(m_afterImageGhosts.size());
            for (int i = 0; i < count; i++) {
                const auto& ghost = m_afterImageGhosts[i];
                if (!ghost.sprite) continue;
                uint8_t alpha = static_cast<uint8_t>(80 * (i + 1) / count);

                sf::Sprite ghostSprite = *ghost.sprite;
                sf::Color c = ghostSprite.getColor();
                c.a = alpha;
                ghostSprite.setColor(c);

                float finalX = ghost.position.x + ghost.frameOffset.x;
                float finalY = ghost.position.y - ghost.spriteHeight + ghost.frameOffset.y;
                ghostSprite.setPosition({finalX, finalY});
                window.draw(ghostSprite);
            }
        }

        std::vector<const ExplodInstance*> sortedExplods;
        if (!m_explods.empty()) {
            sortedExplods.reserve(m_explods.size());
            for (const auto& e : m_explods) sortedExplods.push_back(&e);
            std::sort(sortedExplods.begin(), sortedExplods.end(),
                [](const ExplodInstance* a, const ExplodInstance* b) {
                    return a->sprpriority < b->sprpriority;
                });
        }

        for (const auto* e : sortedExplods) {
            e->animPlayer.draw(window, e->currentPos);
        }

        if (!(m_assertFlags & 1)) {
            m_animationPlayer.draw(window, m_position, &m_drawOverrides);
        }

    }

    void Fighter::drawDebug(sf::RenderWindow& window) const {
        if (!m_showDebug) return;
        {
            const AnimFrame& currentFrame = m_animationPlayer.getCurrentFrame();

            sf::Vector2f frameOffset{
                static_cast<float>(currentFrame.offset.x),
                static_cast<float>(currentFrame.offset.y)
            };

            sf::Vector2f spriteSize = m_animationPlayer.getSpriteSize();
            float spriteHeight = spriteSize.y;

            float anchorX = m_position.x + frameOffset.x;
            float anchorY = m_position.y - spriteHeight + frameOffset.y;

            static bool s_posLogged = false;
            if (!s_posLogged && m_currentStateNo == 0) {
                s_posLogged = true;
                std::cout << "[DEBUG] Frame offset: (" << frameOffset.x << ", " << frameOffset.y << ")" << std::endl;
                std::cout << "[DEBUG] m_position: (" << m_position.x << ", " << m_position.y << ")" << std::endl;
                std::cout << "[DEBUG] anchor (sprite TL): (" << anchorX << ", " << anchorY << ")" << std::endl;
                std::cout << "[DEBUG] spriteSize: (" << spriteSize.x << ", " << spriteSize.y << ")" << std::endl;
                if (!currentFrame.clsn2.empty()) {
                    std::cout << "[DEBUG] Clsn2[0]: (" << currentFrame.clsn2[0].topLeft.x << "," << currentFrame.clsn2[0].topLeft.y
                              << ") to (" << currentFrame.clsn2[0].bottomRight.x << "," << currentFrame.clsn2[0].bottomRight.y << ")" << std::endl;
                }
            }

            auto drawRect = [&](const ClsnRect& rect, sf::Color color) {

                float x1, x2;

                if (isFacingRight()) {
                    x1 = m_position.x + rect.topLeft.x;
                    x2 = m_position.x + rect.bottomRight.x;
                } else {
                    x1 = m_position.x - rect.bottomRight.x;
                    x2 = m_position.x - rect.topLeft.x;
                }

                float y1 = m_position.y + rect.topLeft.y;
                float y2 = m_position.y + rect.bottomRight.y;

                float left = std::min(x1, x2);
                float top = std::min(y1, y2);
                float width = std::abs(x2 - x1);
                float height = std::abs(y2 - y1);

                sf::RectangleShape shape({width, height});
                shape.setPosition({left, top});
                sf::Color fillColor = color;
                fillColor.a = 60;
                shape.setFillColor(fillColor);
                shape.setOutlineThickness(1.5f);
                shape.setOutlineColor(color);
                window.draw(shape);
            };

            for (const auto& rect : currentFrame.clsn1) {
                drawRect(rect, sf::Color::Red);
            }
            for (const auto& rect : currentFrame.clsn2) {
                drawRect(rect, sf::Color::Green);
            }

            sf::FloatRect pushBox = getPushBox();
            sf::RectangleShape pushShape({pushBox.size.x, pushBox.size.y});
            pushShape.setPosition({pushBox.position.x, pushBox.position.y});
            sf::Color blue(0, 100, 255, 50);
            pushShape.setFillColor(blue);
            pushShape.setOutlineColor(sf::Color::Blue);
            pushShape.setOutlineThickness(1.0f);
            window.draw(pushShape);
        }
    }

    int Fighter::getHitVar(const std::string& name) const {
        std::string n = name;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);

        if (n == "damage")          return m_hitInfo.damage;
        if (n == "guarddamage")     return m_hitInfo.guardDamage;
        if (n == "hittime" || n == "ground.hittime")  return m_hitInfo.groundHittime;
        if (n == "air.hittime")     return m_hitInfo.airHittime;
        if (n == "slidetime" || n == "ground.slidetime") return m_hitInfo.groundSlidetime;
        if (n == "yvel")            return static_cast<int>(m_hitInfo.airVelocityY);
        if (n == "xvel")            return static_cast<int>(m_hitInfo.airVelocityX);
        if (n == "fall")            return m_hitInfo.fall ? 1 : 0;
        if (n == "fall.recover")    return m_hitInfo.fallrecover ? 1 : 0;
        if (n == "fall.yvel")       return static_cast<int>(m_hitInfo.fallyvel);
        if (n == "yaccel")          return m_hitInfo.yaccel > 0 ? m_hitInfo.yaccel : static_cast<int>(m_moveData.yaccel * 60.f);
        if (n == "ctrltime")        return m_hitInfo.ctrltime > 0 ? m_hitInfo.ctrltime : m_hitInfo.groundHittime + 1;
        if (n == "animtype") {
            std::string t = m_hitInfo.animtype;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            if (t == "light")    return 0;
            if (t == "medium")   return 1;
            if (t == "hard")     return 2;
            if (t == "back")     return 3;
            if (t == "up")       return 4;
            if (t == "diagup")   return 5;
            return 0;
        }
        if (n == "groundtype") {
            std::string t = m_hitInfo.groundType;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            if (t == "low")    return 2;
            if (t == "trip")   return 3;
            return 1;
        }
        if (n == "airtype") {
            std::string t = m_hitInfo.airType;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            if (t == "low")    return 2;
            if (t == "trip")   return 3;
            return 1;
        }
        if (n == "juggle")          return m_hitInfo.juggle;
        if (n == "hitid")           return m_hitInfo.hitid;
        if (n == "envshaketime" || n == "envshake.time") return m_hitInfo.envshakeTime;

        return 0;
    }

    void Fighter::setMoveType(int type) {
        m_moveType = type;
        if (type == 2) {
            m_isHitOver = false;
            m_isHitShakeOver = false;
        }
    }

    static sf::Vector2f LocalToWorld(const sf::Vector2f& localPos, const sf::Vector2f& position, const sf::Vector2f& offset, bool facingRight) {
        float x = position.x + offset.x;
        float y = position.y + offset.y;

        if (facingRight) {
            return {x + localPos.x, y + localPos.y};
        } else {
            return {x - localPos.x, y + localPos.y};
        }
    }

    sf::FloatRect Fighter::getActiveHitbox() const {
        const auto& frame = m_animationPlayer.getCurrentFrame();
        if (frame.clsn1.empty()) return {};

        constexpr sf::Vector2f zeroOffset{0.f, 0.f};
        bool first = true;
        float minX = 0, minY = 0, maxX = 0, maxY = 0;
        for (const auto& clsn : frame.clsn1) {
            sf::Vector2f p1 = LocalToWorld({static_cast<float>(clsn.topLeft.x), static_cast<float>(clsn.topLeft.y)}, m_position, zeroOffset, isFacingRight());
            sf::Vector2f p2 = LocalToWorld({static_cast<float>(clsn.bottomRight.x), static_cast<float>(clsn.bottomRight.y)}, m_position, zeroOffset, isFacingRight());
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
        return sf::FloatRect{{minX, minY}, {maxX - minX, maxY - minY}};
    }

    sf::FloatRect Fighter::getActiveHurtbox() const {
        const auto& frame = m_animationPlayer.getCurrentFrame();
        if (frame.clsn2.empty()) return {};

        constexpr sf::Vector2f zeroOffset{0.f, 0.f};
        bool first = true;
        float minX = 0, minY = 0, maxX = 0, maxY = 0;
        for (const auto& clsn : frame.clsn2) {
            sf::Vector2f p1 = LocalToWorld({static_cast<float>(clsn.topLeft.x), static_cast<float>(clsn.topLeft.y)}, m_position, zeroOffset, isFacingRight());
            sf::Vector2f p2 = LocalToWorld({static_cast<float>(clsn.bottomRight.x), static_cast<float>(clsn.bottomRight.y)}, m_position, zeroOffset, isFacingRight());
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
        return sf::FloatRect{{minX, minY}, {maxX - minX, maxY - minY}};
    }

    sf::FloatRect Fighter::getPushBox() const {

        float height = m_pushHeight;
        float y = getFeetY() - height;

        float left, right;

        if (m_animationPlayer.isFacingRight()) {

            left = m_position.x - m_pushBack;
            right = m_position.x + m_pushFront;
        } else {

            left = m_position.x - m_pushFront;
            right = m_position.x + m_pushBack;
        }

        return {{left, y}, {right - left, height}};
    }

    void Fighter::setPosition(float x, float y) {
        m_position = sf::Vector2f(x, y);
    }

    void Fighter::resetLife() { m_currentLife = m_maxLife; }

    void Fighter::setSuperPause(int time, bool darken) {
        m_superPauseTime = time;
        m_superPauseDarken = darken;
    }

    bool Fighter::isInGuardDist() const {
        float distX = std::abs(m_opponentPos.x - m_position.x);
        float guardRange = m_pushFront + m_pushBack + 80.f;
        return distX <= guardRange;
    }

    bool Fighter::isGuarding() const {

        return (m_currentStateNo >= 120 && m_currentStateNo <= 155);
    }

    void Fighter::addPower(int amount) {
        m_currentPower += amount;
        if (m_currentPower > m_maxPower) {
            m_currentPower = m_maxPower;
        }
    }

    void Fighter::addExplod(ExplodInstance explod) {

        if (explod.id != 0) {
            m_explods.erase(std::remove_if(m_explods.begin(), m_explods.end(),
                [id = explod.id](const ExplodInstance& e) { return e.id == id; }), m_explods.end());
        }
        m_explods.push_back(std::move(explod));
    }

    void Fighter::removeExplod(int id) {
        m_explods.erase(std::remove_if(m_explods.begin(), m_explods.end(),
            [id](const ExplodInstance& e) { return e.id == id; }), m_explods.end());
    }

    void Fighter::clearExplods() {
        m_explods.clear();
    }

    bool Fighter::getStateDefAttributes(int stateNo, int& outAnim, float& outVelX, float& outVelY) const {
        const auto& states = m_stateRegistry.getStates();
        auto it = states.find(stateNo);
        if (it == states.end()) return false;
        outAnim = (it->second.anim >= 0) ? it->second.anim : stateNo;
        outVelX = it->second.velsetX;
        outVelY = it->second.velsetY;
        return true;
    }

    void Fighter::setAfterImage(int time, int timegap, int length) {
        m_afterImageActive = true;
        m_afterImageTime = time;
        m_afterImageTimegap = timegap;
        m_afterImageLength = length;
        m_afterImageCaptureTimer = 0;
        m_afterImageGhosts.clear();
    }

    void Fighter::addPendingHelper(const PendingHelper& ph) {
        m_pendingHelpers.push_back(ph);
    }

    std::vector<Fighter::PendingHelper> Fighter::drainPendingHelpers() {
        std::vector<PendingHelper> result;
        result.swap(m_pendingHelpers);
        return result;
    }
}