#include "Core/InputManager.h"
#include <algorithm>

namespace db {

    // M.U.G.E.N 按键映射
    // 6键布局: x(轻拳) y(中拳) z(重拳) a(轻踢) b(中踢) c(重踢)
    // 键盘映射: J=x, I=y, O=z, K=a, L=b, U=c, Space=s, L=charge
    static sf::Keyboard::Key mapMugenButton(char btn) {
        switch (btn) {
            case 'x': return sf::Keyboard::Key::J;
            case 'y': return sf::Keyboard::Key::I;
            case 'z': return sf::Keyboard::Key::O;
            case 'a': return sf::Keyboard::Key::K;
            case 'b': return sf::Keyboard::Key::L;
            case 'c': return sf::Keyboard::Key::U;
            case 's': return sf::Keyboard::Key::Space;
            default:  return sf::Keyboard::Key::Unknown;
        }
    }

    void InputManager::onKeyPressed(sf::Keyboard::Key key) {
        int k = static_cast<int>(key);
        if (k >= 0 && k < 128) {
            m_justPressedLatch[k] = true;
            m_eventKeyState[k] = true;
        }
    }

    void InputManager::onKeyReleased(sf::Keyboard::Key key) {
        int k = static_cast<int>(key);
        if (k >= 0 && k < 128) {
            m_eventKeyState[k] = false;
        }
    }

    // 基于事件的按键状态查询 (替代 sf::Keyboard::isKeyPressed 轮询)
    static bool eventKeyHeld(const bool eventState[128], sf::Keyboard::Key key) {
        int k = static_cast<int>(key);
        return k >= 0 && k < 128 && eventState[k];
    }

    void InputManager::update() {
        m_previous = m_current;

        FrameInput snap;

        // 按键状态全部基于事件 (press/release)，不依赖轮询
        snap.x = eventKeyHeld(m_eventKeyState, mapMugenButton('x'));
        snap.y = eventKeyHeld(m_eventKeyState, mapMugenButton('y'));
        snap.z = eventKeyHeld(m_eventKeyState, mapMugenButton('z'));
        snap.a = eventKeyHeld(m_eventKeyState, mapMugenButton('a'));
        snap.b = eventKeyHeld(m_eventKeyState, mapMugenButton('b'));
        snap.c = eventKeyHeld(m_eventKeyState, mapMugenButton('c'));
        snap.s = eventKeyHeld(m_eventKeyState, mapMugenButton('s'));

        // 蓄气键
        snap.charge = eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::L) || snap.s;

        // 方向同样基于事件
        updateDirection(snap);

