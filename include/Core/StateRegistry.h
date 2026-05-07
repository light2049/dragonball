#pragma once
#include <map>
#include <string>
#include <vector>
#include "Utils/CnsParser.h"   // ✅ 引入这里定义的 StateDef
#include "Utils/CNSController.h"

namespace db {
    class Fighter;

    class StateRegistry {
    public:
        bool loadCNS(const std::string& path);
        bool hasState(int stateNo) const;
        const StateDef* getStateDef(int stateNo) const;
        void executeState(int stateNo, Fighter& fighter, InputManager* inputMgr, float dt);

        // 进入状态时应用 StateDef 属性 (ctrl/velset/poweradd/anim)
        void applyStateAttributes(int stateNo, Fighter& fighter) const;

        const std::vector<HitDef>& getHitDefs(int stateNo) const;
        const std::map<int, StateDef>& getStates() const { return m_states; }

    private:
        std::map<int, StateDef> m_states;
    };
}