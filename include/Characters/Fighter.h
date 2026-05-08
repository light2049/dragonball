#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include "Characters/AnimationPlayer.h"
#include "Core/StateRegistry.h"
#include "Core/InputManager.h"
#include "Utils/AirParser.h"
#include "Utils/CnsParser.h"  // ✅ 引入数据结构
#include "Utils/CmdParser.h"   // ✅ 引入 CMD 解析器

namespace db {

    // ✅ 存储当前被命中的信息 (由 Game.cpp 在命中时写入)
    struct HitInfo {
        int damage = 0;              // 基础伤害
        int guardDamage = 0;         // 防御削血
        int groundHittime = 15;      // 受击硬直 (ticks)
        int groundSlidetime = 15;    // 滑行时间 (ticks)
        int airHittime = 12;         // 空中硬直时间
        float groundVelocityX = -3.f; // 地面击退 X
        float groundVelocityY = 0.f;  // 地面击退 Y
        float airVelocityX = -1.4f;   // 空中击退 X
        float airVelocityY = -3.f;    // 空中击退 Y
        float airguardVelocityX = -1.9f; // 空中防御击退
        float airguardVelocityY = -0.8f;
        std::string animtype = "Light"; // Light/Medium/Hard/Diagup/Up/Back
        int p1stateno = 0;           // 攻击方命中后跳转状态(连段)
        int p2stateno = 0;           // 受击方强制跳转状态
        bool fall = false;           // 是否会倒地
        bool fallrecover = true;     // 可否受身
        float fallyvel = 0.f;        // 倒地Y速度
        bool guarded = false;        // 本次是防御还是命中
        int yaccel = 0;              // 受击重力 (默认用 movement.yaccel)
        int ctrltime = 0;            // 恢复控制时间
        int juggle = 0;              // 消耗的 juggle 点数
        int hitid = 0;               // HitDef ID
        std::string groundType = "High"; // 地面命中高度
        std::string airType = "High";    // 空中命中高度
        int envshakeTime = 0;        // 屏幕震动时间
    };

    // ✅ 单个 Explod 实例 (由 ExplodController 创建)
    struct ExplodInstance {
        AnimationPlayer animPlayer;
        int id = 0;
        int bindtime = 0;          // 跟随父级的帧数 (-1 = 永远跟随)
        int removetime = -1;       // 自动移除时间 (-1 = 不限, -2 = 动画播完)
        int sprpriority = 0;       // 绘制层级
        sf::Vector2f pos;          // 相对位置 (px)
        sf::Vector2f vel;          // 每帧速度 (px/tick)
        sf::Vector2f currentPos;   // 当前世界坐标
        int facing = 1;            // 1=同父级方向, -1=相反方向
        bool removeOnGetHit = false; // 被击中时移除
        bool done = false;
    };

    class Fighter {
    public:
        Fighter();

        // ... 保持原有 public 接口不变 ...
        void loadAnimations(const std::string& airPath, const std::string& basePath, const std::string& prefix);
        void loadStats(const std::string& cnsPath);
        void loadCommonStates(const std::string& commonCnsPath);
        void loadCommands(const std::string& cmdPath);
        void update(float dt, const SimpleInputState& input, const sf::Vector2f& opponentPos);
        // 新的 update 重载: 使用 InputManager (支持 CMD 指令检测)
        void update(float dt, class InputManager& inputMgr, const sf::Vector2f& opponentPos);
        void draw(sf::RenderWindow& window) const;
        void drawDebug(sf::RenderWindow& window) const;

        void requestStateChange(int stateNo);

        void takeDamage(int damage);
        void resetLife();
        int getCurrentLife() const;
        int getMaxLife() const;
        bool isDead() const;

        void setVelocityX(float x);
        void setVelocityY(float y);
        float getVelocityX() const;
        float getVelocityY() const;
        void addPositionX(float x);
        void addPositionY(float y);
        sf::Vector2f getPosition() const;
        bool isGrounded() const;
        bool isFacingRight() const;
        void performJump();
        void setFacingRight(bool right);
        void switchAnimation(int animId);

        const std::vector<HitDef>& getCurrentHitDefs() const;

