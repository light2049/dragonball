#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <deque>
#include <vector>
#include <map>

namespace db {

    // M.U.G.E.N 方向常量 (8方向)
    enum class DirInput {
        NONE, B, DB, D, DF, F, UF, U, UB
    };

    // 单帧输入快照
    struct FrameInput {
        // 6键 + start
        bool x = false;  // 轻拳 (J)
        bool y = false;  // 中拳 (I)
        bool z = false;  // 重拳 (O)
        bool a = false;  // 轻踢 (K)
        bool b = false;  // 中踢 (L)
        bool c = false;  // 重踢 (U)
        bool s = false;  // 开始 (Space)
        DirInput dir = DirInput::NONE;
        bool charge = false; // 蓄气 (L key / hold_s)
    };

    // 旧版扁平输入 (兼容 Fighter.cpp 现有代码)
    struct SimpleInputState {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        bool punch = false;     // J = 轻拳
        bool kick = false;      // K = 轻踢
        bool charge = false;    // L = 蓄气
    };

    class InputManager {
    public:
        void update();

        // Event-based key tracking: call from Game::processEvents on KeyPressed/KeyReleased
        void onKeyPressed(sf::Keyboard::Key key);
        void onKeyReleased(sf::Keyboard::Key key);
        void clearJustPressedLatch();
        // Reset all input state (e.g. on fight start to discard menu input)
        void reset();

        // 单键按下检测 (当前帧按下)
        bool isKeyPressed(sf::Keyboard::Key key) const;

        // 边缘检测 (刚按下/刚松开)
        bool justPressed(char btn) const;
        bool justReleased(char btn) const;

        // 通用按键边缘检测 (基于 KeyPressed 事件)
        bool isKeyJustPressed(sf::Keyboard::Key key) const {
            int k = static_cast<int>(key);
            if (k >= 0 && k < 128) return m_justPressedLatch[k];
            return false;
        }

        // M.U.G.E.N 按钮检测 (当前帧状态)
        bool buttonX() const;  // 轻拳
        bool buttonY() const;  // 中拳
        bool buttonZ() const;  // 重拳
        bool buttonA() const;  // 轻踢
        bool buttonB() const;  // 中踢
        bool buttonC() const;  // 重踢
        bool buttonS() const;  // 开始

        // 方向检测
        DirInput getDirection() const;
        bool isHeld(const std::string& cmdName) const;
        bool isDirHeld(DirInput dir) const;

        // 指令缓冲检测 (方向序列/按键组合)
        bool commandBufferMatch(const std::vector<DirInput>& seq, int window = 15) const;
        bool doubleTap(DirInput dir, int window = 10) const;

        // 获取历史帧输入 (用于指令检测)
        const FrameInput& getFrame(int offset) const; // offset=0 是当前帧

        // 命令结果缓存 (由 CmdParser 设置, 用于 command = "xxx" 触发)
        void setCommandResult(const std::string& name, bool active) { m_commandResults[name] = active; }
        void clearCommandResults() { m_commandResults.clear(); }

    private:
        void updateDirection(FrameInput& snap);
        void pushHistory(const FrameInput& snap);

        FrameInput m_current;
        FrameInput m_previous;
        std::deque<FrameInput> m_history; // 指令历史缓冲 (最多60帧)
        std::map<std::string, bool> m_commandResults; // 命令名 → 激活状态 (由 CmdParser 填充)

        // Event-based latch: persists for one frame after KeyPressed event
        bool m_justPressedLatch[128] = {};
        // Event-based key held state (updated by press/release events, not polling)
        bool m_eventKeyState[128] = {};
    };

} // namespace db
