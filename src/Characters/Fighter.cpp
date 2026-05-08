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
    bool Fighter::m_showDebug = true;

    Fighter::Fighter() : m_position(200.f, 480.f), m_velocity(0.f, 0.f), m_isGrounded(true) {
        m_currentStateNo = 0;
        m_previousStateNo = -1;
        m_stateTimer = 0.0f;
        m_stateType = 0; // S = Standing
        m_moveType = 0;  // I = Idle
        m_physicsType = 0; // S = Standing
        m_isHitOver = true;
        m_isHitShakeOver = true;
        m_canRecover = false;
    }

    void Fighter::loadAnimations(const std::string& airPath, const std::string& basePath, const std::string& prefix) {
        m_animations = AirParser::parse(airPath, basePath, prefix);
        std::cout << "[Fighter] Loaded " << m_animations.size() << " animations." << std::endl;
        switchAnimation(0);
    }

    void Fighter::loadStats(const std::string& cnsPath) {
        auto stats = CnsParser::loadStats(cnsPath);
        m_maxLife = stats.maxLife;
        m_currentLife = stats.maxLife;
        m_maxPower = stats.maxPower;     // ✅ 使用解析出的 Power
        m_currentPower = 0;

        m_pushBack = static_cast<float>(stats.groundBack);
        m_pushFront = static_cast<float>(stats.groundFront);
        m_pushHeight = static_cast<float>(stats.height);

        // ✅ 核心修改：加载速度和运动数据
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
        // CMD 文件中包含 [Statedef -1] 块, 需要加载到 StateRegistry
        m_stateRegistry.loadCNS(cmdPath);
    }

    void Fighter::takeDamage(int damage) {
        if (isDead()) return;
        m_currentLife -= damage;
        if (m_currentLife < 0) m_currentLife = 0;
        m_hitFlashTimer = 6;  // 闪白持续 6 帧
        // 移除被击中时应清除的特效
        m_explods.erase(std::remove_if(m_explods.begin(), m_explods.end(),
            [](const ExplodInstance& e) { return e.removeOnGetHit; }), m_explods.end());
    }
    int Fighter::getCurrentLife() const { return m_currentLife; }
    int Fighter::getMaxLife() const { return m_maxLife; }
    bool Fighter::isDead() const { return m_currentLife <= 0; }

    float Fighter::getConstValue(const std::string& path) const {
        // 解析 const(velocity.xxx) 或 const(movement.xxx) 路径
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
        if (path == "movement.stand.friction.threshold")  return 0.5f;  // M.U.G.E.N 默认阈值
        if (path == "movement.air.gethit.groundlevel") {
            // 根据当前动画帧偏移量计算地面高度, 使 Pos Y >= groundlevel 等价于触及地面
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

    // ✅ 修改：跳跃使用 CNS 配置 (像素/tick * 60 = 像素/秒)
    // 注意：CNS 中 jump.neu.y = -8.4，负值代表向上，符合 SFML 坐标系
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
            //std::string msg = "[Fighter] switchAnimation: " + std::to_string(animId) + " NOT FOUND!\n";
            //logMsg(msg.c_str());
        }
    }

    void Fighter::requestStateChange(int stateNo) {
        // 允许引擎级过渡状态 (无条件跳转)
        bool isEngineState = (stateNo == 0 || stateNo == 10 || stateNo == 11 ||
                              stateNo == 12 || stateNo == 20 || stateNo == 40 ||
                              stateNo == 50 || stateNo == 52 ||
                              stateNo == 100 || stateNo == 105 || stateNo == 106);

        bool hasCnsDef = m_stateRegistry.hasState(stateNo);
        bool hasAnim = m_animations.contains(stateNo);

        if (!isEngineState && !hasCnsDef && !hasAnim) {
            //std::string msg = "[Fighter] State " + std::to_string(stateNo) + " has neither CNS def nor animation!\n";
            //logMsg(msg.c_str());
            return;
        }

        // DEBUG: log whether we have CNS def for this state (禁用: 日志过多)
        //{
        //    std::string msg = "[Fighter] requestStateChange(" + std::to_string(stateNo)
        //        + "): hasCnsDef=" + (hasCnsDef ? "1" : "0")
        //        + " hasAnim=" + (hasAnim ? "1" : "0") + "\n";
        //    logMsg(msg.c_str());
        //}
        //
        //{
        //    std::string msg = "[Fighter] State: " + std::to_string(m_currentStateNo) + " -> " + std::to_string(stateNo) + "\n";
        //    logMsg(msg.c_str());
        //}
        m_previousStateNo = m_currentStateNo;
        m_currentStateNo = stateNo;
        m_stateTimer = 0.0f;
        m_hasHitCurrentAttack = false;

        // 回到待机时清理所有特效
        if (stateNo == 0) {
            clearExplods();
            m_shakeTime = 0;
            m_shakeAmpl = 0;
            m_afterImageActive = false;
            m_afterImageGhosts.clear();
        }

        // 应用 CNS StateDef 属性 (type, physics, ctrl, velset, anim, poweradd, movetype)
        int animBefore = m_animationPlayer.getCurrentAnimId();
        if (hasCnsDef) {
            m_stateRegistry.applyStateAttributes(stateNo, *this);
        } else {
            // 无 CNS 定义时设置合理的默认属性
            if (stateNo == 0) {
                // 待机: Idle 移动类型, 开放控制权
                setMoveType(0); // I = Idle
                setControl(true);
            } else if (stateNo > 0 && stateNo < 200) {
                // 基础状态 (跳跃、落地等): Idle
                setMoveType(0);
                setControl(true);
            } else if (stateNo >= 200 && stateNo < 1000) {
                // 攻击类状态: 设为攻击移动类型, 关闭控制权
                setMoveType(1); // A = Attack
                setControl(false);
            } else if (stateNo >= 5000 && stateNo < 6000) {
                // 受击类状态
                setMoveType(2); // H = Hit
                setControl(false);
            }
        }

        // 兜底：仅在 applyStateAttributes 未切换动画时，用状态号查找动画
        if (m_animationPlayer.getCurrentAnimId() == animBefore) {
            auto it = m_animations.find(stateNo);
            if (it != m_animations.end()) {
                switchAnimation(stateNo);
            }
        }

        // Debug: log ctrl state after transition (禁用: 日志过多)
        //{
        //    std::string msg = "[Fighter] requestStateChange to " + std::to_string(stateNo)
        //        + " done. ctrl=" + std::to_string(m_hasControl)
        //        + " moveType=" + std::to_string(m_moveType)
        //        + " stateType=" + std::to_string(m_stateType)
        //        + " anim=" + std::to_string(m_animationPlayer.getCurrentAnimId()) + "\n";
        //    logMsg(msg.c_str());
        //}

        // 引擎级重置: 进入待机/攻击状态时清除命中标志
        if (m_moveType == 0) { // I = Idle
            m_hasMoveContact = false;
            m_hasMoveHit = false;
            m_hasMoveGuarded = false;
            m_isAttacking = false;
        } else if (m_moveType == 1) { // A = Attack
            m_hasMoveContact = false;
            m_hasMoveHit = false;
            m_hasMoveGuarded = false;
            m_isAttacking = true;
        }
    }

    const std::vector<HitDef>& Fighter::getCurrentHitDefs() const {
        const auto& cur = m_stateRegistry.getHitDefs(m_currentStateNo);
        if (!cur.empty()) return cur;
        // 状态切换到 idle 后，保留上帧攻击状态的 HitDefs
        return m_lastAttackHitDefs;
    }

    // 主更新路径: CNS 驱动, 引擎级基本移动保持硬编码
    void Fighter::update(float dt, InputManager& inputMgr, const sf::Vector2f& opponentPos) {
        m_frameStartState = m_currentStateNo; // 帧开始时的状态号
        if (isDead() && m_currentStateNo != 5150 && m_currentStateNo != 180 && m_currentStateNo != 5100) return;
        m_opponentPos = opponentPos;
        dt = std::min(dt, 0.05f);
        m_stateTimer += dt;

        // Diag: print movehit/movecontact flags at start of every frame in state 600
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

        // ==========================================
        // 1. 物理更新 (基于 statetype)
        // ==========================================
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

        // 地面碰撞: 触地时复位 (所有类型)
        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
            // 空中类型不归零 Y 速度 (CNS 用 Vel Y > 0 检测落地)
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

        // physics = N 时接地后施加阻力防止无限滑行
        if (!applyGroundCollision && friction == 1.f && m_isGrounded) {
            m_velocity.x *= 0.88f;
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }
        if (m_isGrounded && friction < 1.f) {
            m_velocity.x *= std::pow(friction, dt * 60.f);
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }

        // Diag: log state 600 pos/vel every ~2s (only in main update path)
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

        // 1.5 重置每帧绘制覆盖和 AssertSpecial 标志
        m_drawOverrides = DrawOverrides();
        m_assertFlags = 0;
        if (m_palFXTime > 0) m_palFXTime--;

        // 1.6 受击闪白计时
        if (m_hitFlashTimer > 0) {
            m_drawOverrides.hitFlash = static_cast<uint8_t>((m_hitFlashTimer * 255) / 6);
            m_hitFlashTimer--;
        }

        // ==========================================
        // 2. CMD 指令评估
        // ==========================================
        inputMgr.clearCommandResults();  // 清空上一帧缓存，确保 evaluate 读取实时按键
        m_cmdParser.evaluate(inputMgr);
        for (const auto& [name, def] : m_cmdParser.getCommands()) {
            inputMgr.setCommandResult(name, m_cmdParser.isActive(name));
        }

        // ==========================================
        // 3. 执行 StateDef -1 (CMD → 状态跳转)
        // ==========================================
        m_stateRegistry.executeState(-1, *this, &inputMgr, dt);
        m_frameStartState = m_currentStateNo; // 在 -1 执行后重新捕获（此时已是目标状态）

        // ==========================================
        // 4. 引擎级基本移动逻辑 (状态 0/20/40/50/52)
        // ==========================================
        bool inputActive = m_hasControl;
        float distToOpponent = std::abs(opponentPos.x - m_position.x);
        bool inGuardRange = isInGuardDist();

        // M.U.G.E.N 标准方向输入: holdfwd/holdback 是屏幕绝对方向
        bool fwd = inputMgr.isHeld("holdfwd");
        bool back = inputMgr.isHeld("holdback");
        bool up = inputMgr.isHeld("holdup");
        bool down = inputMgr.isHeld("holddown");
        bool charge = inputMgr.isHeld("hold_s");

        if (m_currentStateNo == 0 && inputActive) {
            // 待机 → 跳跃/行走/蹲下/防御/蓄气
            if (up) { requestStateChange(40); }
            else if (down) { requestStateChange(10); }
            else if (fwd || back) {
                bool holdingBack = (isFacingRight() && back) || (!isFacingRight() && fwd);
                if (holdingBack && inGuardRange) {
                    requestStateChange(120);
                } else {
                    // 行走:CNS StateDef 20 处理速度, 这里只设 facing
                    setFacingRight(fwd);
                    requestStateChange(20);
                }
            }
            if (charge && m_currentPower < m_maxPower) {
                requestStateChange(195);
            }
        }
        else if (m_currentStateNo == 20) {
            // 行走: 可跳跃/蹲下
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
            // 跳跃起跳: 记录方向, 动画播完后赋予速度
            if (fwd) m_lastJumpDir = 1;
            else if (back) m_lastJumpDir = -1;
            else m_lastJumpDir = 0;

            if (m_stateTimer >= 0.05f) {
                // 起跳: 保留水平速度 + 跳到空中的基础速度
                // 行走中跳跃 = 保留行走速度 + 跳前方向速度
                // 跑动中跳跃 = 使用跑跳速度(runjump)
                bool wasRunning = (m_previousStateNo == 100 || m_previousStateNo == 106);
                if (wasRunning) {
                    // 跑动跳跃: 使用 runjump 速度
                    float vx = (m_lastJumpDir >= 0) ? m_velData.runFwdX * 60.f : m_velData.runBackX * 60.f;
                    setVelocityX(vx);
                    setVelocityY(m_velData.runFwdY * 60.f);
                } else {
                    // 普通跳跃: 保留现有水平速度 + 跳前方向的基础速度
                    if (m_lastJumpDir == 1) m_velocity.x += m_velData.jumpFwd * 60.f;
                    else if (m_lastJumpDir == -1) m_velocity.x += m_velData.jumpBack * 60.f;
                    // 跳前行走速度保留，仅限制最大
                    float maxVx = std::max(std::abs(m_velData.walkFwd), std::abs(m_velData.jumpFwd)) * 60.f;
                    if (std::abs(m_velocity.x) > maxVx) m_velocity.x = (m_velocity.x > 0 ? 1 : -1) * maxVx;
                    setVelocityY(m_velData.jumpNeuY * 60.f);
                }
                requestStateChange(50);
            }
        }
        else if (m_currentStateNo == 51 || m_currentStateNo == 50) {
            // 空中控制: 方向键影响水平速度 (M.U.G.E.N air.fwd/back)
            float airAccel = m_velData.walkFwd * 20.f;  // 空中操控灵敏度
            if (fwd) { m_velocity.x += airAccel * dt * 60.f; }
            if (back) { m_velocity.x -= airAccel * dt * 60.f; }
            // 限制空中最高速度
            float airMax = std::abs(m_velData.jumpFwd) * 60.f;
            if (m_velocity.x > airMax) m_velocity.x = airMax;
            if (m_velocity.x < -airMax) m_velocity.x = -airMax;
        }
        if (m_currentStateNo == 51) {
            // 空中待机: CNS 控制器处理, 落地检测自动切到 52
        }
        else if (m_currentStateNo == 50) {
            // 空中: 速度向下时切到 51
            if (m_velocity.y >= 0.f && m_stateTimer > 0.05f) {
                requestStateChange(51);
            }
            // 动画切换 (上升→下降)
            if (m_velocity.y > -480.f) {
                int curAnim = m_animationPlayer.getCurrentAnimId();
                if (curAnim >= 41 && curAnim <= 43) {
                    int fallAnim = curAnim + 3;
                    if (m_animations.contains(fallAnim)) switchAnimation(fallAnim);
                }
            }
        }
        else if (m_currentStateNo == 52) {
            // 落地缓冲 → 待机
            if (m_stateTimer > 0.1f) requestStateChange(0);
        }
        else if (m_currentStateNo == 11 && inputActive) {
            // 蹲下 → 起立 (放开↓)
            if (!down) { requestStateChange(12); }
            // 蹲下防御
            else if (back && inGuardRange) { requestStateChange(120); }
        }
        // 跑动跳跃 (100 → 106)
        else if (m_currentStateNo == 100 && up) {
            requestStateChange(106);
        }
        // 45: 跳跃特效 (由 CNS 处理退出)
        else if (m_currentStateNo == 45) {
            // CNS 控制器有 ChangeState 退出
        }
        // 106: 跑动跳跃 — 起跳后切到 50(空中)
        else if (m_currentStateNo == 106) {
            if (m_stateTimer >= 0.05f) requestStateChange(50);
        }
        // 132: 空中防御 — 松后 → 140(收防) / 落地 → 50
        else if (m_currentStateNo == 132) {
            if (!back) {
                requestStateChange(140);
            } else if (m_isGrounded) {
                requestStateChange(52);
            }
        }
        else if (m_currentStateNo == 10 && inputActive) {
            // 站立→蹲下过渡中放开↓ → 取消蹲下
            if (!down) { requestStateChange(0); }
        }
        // 防御状态: 120(防御开始) → 130(站防) / 131(蹲防) / 132(空防)
        else if (m_currentStateNo == 120) {
            if (!back) {
                requestStateChange(140);
            } else if (m_animationPlayer.hasJustLooped()) {
                if (m_stateType == 2) {
                    requestStateChange(132);  // 空中防御
                } else if (down) {
                    requestStateChange(131);  // 蹲防
                } else {
                    requestStateChange(130);  // 站防
                }
                m_animationPlayer.clearLoopFlag();
            }
        }
        // 蹲防→站防 (CNS 里处理了 holddown 的检测, 但这里的键盘释放转换需引擎辅助)
        else if (m_currentStateNo == 131) {
            if (!down) { requestStateChange(130); }
        }

        // ==========================================
        // 4.5 引擎级倒地/起身/败退
        // 5110(躺地) → 5120(起身): M.U.G.E.N 引擎内置行为
        if (m_currentStateNo == 5110) {
            int lieTicks = static_cast<int>(m_stateTimer * 60.f);
            int getupTime = std::max(m_hitInfo.groundHittime, m_hitInfo.groundSlidetime) + 20;
            if (lieTicks >= getupTime) {
                requestStateChange(5120);
            }
        }
        // 5150(死亡躺地) → 170(败退姿势)
        if (m_currentStateNo == 5150 && m_animationPlayer.hasJustLooped()) {
            if (m_stateRegistry.hasState(170)) requestStateChange(170);
            m_animationPlayer.clearLoopFlag();
        }

        // 4.9 递减 AnimTime (CNS 执行前, 确保读到最新值)
        m_animationPlayer.tickAnimTime();

        // 5. 执行当前状态的 CNS 控制器 (行走状态完全由引擎控制，跳过 CNS)
        if (m_currentStateNo != 20) {
            m_stateRegistry.executeState(m_currentStateNo, *this, &inputMgr, dt);
        }
        // 5a. 执行 State -2 (每帧控制器, 如 AssertSpecial, 自动切换等)
        m_stateRegistry.executeState(-2, *this, &inputMgr, dt);
        // 5b. 执行 State -3 (角色自身每帧执行, 如 shadow aura, 落地音效等)
        m_stateRegistry.executeState(-3, *this, &inputMgr, dt);
        // 5.1 保存 HitDefs 到缓存
        {
            const auto& hd = m_stateRegistry.getHitDefs(m_currentStateNo);
            if (!hd.empty()) m_lastAttackHitDefs = hd;
        }

        // ==========================================
        // 6. 更新受击状态标志 (供 GetHitVar 使用)
        // ==========================================
        if (m_moveType == 2) { // H = Hit
            int stateTicks = static_cast<int>(m_stateTimer * 60.f);
            if (stateTicks >= m_hitInfo.groundHittime) {
                m_isHitOver = true;
            }
            // 受击震动持续 groundHittime 帧后结束 (让受击动画有显示时间)
            if (!m_isHitShakeOver && stateTicks >= m_hitInfo.groundHittime) {
                m_isHitShakeOver = true;
            }
        }

        // ==========================================
        // 7. 自动返回待机 (用于没有 CNS 定义但有动画的状态)
        // ==========================================
        if (m_currentStateNo != 0 && !m_stateRegistry.hasState(m_currentStateNo)) {
            bool shouldReturn = false;
            if (m_animations.contains(m_currentStateNo)) {
                // 动画完成一次完整循环后才返回待机
                if (m_animationPlayer.hasJustLooped()) {
                    m_animationPlayer.clearLoopFlag();
                    shouldReturn = true;
                }
                // 安全兜底: 超过 60 ticks (1秒) 强制返回
                int stateTicks = static_cast<int>(m_stateTimer * 60.f);
                if (stateTicks > 60) {
                    shouldReturn = true;
                }
            } else {
                // 连动画都没有的状态, 立即返回
                shouldReturn = true;
            }
            if (shouldReturn) {
                requestStateChange(0);
            }
        }
        // 安全兜底: 任何状态超过 600 ticks (10秒) 未切换则强制回待机
        if (m_currentStateNo != 0 && m_stateRegistry.hasState(m_currentStateNo)) {
            int stateTicks = static_cast<int>(m_stateTimer * 60.f);
            if (stateTicks > 600) {
                requestStateChange(0);
            }
        }

        // ==========================================
        // 8. 每帧清除命中标志 (让 movehit 只在下帧状态执行前可见)
        // ==========================================
        m_hasMoveHit = false;

        // ==========================================
        // 9. 动画更新
        // ==========================================
        m_animationPlayer.update(dt);

        // 9.5 安全地面碰撞 (在 CNS PosSet 等可能修改位置后再次执行)
        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
        }

        // ==========================================
        // 10. Explod 每帧更新
        // ==========================================
        sf::Vector2f fighterPos = m_position;
        for (auto& explod : m_explods) {
            explod.animPlayer.update(dt);

            // 位置更新: bindtime 期间跟随父级
            if (explod.bindtime != 0) {
                if (explod.bindtime > 0) explod.bindtime--;
                // Explod 位置相对于 fighters 轴位置 (与 ExplodController 一致)
                float dir = isFacingRight() ? 1.f : -1.f;
                explod.currentPos = {
                    fighterPos.x + explod.pos.x * dir,
                    fighterPos.y + explod.pos.y
                };
                // 更新 facing: 始终跟随父级方向
                bool parentFacing = isFacingRight();
                explod.animPlayer.setFacingRight(explod.facing == 1 ? parentFacing : !parentFacing);
            } else {
                // 独立后: 按 vel 移动 (px/tick)
                explod.currentPos.x += explod.vel.x;
                explod.currentPos.y += explod.vel.y;
            }

            // 移除判定
            if (explod.removetime == -2) {
                // 动画播完后移除
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

        // ==========================================
        // 11. 画面震动递减
        // ==========================================
        if (m_shakeTime > 0) {
            m_shakeTime--;
            if (m_shakeTime <= 0) m_shakeAmpl = 0;
        }

        // ==========================================
        // 12. AfterImage 快照捕获
        // ==========================================
        if (m_afterImageActive) {
            m_afterImageTime--;
            if (m_afterImageTime <= 0) {
                m_afterImageActive = false;
                m_afterImageGhosts.clear();
            } else {
                m_afterImageCaptureTimer++;
                if (m_afterImageCaptureTimer >= m_afterImageTimegap) {
                    m_afterImageCaptureTimer = 0;

                    // 克隆当前精灵状态
                    AfterImageGhost ghost;
                    auto cloned = m_animationPlayer.cloneSprite();
                    if (cloned.has_value()) {
                        ghost.sprite = std::make_unique<sf::Sprite>(std::move(cloned.value()));
                    }
                    ghost.position = m_position;

                    // 计算 frame offset 和 sprite height（与 draw 逻辑一致）
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

    // 简化的更新路径 (P2 训练用, 仅物理 + 动画)
    void Fighter::update(float dt, const SimpleInputState& input, const sf::Vector2f& opponentPos) {
        if (isDead() && m_currentStateNo != 5150 && m_currentStateNo != 180 && m_currentStateNo != 5100) return;
        m_opponentPos = opponentPos;
        dt = std::min(dt, 0.05f);
        m_stateTimer += dt;

        // 物理更新 (与 InputManager 路径相同)
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

        // 地面碰撞: 触地时复位 (所有类型)
        if (getFeetY() >= GROUND_Y) {
            m_position.y = GROUND_Y - static_cast<float>(m_animationPlayer.getCurrentFrame().offset.y);
            // 空中类型不归零 Y 速度 (CNS 用 Vel Y > 0 检测落地)
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

        // physics = N 时接地后施加阻力防止无限滑行
        if (!applyGroundCollision && friction == 1.f && m_isGrounded) {
            m_velocity.x *= 0.88f;
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }
        if (m_isGrounded && friction < 1.f) {
            m_velocity.x *= std::pow(friction, dt * 60.f);
            if (std::abs(m_velocity.x) < 5.f) m_velocity.x = 0;
        }

        // 受击标志更新
        if (m_moveType == 2) {
            if (static_cast<int>(m_stateTimer * 60.f) >= m_hitInfo.groundHittime) {
                m_isHitOver = true;
            }
        }

        m_animationPlayer.update(dt);
    }
    // ... existing code ...
    void Fighter::draw(sf::RenderWindow& window) const {
        // 1. 绘制 AfterImage 残影 (使用快照中的精灵副本, 半透明)
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

        // 2. 构建 Explod 排序列表
        std::vector<const ExplodInstance*> sortedExplods;
        if (!m_explods.empty()) {
            sortedExplods.reserve(m_explods.size());
            for (const auto& e : m_explods) sortedExplods.push_back(&e);
            std::sort(sortedExplods.begin(), sortedExplods.end(),
                [](const ExplodInstance* a, const ExplodInstance* b) {
                    return a->sprpriority < b->sprpriority;
                });
        }

        // 3. 所有 Explod 画在角色之前
        for (const auto* e : sortedExplods) {
            e->animPlayer.draw(window, e->currentPos);
        }

        // 4. 角色主体 (应用 AngleDraw/Trans 覆盖, 检查 Invisible)
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

            // ✅ 关键修复：获取精灵高度，用于正确计算脚底对齐
            sf::Vector2f spriteSize = m_animationPlayer.getSpriteSize();
            float spriteHeight = spriteSize.y;

            // ✅ 修复：与 AnimationPlayer::draw 使用相同的 Y 轴计算
            float anchorX = m_position.x + frameOffset.x;
            float anchorY = m_position.y - spriteHeight + frameOffset.y; // ← 修复这里！

            // 调试：输出第一次调试帧时的位置
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
                // Clsn 框坐标相对于角色轴位置 (m_position)
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
            return 1; // High
        }
        if (n == "airtype") {
            std::string t = m_hitInfo.airType;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            if (t == "low")    return 2;
            if (t == "trip")   return 3;
            return 1; // High
        }
        if (n == "juggle")          return m_hitInfo.juggle;
        if (n == "hitid")           return m_hitInfo.hitid;
        if (n == "envshaketime" || n == "envshake.time") return m_hitInfo.envshakeTime;

        return 0;
    }

    void Fighter::setMoveType(int type) {
        m_moveType = type;
        if (type == 2) { // H = getting hit
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
            return {x - localPos.x, y + localPos.y}; // 镜像翻转 X 轴
        }
    }

    // 修改 getActiveHitbox：读取所有 Clsn1（攻击框）并计算整体包围盒
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

    // 修改 getActiveHurtbox：读取所有 Clsn2（受击框）并计算整体包围盒
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
        // Pushbox 的高度我们暂时固定为 100（覆盖大部分动作）
        // Y 轴从脚底向上延伸
        float height = m_pushHeight;
        float y = getFeetY() - height;

        float left, right;

        // ✅ 核心逻辑：根据朝向决定 Pushbox 的左右边界
        if (m_animationPlayer.isFacingRight()) {
            // 面向右：左边界是 back，右边界是 front
            left = m_position.x - m_pushBack;
            right = m_position.x + m_pushFront;
        } else {
            // 面向左：左边界变成 front，右边界变成 back
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
        // M.U.G.E.N 规范: 120-155 是防御相关状态 (含防御受击 150-155)
        return (m_currentStateNo >= 120 && m_currentStateNo <= 155);
    }

    void Fighter::addPower(int amount) {
        m_currentPower += amount;
        if (m_currentPower > m_maxPower) {
            m_currentPower = m_maxPower;
        }
    }

    void Fighter::addExplod(ExplodInstance explod) {
        // 如果已有同 ID 的 explod, 先移除旧的
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