        bool isAttacking() const;
        void setAttacking(bool attacking);
        bool hasHitThisAttack() const;
        void markAttackAsHit();

        sf::FloatRect getActiveHitbox() const;
        sf::FloatRect getActiveHurtbox() const;

        void setPosition(float x, float y);
        sf::FloatRect getPushBox() const;
        // 获取当前脚底 Y 坐标 (基于轴位置和当前帧 offset.y)
        float getFeetY() const;
        sf::Vector2f getOpponentPos() const { return m_opponentPos; }
        void setRoundState(int s) { m_roundState = s; }
        int getRoundState() const { return m_roundState; }
        void setRoundNo(int n) { m_roundNo = n; }
        int getRoundNo() const { return m_roundNo; }

        static void setShowDebug(bool show) { m_showDebug = show; }
        static bool getShowDebug() { return m_showDebug; }

        bool isGuarding() const;
        void addPower(int amount);
        int getPower() const { return m_currentPower; }
        int getMaxPower() const { return m_maxPower; }

        // 查询 CNS const() 值 (返回 px/tick 原始值)
        float getConstValue(const std::string& path) const;

        // ✅ 新增：获取当前动画帧索引 (用于 AnimElem 判定)
        int getCurrentAnimFrameIndex() const;

        // ✅ 新增：Mugen movecontact 标志 (用于连招判断)
        bool hasMoveContact() const { return m_hasMoveContact; }
        void setMoveContact(bool contact) { m_hasMoveContact = contact; }

        // ✅ 新增：暴露 AnimationPlayer (供 Game.cpp 帧判定使用)
        const AnimationPlayer& getAnimationPlayer() const { return m_animationPlayer; }
        AnimationPlayer& getAnimationPlayer() { return m_animationPlayer; }

        // ✅ 新增：控制权 (Ctrl)
        bool hasControl() const { return m_hasControl; }
        void setControl(bool ctrl) { m_hasControl = ctrl; }

        // ✅ 新增：获取当前状态编号 (用于调试日志)
        int getCurrentStateNo() const { return m_currentStateNo; }

        // ✅ 新增：CNS 状态执行 (供 Game.cpp 在外部调用)
        StateRegistry& getStateRegistry() { return m_stateRegistry; }
        const StateRegistry& getStateRegistry() const { return m_stateRegistry; }

        void executeCurrentStateCNS(InputManager* inputMgr, float dt) {
            m_stateRegistry.executeState(m_currentStateNo, *this, inputMgr, dt);
        }

        // ✅ 新增：写入/读取 HitInfo (由 Game.cpp 调用)
        void setHitInfo(const HitInfo& info) { m_hitInfo = info; }
        const HitInfo& getHitInfo() const { return m_hitInfo; }

        // ==========================================
        // 以下方法供 CNSController::Condition::evaluate() 调用
        // ==========================================

        // 状态时间 (ticks, 1/60 秒)
        int getStateTime() const { return static_cast<int>(m_stateTimer * 60.f); }

        // 上一个状态号
        int getPreviousStateNo() const { return m_previousStateNo; }
        int getFrameStartState() const { return m_frameStartState; }

        // 当前动画 ID
        int getCurrentAnimId() const { return m_animationPlayer.getCurrentAnimId(); }

        // 当前 AnimElem (1-based frame index)
        int getCurrentAnimElem() const { return m_animationPlayer.getCurrentAnimElem(); }

        // AnimTime: 动画剩余帧数
        int getAnimTime() const { return m_animationPlayer.getAnimTime(); }

        // 是否命中对手 (movehit / moveguarded)
        bool hasMoveHit() const { return m_hasMoveHit; }
        void setMoveHit(bool hit) { m_hasMoveHit = hit; }
        bool hasMoveGuarded() const { return m_hasMoveGuarded; }
        void setMoveGuarded(bool g) { m_hasMoveGuarded = g; }

        // 受击状态查询
        bool isHitOver() const { return m_isHitOver; }
        void setHitOver(bool over) { m_isHitOver = over; }
        bool isHitShakeOver() const { return m_isHitShakeOver; }
        void setHitShakeOver(bool over) { m_isHitShakeOver = over; }

