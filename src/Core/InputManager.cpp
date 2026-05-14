#include "Core/InputManager.h"
#include <algorithm>

namespace db {

    // ========== KeyMapping presets ==========

    KeyMapping KeyMapping::p1() {
        return {{
            /* BTN_X */ sf::Keyboard::Key::J,
            /* BTN_Y */ sf::Keyboard::Key::I,
            /* BTN_Z */ sf::Keyboard::Key::O,
            /* BTN_A */ sf::Keyboard::Key::K,
            /* BTN_B */ sf::Keyboard::Key::L,
            /* BTN_C */ sf::Keyboard::Key::U,
            /* BTN_S */ sf::Keyboard::Key::Space,
        },{
            /* DIR_U */ sf::Keyboard::Key::W,
            /* DIR_D */ sf::Keyboard::Key::S,
            /* DIR_L */ sf::Keyboard::Key::A,
            /* DIR_R */ sf::Keyboard::Key::D,
        }};
    }

    KeyMapping KeyMapping::p2() {
        return {{
            /* BTN_X */ sf::Keyboard::Key::Numpad1,
            /* BTN_Y */ sf::Keyboard::Key::Numpad5,
            /* BTN_Z */ sf::Keyboard::Key::Numpad6,
            /* BTN_A */ sf::Keyboard::Key::Numpad2,
            /* BTN_B */ sf::Keyboard::Key::Numpad3,
            /* BTN_C */ sf::Keyboard::Key::Numpad4,
            /* BTN_S */ sf::Keyboard::Key::Numpad0,
        },{
            /* DIR_U */ sf::Keyboard::Key::Up,
            /* DIR_D */ sf::Keyboard::Key::Down,
            /* DIR_L */ sf::Keyboard::Key::Left,
            /* DIR_R */ sf::Keyboard::Key::Right,
        }};
    }

    // ========== InputManager ==========

    InputManager::InputManager()
        : m_mapping(KeyMapping::p1()) {}

    InputManager::InputManager(const KeyMapping& mapping)
        : m_mapping(mapping) {}

    void InputManager::setMapping(const KeyMapping& mapping) {
        m_mapping = mapping;
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

    static bool eventKeyHeld(const bool eventState[128], sf::Keyboard::Key key) {
        int k = static_cast<int>(key);
        return k >= 0 && k < 128 && eventState[k];
    }

    void InputManager::update() {
        m_previous = m_current;

        FrameInput snap;

        snap.x = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_X]);
        snap.y = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_Y]);
        snap.z = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_Z]);
        snap.a = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_A]);
        snap.b = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_B]);
        snap.c = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_C]);
        snap.s = eventKeyHeld(m_eventKeyState, m_mapping.buttons[BTN_S]);

        snap.charge = snap.s;

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
        bool up    = eventKeyHeld(m_eventKeyState, m_mapping.dirs[DIR_U]);
        bool down  = eventKeyHeld(m_eventKeyState, m_mapping.dirs[DIR_D]);
        bool left  = eventKeyHeld(m_eventKeyState, m_mapping.dirs[DIR_L]);
        bool right = eventKeyHeld(m_eventKeyState, m_mapping.dirs[DIR_R]);

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

        if (m_history.size() > 60) {
            m_history.pop_front();
        }
    }

    bool InputManager::isKeyPressed(sf::Keyboard::Key key) const {
        return sf::Keyboard::isKeyPressed(key);
    }

    sf::Keyboard::Key InputManager::keyForBtn(char btn) const {
        switch (btn) {
            case 'x': return m_mapping.buttons[BTN_X];
            case 'y': return m_mapping.buttons[BTN_Y];
            case 'z': return m_mapping.buttons[BTN_Z];
            case 'a': return m_mapping.buttons[BTN_A];
            case 'b': return m_mapping.buttons[BTN_B];
            case 'c': return m_mapping.buttons[BTN_C];
            case 's': return m_mapping.buttons[BTN_S];
            default:  return sf::Keyboard::Key::Unknown;
        }
    }

    bool InputManager::justPressed(char btn) const {
        sf::Keyboard::Key key = keyForBtn(btn);
        int k = static_cast<int>(key);
        bool latch = (k >= 0 && k < 128) ? m_justPressedLatch[k] : false;

        switch (btn) {
            case 'x': return latch || (m_current.x && !m_previous.x);
            case 'y': return latch || (m_current.y && !m_previous.y);
            case 'z': return latch || (m_current.z && !m_previous.z);
            case 'a': return latch || (m_current.a && !m_previous.a);
            case 'b': return latch || (m_current.b && !m_previous.b);
            case 'c': return latch || (m_current.c && !m_previous.c);
            case 's': return latch || (m_current.s && !m_previous.s);
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

        auto it = m_commandResults.find(cmdName);
        if (it != m_commandResults.end()) {
            return it->second;
        }

        DirInput dir = m_current.dir;

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

        if (cmdName == "holddownfwd" || cmdName == "DF") return dir == DirInput::DF;
        if (cmdName == "holddownback" || cmdName == "DB") return dir == DirInput::DB;
        if (cmdName == "holdupfwd" || cmdName == "UF") return dir == DirInput::UF;
        if (cmdName == "holdupback" || cmdName == "UB") return dir == DirInput::UB;

        if (cmdName == "hold_x") return m_current.x;
        if (cmdName == "hold_y") return m_current.y;
        if (cmdName == "hold_z") return m_current.z;
        if (cmdName == "hold_a") return m_current.a;
        if (cmdName == "hold_b") return m_current.b;
        if (cmdName == "hold_c") return m_current.c;
        if (cmdName == "hold_s") return m_current.s;

        if (cmdName == "x") return m_current.x;
        if (cmdName == "y") return m_current.y;
        if (cmdName == "z") return m_current.z;
        if (cmdName == "a") return m_current.a;
        if (cmdName == "b") return m_current.b;
        if (cmdName == "c") return m_current.c;
        if (cmdName == "s") return m_current.s;

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

        int seqIdx = static_cast<int>(seq.size()) - 1;
        for (int i = histSize - 1; i >= histSize - checkLen && seqIdx >= 0; i--) {
            if (i < 0) break;
            if (m_history[i].dir == seq[seqIdx]) {
                seqIdx--;
            }
        }
        return seqIdx < 0;
    }

    bool InputManager::doubleTap(DirInput dir, int window) const {
        if (m_history.size() < 3) return false;

        int lastFrame = static_cast<int>(m_history.size()) - 1;

        if (m_history[lastFrame].dir != dir) return false;

        bool foundGap = false;
        for (int i = lastFrame - 1; i >= std::max(0, lastFrame - window); i--) {
            if (m_history[i].dir == dir) {
                return foundGap;
            }
            if (m_history[i].dir == DirInput::NONE) {
                foundGap = true;
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

}
