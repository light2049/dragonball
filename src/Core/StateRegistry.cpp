#include "Core/StateRegistry.h"
#include "Characters/Fighter.h"
#include "Utils/CnsParser.h"
#include <iostream>

namespace db {

    bool StateRegistry::loadCNS(const std::string& path) {
        try {
            auto states = CnsParser::loadStateDefs(path);

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

            int currentState = fighter.getCurrentStateNo();
            int stateTime = fighter.getStateTime();
            if (!ctrl->checkTriggers(fighter, inputMgr, stateTime)) continue;

            ctrl->execute(fighter, inputMgr, dt);

            if (fighter.getCurrentStateNo() != currentState) {
                if (stateNo == -1 && ctrl->type == ControllerType::CHANGESTATE) {
                    std::cout << "[State-1] state=" << fighter.getCurrentStateNo()
                              << " ctrl=" << fighter.hasControl()
                              << " val=" << ctrl->value << std::endl;
                }
                if ((currentState == 195 || currentState == 196) && ctrl->type == ControllerType::CHANGESTATE) {
                }
                break;
            }
        }
    }

    void StateRegistry::applyStateAttributes(int stateNo, Fighter& fighter) const {
        auto it = m_states.find(stateNo);
        if (it == m_states.end()) return;

        const StateDef& s = it->second;

        fighter.setStateType(s.type);

        fighter.setMoveType(s.movetype);

        fighter.setPhysicsType(s.physics);

        if (s.ctrl >= 0) {
            fighter.setControl(s.ctrl != 0);
        } else {

            bool defaultCtrl = (s.movetype == 0);
            fighter.setControl(defaultCtrl);
        }

        if (s.hasVelset) {
            fighter.setVelocityX(s.velsetX * 60.f);
            fighter.setVelocityY(s.velsetY * 60.f);
        } else if (s.velsetX != 0.f || s.velsetY != 0.f) {

            fighter.setVelocityX(s.velsetX * 60.f);
            fighter.setVelocityY(s.velsetY * 60.f);
        }

        if (s.anim >= 0) {
            fighter.switchAnimation(s.anim);
        }

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

}