        // 动画存在检测
        bool doesAnimExist(int animId) const { return m_animations.contains(animId); }
        const Animation* getAnimation(int animId) const {
            auto it = m_animations.find(animId);
            return it != m_animations.end() ? &it->second : nullptr;
        }

        // 受击恢复 (受身)
        bool canRecover() const { return m_canRecover; }
        void setCanRecover(bool rec) { m_canRecover = rec; }

        // 防御距离检测
        bool isInGuardDist() const;

        // 地面碰撞高度设置 (由 Game 构造函数初始化)
        void setGroundLevel(float y) { m_groundLevel = y; }
        float getGroundLevel() const { return m_groundLevel; }

        // 状态类型 (return 0=S, 1=C, 2=A)
        int getStateType() const { return m_stateType; }

        // 移动类型 (return 0=I, 1=A, 2=H)
        int getMoveType() const { return m_moveType; }

        // 物理类型 (0=S, 1=C, 2=A, 3=N)
        int getPhysicsType() const { return m_physicsType; }
        void setPhysicsType(int type) { m_physicsType = type; }
        void setMoveType(int type);

        // 速度增加
        void addVelocity(float x, float y) { m_velocity.x += x; m_velocity.y += y; }

        // 设置状态类型 (0=S, 1=C, 2=A)
        void setStateType(int type) { m_stateType = type; }

        // 设置系统变量
        void setSysVar(int idx, int val) {
            if (idx >= 0 && idx < 60) m_sysVar[idx] = val;
        }
        int getSysVar(int idx) const {
            if (idx >= 0 && idx < 60) return m_sysVar[idx];
            return 0;
        }

        // GetHitVar: 获取受击变量
        int getHitVar(const std::string& name) const;

        // 设置 NotHitBy 保护 (占位)
        void setNotHitBy(const std::string& val) { /* TODO */ }

        // 查询某状态的状态属性 (供 Helper 使用)
        bool getStateDefAttributes(int stateNo, int& outAnim, float& outVelX, float& outVelY) const;

        // ✅ 画面震动 (由 EnvShakeController 设置, Game 读取)
        void setShake(int time, int ampl) { m_shakeTime = time; m_shakeAmpl = ampl; }
        int getShakeTime() const { return m_shakeTime; }
        int getShakeAmpl() const { return m_shakeAmpl; }

        // SuperPause
        void setSuperPause(int time, bool darken);

        // ✅ 绘制覆盖 (每帧)
        DrawOverrides& getDrawOverrides() { return m_drawOverrides; }
        const DrawOverrides& getDrawOverrides() const { return m_drawOverrides; }
        bool isInSuperPause() const { return m_superPauseTime > 0; }
        int getSuperPauseTime() const { return m_superPauseTime; }
        bool getSuperPauseDarken() const { return m_superPauseDarken; }
        void tickSuperPause() { if (m_superPauseTime > 0) m_superPauseTime--; }

        // ✅ Explod 管理
        void addExplod(ExplodInstance explod);
        void removeExplod(int id);
        const std::vector<ExplodInstance>& getExplods() const { return m_explods; }
        void clearExplods();

        // ✅ AfterImage 管理
        struct AfterImageGhost {
            std::unique_ptr<sf::Sprite> sprite;
            sf::Vector2f position;
            float spriteHeight = 0.f;
            sf::Vector2f frameOffset;
        };
        void setAfterImage(int time, int timegap, int length);
        void clearAfterImage() { m_afterImageActive = false; m_afterImageGhosts.clear(); }
        bool isAfterImageActive() const { return m_afterImageActive; }
        const std::vector<AfterImageGhost>& getAfterImageGhosts() const { return m_afterImageGhosts; }

        // ✅ Helper 待创建列表 (由 HelperController 写入, Game 读取)
        struct PendingHelper {
            int id = 0;
            int animId = -1;
            int stateNo = 0;        // Helper 状态号(用于 CNS 执行)
            sf::Vector2f position;
            sf::Vector2f velocity;  // px/tick
            bool facingRight = true;
            int damage = 20;
            int sparkno = 1200;
            int parentStateno = 0;  // 创建时的父状态号
        };
        void addPendingHelper(const PendingHelper& ph);
        std::vector<PendingHelper> drainPendingHelpers();

