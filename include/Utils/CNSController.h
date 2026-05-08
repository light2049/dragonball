#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace db {
    class Fighter;
    class InputManager;

    // ==========================================
    // 触发器条件系统
    // ==========================================

    // 触发条件类型
    enum class CondType {
        // 标准 M.U.G.E.N 可检测条件
        TIME,           // Time = N
        ANIMELEM,       // AnimElem = N
        ANIMTIME,       // AnimTime = N
        HITOVERTIME,    // 别名: TimeAfter(HitOver)
        COMMAND,        // command = "xxx"
        CTRL,           // ctrl (布尔)
        STATETYPE,      // statetype = S/C/A
        STATENO,        // stateno = N
        PREVSTATENO,    // prevstateno = N
        ROUNDSTATE,     // roundstate = N (0=开场, 1=战斗中)
        MOVECONTACT,    // movecontact (布尔)
        MOVEHIT,        // movehit (布尔)
        MOVEGUARDED,    // moveguarded
        HITOVER,        // HitOver
        HITSHAKEOVER,   // HitShakeOver
        ALIVE,          // alive
        LIFE,           // life
        POWER,          // power
        VEL_X,          // vel x
        VEL_Y,          // vel y
        POS_Y,          // pos y
        RANDOM,         // random
        SELFANIMEXIST,  // SelfAnimExist(N)
        MATCHOVER,      // MatchOver (布尔)
        ROUNDNO,        // roundno = N
        INGUARDDIST,    // inguarddist (布尔)
        P2DIST_X,       // p2dist X
        P2STATETYPE,    // p2statetype = S/C/A
        GETHITVAR,      // GetHitVar(xxx) = N
        CONST,          // const(velocity.xxx)
        ANIM,           // anim = N (当前动画ID)
        CANRECOVER,     // CanRecover (布尔)
        TIMEMOD,        // timemod = N,M
        AILEVEL,        // ailevel = N
        POS_X,          // pos x
        P2BODYDIST_X,   // p2bodydist x
        MOVETYPE,       // movetype = I/A/H
        COMMAND_COUNT,  // command != "xxx" 等
        HITFALL,        // HitFall (布尔)
        PARENT_STATENO, // parent,stateno = N
        FRONTEDGEBODYDIST, // FrontEdgeBodyDist (画面边缘距离)
        BACKEDGEBODYDIST,  // BackEdgeBodyDist (画面边缘距离)
        // 字面量 (用于比较)
        LITERAL_TRUE,   // 永真
        LITERAL_FALSE,  // 永假
    };

    // 比较操作符
    enum class CondOp {
        EQ, NEQ, GT, GTE, LT, LTE, NONE
    };

    // 单个条件
    struct Condition {
        CondType type = CondType::LITERAL_TRUE;
        CondOp op = CondOp::NONE;
        bool negated = false;      // ! 前缀

        // 比较值 (普通比较)
        int rhsInt = 0;
        float rhsFloat = 0.f;
        std::string rhsStr;

        // 范围比较 [low, high]
        bool isRange = false;
        int rangeLow = 0;
        int rangeHigh = 0;

        // 范围表达式 (如 [ground.SlideTime])
        std::string rangeExpr;

        // 参数 (如 GetHitVar 的参数名, const 的路径)
        std::string paramStr;
        int paramInt = 0;
        int paramInt2 = 0; // 用于 timemod = N,M

        // 评估此条件
        bool evaluate(const Fighter& fighter, const InputManager* inputMgr) const;
    };

    // 单条 trigger (trigger1, trigger2 等)
    struct TriggerLine {
        int id = 1;         // trigger1, trigger2, ...
        std::vector<Condition> conditions;   // AND 组合
        bool allFlag = false; // triggerall
    };

    // ==========================================
    // CNS 控制器基类
    // ==========================================

    enum class ControllerType {
        NONE,
        CHANGESTATE,
        CHANGEANIM,
        VELSET,
        VELADD,
        VELMUL,
        HITDEF,
        STATETYPESET,
        CTRLSET,
        POSSET,
        POSADD,
        POWERADD,
        PLAYSND,
        EXPLOD,
        HITVELSET,
        VARSET,
        VARADD,
        VARRANGESET,
        NOTHITBY,
        ASSERTSPECIAL,
        TURN,
        SELFSTATE,
        NULL_CTRL,
        POSFREEZE,
        MAKEDUST,
        NOTHITBY_D,
        DEFENCEMULSET,
        PALFX,
        REMOVEEXPLOD,
        HITFALLDAMAGE,
        HITFALLSET,
        HITFALLVEL,
        FALLENVSHAKE,
        FORCEFEEDBACK,
        STOPLAYING,
        STOPBGM,
        ALLPALFX,
        AFTERIMAGE,
        BGPALFX,
        HITBY,
        SPRPRIORITY,
        DISPLAYTOCLIPBOARD,
        PARENTVARADD,
        CTRL_NULL,
        ENVSHAKE,
        HELPER,
        SUPERPAUSE,
        BIND_TO_ROOT,
        DESTROY_SELF,
        ANGLEDRAW,
        ANGLEADD,
        ANGLESET,
        TRANS,
    };

    class CNSController {
    public:
        virtual ~CNSController() = default;

        // 控制器类型
        ControllerType type = ControllerType::NONE;

        // 触发条件列表
        std::vector<TriggerLine> triggers;

        // 解析参数 (来自 CNS 键值行)
        virtual void parse(const std::string& key, const std::string& value);

        // 检查 trigger 是否满足
        bool checkTriggers(const Fighter& fighter, const InputManager* inputMgr, int stateTime) const;

        // 执行控制器逻辑
        virtual void execute(Fighter& fighter, InputManager* inputMgr, float dt) const;

        // 通用参数
        int paramInt = 0;
        int paramInt2 = 0;
        float paramFloat = 0.f;
        float paramFloat2 = 0.f;
        std::string paramStr;
        int value = 0;         // 通用 value
        float valueX = 0.f;    // x 值
        float valueY = 0.f;    // y 值
        bool hasX = false;     // x 是否被显式设置
        bool hasY = false;     // y 是否被显式设置
        std::string valueXStr; // x 表达式字符串 (如 const(velocity.walk.fwd.x))
        std::string valueYStr; // y 表达式字符串
        std::string valueStr;  // 字符串值
        int persistent = 1;    // M.U.G.E.N: 0=触发一次后禁用, 1=每次触发
        mutable int m_firedThisState = -1; // 本状态中是否已触发(-1=未进状态, 0=未触发, 1=已触发)
    };

    // ==========================================
    // 具体控制器实现
    // ==========================================

    class ChangeStateController : public CNSController {
    public:
        ChangeStateController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class ChangeAnimController : public CNSController {
    public:
        ChangeAnimController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VelSetController : public CNSController {
    public:
        VelSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VelAddController : public CNSController {
    public:
        VelAddController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VelMulController : public CNSController {
    public:
        VelMulController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class StateTypeSetController : public CNSController {
    public:
        StateTypeSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class CtrlSetController : public CNSController {
    public:
        CtrlSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class PosSetController : public CNSController {
    public:
        PosSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class PosAddController : public CNSController {
    public:
        PosAddController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class PowerAddController : public CNSController {
    public:
        PowerAddController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class PlaySndController : public CNSController {
    public:
        PlaySndController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class HitDefController : public CNSController {
    public:
        HitDefController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VarSetController : public CNSController {
    public:
        VarSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VarAddController : public CNSController {
    public:
        VarAddController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class NotHitByController : public CNSController {
    public:
        NotHitByController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class TurnController : public CNSController {
    public:
        TurnController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class SelfStateController : public CNSController {
    public:
        SelfStateController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class NullController : public CNSController {
    public:
        NullController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    class VarRangeSetController : public CNSController {
    public:
        VarRangeSetController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_value = 0;
    };

    class AssertSpecialController : public CNSController {
    public:
        AssertSpecialController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_flags = 0; // 位掩码: 1=invisible
    };

    class AngleDrawController : public CNSController {
    public:
        AngleDrawController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        float m_scaleX = 1.f, m_scaleY = 1.f;
    };

    class TransController : public CNSController {
    public:
        TransController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        std::string m_transType;
        int m_alphaSrc = 256;
        int m_alphaDst = 256;
        std::string m_alphaSrcExpr;  // 表达式字符串 (含 time 等变量)
        std::string m_alphaDstExpr;
    };

    // --- EnvShake ---
    class EnvShakeController : public CNSController {
    public:
        EnvShakeController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_shakeTime = 0;
        int m_shakeAmpl = 0;
    };

    // --- FallEnvShake ---
    class FallEnvShakeController : public CNSController {
    public:
        FallEnvShakeController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- Explod ---
    class ExplodController : public CNSController {
    public:
        ExplodController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_anim = -1;
        int m_id = 0;
        float m_posX = 0.f, m_posY = 0.f;
        std::string m_postype = "p1";
        int m_facing = 1;
        int m_vfacing = 1;
        int m_bindtime = 1;
        float m_velX = 0.f, m_velY = 0.f;
        int m_removetime = -2;
        int m_sprpriority = 0;
        float m_scaleX = 1.f, m_scaleY = 1.f;
        bool m_removeOnGetHit = false;
    };

    // --- RemoveExplod ---
    class RemoveExplodController : public CNSController {
    public:
        RemoveExplodController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_id = 0;
    };

    // --- Helper ---
    class HelperController : public CNSController {
    public:
        HelperController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_stateno = 0;
        int m_id = 0;
        float m_posX = 0.f, m_posY = 0.f;
        std::string m_postype = "p1";
        int m_facing = 1;
    };

    // --- AfterImage ---
    class AfterImageController : public CNSController {
    public:
        AfterImageController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_time = 10;
        int m_timegap = 2;
        int m_framegap = 1;
        int m_length = 3;
    };

    // --- SuperPause ---
    class SuperPauseController : public CNSController {
    public:
        SuperPauseController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_time = 0;
        int m_posX = 0, m_posY = 0;
        int m_darken = 0;
    };

    // --- DestroySelf (Helper 自毁) ---
    class DestroySelfController : public CNSController {
    public:
        DestroySelfController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- BindToRoot (挂载到父人物) ---
    class BindToRootController : public CNSController {
    public:
        BindToRootController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override {}
        int m_posX = 0, m_posY = 0;
    };

    // --- AngleAdd (逐帧累加旋转角度) ---
    class AngleAddController : public CNSController {
    public:
        AngleAddController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        float m_angle = 0.f;
    };

    // --- AngleSet (设置旋转角度) ---
    class AngleSetController : public CNSController {
    public:
        AngleSetController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        float m_angle = 0.f;
    };

    // --- MakeDust (跑步灰尘) ---
    class MakeDustController : public CNSController {
    public:
        MakeDustController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- PalFX (调色板特效) ---
    class PalFXController : public CNSController {
    public:
        PalFXController();
        void parse(const std::string& key, const std::string& value) override;
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
        int m_time = 0;
        int m_addR = 0, m_addG = 0, m_addB = 0;
        int m_mulR = 256, m_mulG = 256, m_mulB = 256;
        int m_sinAddR = 0, m_sinAddG = 0, m_sinAddB = 0;
    };

    // --- DefenceMulSet (防御倍率) ---
    class DefenceMulSetController : public CNSController {
    public:
        DefenceMulSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- HitVelSet (受击击退速度) ---
    class HitVelSetController : public CNSController {
    public:
        HitVelSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- PosFreeze (锁定位置) ---
    class PosFreezeController : public CNSController {
    public:
        PosFreezeController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- HitFallDamage (落地伤害) ---
    class HitFallDamageController : public CNSController {
    public:
        HitFallDamageController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- HitFallVel (弹地速度) ---
    class HitFallVelController : public CNSController {
    public:
        HitFallVelController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // --- HitFallSet (结束倒地状态) ---
    class HitFallSetController : public CNSController {
    public:
        HitFallSetController();
        void execute(Fighter& fighter, InputManager* inputMgr, float dt) const override;
    };

    // CNS 表达式求值 (p2dist, const(), etc.)
    class Fighter;
    int evaluateCNSExpression(const std::string& expr, const Fighter& fighter);

    // 工厂函数
    std::unique_ptr<CNSController> createController(ControllerType type);

    // 根据字符串获取控制器类型
    ControllerType parseControllerType(const std::string& typeStr);

    // 条件解析
    std::vector<Condition> parseConditionString(const std::string& condStr);

} // namespace db
