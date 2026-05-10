#include "Core/StateRegistry.h"
#include "Characters/Fighter.h"
#include "Utils/CnsParser.h"
#include <iostream>

namespace db {

    bool StateRegistry::loadCNS(const std::string& path) {
        try {
            auto states = CnsParser::loadStateDefs(path);

            // 合并新状态到现有映射 (不覆盖已有状态, 用新数据补充)
            int added = 0;
            for (auto& [id, def] : states) {
                if (m_states.find(id) == m_states.end()) {
                    m_states[id] = std::move(def);
                    added++;
                }
            }

            std::cout << "[StateRegistry] Loaded " << states.size() << " StateDefs from "
                      << path << " (" << added << " new, "
                      << (states.size() - added) << " duplicate). Total: " << m_states.size() << "\n";
            if (states.empty()) {
                std::cerr << "[StateRegistry] WARNING: 0 states loaded from " << path << "\n";
            }
            return !m_states.empty();
        } catch (const std::exception& e) {
            std::cerr << "[StateRegistry] Failed to load CNS states: " << e.what() << std::endl;
            return false;
        }
    }

    bool StateRegistry::hasState(int stateNo) const {
        return m_states.find(stateNo) != m_states.end();
    }

    const StateDef* StateRegistry::getStateDef(int stateNo) const {
        auto it = m_states.find(stateNo);
        return (it != m_states.end()) ? &it->second : nullptr;
    }

    void StateRegistry::executeState(int stateNo, Fighter& fighter, InputManager* inputMgr, float dt) {
        auto it = m_states.find(stateNo);
        if (it == m_states.end()) return;

        const StateDef& stateDef = it->second;

        for (const auto& ctrl : stateDef.controllers) {
            // 如果状态已变更 (ChangeState 已触发), 停止执行当前状态
            int currentState = fighter.getCurrentStateNo();
            int stateTime = fighter.getStateTime();
            if (!ctrl->checkTriggers(fighter, inputMgr, stateTime)) continue;

            ctrl->execute(fighter, inputMgr, dt);

            // ChangeState 执行后立即停止
            if (fighter.getCurrentStateNo() != currentState) break;
        }
    }

    void StateRegistry::applyStateAttributes(int stateNo, Fighter& fighter) const {
        auto it = m_states.find(stateNo);
        if (it == m_states.end()) return;

        const StateDef& s = it->second;

        // 状态类型 (S/C/A)
        fighter.setStateType(s.type);

        // 移动类型 (I/A/H)
        fighter.setMoveType(s.movetype);

        // 物理类型 (S/C/A/N)
        fighter.setPhysicsType(s.physics);

        // 控制权 (ctrl < 0 表示未显式设置, 根据 movetype 推断)
        if (s.ctrl >= 0) {
            fighter.setControl(s.ctrl != 0);
        } else {
            // 默认: 空闲/普通状态有控制权, 攻击/受击状态无控制权
            bool defaultCtrl = (s.movetype == 0); // I = idle, has control
            fighter.setControl(defaultCtrl);
        }

        // DEBUG: log state attribute application (禁用: 日志过多)
        //{
        //    std::string msg = "[StateRegistry] applyStateAttrs state=" + std::to_string(stateNo)
        //        + " ctrl_param=" + std::to_string(s.ctrl)
        //        + " movetype=" + std::to_string(s.movetype)
        //        + " result_ctrl=" + (fighter.hasControl() ? "1" : "0") + "\n";
        //    FILE* f = fopen("D:\\DragonBall\\build\\fighter_log.txt", "a");
        //    if (f) { fprintf(f, "%s", msg.c_str()); fclose(f); }
        //}

        // 初始速度 (仅当 CNS 显式指定 velset 时应用, 包括 velset=0,0)
        if (s.hasVelset) {
            fighter.setVelocityX(s.velsetX * 60.f);
            fighter.setVelocityY(s.velsetY * 60.f);
        } else if (s.velsetX != 0.f || s.velsetY != 0.f) {
            // 兼容旧数据: 未标记 hasVelset 但有非零值
            fighter.setVelocityX(s.velsetX * 60.f);
            fighter.setVelocityY(s.velsetY * 60.f);
        }

        // 动画
        if (s.anim >= 0) {
            fighter.switchAnimation(s.anim);
        }

        // Power
        if (s.poweradd != 0) {
            fighter.addPower(s.poweradd);
        }
    }

    const std::vector<HitDef>& StateRegistry::getHitDefs(int stateNo) const {
        static const std::vector<HitDef> emptyDefs;
        auto it = m_states.find(stateNo);
        if (it != m_states.end()) {
            return it->second.hitDefs;
        }
        return emptyDefs;
    }

} // namespace db
