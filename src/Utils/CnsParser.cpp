#include "Utils/CnsParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace db {

    // ✅ 辅助函数：解析逗号分隔的值 (如 "20,2" 或 "0,-8.4")
    template <typename T>
    static std::pair<T, T> parsePair(const std::string& value) {
        size_t commaPos = value.find(',');
        if (commaPos != std::string::npos) {
            try {
                T v1 = std::stoi(value.substr(0, commaPos));
                T v2 = std::stoi(value.substr(commaPos + 1));
                return {v1, v2};
            } catch (...) {}
            try {
                T v1 = std::stof(value.substr(0, commaPos));
                T v2 = std::stof(value.substr(commaPos + 1));
                return {v1, v2};
            } catch (...) {}
        }
        try { return {static_cast<T>(std::stoi(value)), static_cast<T>(0)}; } catch (...) {}
        try { return {static_cast<T>(std::stof(value)), static_cast<T>(0.f)}; } catch (...) {}
        return {static_cast<T>(0), static_cast<T>(0)};
    }

    CharacterStats CnsParser::loadStats(const std::string& filePath) {
        CharacterStats stats;
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[CnsParser] Error: Cannot open " << filePath << "\n";
            return stats;
        }

        std::string line;
        int currentBlock = 0; // 0=None, 1=Data, 2=Size, 3=Velocity, 4=Movement

        while (std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start, line.find_last_not_of(" \t\r\n") - start + 1);

            if (line.starts_with(";")) continue;

            if (line.starts_with("[")) {
                std::string lower = line;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("[data]") != std::string::npos) currentBlock = 1;
                else if (lower.find("[size]") != std::string::npos) currentBlock = 2;
                else if (lower.find("[velocity]") != std::string::npos) currentBlock = 3; // ✅ 新增
                else if (lower.find("[movement]") != std::string::npos) currentBlock = 4; // ✅ 新增
                else currentBlock = 0;
                continue;
            }

            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // 去掉行内注释 (; 后面的内容)
                size_t semicolon = value.find(';');
                if (semicolon != std::string::npos) {
                    value = value.substr(0, semicolon);
                    // 重新去掉尾随空格
                    size_t end = value.find_last_not_of(" \t\r\n");
                    if (end != std::string::npos) value = value.substr(0, end + 1);
                }

                // 处理 .44 格式 → 补 0 变成 0.44，避免 std::stof 解析失败
                if (!value.empty() && value[0] == '.') {
                    value = "0" + value;
                }

                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                try {
                    switch (currentBlock) {
                        case 1: // [Data]
                            if (key == "life") stats.maxLife = std::stoi(value);
                            else if (key == "power") stats.maxPower = std::stoi(value); // ✅ 新增
                            else if (key == "attack") stats.attack = std::stoi(value);
                            else if (key == "defence") stats.defence = std::stoi(value);
                            else if (key == "height") stats.height = std::stoi(value);
                            break;
                        case 2: // [Size]
                            if (key == "ground.back") stats.groundBack = std::stoi(value);
                            else if (key == "ground.front") stats.groundFront = std::stoi(value);
                            else if (key == "air.back") stats.airBack = std::stoi(value);
                            else if (key == "air.front") stats.airFront = std::stoi(value);
                            break;
                        case 3: // [Velocity] ✅ 新增解析逻辑
                            if (key == "walk.fwd") stats.velocity.walkFwd = std::stof(value);
                            else if (key == "walk.back") stats.velocity.walkBack = std::stof(value);
                            else if (key == "jump.neu") {
                                auto p = parsePair<float>(value);
                                stats.velocity.jumpNeuX = p.first;
                                stats.velocity.jumpNeuY = p.second;
                            }
                            else if (key == "jump.fwd") stats.velocity.jumpFwd = std::stof(value);
                            else if (key == "jump.back") stats.velocity.jumpBack = std::stof(value);
                            else if (key == "run.fwd") {
                                auto p = parsePair<float>(value);
                                stats.velocity.runFwdX = p.first;
                                stats.velocity.runFwdY = p.second;
                            }
                            else if (key == "run.back") {
                                auto p = parsePair<float>(value);
                                stats.velocity.runBackX = p.first;
                                stats.velocity.runBackY = p.second;
                            }
                            break;
                        case 4: // [Movement] ✅ 新增解析逻辑
                            if (key == "yaccel") stats.movement.yaccel = std::stof(value);
                            else if (key == "stand.friction") stats.movement.standFriction = std::stof(value);
                            else if (key == "crouch.friction") stats.movement.crouchFriction = std::stof(value);
                            break;
                    }
                } catch (...) {}
            }
        }

        std::cout << "[CnsParser] Loaded Stats -> Life: " << stats.maxLife
                  << ", Power: " << stats.maxPower
                  << ", YAccel: " << stats.movement.yaccel
                  << ", WalkFwd: " << stats.velocity.walkFwd << "\n";
        return stats;
    }

    // 局部缩进辅助函数 (CNSController.cpp 中也有同名函数)
    static std::string cnsTrim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::map<int, StateDef> CnsParser::loadStateDefs(const std::string& filePath) {
        std::map<int, StateDef> states;
        std::ifstream file(filePath);
        std::cout << "[CnsParser] Opening " << filePath << " for state defs\n";
        if (!file.is_open()) {
            std::cerr << "[CnsParser] Error: Cannot open CNS file for states: " << filePath << "\n";
            return states;
        }

        std::string line;
        int currentStateNo = -1;
        bool inStateDef = false;
        std::unique_ptr<CNSController> currentController;

        auto finalizeController = [&]() {
            if (currentController) {
                states[currentStateNo].controllers.push_back(std::move(currentController));
                currentController = nullptr;
            }
        };

        while (std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start, line.find_last_not_of(" \t\r\n") - start + 1);

            std::string lowerLine = line;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

            // 1. [StateDef XXX] — 状态定义块
            if (lowerLine.starts_with("[statedef")) {
                finalizeController();
                inStateDef = true;

                size_t bracketEnd = line.find(']');
                if (bracketEnd != std::string::npos) {
                    std::string content = line.substr(0, bracketEnd);
                    size_t spacePos = content.find(' ');
                    if (spacePos != std::string::npos) {
                        try {
                            int id = std::stoi(content.substr(spacePos + 1));
                            states[id] = StateDef();
                            states[id].stateNo = id;
                            currentStateNo = id;
                        } catch (...) {}
                    }
                }
                continue;
            }

            // 2. [State XXX, Y] — 控制器块
            // [State X, Y] 中的 X 只是标签，控制器归属于当前的 [StateDef]
            // 因此不改变 currentStateNo
            if (lowerLine.starts_with("[state")) {
                finalizeController();
                inStateDef = false;
                // currentStateNo 保持不变 — 继承当前 [StateDef] 的状态号
                continue;
            }

            // 3. 空行/注释 — 跳过
            if (line.empty() || line[0] == ';') continue;

            // 4. 解析 Key = Value
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::string lowerVal = value;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

            // 5. [StateDef] 属性解析
            if (inStateDef) {
                if (lowerKey == "type") {
                    if (lowerVal == "s") states[currentStateNo].type = 0;
                    else if (lowerVal == "c") states[currentStateNo].type = 1;
                    else if (lowerVal == "a") states[currentStateNo].type = 2;
                    else if (lowerVal == "l") states[currentStateNo].type = 3;
                }
                else if (lowerKey == "movetype") {
                    if (lowerVal == "i") states[currentStateNo].movetype = 0;
                    else if (lowerVal == "a") states[currentStateNo].movetype = 1;
                    else if (lowerVal == "h") states[currentStateNo].movetype = 2;
                }
                else if (lowerKey == "physics") {
                    std::string pLower = value;
                    std::transform(pLower.begin(), pLower.end(), pLower.begin(), ::tolower);
                    if (pLower == "s") states[currentStateNo].physics = 0;
                    else if (pLower == "c") states[currentStateNo].physics = 1;
                    else if (pLower == "a") states[currentStateNo].physics = 2;
                    else if (pLower == "n") states[currentStateNo].physics = 3;
                    else try { states[currentStateNo].physics = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "ctrl") {
                    try { states[currentStateNo].ctrl = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "poweradd") {
                    try { states[currentStateNo].poweradd = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "velset") {
                    auto p = parsePair<float>(value);
                    states[currentStateNo].velsetX = p.first;
                    states[currentStateNo].velsetY = p.second;
                }
                else if (lowerKey == "anim") {
                    try { states[currentStateNo].anim = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "sprpriority") {
                    try { states[currentStateNo].sprpriority = std::stoi(value); } catch (...) {}
                }
                continue;
            }

            // ==========================================
            // [State XXX, Y] 块内的控制器解析
            // ==========================================

            // 5. 控制器类型检测
            if (lowerKey == "type") {
                finalizeController();

                ControllerType ctrlType = parseControllerType(value);
                currentController = createController(ctrlType);

                // 对 HitDef 特殊处理：确保 StateDef 中有一个 HitDef 条目
                if (ctrlType == ControllerType::HITDEF) {
                    states[currentStateNo].hitDefs.push_back(HitDef());
                    auto& hit = states[currentStateNo].hitDefs.back();
                    hit.active = true;
                }
                continue;
            }

            // 6. 触发器解析
            // M.U.G.E.N 规范: 多个 triggerN 行 (相同编号) 是 AND 关系, 需要合并到同一个 TriggerLine
            // triggerall 也支持多行, 同样合并
            if (lowerKey.starts_with("trigger")) {
                if (!currentController) continue;

                bool isAll = (lowerKey == "triggerall");
                int triggerId = 1;
                if (!isAll) {
                    std::string numStr = lowerKey.substr(7);
                    try { triggerId = std::stoi(numStr); } catch (...) {}
                }

                // 解析条件，支持 && 连接
                std::vector<Condition> newConds;
                std::string condStr = value;
                size_t andPos;
                while ((andPos = condStr.find("&&")) != std::string::npos) {
                    std::string part = cnsTrim(condStr.substr(0, andPos));
                    if (!part.empty()) {
                        auto conds = parseConditionString(part);
                        newConds.insert(newConds.end(), conds.begin(), conds.end());
                    }
                    condStr = cnsTrim(condStr.substr(andPos + 2));
                }
                if (!condStr.empty()) {
                    auto conds = parseConditionString(condStr);
                    newConds.insert(newConds.end(), conds.begin(), conds.end());
                }

                // 合并到已有的同类型/同编号 TriggerLine (M.U.G.E.N AND 语义)
                bool merged = false;
                for (auto& tl : currentController->triggers) {
                    if (tl.allFlag == isAll && (isAll || tl.id == triggerId)) {
                        tl.conditions.insert(tl.conditions.end(), newConds.begin(), newConds.end());
                        merged = true;
                        break;
                    }
                }

                if (!merged) {
                    TriggerLine tl;
                    tl.allFlag = isAll;
                    tl.id = triggerId;
                    tl.conditions = std::move(newConds);
                    currentController->triggers.push_back(tl);
                }

                continue;
            }

            // 7. HitDef 参数解析
            if (currentController && currentController->type == ControllerType::HITDEF) {
                if (states[currentStateNo].hitDefs.empty()) continue;
                auto& hit = states[currentStateNo].hitDefs.back();

                if (lowerKey == "damage") {
                    auto p = parsePair<int>(value);
                    hit.damage = p.first;
                    hit.guardDamage = p.second;
                }
                else if (lowerKey == "p1stateno") {
                    try { hit.p1stateno = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "p2stateno") {
                    try { hit.p2stateno = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "pausetime") {
                    auto p = parsePair<int>(value);
                    hit.pausetime = p.first;
                    hit.guardPausetime = p.second;
                }
                else if (lowerKey == "ground.hittime") {
                    try { hit.groundHittime = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "ground.slidetime") {
                    try { hit.groundSlidetime = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "ground.velocity") {
                    try { hit.groundVelocityX = std::stof(value); } catch (...) {}
                }
                else if (lowerKey == "air.velocity") {
                    auto p = parsePair<float>(value);
                    hit.airVelocityX = p.first;
                    hit.airVelocityY = p.second;
                }
                else if (lowerKey == "air.hittime") {
                    try { hit.airHittime = std::stof(value); } catch (...) {}
                }
                else if (lowerKey == "airguard.velocity") {
                    auto p = parsePair<float>(value);
                    hit.airguardVelocityX = p.first;
                    hit.airguardVelocityY = p.second;
                }
                else if (lowerKey == "animtype") {
                    hit.animtype = value;
                }
                else if (lowerKey == "attr") {
                    hit.attr = value;
                }
                else if (lowerKey == "guardflag") {
                    hit.guardflag = value;
                }
                else if (lowerKey == "hitflag") {
                    hit.hitflag = value;
                }
                else if (lowerKey == "priority") {
                    size_t commaPos = value.find(',');
                    if (commaPos != std::string::npos) {
                        try { hit.priority = std::stoi(value); } catch (...) {}
                        std::string pType = value.substr(commaPos + 1);
                        pType.erase(0, pType.find_first_not_of(" \t"));
                        pType.erase(pType.find_last_not_of(" \t") + 1);
                        hit.priorityType = pType;
                    } else {
                        try { hit.priority = std::stoi(value); } catch (...) {}
                    }
                }
                else if (lowerKey == "sparkno") {
                    try { hit.sparkno = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "sparkxy") {
                    auto p = parsePair<int>(value);
                    hit.sparkX = p.first;
                    hit.sparkY = p.second;
                }
                else if (lowerKey == "hitsound") {
                    hit.hitsound = value;
                }
                else if (lowerKey == "guardsound") {
                    hit.guardsound = value;
                }
                else if (lowerKey == "ground.type") {
                    hit.groundType = value;
                }
                else if (lowerKey == "air.type") {
                    hit.airType = value;
                }
                else if (lowerKey == "envshake.time") {
                    try { hit.envshakeTime = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "getpower") {
                    auto p = parsePair<int>(value);
                    hit.getpower = p.first;
                    hit.guardGetPower = p.second;
                }
                else if (lowerKey == "givepower") {
                    auto p = parsePair<int>(value);
                    hit.givepower = p.first;
                    hit.guardGivePower = p.second;
                }
                else if (lowerKey == "juggle") {
                    try { hit.juggle = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "fall") {
                    try { hit.fall = (std::stoi(value) != 0); } catch (...) {}
                }
                else if (lowerKey == "fall.recover") {
                    try { hit.fallRecover = (std::stoi(value) != 0); } catch (...) {}
                }
                else if (lowerKey == "fall.yvel") {
                    try { hit.fallYVel = std::stoi(value); } catch (...) {}
                }
                else if (lowerKey == "id") {
                    try { hit.id = std::stoi(value); } catch (...) {}
                }
                // 其他 HitDef 参数暂不处理
                continue;
            }

            // 8. 通用控制器参数解析
            if (currentController) {
                if (lowerKey == "value") {
                    try { currentController->value = std::stoi(value); } catch (...) {
                        currentController->valueStr = value;
                    }
                }
                else if (lowerKey == "x") {
                    currentController->hasX = true;
                    try { currentController->valueX = std::stof(value); } catch (...) {
                        currentController->valueXStr = value;
                    }
                }
                else if (lowerKey == "y") {
                    currentController->hasY = true;
                    try { currentController->valueY = std::stof(value); } catch (...) {
                        currentController->valueYStr = value;
                    }
                }
                else if (lowerKey.starts_with("sysvar")) {
                    // sysvar(1) = 0 → index=1, value=0
                    size_t parenL = lowerKey.find('(');
                    size_t parenR = lowerKey.find(')');
                    if (parenL != std::string::npos && parenR > parenL) {
                        try {
                            currentController->paramInt = std::stoi(lowerKey.substr(parenL + 1, parenR - parenL - 1));
                            currentController->value = std::stoi(value);
                            currentController->paramStr = "sysvar";
                        } catch (...) {}
                    }
                }
                else {
                    // 委托给控制器的 parse 方法
                    currentController->parse(key, value);
                }
            }
        }

        // 不要忘记最后一个控制器
        finalizeController();

        // 打印统计
        int totalDefs = 0, totalCtls = 0;
        for (const auto& s : states) {
            totalDefs += s.second.hitDefs.size();
            totalCtls += s.second.controllers.size();
        }
        std::cout << "[CnsParser] Loaded " << states.size() << " StateDefs with "
                  << totalCtls << " controllers, " << totalDefs << " HitDefs.\n";

        return states;
    }

} // namespace db