    private:
        int m_currentStateNo;
        int m_previousStateNo = -1; // 上一个状态号
        float m_stateTimer;
        int m_lastJumpDir = 0; // ✅ 新增：记录跳跃方向 (-1:后, 0:中, 1:前)
        int m_stateType = 0;   // 0=S(站立), 1=C(蹲下), 2=A(空中)
        int m_moveType = 0;    // 0=I(待机), 1=A(攻击), 2=H(受击)
        int m_physicsType = 0; // 0=S, 1=C, 2=A, 3=N
        sf::Vector2f m_position;
        sf::Vector2f m_velocity;
        bool m_isGrounded;

        // ✅ 新增：从 CNS 读取的物理配置
        VelocityData m_velData;   // 速度数据
        MovementData m_moveData;  // 运动/摩擦/重力数据

        AnimationPlayer m_animationPlayer;
        std::map<int, Animation> m_animations;
        StateRegistry m_stateRegistry;
        CmdParser m_cmdParser;  // CMD 指令解析器

        int m_maxLife = 1000;
        int m_currentLife = 1000;
        float m_pushBack = 0.f;
        float m_pushFront = 0.f;
        float m_pushHeight = 60.f;

        bool m_isAttacking = false;
        bool m_hasHitCurrentAttack = false;
        bool m_hasMoveContact = false; // ✅ 新增：记录本次攻击是否已命中/被防
        bool m_hasMoveHit = false;     // 是否实际命中 (非被防)
        bool m_hasMoveGuarded = false; // 是否被防御
        bool m_hasControl = true; // ✅ 新增：是否有控制权
        bool m_isHitOver = true;  // 受击状态是否结束
        bool m_isHitShakeOver = true; // 受击震动是否结束
        bool m_canRecover = false; // 是否可以受身恢复
        float m_groundLevel = 478.0f; // 地面轴高度 (由 Game 初始化)

        int m_currentPower = 0;
        int m_maxPower = 3000;

        // ✅ 新增：被命中信息存储
        HitInfo m_hitInfo;

        // ✅ 系统变量 (60 个整数, M.U.G.E.N 标准)
        int m_sysVar[60] = {0};

        // ✅ 画面震动 (EnvShake)
        int m_shakeTime = 0;
        int m_shakeAmpl = 0;
        int m_hitFlashTimer = 0;    // 受击闪白计时器 (帧数)
        int m_superPauseTime = 0;
        bool m_superPauseDarken = false;
        int m_roundState = 0;
        int m_roundNo = 1;
        sf::Vector2f m_opponentPos;  // 对手位置(用于 CNS 表达式)
        mutable std::vector<HitDef> m_lastAttackHitDefs; // 保留上帧攻击的 HitDefs

        // ✅ 记录帧开始时的状态号 (供 Helper 的 parent,stateno 使用)
        int m_frameStartState = 0;

        // ✅ 每帧绘制覆盖 (由 AngleDraw / Trans 控制器设置)
        DrawOverrides m_drawOverrides;

        // ✅ Explod 特效实例列表
        std::vector<ExplodInstance> m_explods;

        // ✅ AfterImage 状态
        bool m_afterImageActive = false;
        int m_afterImageTime = 0;       // 剩余持续帧数
        int m_afterImageTimegap = 2;    // 快照间隔帧数
        int m_afterImageLength = 3;     // 最大快照数量
        int m_afterImageCaptureTimer = 0; // 距上次快照的帧数
        std::vector<AfterImageGhost> m_afterImageGhosts; // 残影快照列表

        // ✅ Helper 待创建列表
        std::vector<PendingHelper> m_pendingHelpers;

        // ✅ 修改：移除硬编码常量，使用辅助函数或变量替代
        // const float GRAVITY = 1800.f;  <-- 删除
        // const float JUMP_FORCE = -900.f; <-- 删除
        const float GROUND_Y = 480.f;

        static bool m_showDebug;
    };
}