        m_current = snap;
        pushHistory(snap);
    }

    void InputManager::clearJustPressedLatch() {
        for (int i = 0; i < 128; i++) m_justPressedLatch[i] = false;
    }

    void InputManager::reset() {
        m_current = FrameInput{};
        m_previous = FrameInput{};
        m_history.clear();
        m_commandResults.clear();
        clearJustPressedLatch();
        for (int i = 0; i < 128; i++) m_eventKeyState[i] = false;
    }

    void InputManager::updateDirection(FrameInput& snap) {
        bool up = eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::W) || eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::Up);
        bool down = eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::S) || eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::Down);
        bool left = eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::A) || eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::Left);
        bool right = eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::D) || eventKeyHeld(m_eventKeyState, sf::Keyboard::Key::Right);

        // M.U.G.E.N 8方向判定 (优先级: 斜方向优先)
        if (up && right)       snap.dir = DirInput::UF;
        else if (up && left)   snap.dir = DirInput::UB;
        else if (down && right) snap.dir = DirInput::DF;
        else if (down && left) snap.dir = DirInput::DB;
        else if (up)           snap.dir = DirInput::U;
        else if (down)         snap.dir = DirInput::D;
        else if (left)         snap.dir = DirInput::B;
        else if (right)        snap.dir = DirInput::F;
        else                   snap.dir = DirInput::NONE;
    }

    void InputManager::pushHistory(const FrameInput& snap) {
        m_history.push_back(snap);
        // 保留最近 60 帧 (1秒)
        if (m_history.size() > 60) {
            m_history.pop_front();
        }
    }

    bool InputManager::isKeyPressed(sf::Keyboard::Key key) const {
        return sf::Keyboard::isKeyPressed(key);
    }

    bool InputManager::justPressed(char btn) const {
        switch (btn) {
            case 'x': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::J)] || (m_current.x && !m_previous.x);
            case 'y': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::I)] || (m_current.y && !m_previous.y);
            case 'z': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::O)] || (m_current.z && !m_previous.z);
            case 'a': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::K)] || (m_current.a && !m_previous.a);
            case 'b': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::L)] || (m_current.b && !m_previous.b);
            case 'c': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::U)] || (m_current.c && !m_previous.c);
            case 's': return m_justPressedLatch[static_cast<int>(sf::Keyboard::Key::Space)] || (m_current.s && !m_previous.s);
            default:  return false;
        }
    }

    bool InputManager::justReleased(char btn) const {
        switch (btn) {
            case 'x': return !m_current.x && m_previous.x;
            case 'y': return !m_current.y && m_previous.y;
            case 'z': return !m_current.z && m_previous.z;
            case 'a': return !m_current.a && m_previous.a;
            case 'b': return !m_current.b && m_previous.b;
            case 'c': return !m_current.c && m_previous.c;
            case 's': return !m_current.s && m_previous.s;
            default:  return false;
        }
    }

    bool InputManager::buttonX() const { return m_current.x; }
    bool InputManager::buttonY() const { return m_current.y; }
    bool InputManager::buttonZ() const { return m_current.z; }
    bool InputManager::buttonA() const { return m_current.a; }
    bool InputManager::buttonB() const { return m_current.b; }
    bool InputManager::buttonC() const { return m_current.c; }
    bool InputManager::buttonS() const { return m_current.s; }

    DirInput InputManager::getDirection() const {
        return m_current.dir;
    }

    bool InputManager::isDirHeld(DirInput dir) const {
        return m_current.dir == dir;
    }

    bool InputManager::isHeld(const std::string& cmdName) const {
        // 先查命令结果缓存 (由 CmdParser 填充)
        auto it = m_commandResults.find(cmdName);
        if (it != m_commandResults.end()) {
            return it->second;
        }

        DirInput dir = m_current.dir;

        // /$F = 按住前 (F, UF, DF 任一)
        if (cmdName == "holdfwd" || cmdName == "F") {
            return dir == DirInput::F || dir == DirInput::UF || dir == DirInput::DF;
        }
        if (cmdName == "holdback" || cmdName == "B") {
            return dir == DirInput::B || dir == DirInput::UB || dir == DirInput::DB;
        }
        if (cmdName == "holddown" || cmdName == "D") {
            return dir == DirInput::D || dir == DirInput::DF || dir == DirInput::DB;
        }
        if (cmdName == "holdup" || cmdName == "U") {
            return dir == DirInput::U || dir == DirInput::UF || dir == DirInput::UB;
        }
        // 精确方向
        if (cmdName == "holddownfwd" || cmdName == "DF") return dir == DirInput::DF;
        if (cmdName == "holddownback" || cmdName == "DB") return dir == DirInput::DB;
        if (cmdName == "holdupfwd" || cmdName == "UF") return dir == DirInput::UF;
        if (cmdName == "holdupback" || cmdName == "UB") return dir == DirInput::UB;

        // 按钮长按
        if (cmdName == "hold_x") return m_current.x;
        if (cmdName == "hold_y") return m_current.y;
        if (cmdName == "hold_z") return m_current.z;
        if (cmdName == "hold_a") return m_current.a;
        if (cmdName == "hold_b") return m_current.b;
        if (cmdName == "hold_c") return m_current.c;
        if (cmdName == "hold_s") return m_current.s;

        // 普通按钮 (非 hold)
        if (cmdName == "x") return m_current.x;
        if (cmdName == "y") return m_current.y;
        if (cmdName == "z") return m_current.z;
        if (cmdName == "a") return m_current.a;
        if (cmdName == "b") return m_current.b;
        if (cmdName == "c") return m_current.c;
        if (cmdName == "s") return m_current.s;

        // 方向 (非 hold)
        if (cmdName == "fwd" || cmdName == "F") return dir == DirInput::F;
        if (cmdName == "back" || cmdName == "B") return dir == DirInput::B;
        if (cmdName == "down" || cmdName == "D") return dir == DirInput::D;
        if (cmdName == "up" || cmdName == "U") return dir == DirInput::U;
        if (cmdName == "downfwd" || cmdName == "DF") return dir == DirInput::DF;
        if (cmdName == "downback" || cmdName == "DB") return dir == DirInput::DB;
        if (cmdName == "upfwd" || cmdName == "UF") return dir == DirInput::UF;
        if (cmdName == "upback" || cmdName == "UB") return dir == DirInput::UB;

        return false;
    }

    bool InputManager::commandBufferMatch(const std::vector<DirInput>& seq, int window) const {
        if (seq.empty() || m_history.size() < seq.size()) return false;

        int histSize = static_cast<int>(m_history.size());
        int checkLen = std::min(window, histSize);

        // 从最新帧往前匹配序列 (从seq的末尾开始匹配)
        int seqIdx = static_cast<int>(seq.size()) - 1;
        for (int i = histSize - 1; i >= histSize - checkLen && seqIdx >= 0; i--) {
            if (i < 0) break;
            if (m_history[i].dir == seq[seqIdx]) {
                seqIdx--;
            }
        }
        return seqIdx < 0;
    }

    // 双击方向检测
    bool InputManager::doubleTap(DirInput dir, int window) const {
        if (m_history.size() < 3) return false;

        int lastFrame = static_cast<int>(m_history.size()) - 1;
        // 当前帧方向必须是指定方向
        if (m_history[lastFrame].dir != dir) return false;

        // 往前找相同方向，中间有间隔(松开)
        bool foundGap = false;
        for (int i = lastFrame - 1; i >= std::max(0, lastFrame - window); i--) {
            if (m_history[i].dir == dir) {
                return foundGap; // 找到第二次出现，且中间有间隔
            }
            if (m_history[i].dir == DirInput::NONE) {
                foundGap = true; // 中间有过方向松开
            }
        }
        return false;
    }

    const FrameInput& InputManager::getFrame(int offset) const {
        if (offset == 0) return m_current;
        int idx = static_cast<int>(m_history.size()) - 1 - offset;
        if (idx >= 0 && idx < static_cast<int>(m_history.size())) {
            return m_history[idx];
        }
        return m_current;
    }

} // namespace db
