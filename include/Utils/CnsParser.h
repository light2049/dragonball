#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Utils/CNSController.h"

namespace db {

    // ✅ 新增：速度数据 (从 CNS [Velocity] 块读取)
    // MUGEN 中速度单位是 像素/tick，1 tick = 1/60 秒
    struct VelocityData {
        float walkFwd = 7.f;      // walk.fwd.x (默认值 7)
        float walkBack = -6.f;    // walk.back.x
        float runFwdX = 8.f;      // run.fwd
        float runFwdY = 0.f;
        float runBackX = -5.f;    // run.back
        float runBackY = -5.f;
        float jumpNeuX = 0.f;     // jump.neu (x,y)
        float jumpNeuY = -8.4f;   // jump.y 是核心，决定跳跃高度
        float jumpFwd = 3.3f;     // jump.fwd.x
        float jumpBack = -3.33f;  // jump.back.x
        float runjumpFwd = 4.f;
        float runjumpBack = -2.55f;
        float airjumpNeuX = 0.f;
        float airjumpNeuY = -8.1f;
        float airjumpFwd = 2.6f;
        float airjumpBack = -2.66f;
    };

    // ✅ 新增：运动数据 (从 CNS [Movement] 块读取)
    struct MovementData {
        float yaccel = 0.44f;             // 垂直加速度 (yaccel)
        float standFriction = 0.85f;      // 站立摩擦系数 (stand.friction)
        float crouchFriction = 0.82f;     // 蹲下摩擦系数
        int airjumpNum = 1;
        int airjumpHeight = 35;
    };

    // 完整 HitDef 结构 (M.U.G.E.N 1.0 标准)
    struct HitDef {
        int damage = 0;              // 基础伤害
        int guardDamage = 0;         // 防御伤害
        int p1stateno = 0;           // 攻击方命中后跳转状态(连段用)
        int p2stateno = 0;           // 受击方跳转状态
        int pausetime = 0;           // 命中停顿
        int guardPausetime = 0;      // 防御停顿
        int groundHittime = 0;       // 地面受击硬直
        int groundSlidetime = 0;     // 地面滑行时间
        float groundVelocityX = 0.f; // 地面击退 X
        float groundVelocityY = 0.f; // 地面击退 Y
        float airHittime = 0;        // 空中受击硬直
        float airVelocityX = 0.f;    // 空中击退 X
        float airVelocityY = 0.f;    // 空中击退 Y
        float airguardVelocityX = 0.f; // 空中防御击退 X
        float airguardVelocityY = 0.f; // 空中防御击退 Y
        std::string animtype = "";   // 动画类型
        int animElemNo = 0;          // 命中判定帧

        // 攻击属性
        std::string attr = "";       // e.g. "S, NA" (攻击类型, 伤害类型)
        std::string guardflag = "";  // e.g. "MA" (可防御类型)
        std::string hitflag = "";    // e.g. "MAFDE" (可命中类型)
        int priority = 3;            // 攻击优先级
        std::string priorityType = "Hit"; // Hit/Miss/Dodge

        // 特效与音效
        int sparkno = -1;            // 打击火花编号 (-1=默认)
        int sparkX = 0;              // 火花X偏移
        int sparkY = 0;              // 火花Y偏移
        std::string hitsound = "";   // 命中音效 (e.g. "s0,29")
        std::string guardsound = ""; // 防御音效 (e.g. "6,0")

        // 命中高度类型
        std::string groundType = ""; // e.g. "High", "Low", "Trip"
        std::string airType = "";    // e.g. "High", "Low", "Trip"

        // 屏幕震动
        int envshakeTime = 0;

        // 能量
        int getpower = 0;            // 攻击方得能量
        int givepower = 0;           // 受击方得能量
        int guardGetPower = 0;       // 防御方给攻击方的能量
        int guardGivePower = 0;      // 攻击方给防御方的能量

        // 连击与 juggle
        int juggle = 0;              // 空中连击点数 (0=不消耗)

        // fall 参数
        bool fall = false;
        int fallYVel = 0;
        bool fallRecover = true;     // 可否受身

        int id = 0;
        bool active = false;
    };

    struct StateDef {
        int stateNo = 0;
        int type = 0;
        int movetype = 0;
        int physics = 0;
        int ctrl = -1;               // 新增：控制权 (-1=不变, 0=无控, 1=有控)
        float velsetX = 0.f;         // 新增：状态初始速度 VelSet
        float velsetY = 0.f;
        int poweradd = 0;            // 新增：进入状态增加的 Power
        int anim = -1;               // 状态默认动画 (-1=不切换)
        int sprpriority = 0;          // 绘制层级
        std::vector<std::unique_ptr<CNSController>> controllers;
        std::vector<HitDef> hitDefs;
    };

    struct CharacterStats {
        int maxLife = 1000;
        int maxPower = 3000;    // ✅ 新增：从 CNS [Data] power 读取
        int attack = 10;
        int defence = 5;
        int height = 60;

        int groundBack = 0;
        int groundFront = 0;
        int airBack = 0;
        int airFront = 0;

        VelocityData velocity;   // ✅ 新增：速度配置
        MovementData movement;   // ✅ 新增：运动配置
    };

    class CnsParser {
    public:
        static CharacterStats loadStats(const std::string& filePath);
        static std::map<int, StateDef> loadStateDefs(const std::string& filePath);
    };

} // namespace db