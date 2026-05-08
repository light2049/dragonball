#include "Utils/CNSController.h"
#include "Characters/Fighter.h"
#include "Core/InputManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace db {

    // ==========================================
    // 控制器工厂
    // ==========================================

    ControllerType parseControllerType(const std::string& typeStr) {
        std::string t = typeStr;
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);

        if (t == "changestate")     return ControllerType::CHANGESTATE;
        if (t == "changeanim")      return ControllerType::CHANGEANIM;
        if (t == "velset")          return ControllerType::VELSET;
        if (t == "veladd")          return ControllerType::VELADD;
        if (t == "velmul")          return ControllerType::VELMUL;
        if (t == "hitdef")          return ControllerType::HITDEF;
        if (t == "statetypeset")    return ControllerType::STATETYPESET;
        if (t == "ctrlset")         return ControllerType::CTRLSET;
        if (t == "posset")          return ControllerType::POSSET;
        if (t == "posadd")          return ControllerType::POSADD;
        if (t == "poweradd")        return ControllerType::POWERADD;
        if (t == "playsnd")         return ControllerType::PLAYSND;
        if (t == "explod")          return ControllerType::EXPLOD;
        if (t == "hitvelset")       return ControllerType::HITVELSET;
        if (t == "varset")          return ControllerType::VARSET;
        if (t == "varadd")          return ControllerType::VARADD;
        if (t == "varrangeset")     return ControllerType::VARRANGESET;
        if (t == "nothitby")        return ControllerType::NOTHITBY;
        if (t == "assertspecial")   return ControllerType::ASSERTSPECIAL;
        if (t == "turn")            return ControllerType::TURN;
        if (t == "selfstate")       return ControllerType::SELFSTATE;
        if (t == "posfreeze")       return ControllerType::POSFREEZE;
        if (t == "makedust")        return ControllerType::MAKEDUST;
        if (t == "defencemulset")   return ControllerType::DEFENCEMULSET;
        if (t == "palfx")           return ControllerType::PALFX;
        if (t == "removeexplod")    return ControllerType::REMOVEEXPLOD;
        if (t == "hitfalldamage")   return ControllerType::HITFALLDAMAGE;
        if (t == "hitfallset")      return ControllerType::HITFALLSET;
        if (t == "hitfallvel")      return ControllerType::HITFALLVEL;
        if (t == "fallemvshake")    return ControllerType::FALLENVSHAKE;
        if (t == "envshake")        return ControllerType::ENVSHAKE;
        if (t == "forcefeedback")   return ControllerType::FORCEFEEDBACK;
        if (t == "null")            return ControllerType::NULL_CTRL;
        if (t == "hitby")           return ControllerType::HITBY;
        if (t == "sprpriority")     return ControllerType::SPRPRIORITY;
        if (t == "helper")          return ControllerType::HELPER;
        if (t == "superpause")      return ControllerType::SUPERPAUSE;
        if (t == "bindtoroot")      return ControllerType::BIND_TO_ROOT;
        if (t == "destroyself")      return ControllerType::DESTROY_SELF;
        if (t == "angledraw")        return ControllerType::ANGLEDRAW;
        if (t == "trans")            return ControllerType::TRANS;

        return ControllerType::NONE;
    }

    std::unique_ptr<CNSController> createController(ControllerType type) {
        switch (type) {
            case ControllerType::CHANGESTATE:   return std::make_unique<ChangeStateController>();
            case ControllerType::CHANGEANIM:    return std::make_unique<ChangeAnimController>();
            case ControllerType::VELSET:        return std::make_unique<VelSetController>();
            case ControllerType::VELADD:        return std::make_unique<VelAddController>();
            case ControllerType::VELMUL:        return std::make_unique<VelMulController>();
            case ControllerType::STATETYPESET:  return std::make_unique<StateTypeSetController>();
            case ControllerType::CTRLSET:       return std::make_unique<CtrlSetController>();
            case ControllerType::POSSET:        return std::make_unique<PosSetController>();
            case ControllerType::POSADD:        return std::make_unique<PosAddController>();
            case ControllerType::POWERADD:      return std::make_unique<PowerAddController>();
            case ControllerType::PLAYSND:       return std::make_unique<PlaySndController>();
            case ControllerType::HITDEF:        return std::make_unique<HitDefController>();
            case ControllerType::VARSET:        return std::make_unique<VarSetController>();
            case ControllerType::VARADD:        return std::make_unique<VarAddController>();
            case ControllerType::NOTHITBY:      return std::make_unique<NotHitByController>();
            case ControllerType::TURN:          return std::make_unique<TurnController>();
            case ControllerType::SELFSTATE:     return std::make_unique<SelfStateController>();
            case ControllerType::NULL_CTRL:     return std::make_unique<NullController>();
            case ControllerType::ENVSHAKE:      return std::make_unique<EnvShakeController>();
            case ControllerType::FALLENVSHAKE:  return std::make_unique<FallEnvShakeController>();
            case ControllerType::EXPLOD:        return std::make_unique<ExplodController>();
            case ControllerType::REMOVEEXPLOD:  return std::make_unique<RemoveExplodController>();
            case ControllerType::HELPER:        return std::make_unique<HelperController>();
            case ControllerType::AFTERIMAGE:    return std::make_unique<AfterImageController>();
            case ControllerType::SUPERPAUSE:    return std::make_unique<SuperPauseController>();
            case ControllerType::BIND_TO_ROOT:  return std::make_unique<BindToRootController>();
            case ControllerType::HITVELSET:     return std::make_unique<HitVelSetController>();
            case ControllerType::POSFREEZE:     return std::make_unique<PosFreezeController>();
            case ControllerType::HITFALLDAMAGE: return std::make_unique<HitFallDamageController>();
            case ControllerType::HITFALLVEL:    return std::make_unique<HitFallVelController>();
            case ControllerType::HITFALLSET:    return std::make_unique<HitFallSetController>();
            case ControllerType::DESTROY_SELF:  return std::make_unique<DestroySelfController>();
            case ControllerType::VARRANGESET:   return std::make_unique<VarRangeSetController>();
            case ControllerType::ASSERTSPECIAL: return std::make_unique<AssertSpecialController>();
            case ControllerType::ANGLEDRAW:     return std::make_unique<AngleDrawController>();
            case ControllerType::TRANS:          return std::make_unique<TransController>();
            default:                            return nullptr;
        }
    }

    // ==========================================
    // CNSController 基类实现
    // ==========================================

    void CNSController::parse(const std::string& key, const std::string& value) {
        // 默认实现：存成字符串参数
        paramStr = value;
    }

    bool CNSController::checkTriggers(const Fighter& fighter, const InputManager* inputMgr, int stateTime) const {
        if (triggers.empty()) return true; // 没有 trigger = 总是执行

        // M.U.G.E.N 规则: triggerall 必须全部满足
        // 然后 trigger1, trigger2, ... 中任何一个满足即可
        // 先检查 triggerall
        for (const auto& tl : triggers) {
            if (!tl.allFlag) continue;
            for (const auto& cond : tl.conditions) {
                if (!cond.evaluate(fighter, inputMgr)) return false;
            }
        }

        // 检查 trigger1, trigger2, ...
        for (const auto& tl : triggers) {
            if (tl.allFlag) continue; // triggerall 已经检查过了
            bool allMet = true;
            for (const auto& cond : tl.conditions) {
                if (!cond.evaluate(fighter, inputMgr)) {
                    allMet = false;
                    break;
                }
            }
            if (allMet) return true; // 任一 trigger 满足即可
        }

        return false;
    }

    void CNSController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 基类什么都不做
    }

    // ==========================================
    // 条件评估器
    // ==========================================

    // 辅助: 将字符串转换为 statetype 数值 (0=S, 1=C, 2=A, 3=L)
    static int statetypeToInt(const std::string& s) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "s") return 0;
        if (lower == "c") return 1;
        if (lower == "a") return 2;
        if (lower == "l") return 3;
        return -1;
    }

    bool Condition::evaluate(const Fighter& fighter, const InputManager* inputMgr) const {
        bool result = false;

        switch (type) {
            case CondType::LITERAL_TRUE:
                result = true;
                break;

            case CondType::LITERAL_FALSE:
                result = false;
                break;

            case CondType::TIME: {
                int time = fighter.getStateTime();
                int val = rhsInt;
                if (isRange && !rangeExpr.empty()) {
                    val = fighter.getHitVar(rangeExpr);
                }
                switch (op) {
                    case CondOp::EQ:  result = (time == val); break;
                    case CondOp::NEQ: result = (time != val); break;
                    case CondOp::GT:  result = (time > val); break;
                    case CondOp::GTE: result = (time >= val); break;
                    case CondOp::LT:  result = (time < val); break;
                    case CondOp::LTE: result = (time <= val); break;
                    default: result = (time == val);
                }
                break;
            }

            case CondType::ANIMELEM: {
                int elem = fighter.getCurrentAnimElem();
                switch (op) {
                    case CondOp::EQ:  result = (elem == rhsInt); break;
                    case CondOp::NEQ: result = (elem != rhsInt); break;
                    case CondOp::GT:  result = (elem > rhsInt); break;
                    case CondOp::GTE: result = (elem >= rhsInt); break;
                    case CondOp::LT:  result = (elem < rhsInt); break;
                    case CondOp::LTE: result = (elem <= rhsInt); break;
                    default: result = (elem == rhsInt);
                }
                break;
            }

            case CondType::ANIMTIME: {
                int animTime = fighter.getAnimTime();
                switch (op) {
                    case CondOp::EQ:  result = (animTime == rhsInt); break;
                    case CondOp::GT:  result = (animTime > rhsInt); break;
                    case CondOp::GTE: result = (animTime >= rhsInt); break;
                    case CondOp::LT:  result = (animTime < rhsInt); break;
                    case CondOp::LTE: result = (animTime <= rhsInt); break;
                    case CondOp::NEQ: result = (animTime != rhsInt); break;
                    default: result = (animTime == 0);
                }
                break;
            }

            case CondType::COMMAND: {
                if (!inputMgr) { result = false; break; }
                bool cmdMet = inputMgr->isHeld(rhsStr);
                result = (op == CondOp::EQ) ? cmdMet : !cmdMet;
                break;
            }

            case CondType::CTRL: {
                result = fighter.hasControl();
                break;
            }

            case CondType::STATETYPE: {
                result = (fighter.getStateType() == statetypeToInt(rhsStr));
                break;
            }

            case CondType::STATENO: {
                int stno = fighter.getCurrentStateNo();
                switch (op) {
                    case CondOp::EQ:  result = (stno == rhsInt); break;
                    case CondOp::NEQ: result = (stno != rhsInt); break;
                    case CondOp::GT:  result = (stno > rhsInt); break;
                    case CondOp::GTE: result = (stno >= rhsInt); break;
                    case CondOp::LT:  result = (stno < rhsInt); break;
                    case CondOp::LTE: result = (stno <= rhsInt); break;
                    default: result = (stno == rhsInt);
                }
                break;
            }

            case CondType::PREVSTATENO: {
                int prev = fighter.getPreviousStateNo();
                switch (op) {
                    case CondOp::EQ:  result = (prev == rhsInt); break;
                    case CondOp::NEQ: result = (prev != rhsInt); break;
                    default: result = (prev == rhsInt);
                }
                break;
            }

            case CondType::PARENT_STATENO: {
                int ps = fighter.getCurrentStateNo();
                switch (op) {
                    case CondOp::EQ:  result = (ps == rhsInt); break;
                    case CondOp::NEQ: result = (ps != rhsInt); break;
                    default: result = (ps == rhsInt);
                }
                break;
            }

            case CondType::MOVECONTACT: {
                result = fighter.hasMoveContact();
                break;
            }

            case CondType::MOVEHIT: {
                result = fighter.hasMoveHit();
                break;
            }

            case CondType::HITOVER: {
                result = fighter.isHitOver();
                break;
            }

            case CondType::HITSHAKEOVER: {
                result = fighter.isHitShakeOver();
                break;
            }

            case CondType::ALIVE: {
                result = !fighter.isDead();
                break;
            }

            case CondType::LIFE: {
                int life = fighter.getCurrentLife();
                switch (op) {
                    case CondOp::EQ:  result = (life == rhsInt); break;
                    case CondOp::NEQ: result = (life != rhsInt); break;
                    case CondOp::GT:  result = (life > rhsInt); break;
                    case CondOp::GTE: result = (life >= rhsInt); break;
                    case CondOp::LT:  result = (life < rhsInt); break;
                    case CondOp::LTE: result = (life <= rhsInt); break;
                    default: result = (life == rhsInt);
                }
                break;
            }

            case CondType::POWER: {
                int power = fighter.getPower();
                switch (op) {
                    case CondOp::EQ:  result = (power == rhsInt); break;
                    case CondOp::NEQ: result = (power != rhsInt); break;
                    case CondOp::GT:  result = (power > rhsInt); break;
                    case CondOp::GTE: result = (power >= rhsInt); break;
                    case CondOp::LT:  result = (power < rhsInt); break;
                    case CondOp::LTE: result = (power <= rhsInt); break;
                    default: result = (power >= rhsInt);
                }
                break;
            }

            case CondType::VEL_X: {
                float vx = fighter.getVelocityX();
                // M.U.G.E.N: vel x 是相对于角色朝向
                if (!fighter.isFacingRight()) { vx = -vx; }
                switch (op) {
                    case CondOp::GT:  result = (vx > rhsFloat); break;
                    case CondOp::GTE: result = (vx >= rhsFloat); break;
                    case CondOp::LT:  result = (vx < rhsFloat); break;
                    case CondOp::LTE: result = (vx <= rhsFloat); break;
                    case CondOp::EQ:  result = (std::abs(vx - rhsFloat) < 0.1f); break;
                    case CondOp::NEQ: result = (std::abs(vx - rhsFloat) >= 0.1f); break;
                    default: result = (vx > 0);
                }
                break;
            }

            case CondType::VEL_Y: {
                float vy = fighter.getVelocityY();
                switch (op) {
                    case CondOp::GT:  result = (vy > rhsFloat); break;
                    case CondOp::GTE: result = (vy >= rhsFloat); break;
                    case CondOp::LT:  result = (vy < rhsFloat); break;
                    case CondOp::LTE: result = (vy <= rhsFloat); break;
                    case CondOp::EQ:  result = (std::abs(vy - rhsFloat) < 0.1f); break;
                    case CondOp::NEQ: result = (std::abs(vy - rhsFloat) >= 0.1f); break;
                    default: result = (vy > 0);
                }
                break;
            }

            case CondType::POS_Y: {
                float py = fighter.getPosition().y;
                switch (op) {
                    case CondOp::EQ:  result = (std::abs(py - rhsFloat) < 0.1f); break;
                    case CondOp::GT:  result = (py > rhsFloat); break;
                    case CondOp::GTE: result = (py >= rhsFloat); break;
                    case CondOp::LT:  result = (py < rhsFloat); break;
                    case CondOp::LTE: result = (py <= rhsFloat); break;
                    default: result = (py == 0);
                }
                break;
            }

            case CondType::ANIM: {
                int anim = fighter.getCurrentAnimId();
                switch (op) {
                    case CondOp::EQ:  result = (anim == rhsInt); break;
                    case CondOp::NEQ: result = (anim != rhsInt); break;
                    default: result = (anim == rhsInt);
                }
                break;
            }

            case CondType::SELFANIMEXIST: {
                result = fighter.doesAnimExist(rhsInt);
                break;
            }

            case CondType::ROUNDSTATE: {
                int rs = fighter.getRoundState();
                switch (op) {
                    case CondOp::EQ:  result = (rs == rhsInt); break;
                    case CondOp::NEQ: result = (rs != rhsInt); break;
                    default: result = (rs == rhsInt); break;
                }
                break;
            }

            case CondType::ROUNDNO: {
                int rn = fighter.getRoundNo();
                switch (op) {
                    case CondOp::EQ:  result = (rn == rhsInt); break;
                    case CondOp::NEQ: result = (rn != rhsInt); break;
                    default: result = (rn == rhsInt); break;
                }
                break;
            }
            case CondType::INGUARDDIST: {
                result = fighter.isInGuardDist();
                break;
            }

            case CondType::CANRECOVER: {
                result = fighter.canRecover();
                break;
            }

            case CondType::HITFALL: {
                result = fighter.getHitVar("fall") != 0;
                break;
            }

            case CondType::MOVETYPE: {
                result = (fighter.getMoveType() == rhsInt);
                break;
            }

            case CondType::GETHITVAR: {
                int val = fighter.getHitVar(paramStr);
                if (isRange) {
                    bool inRange = (val >= rangeLow && val <= rangeHigh);
                    switch (op) {
                        case CondOp::EQ:  result = inRange; break;
                        case CondOp::NEQ: result = !inRange; break;
                        default: result = inRange; break;
                    }
                } else {
                    switch (op) {
                        case CondOp::EQ:  result = (val == rhsInt); break;
                        case CondOp::NEQ: result = (val != rhsInt); break;
                        case CondOp::GT:  result = (val > rhsInt); break;
                        case CondOp::GTE: result = (val >= rhsInt); break;
                        case CondOp::LT:  result = (val < rhsInt); break;
                        case CondOp::LTE: result = (val <= rhsInt); break;
                        default: result = (val != 0); break;
                    }
                }
                break;
            }

            case CondType::P2STATETYPE: {
                // 简化: 假设对手是站立状态
                result = (1 == statetypeToInt(rhsStr)); // 默认 C
                break;
            }

            case CondType::P2DIST_X: {
                // 简化: 默认距离为 999
                result = false;
                break;
            }

            case CondType::RANDOM: {
                int r = std::rand() % 1000;
                switch (op) {
                    case CondOp::EQ:  result = (r == rhsInt); break;
                    case CondOp::NEQ: result = (r != rhsInt); break;
                    case CondOp::GT:  result = (r > rhsInt); break;
                    case CondOp::GTE: result = (r >= rhsInt); break;
                    case CondOp::LT:  result = (r < rhsInt); break;
                    case CondOp::LTE: result = (r <= rhsInt); break;
                    default: result = (r < rhsInt); break;
                }
                break;
            }

            case CondType::TIMEMOD: {
                int time = fighter.getStateTime();
                int mod = rhsInt;
                int eq = paramInt;
                result = (mod > 0) && (time % mod == eq);
                break;
            }

            case CondType::CONST: {
                // const() 简化处理 — 返回默认 true
                result = true;
                break;
            }

            default:
                result = true; // 未实现的条件默认返回 true
                break;
        }

        if (negated) result = !result;
        return result;
    }

    // ==========================================
    // 条件解析
    // ==========================================

    static std::string trimStr(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static CondOp parseOp(const std::string& s) {
        if (s == "=")       return CondOp::EQ;
        if (s == "!=")      return CondOp::NEQ;
        if (s == ">")       return CondOp::GT;
        if (s == ">=")      return CondOp::GTE;
        if (s == "<")       return CondOp::LT;
        if (s == "<=")      return CondOp::LTE;
        return CondOp::NONE;
    }

    static bool isSimpleNumber(const std::string& s) {
        if (s.empty()) return false;
        size_t start = 0;
        if (s[0] == '-') start = 1;
        for (size_t i = start; i < s.size(); i++) {
            if (!std::isdigit(s[i]) && s[i] != '.') return false;
        }
        return true;
    }

    std::vector<Condition> parseConditionString(const std::string& condStr) {
        std::vector<Condition> result;
        std::string str = trimStr(condStr);
        if (str.empty()) return result;

        // 检查是否有 NOT (!)
        bool negated = false;
        if (str[0] == '!') {
            negated = true;
            str = trimStr(str.substr(1));
        }

        // 去掉整个表达式的外层括号: "(statetype = a)" → "statetype = a"
        while (str.size() >= 2 && str.front() == '(' && str.back() == ')') {
            str = trimStr(str.substr(1, str.size() - 2));
        }

        // 简单布尔条件 (无操作符)
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        Condition cond;
        cond.negated = negated;

        // 先检查是否有比较操作符
        std::string opStr;
        size_t opPos = std::string::npos;

        // 查找操作符位置
        for (const auto& opC : {"<=", ">=", "!=", "=", "<", ">"}) {
            size_t pos = str.find(opC);
            if (pos != std::string::npos) {
                // 确保 opC 不是被引号括起来的
                bool inQuote = false;
                for (size_t i = 0; i < pos; i++) {
                    if (str[i] == '"') inQuote = !inQuote;
                }
                if (!inQuote) {
                    opStr = opC;
                    opPos = pos;
                    break;
                }
            }
        }

        if (opPos != std::string::npos) {
            std::string lhs = trimStr(str.substr(0, opPos));
            // 去掉括号 (M.U.G.E.N 允许 (statetype = s))
            while (lhs.size() >= 2 && lhs.front() == '(' && lhs.back() == ')') {
                lhs = trimStr(lhs.substr(1, lhs.size() - 2));
            }
            std::string opPart = opStr;
            std::string rhs = trimStr(str.substr(opPos + opStr.size()));

            // 去掉引号
            if (rhs.size() >= 2 && rhs[0] == '"' && rhs.back() == '"') {
                rhs = rhs.substr(1, rhs.size() - 2);
            }

            // 如果 rhs 是字符串 (带引号) 或者无法转换为数字
            bool isNumeric = isSimpleNumber(rhs);

            cond.op = parseOp(opPart);

            std::string lhsLower = lhs;
            std::transform(lhsLower.begin(), lhsLower.end(), lhsLower.begin(), ::tolower);

            // === 统一区间语法处理: [low, high] (M.U.G.E.N 1.0 标准) ===
            if (rhs.size() >= 2 && rhs[0] == '[' && rhs.back() == ']') {
                cond.isRange = true;
                std::string rangeContent = rhs.substr(1, rhs.size() - 2);
                size_t commaPos = rangeContent.find(',');
                if (commaPos != std::string::npos) {
                    try { cond.rangeLow = std::stoi(rangeContent.substr(0, commaPos)); } catch(...) {}
                    try { cond.rangeHigh = std::stoi(rangeContent.substr(commaPos + 1)); } catch(...) {}
                } else if (!isSimpleNumber(rangeContent)) {
                    // [ground.SlideTime] 等表达式: 运行时求值
                    cond.rangeExpr = rangeContent;
                }
                if (!cond.rangeExpr.empty()) {
                    rhs = "0"; // 占位, 运行时由 evaluate 处理
                } else {
                    // 区间条件下，所有 stoi/stof 都用 rangeLow 替代，防止异常
                    rhs = std::to_string(cond.rangeLow);
                }
            }

            // 匹配左侧的变量名
            if (lhsLower == "time") {
                cond.type = CondType::TIME;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "animelem") {
                cond.type = CondType::ANIMELEM;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "animtime") {
                cond.type = CondType::ANIMTIME;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "command") {
                cond.type = CondType::COMMAND;
                cond.rhsStr = rhs;
            }
            else if (lhsLower == "statetype") {
                cond.type = CondType::STATETYPE;
                cond.rhsStr = rhs;
            }
            else if (lhsLower == "stateno") {
                cond.type = CondType::STATENO;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "prevstateno") {
                cond.type = CondType::PREVSTATENO;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "parent,stateno") {
                cond.type = CondType::PARENT_STATENO;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "power") {
                cond.type = CondType::POWER;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "life") {
                cond.type = CondType::LIFE;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "roundstate") {
                cond.type = CondType::ROUNDSTATE;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "anim") {
                cond.type = CondType::ANIM;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "random") {
                cond.type = CondType::RANDOM;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "selfanimexist") {
                cond.type = CondType::SELFANIMEXIST;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "vel x" || lhsLower == "velx") {
                cond.type = CondType::VEL_X;
                try { cond.rhsFloat = std::stof(rhs); } catch(...) { cond.rhsFloat = 0.f; }
            }
            else if (lhsLower == "vel y" || lhsLower == "vely") {
                cond.type = CondType::VEL_Y;
                try { cond.rhsFloat = std::stof(rhs); } catch(...) { cond.rhsFloat = 0.f; }
            }
            else if (lhsLower == "pos y" || lhsLower == "posy") {
                cond.type = CondType::POS_Y;
                try { cond.rhsFloat = std::stof(rhs); } catch(...) { cond.rhsFloat = 0.f; }
            }
            else if (lhsLower == "pos x" || lhsLower == "posx") {
                cond.type = CondType::POS_X;
                try { cond.rhsFloat = std::stof(rhs); } catch(...) { cond.rhsFloat = 0.f; }
            }
            else if (lhsLower == "roundno") {
                cond.type = CondType::ROUNDNO;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "movetype") {
                cond.type = CondType::MOVETYPE;
                std::string rLower = rhs;
                std::transform(rLower.begin(), rLower.end(), rLower.begin(), ::tolower);
                if (rLower == "i") cond.rhsInt = 0;
                else if (rLower == "a") cond.rhsInt = 1;
                else if (rLower == "h") cond.rhsInt = 2;
                else try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower.find("gethitvar") != std::string::npos) {
                cond.type = CondType::GETHITVAR;
                // 从 LHS 中提取参数名: "GetHitVar(yvel)" → "yvel"
                size_t parenL = lhs.find('(');
                size_t parenR = lhs.find(')');
                if (parenL != std::string::npos && parenR != std::string::npos && parenR > parenL) {
                    cond.paramStr = trimStr(lhs.substr(parenL + 1, parenR - parenL - 1));
                } else {
                    cond.paramStr = lhs;
                }
                if (!cond.isRange && isNumeric) {
                    try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
                }
            }
            else if (lhsLower == "timemod") {
                cond.type = CondType::TIMEMOD;
                // timemod = N,M 格式
                size_t comma = rhs.find(',');
                if (comma != std::string::npos) {
                    try { cond.rhsInt = std::stoi(rhs.substr(0, comma)); } catch(...) { cond.rhsInt = 0; }
                    try { cond.paramInt = std::stoi(rhs.substr(comma + 1)); } catch(...) { cond.paramInt = 0; }
                } else {
                    try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
                    cond.paramInt = 1;
                }
            }
            else if (lhsLower == "ailevel") {
                cond.type = CondType::AILEVEL;
                try { cond.rhsInt = std::stoi(rhs); } catch(...) { cond.rhsInt = 0; }
            }
            else if (lhsLower == "p2bodydist x") {
                cond.type = CondType::P2BODYDIST_X;
                try { cond.rhsFloat = std::stof(rhs); } catch(...) { cond.rhsFloat = 0.f; }
            }
            else if (lhsLower == "movehit")         cond.type = CondType::MOVEHIT;
            else if (lhsLower == "movecontact")     cond.type = CondType::MOVECONTACT;
            else if (lhsLower == "ctrl")            cond.type = CondType::CTRL;
            else if (lhsLower == "alive")           cond.type = CondType::ALIVE;
            else if (lhsLower == "moveguarded")     cond.type = CondType::MOVEGUARDED;
            else if (lhsLower == "hitover")         cond.type = CondType::HITOVER;
            else if (lhsLower == "hitshakeover")    cond.type = CondType::HITSHAKEOVER;
            else if (lhsLower == "canrecover")      cond.type = CondType::CANRECOVER;
            else if (lhsLower == "inguarddist")     cond.type = CondType::INGUARDDIST;
            else if (lhsLower == "matchover")       cond.type = CondType::MATCHOVER;
            else if (lhsLower == "hitfall")         cond.type = CondType::HITFALL;
            else if (lhsLower.find("ifelse") != std::string::npos) {
                // ifelse 在 CNS 中作为函数参数使用，这里简化处理
                cond.type = CondType::LITERAL_TRUE;
            }
            else {
                // 未知条件，默认为 true
                cond.type = CondType::LITERAL_TRUE;
            }
        } else {
            // 无操作符 — 布尔条件
            cond.op = CondOp::NONE;

            if (lower == "ctrl")                cond.type = CondType::CTRL;
            else if (lower == "movecontact")    cond.type = CondType::MOVECONTACT;
            else if (lower == "movehit")        cond.type = CondType::MOVEHIT;
            else if (lower == "moveguarded")    cond.type = CondType::MOVEGUARDED;
            else if (lower == "hitover")        cond.type = CondType::HITOVER;
            else if (lower == "hitshakeover")   cond.type = CondType::HITSHAKEOVER;
            else if (lower == "alive")          cond.type = CondType::ALIVE;
            else if (lower == "inguarddist")    cond.type = CondType::INGUARDDIST;
            else if (lower == "canrecover")     cond.type = CondType::CANRECOVER;
            else if (lower == "matchover")      cond.type = CondType::MATCHOVER;
            else if (lower == "hitfall")        cond.type = CondType::HITFALL;
            else if (lower == "1" || lower == "true") cond.type = CondType::LITERAL_TRUE;
            else {
                // 可能是一个数字常量 (如 1)
                if (isSimpleNumber(lower)) {
                    cond.type = CondType::LITERAL_TRUE;
                } else {
                    cond.type = CondType::LITERAL_TRUE; // 默认为 true
                }
            }
        }

        result.push_back(cond);
        return result;
    }

    // ==========================================
    // 具体控制器实现
    // ==========================================

    // 前向声明: 简易 CNS 表达式求值
    // --- ChangeState ---
    ChangeStateController::ChangeStateController() {
        type = ControllerType::CHANGESTATE;
    }
    void ChangeStateController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        int targetState = value;
        if (!valueStr.empty()) {
            targetState = evaluateCNSExpression(valueStr, fighter);
        }
        fighter.requestStateChange(targetState);
    }

    // --- ChangeAnim ---
    // 简易 CNS 表达式求值: 支持 ifelse(), GetHitVar(), statetype, 数字, +, *, p2dist
    int evaluateCNSExpression(const std::string& expr, const Fighter& fighter);

    // 辅助: 判断布尔条件并返回 0 或 1
    static int evaluateCNSBool(const std::string& cond, const Fighter& fighter) {
        std::string s = trimStr(cond);
        if (s.empty()) return 0;

        // 去掉外层括号
        while (s.size() >= 2 && s.front() == '(' && s.back() == ')') {
            s = trimStr(s.substr(1, s.size() - 2));
        }

        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // 纯布尔变量
        if (lower == "ctrl")        return fighter.hasControl() ? 1 : 0;
        if (lower == "movecontact") return fighter.hasMoveContact() ? 1 : 0;
        if (lower == "movehit")     return fighter.hasMoveHit() ? 1 : 0;
        if (lower == "inguarddist") return fighter.isInGuardDist() ? 1 : 0;
        if (lower == "alive")       return !fighter.isDead() ? 1 : 0;
        if (lower == "hitover")     return fighter.isHitOver() ? 1 : 0;
        if (lower == "hitshakeover") return fighter.isHitShakeOver() ? 1 : 0;

        // 包含比较操作符: "statetype = C", "command != holddown"
        for (const auto& opC : {"!=", "=", "<=", ">=", "<", ">"}) {
            size_t opPos = s.find(opC);
            if (opPos != std::string::npos) {
                std::string lhs = trimStr(s.substr(0, opPos));
                std::string rhs = trimStr(s.substr(opPos + strlen(opC)));
                std::string lhsLower = lhs;
                std::transform(lhsLower.begin(), lhsLower.end(), lhsLower.begin(), ::tolower);
                std::string rhsLower = rhs;
                std::transform(rhsLower.begin(), rhsLower.end(), rhsLower.begin(), ::tolower);
                // 去掉 rhs 引号
                if (rhs.size() >= 2 && rhs[0] == '"' && rhs.back() == '"') {
                    rhs = rhs.substr(1, rhs.size() - 2);
                    rhsLower = rhs;
                    std::transform(rhsLower.begin(), rhsLower.end(), rhsLower.begin(), ::tolower);
                }

                int lhsVal = evaluateCNSExpression(lhs, fighter);
                int rhsVal = evaluateCNSExpression(rhs, fighter);

                if (lhsLower == "statetype") {
                    int st = fighter.getStateType(); // 0=S, 1=C, 2=A
                    if (rhsLower == "s") rhsVal = 0;
                    else if (rhsLower == "c") rhsVal = 1;
                    else if (rhsLower == "a") rhsVal = 2;
                    lhsVal = st;
                }

                bool result = false;
                std::string opStr = opC;
                if (opStr == "=")  result = (lhsVal == rhsVal);
                else if (opStr == "!=") result = (lhsVal != rhsVal);
                else if (opStr == ">")  result = (lhsVal > rhsVal);
                else if (opStr == "<")  result = (lhsVal < rhsVal);
                else if (opStr == ">=") result = (lhsVal >= rhsVal);
                else if (opStr == "<=") result = (lhsVal <= rhsVal);
                return result ? 1 : 0;
            }
        }

        // 纯数字
        try { return std::stoi(s); } catch (...) {}
        return 0;
    }

    // 辅助: 解析单个因子 (数字, GetHitVar, ifelse, 或括号表达式)
    static int evaluateCNSFactor(const std::string& factor, const Fighter& fighter) {
        std::string s = trimStr(factor);
        if (s.empty()) return 0;

        // 前面有 ! 号
        bool negated = false;
        if (s[0] == '!') {
            negated = true;
            s = trimStr(s.substr(1));
        }

        // 去掉外层括号 → 布尔条件
        if (s.size() >= 2 && s.front() == '(' && s.back() == ')') {
            int val = evaluateCNSBool(s, fighter);
            return negated ? (val ? 0 : 1) : val;
        }

        // ifelse(cond, trueVal, falseVal)
        size_t ifelsePos = s.find("ifelse");
        if (ifelsePos != std::string::npos) {
            size_t parenL = s.find('(', ifelsePos);
            size_t parenR = s.rfind(')');
            if (parenL != std::string::npos && parenR != std::string::npos && parenR > parenL) {
                std::string args = s.substr(parenL + 1, parenR - parenL - 1);
                std::vector<std::string> parts;
                int depth = 0;
                size_t start = 0;
                for (size_t i = 0; i < args.size(); i++) {
                    if (args[i] == '(') depth++;
                    else if (args[i] == ')') depth--;
                    else if (args[i] == ',' && depth == 0) {
                        parts.push_back(trimStr(args.substr(start, i - start)));
                        start = i + 1;
                    }
                }
                parts.push_back(trimStr(args.substr(start)));
                if (parts.size() >= 3) {
                    int condVal = evaluateCNSBool(parts[0], fighter);
                    int trueVal = evaluateCNSExpression(parts[1], fighter);
                    int falseVal = evaluateCNSExpression(parts[2], fighter);
                    return negated ? - (condVal ? trueVal : falseVal) : (condVal ? trueVal : falseVal);
                }
            }
        }

        // GetHitVar(name)
        size_t ghvPos = s.find("GetHitVar");
        if (ghvPos != std::string::npos) {
            size_t parenL = s.find('(', ghvPos);
            size_t parenR = s.find(')', parenL);
            if (parenL != std::string::npos && parenR != std::string::npos && parenR > parenL) {
                std::string param = trimStr(s.substr(parenL + 1, parenR - parenL - 1));
                int val = fighter.getHitVar(param);
                return negated ? -val : val;
            }
        }

        // statetype 关键字 → 返回数值
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == "statetype") {
                return fighter.getStateType(); // 0=S, 1=C, 2=A
            }
        }

        // p2dist x / p2dist y → 到对手的距离
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == "p2dist x") {
                sf::Vector2f opp = fighter.getOpponentPos();
                float dist = opp.x - fighter.getPosition().x;
                return negated ? -static_cast<int>(dist) : static_cast<int>(dist);
            }
            if (lower == "p2dist y") {
                sf::Vector2f opp = fighter.getOpponentPos();
                float dist = opp.y - fighter.getPosition().y;
                return negated ? -static_cast<int>(dist) : static_cast<int>(dist);
            }
        }

        // const(path) → 查找速度/运动常量 (返回 raw px/tick)
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.starts_with("const(") && s.back() == ')') {
                std::string path = s.substr(6, s.size() - 7);
                float rawValue = fighter.getConstValue(path);
                int val = static_cast<int>(rawValue);
                return negated ? -val : val;
            }
        }

        // 数字
        try {
            int val = std::stoi(s);
            return negated ? -val : val;
        } catch (...) {}
        return 0;
    }

    // 主表达式求值: 处理 + 和 *
    int evaluateCNSExpression(const std::string& expr, const Fighter& fighter) {
        std::string s = trimStr(expr);
        if (s.empty()) return 0;

        // 先按 + 分割
        int total = 0;
        size_t start = 0;
        int depth = 0;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') depth--;
            else if (s[i] == '+' && depth == 0) {
                total += evaluateCNSExpression(s.substr(start, i - start), fighter);
                start = i + 1;
            }
        }
        if (start == 0) {
            // 没有 +: 按 * 分割
            total = 1;
            size_t mulStart = 0;
            for (size_t i = 0; i < s.size(); i++) {
                if (s[i] == '(') depth++;
                else if (s[i] == ')') depth--;
                else if (s[i] == '*' && depth == 0) {
                    total *= evaluateCNSFactor(s.substr(mulStart, i - mulStart), fighter);
                    mulStart = i + 1;
                }
            }
            total *= evaluateCNSFactor(s.substr(mulStart), fighter);
        } else {
            // 有 +: 递归处理剩余部分
            total += evaluateCNSExpression(s.substr(start), fighter);
        }
        return total;
    }

    ChangeAnimController::ChangeAnimController() {
        type = ControllerType::CHANGEANIM;
    }
    void ChangeAnimController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        int animId = value;
        if (!valueStr.empty()) {
            animId = evaluateCNSExpression(valueStr, fighter);
        }
        if (animId >= 0 && fighter.doesAnimExist(animId)) {
            fighter.switchAnimation(animId);
        }
    }

    // --- VelSet ---
    VelSetController::VelSetController() {
        type = ControllerType::VELSET;
    }
    void VelSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        if (hasX) {
            float vx = valueX;
            if (!valueXStr.empty()) {
                vx = static_cast<float>(evaluateCNSExpression(valueXStr, fighter));
            }
            // M.U.G.E.N: 正X=向前，面朝左时翻转
            if (!fighter.isFacingRight()) { vx = -vx; }
            fighter.setVelocityX(vx * 60.f);
        }
        if (hasY) {
            float vy = valueY;
            if (!valueYStr.empty()) {
                vy = static_cast<float>(evaluateCNSExpression(valueYStr, fighter));
            }
            fighter.setVelocityY(vy * 60.f);  // px/tick → px/s
        }
    }

    // --- VelAdd ---
    VelAddController::VelAddController() {
        type = ControllerType::VELADD;
    }
    void VelAddController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        float addX = valueX;
        if (!fighter.isFacingRight()) { addX = -addX; }
        fighter.addVelocity(addX, valueY);
    }

    // --- VelMul ---
    VelMulController::VelMulController() {
        type = ControllerType::VELMUL;
    }
    void VelMulController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        float vx = fighter.getVelocityX() * valueX;
        float vy = fighter.getVelocityY() * valueY;
        fighter.setVelocityX(vx);
        fighter.setVelocityY(vy);
    }

    // --- StateTypeSet ---
    StateTypeSetController::StateTypeSetController() {
        type = ControllerType::STATETYPESET;
    }
    void StateTypeSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // statetype/physics/movetype 通过 paramStr 设置
        // 格式: "statetype = S" 或 "physics = N" 等
        fighter.setStateType(value);
    }

    // --- CtrlSet ---
    CtrlSetController::CtrlSetController() {
        type = ControllerType::CTRLSET;
    }
    void CtrlSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.setControl(value != 0);
    }

    // --- PosSet ---
    PosSetController::PosSetController() {
        type = ControllerType::POSSET;
    }
    void PosSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        float setX = valueX;
        float setY = valueY;
        if (!valueXStr.empty()) setX = static_cast<float>(evaluateCNSExpression(valueXStr, fighter));
        if (!valueYStr.empty()) setY = static_cast<float>(evaluateCNSExpression(valueYStr, fighter));
        fighter.setPosition(fighter.getPosition().x + setX, fighter.getPosition().y + setY);
    }

    // --- PosAdd ---
    PosAddController::PosAddController() {
        type = ControllerType::POSADD;
    }
    void PosAddController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        float addX = valueX;
        float addY = valueY;
        if (!valueXStr.empty()) addX = static_cast<float>(evaluateCNSExpression(valueXStr, fighter));
        if (!valueYStr.empty()) addY = static_cast<float>(evaluateCNSExpression(valueYStr, fighter));
        fighter.addPositionX(addX);
        fighter.addPositionY(addY);
    }

    // --- PowerAdd ---
    PowerAddController::PowerAddController() {
        type = ControllerType::POWERADD;
    }
    void PowerAddController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.addPower(value);
    }

    // --- PlaySnd ---
    PlaySndController::PlaySndController() {
        type = ControllerType::PLAYSND;
    }
    void PlaySndController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 播放音效 (简化: 只打印日志)
        // value = sndGroup, paramInt = sndIndex
        // std::cout << "[PlaySnd] Group: " << value << ", Index: " << paramInt << std::endl;
    }

    // --- HitDef ---
    HitDefController::HitDefController() {
        type = ControllerType::HITDEF;
    }
    void HitDefController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // HitDef 由 Game::checkCombat() 处理, 这里不需要做任何事
        // 触发条件由 trigger 系统管理
    }

    // --- VarSet ---
    VarSetController::VarSetController() {
        type = ControllerType::VARSET;
    }
    void VarSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 设置变量 (简化: 仅支持 sysvar)
        if (paramStr.find("sysvar") != std::string::npos || value == 0) {
            fighter.setSysVar(paramInt, value);
        }
    }

    // --- VarAdd ---
    VarAddController::VarAddController() {
        type = ControllerType::VARADD;
    }
    void VarAddController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        int cur = fighter.getSysVar(paramInt);
        fighter.setSysVar(paramInt, cur + value);
    }

    // --- NotHitBy ---
    NotHitByController::NotHitByController() {
        type = ControllerType::NOTHITBY;
    }
    void NotHitByController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 设置不可命中状态
        fighter.setNotHitBy(valueStr);
    }

    // --- Turn ---
    TurnController::TurnController() {
        type = ControllerType::TURN;
    }
    void TurnController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.setFacingRight(!fighter.isFacingRight());
    }

    // --- SelfState ---
    SelfStateController::SelfStateController() {
        type = ControllerType::SELFSTATE;
    }
    void SelfStateController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.requestStateChange(value);
    }

    // --- Null ---
    NullController::NullController() {
        type = ControllerType::NULL_CTRL;
    }
    void NullController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 什么都不做
    }

    // ==========================================
    // EnvShakeController
    // ==========================================
    EnvShakeController::EnvShakeController() {
        type = ControllerType::ENVSHAKE;
    }
    void EnvShakeController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "time") {
            try { m_shakeTime = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "ampl") {
            try { m_shakeAmpl = std::stoi(value); } catch (...) {}
        }
    }
    void EnvShakeController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.setShake(m_shakeTime, m_shakeAmpl);
    }

    // ==========================================
    // FallEnvShakeController
    // ==========================================
    FallEnvShakeController::FallEnvShakeController() {
        type = ControllerType::FALLENVSHAKE;
    }
    void FallEnvShakeController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 使用命中信息中的 envshakeTime
        int time = fighter.getHitInfo().envshakeTime;
        if (time > 0) {
            fighter.setShake(time, 4);  // 默认振幅 4
        }
    }

    // ==========================================
    // ExplodController
    // ==========================================
    ExplodController::ExplodController() {
        type = ControllerType::EXPLOD;
    }
    void ExplodController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "anim") {
            try { m_anim = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "id") {
            try { m_id = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "pos") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_posX = std::stof(value.substr(0, comma)); } catch (...) {}
                try { m_posY = std::stof(value.substr(comma + 1)); } catch (...) {}
            }
        } else if (lowerKey == "postype") {
            m_postype = value;
            std::transform(m_postype.begin(), m_postype.end(), m_postype.begin(), ::tolower);
        } else if (lowerKey == "facing") {
            try { m_facing = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "vfacing") {
            try { m_vfacing = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "bindtime") {
            try { m_bindtime = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "vel") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_velX = std::stof(value.substr(0, comma)); } catch (...) {}
                try { m_velY = std::stof(value.substr(comma + 1)); } catch (...) {}
            }
        } else if (lowerKey == "removetime") {
            try { m_removetime = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "sprpriority") {
            try { m_sprpriority = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "removeongethit") {
            try { m_removeOnGetHit = (std::stoi(value) != 0); } catch (...) {}
        } else if (lowerKey == "scale") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_scaleX = std::stof(value.substr(0, comma)); } catch (...) {}
                try { m_scaleY = std::stof(value.substr(comma + 1)); } catch (...) {}
            }
        }
    }
    void ExplodController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        if (m_anim < 0) return;
        const Animation* anim = fighter.getAnimation(m_anim);
        if (!anim) return;

        ExplodInstance explod;
        explod.animPlayer.play(*anim);
        explod.id = m_id;
        explod.bindtime = m_bindtime;
        explod.removetime = m_removetime;
        explod.sprpriority = m_sprpriority;
        explod.removeOnGetHit = m_removeOnGetHit;
        explod.pos = {m_posX, m_posY};
        explod.vel = {m_velX, m_velY};
        explod.facing = m_facing;  // 1=same as parent, -1=opposite

        // facing: 1 means same direction as parent
        bool parentFacing = fighter.isFacingRight();
        explod.animPlayer.setFacingRight(explod.facing == 1 ? parentFacing : !parentFacing);

        // Scale (from CNS, default 1,1)
        explod.animPlayer.setScale(m_scaleX, m_scaleY);

        // Explod 位置 = fighter 轴位置 + CNS pos 偏移
        // draw() 会自动处理精灵轴对齐 (axisX/axisY + scale)
        sf::Vector2f fPos = fighter.getPosition();
        float dir = parentFacing ? 1.f : -1.f;

        if (m_postype == "p1") {
            explod.currentPos = {fPos.x + m_posX * dir, fPos.y + m_posY};
        } else if (m_postype == "p2") {
            explod.currentPos = {fPos.x + m_posX * dir, fPos.y + m_posY};
        } else if (m_postype == "front" || m_postype == "back" || m_postype == "left" || m_postype == "right") {
            explod.currentPos = {m_posX, m_posY};
        } else {
            explod.currentPos = {m_posX, m_posY};
        }

        fighter.addExplod(std::move(explod));
    }

    // ==========================================
    // RemoveExplodController
    // ==========================================
    RemoveExplodController::RemoveExplodController() {
        type = ControllerType::REMOVEEXPLOD;
    }
    void RemoveExplodController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "id") {
            try { m_id = std::stoi(value); } catch (...) {}
        }
    }
    void RemoveExplodController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.removeExplod(m_id);
    }

    // ==========================================
    // HelperController
    // ==========================================
    HelperController::HelperController() {
        type = ControllerType::HELPER;
    }
    void HelperController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "stateno") {
            try { m_stateno = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "id") {
            try { m_id = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "pos") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_posX = std::stof(value.substr(0, comma)); } catch (...) {}
                try { m_posY = std::stof(value.substr(comma + 1)); } catch (...) {}
            }
        } else if (lowerKey == "postype") {
            m_postype = value;
            std::transform(m_postype.begin(), m_postype.end(), m_postype.begin(), ::tolower);
        } else if (lowerKey == "facing") {
            try { m_facing = std::stoi(value); } catch (...) {}
        }
    }
    void HelperController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        if (m_stateno <= 0) return;

        // 从 StateDef 查找动画和速度
        int animId = m_stateno;
        float velX = 0.f, velY = 0.f;
        fighter.getStateDefAttributes(m_stateno, animId, velX, velY);

        // 计算初始位置 (相对于父级)
        sf::Vector2f fPos = fighter.getPosition();
        float dir = fighter.isFacingRight() ? 1.f : -1.f;
        sf::Vector2f pos = {fPos.x + m_posX * dir, fPos.y + m_posY};

        Fighter::PendingHelper ph;
        ph.id = m_id;
        ph.animId = animId;
        ph.stateNo = m_stateno;
        ph.position = pos;
        ph.velocity = {velX, velY};  // px/tick
        std::cout << "[HelperCtrl] fire state=" << m_stateno
                  << " parentState=" << fighter.getCurrentStateNo()
                  << " moveContact=" << fighter.hasMoveContact()
                  << " moveHit=" << fighter.hasMoveHit() << std::endl;
        ph.facingRight = (m_facing == 1) ? fighter.isFacingRight() : !fighter.isFacingRight();
        ph.parentStateno = fighter.getCurrentStateNo();
        // 从攻击者的 HitDef 取伤害和火花
        const auto& hitDefs = fighter.getCurrentHitDefs();
        if (!hitDefs.empty()) {
            ph.damage = hitDefs[0].damage;
            ph.sparkno = hitDefs[0].sparkno;
        }
        fighter.addPendingHelper(ph);
    }

    // ==========================================
    // AfterImageController
    // ==========================================
    AfterImageController::AfterImageController() {
        type = ControllerType::AFTERIMAGE;
    }
    void AfterImageController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "time") {
            try { m_time = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "timegap") {
            try { m_timegap = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "framegap") {
            try { m_framegap = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "length") {
            try { m_length = std::stoi(value); } catch (...) {}
        }
    }
    void AfterImageController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.setAfterImage(m_time, m_timegap, m_length);
    }

    // ==========================================
    // SuperPauseController
    // ==========================================
    SuperPauseController::SuperPauseController() {
        type = ControllerType::SUPERPAUSE;
    }
    void SuperPauseController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "time") {
            try { m_time = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "darken") {
            try { m_darken = std::stoi(value); } catch (...) {}
        } else if (lowerKey == "pos") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_posX = std::stoi(value.substr(0, comma)); } catch (...) {}
                try { m_posY = std::stoi(value.substr(comma + 1)); } catch (...) {}
            }
        }
    }
    void SuperPauseController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // SuperPause 效果由 Game 层处理，这里标记给 Game
        // 通过 Fighter 上的方法设置 SuperPause
        fighter.setSuperPause(m_time, m_darken != 0);
    }

    // ==========================================
    // BindToRootController
    // ==========================================
    BindToRootController::BindToRootController() {
        type = ControllerType::BIND_TO_ROOT;
    }
    void BindToRootController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "pos") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_posX = std::stoi(value.substr(0, comma)); } catch (...) {}
                try { m_posY = std::stoi(value.substr(comma + 1)); } catch (...) {}
            }
        }
    }

    // ==========================================
    // DestroySelfController (Helper 自毁)
    // ==========================================
    DestroySelfController::DestroySelfController() {
        type = ControllerType::DESTROY_SELF;
    }
    void DestroySelfController::parse(const std::string& key, const std::string& value) {}

    // ==========================================
    // VarRangeSetController
    // ==========================================
    VarRangeSetController::VarRangeSetController() {
        type = ControllerType::VARRANGESET;
    }
    void VarRangeSetController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "value") {
            try { m_value = std::stoi(value); } catch(...) {}
        }
    }
    void VarRangeSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // 清空所有 sysvar (0-59)
        for (int i = 0; i < 60; i++) fighter.setSysVar(i, m_value);
    }

    // ==========================================
    // AssertSpecialController
    // ==========================================
    AssertSpecialController::AssertSpecialController() {
        type = ControllerType::ASSERTSPECIAL;
    }
    void AssertSpecialController::parse(const std::string& key, const std::string& value) {
        // 简化: 不处理特殊标志
    }

    // ==========================================
    // AngleDrawController
    // ==========================================
    AngleDrawController::AngleDrawController() {
        type = ControllerType::ANGLEDRAW;
    }
    void AngleDrawController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "scale") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                try { m_scaleX = std::stof(value.substr(0, comma)); } catch(...) {}
                try { m_scaleY = std::stof(value.substr(comma + 1)); } catch(...) {}
            }
        }
    }

    // ==========================================
    // TransController
    // ==========================================
    TransController::TransController() {
        type = ControllerType::TRANS;
    }
    void TransController::parse(const std::string& key, const std::string& value) {
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
        if (lowerKey == "trans") {
            m_transType = value;
            std::transform(m_transType.begin(), m_transType.end(), m_transType.begin(), ::tolower);
        } else if (lowerKey == "alpha") {
            size_t comma = value.find(',');
            if (comma != std::string::npos) {
                std::string srcStr = value.substr(0, comma);
                std::string dstStr = value.substr(comma + 1);
                // 尝试解析为纯数字，失败则存为表达式
                try {
                    size_t pos = 0;
                    m_alphaSrc = std::stoi(srcStr, &pos);
                    if (pos < srcStr.size()) { m_alphaSrcExpr = srcStr; m_alphaSrc = 0; }
                } catch (...) { m_alphaSrcExpr = srcStr; m_alphaSrc = 0; }
                try {
                    size_t pos = 0;
                    m_alphaDst = std::stoi(dstStr, &pos);
                    if (pos < dstStr.size()) { m_alphaDstExpr = dstStr; m_alphaDst = 256; }
                } catch (...) { m_alphaDstExpr = dstStr; m_alphaDst = 256; }
            }
        }
    }
    void TransController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        auto& overrides = fighter.getDrawOverrides();
        // 仅设置 alpha 透明度，不使用 additive 混合（与 Explod 一致）
        int src = std::max(0, std::min(256, m_alphaSrc));
        overrides.alpha = static_cast<uint8_t>((src * 255) / 256);
    }
    void AssertSpecialController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {}
    void DestroySelfController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {}
    void AngleDrawController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        auto& overrides = fighter.getDrawOverrides();
        overrides.scaleX = m_scaleX;
        overrides.scaleY = m_scaleY;
    }

    // ==========================================
    // HitVelSetController
    // ==========================================
    HitVelSetController::HitVelSetController() { type = ControllerType::HITVELSET; }
    void HitVelSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        const auto& info = fighter.getHitInfo();
        if (hasX && info.groundVelocityX != 0.f) {
            float vx = info.groundVelocityX * valueX * 60.f;
            fighter.setVelocityX(vx);
        }
        if (hasY) {
            float vy = (info.groundVelocityY != 0.f ? info.groundVelocityY : info.airVelocityY) * valueY * 60.f;
            fighter.setVelocityY(vy);
        }
    }

    // ==========================================
    // PosFreezeController
    // ==========================================
    PosFreezeController::PosFreezeController() { type = ControllerType::POSFREEZE; }
    void PosFreezeController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        fighter.setVelocityX(0.f);
        fighter.setVelocityY(0.f);
    }

    // ==========================================
    // HitFallDamageController
    // ==========================================
    HitFallDamageController::HitFallDamageController() { type = ControllerType::HITFALLDAMAGE; }
    void HitFallDamageController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        int fallYVel = fighter.getHitVar("fall.yvel");
        int damage = std::abs(fallYVel);
        if (damage > 0) {
            fighter.takeDamage(damage);
        }
    }

    // ==========================================
    // HitFallVelController
    // ==========================================
    HitFallVelController::HitFallVelController() { type = ControllerType::HITFALLVEL; }
    void HitFallVelController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        float vy = fighter.getHitInfo().fallyvel * 60.f;
        fighter.setVelocityY(vy);
    }

    // ==========================================
    // HitFallSetController
    // ==========================================
    HitFallSetController::HitFallSetController() { type = ControllerType::HITFALLSET; }
    void HitFallSetController::execute(Fighter& fighter, InputManager* inputMgr, float dt) const {
        // HitFallSet 的 value 来自 CNS 的 value = 1
        // 目前在 HitInfo 中由 HitDef 的 fall 参数控制
    }

} // namespace db
