#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <deque>
#include <vector>
#include <map>

namespace db {

    enum class DirInput {
        NONE, B, DB, D, DF, F, UF, U, UB
    };

    struct FrameInput {

        bool x = false;
        bool y = false;
        bool z = false;
        bool a = false;
        bool b = false;
        bool c = false;
        bool s = false;
        DirInput dir = DirInput::NONE;
        bool charge = false;
    };

    struct SimpleInputState {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        bool punch = false;
        bool kick = false;
        bool charge = false;
    };

    class InputManager {
    public:
        void update();

        void onKeyPressed(sf::Keyboard::Key key);
        void onKeyReleased(sf::Keyboard::Key key);
        void clearJustPressedLatch();

        void reset();

        bool isKeyPressed(sf::Keyboard::Key key) const;

        bool justPressed(char btn) const;
        bool justReleased(char btn) const;

        bool isKeyJustPressed(sf::Keyboard::Key key) const {
            int k = static_cast<int>(key);
            if (k >= 0 && k < 128) return m_justPressedLatch[k];
            return false;
        }

        bool buttonX() const;
        bool buttonY() const;
        bool buttonZ() const;
        bool buttonA() const;
        bool buttonB() const;
        bool buttonC() const;
        bool buttonS() const;

        DirInput getDirection() const;
        bool isHeld(const std::string& cmdName) const;
        bool isDirHeld(DirInput dir) const;

        bool commandBufferMatch(const std::vector<DirInput>& seq, int window = 15) const;
        bool doubleTap(DirInput dir, int window = 10) const;

        const FrameInput& getFrame(int offset) const;

        void setCommandResult(const std::string& name, bool active) { m_commandResults[name] = active; }
        void clearCommandResults() { m_commandResults.clear(); }

    private:
        void updateDirection(FrameInput& snap);
        void pushHistory(const FrameInput& snap);

        FrameInput m_current;
        FrameInput m_previous;
        std::deque<FrameInput> m_history;
        std::map<std::string, bool> m_commandResults;

        bool m_justPressedLatch[128] = {};

        bool m_eventKeyState[128] = {};
    };

}
