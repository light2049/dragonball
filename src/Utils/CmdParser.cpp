#include "Utils/CmdParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace db {

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string toLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    static DirInput strToDir(const std::string& s) {
        std::string u = s;
        std::transform(u.begin(), u.end(), u.begin(), ::toupper);
        if (u == "F")  return DirInput::F;
        if (u == "B")  return DirInput::B;
        if (u == "D")  return DirInput::D;
        if (u == "U")  return DirInput::U;
        if (u == "DF") return DirInput::DF;
        if (u == "DB") return DirInput::DB;
        if (u == "UF") return DirInput::UF;
        if (u == "UB") return DirInput::UB;
        return DirInput::NONE;
    }

    static bool dirMatches(DirInput actual, const std::string& expected, bool facingRight) {
        DirInput target = strToDir(expected);
        if (actual == target) return true;
        if (actual == DirInput::NONE) return false;
        if (expected == "F") {
            if (facingRight) return actual == DirInput::UF || actual == DirInput::DF;
            else return actual == DirInput::UB || actual == DirInput::DB;
        }
        if (expected == "B") {
            if (facingRight) return actual == DirInput::UB || actual == DirInput::DB;
            else return actual == DirInput::UF || actual == DirInput::DF;
        }
        if (expected == "D") return actual == DirInput::DF || actual == DirInput::DB;
        if (expected == "U") return actual == DirInput::UF || actual == DirInput::UB;
        return false;
    }

    static std::vector<DirInput> compressDirHistory(const InputManager& input, int maxFrames) {
        std::vector<DirInput> hist;
        DirInput last = DirInput::NONE;
        for (int i = maxFrames - 1; i >= 0; i--) {
            auto frame = input.getFrame(i);
            if (frame.dir != last && frame.dir != DirInput::NONE) {
                hist.push_back(frame.dir);
                last = frame.dir;
            }
        }
        return hist;
    }

    void CmdParser::load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[CmdParser] Error: Cannot open " << path << "\n";
            return;
        }

        std::string line;
        bool inCommand = false;
        CommandDef current;

        auto finalizeCommand = [&]() {
            if (!current.name.empty()) {

                if (m_commands.find(current.name) != m_commands.end()) {

                    int alt = 1;
                    std::string altName;
                    do {
                        altName = current.name + "_alt" + std::to_string(alt++);
                    } while (m_commands.find(altName) != m_commands.end());
                    m_commands[altName] = current;
                } else {
                    m_commands[current.name] = current;
                }
                current = CommandDef();
            }
        };

        while (std::getline(file, line)) {

            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }

            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start);

            if (line[0] == ';') continue;

            std::string lowerLine = toLower(line);

            if (lowerLine.starts_with("[command]")) {
                finalizeCommand();
                inCommand = true;
                continue;
            }

            if (line[0] == '[' && line.back() == ']' && !lowerLine.starts_with("[command]")) {
                finalizeCommand();
                inCommand = false;
                continue;
            }

            if (!inCommand) continue;

            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = trim(line.substr(0, eqPos));
            std::string value = trim(line.substr(eqPos + 1));
            std::string lowerKey = toLower(key);

            if (lowerKey == "name") {

                size_t semicolon = value.find(';');
                if (semicolon != std::string::npos) value = value.substr(0, semicolon);
                value = trim(value);

                if (value.size() >= 2 && value[0] == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                current.name = value;
            }
            else if (lowerKey == "command") {
                current.rawCommand = value;
            }
            else if (lowerKey == "time") {
                try { current.time = std::stoi(value); } catch (...) {}
            }
        }

        finalizeCommand();

        std::cout << "[CmdParser] Loaded " << m_commands.size() << " commands from " << path << "\n";
    }

    std::vector<CmdParser::CommandToken> CmdParser::tokenize(const std::string& cmdStr) const {
        std::vector<CommandToken> tokens;
        std::string str = trim(cmdStr);
        if (str.empty()) return tokens;

        std::vector<std::string> parts;
        size_t pos = 0;
        while ((pos = str.find(',')) != std::string::npos) {
            parts.push_back(trim(str.substr(0, pos)));
            str = trim(str.substr(pos + 1));
        }
        if (!str.empty()) parts.push_back(trim(str));

        for (const auto& part : parts) {
            CommandToken token;

            if (part.find('+') != std::string::npos) {
                token.type = CommandToken::SIMUL;
                token.value = part;
                tokens.push_back(token);
                continue;
            }

            if (part[0] == '/') {
                std::string rest = part.substr(1);

                if (rest[0] == '$' && rest.size() > 1) {
                    token.type = CommandToken::HOLD_DIR;
                    token.value = rest.substr(1);
                } else {

                    std::string upper = rest;
                    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                    if ((upper == "F" || upper == "B" || upper == "D" || upper == "U" ||
                         upper == "DF" || upper == "DB" || upper == "UF" || upper == "UB") &&
                        rest == upper) {
                        token.type = CommandToken::HOLD_DIR;
                        token.value = upper;
                    } else {
                        token.type = CommandToken::HOLD_BTN;
                        token.value = toLower(rest);
                    }
                }
                tokens.push_back(token);
                continue;
            }

            if (part[0] == '$' && part.size() > 1) {
                token.type = CommandToken::DIR;
                token.value = part.substr(1);
                std::transform(token.value.begin(), token.value.end(), token.value.begin(), ::toupper);
                tokens.push_back(token);
                continue;
            }

            if (part[0] == '~') {
                std::string rest = part.substr(1);

                while (!rest.empty() && std::isdigit(rest[0])) rest = rest.substr(1);
                if (rest.empty()) continue;

                if (rest[0] == '$' && rest.size() > 1) {
                    token.type = CommandToken::HOLD_DIR;
                    token.value = rest.substr(1);
                    std::transform(token.value.begin(), token.value.end(), token.value.begin(), ::toupper);
                } else {
                    std::string upper = rest;
                    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                    if ((upper == "F" || upper == "B" || upper == "D" || upper == "U" ||
                         upper == "DF" || upper == "DB" || upper == "UF" || upper == "UB") &&
                        rest == upper) {
                        token.type = CommandToken::DIR;
                        token.value = upper;
                    } else if (rest.size() == 1) {
                        token.type = CommandToken::BUTTON;
                        token.value = toLower(rest);
                    } else {
                        continue;
                    }
                }
                token.released = true;
                tokens.push_back(token);
                continue;
            }

            std::string upper = part;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if ((upper == "F" || upper == "B" || upper == "D" || upper == "U" ||
                 upper == "DF" || upper == "DB" || upper == "UF" || upper == "UB") &&
                part == upper) {
                token.type = CommandToken::DIR;
                token.value = upper;
            } else {
                token.type = CommandToken::BUTTON;
                token.value = toLower(part);
            }
            tokens.push_back(token);
        }

        return tokens;
    }

    bool CmdParser::evaluateCommand(const CommandDef& cmd, const InputManager& input, bool facingRight) const {
        auto tokens = tokenize(cmd.rawCommand);
        if (tokens.empty()) return false;

        if (tokens.size() == 2 &&
            tokens[0].type == CommandToken::DIR &&
            tokens[1].type == CommandToken::DIR &&
            tokens[0].value == tokens[1].value) {
            DirInput dir = strToDir(tokens[0].value);

            if (tokens[0].value == "F") {
                dir = facingRight ? DirInput::F : DirInput::B;
                bool result = input.doubleTap(dir, cmd.time);
                if (result) std::cout << "[Cmd] " << cmd.name << " triggered dir=" << (int)dir << " facing=" << (facingRight?"R":"L") << std::endl;
                return result;
            }
            if (tokens[0].value == "B") {
                dir = facingRight ? DirInput::B : DirInput::F;
                bool result = input.doubleTap(dir, cmd.time);
                if (result) std::cout << "[Cmd] " << cmd.name << " triggered dir=" << (int)dir << " facing=" << (facingRight?"R":"L") << std::endl;
                return result;
            }
            if (dir != DirInput::NONE) {
                bool result = input.doubleTap(dir, cmd.time);
                if (result) std::cout << "[Cmd] " << cmd.name << " triggered dir=" << (int)dir << " facing=" << (facingRight?"R":"L") << std::endl;
                return result;
            }
        }

        if (tokens.size() == 1) {
            const auto& t = tokens[0];
            switch (t.type) {
                case CommandToken::BUTTON: {

                    if (t.value == "x") return input.justPressed('x');
                    if (t.value == "y") return input.justPressed('y');
                    if (t.value == "z") return input.justPressed('z');
                    if (t.value == "a") return input.justPressed('a');
                    if (t.value == "b") return input.justPressed('b');
                    if (t.value == "c") return input.justPressed('c');
                    if (t.value == "s") return input.justPressed('s');
                    return false;
                }
                case CommandToken::DIR:
                    return input.isDirHeld(strToDir(t.value));
                case CommandToken::HOLD_DIR: {
                    DirInput dir = strToDir(t.value);

                    if (t.value == "F") {
                        if (facingRight) return input.isDirHeld(DirInput::F) || input.isDirHeld(DirInput::UF) || input.isDirHeld(DirInput::DF);
                        else return input.isDirHeld(DirInput::B) || input.isDirHeld(DirInput::UB) || input.isDirHeld(DirInput::DB);
                    }
                    if (t.value == "B") {
                        if (facingRight) return input.isDirHeld(DirInput::B) || input.isDirHeld(DirInput::UB) || input.isDirHeld(DirInput::DB);
                        else return input.isDirHeld(DirInput::F) || input.isDirHeld(DirInput::UF) || input.isDirHeld(DirInput::DF);
                    }
                    if (t.value == "D") return input.isDirHeld(DirInput::D) || input.isDirHeld(DirInput::DF) || input.isDirHeld(DirInput::DB);
                    if (t.value == "U") return input.isDirHeld(DirInput::U) || input.isDirHeld(DirInput::UF) || input.isDirHeld(DirInput::UB);
                    return input.isDirHeld(dir);
                }
                case CommandToken::HOLD_BTN:
                    return input.isHeld("hold_" + t.value);
                case CommandToken::SIMUL: {

                    std::vector<std::string> btns;
                    std::stringstream ss(t.value);
                    std::string item;
                    while (std::getline(ss, item, '+')) {
                        btns.push_back(trim(item));
                    }
                    if (btns.empty()) return false;
                    for (const auto& b : btns) {
                        if (b == "x" && !input.buttonX()) return false;
                        if (b == "y" && !input.buttonY()) return false;
                        if (b == "z" && !input.buttonZ()) return false;
                        if (b == "a" && !input.buttonA()) return false;
                        if (b == "b" && !input.buttonB()) return false;
                        if (b == "c" && !input.buttonC()) return false;
                        if (b == "s" && !input.buttonS()) return false;
                    }
                    return true;
                }
            }
        }

        if (tokens.size() == 2) {

            bool firstHeld = false;

            const auto& t0 = tokens[0];
            switch (t0.type) {
                case CommandToken::DIR: {

                    DirInput targetDir = strToDir(t0.value);
                    auto dirMatch = [&](DirInput d) -> bool {
                        if (d == targetDir) return true;
                        if (t0.value == "F") {
                            if (facingRight) return d == DirInput::UF || d == DirInput::DF;
                            else return d == DirInput::UB || d == DirInput::DB;
                        }
                        if (t0.value == "B") {
                            if (facingRight) return d == DirInput::UB || d == DirInput::DB;
                            else return d == DirInput::UF || d == DirInput::DF;
                        }
                        if (t0.value == "D") return d == DirInput::DF || d == DirInput::DB;
                        if (t0.value == "U") return d == DirInput::UF || d == DirInput::UB;
                        return false;
                    };
                    firstHeld = dirMatch(input.getDirection());
                    if (!firstHeld) {
                        for (int i = 1; i <= std::min(cmd.time, 60); i++) {
                            auto frame = input.getFrame(i);
                            if (dirMatch(frame.dir)) {
                                firstHeld = true;
                                break;
                            }
                        }
                    }
                    break;
                }
                case CommandToken::HOLD_DIR: {

                    std::string dirLower = toLower(t0.value);
                    if (dirLower == "f") firstHeld = input.isHeld("holdfwd");
                    else if (dirLower == "b") firstHeld = input.isHeld("holdback");
                    else if (dirLower == "d") firstHeld = input.isHeld("holddown");
                    else if (dirLower == "u") firstHeld = input.isHeld("holdup");
                    else if (dirLower == "df") firstHeld = input.isHeld("holddownfwd");
                    else if (dirLower == "db") firstHeld = input.isHeld("holddownback");
                    else if (dirLower == "uf") firstHeld = input.isHeld("holdupfwd");
                    else if (dirLower == "ub") firstHeld = input.isHeld("holdupback");
                    else firstHeld = false;
                    break;
                }
                case CommandToken::HOLD_BTN:
                    firstHeld = input.isHeld("hold_" + t0.value);
                    break;
                case CommandToken::BUTTON:
                    firstHeld = input.isHeld(t0.value);
                    break;
                default:
                    firstHeld = false;
            }

            if (!firstHeld) return false;

            const auto& t1 = tokens[1];
            if (t1.type == CommandToken::BUTTON) {
                return input.justPressed(t1.value[0]);
            }
            if (t1.type == CommandToken::DIR) {
                return input.isDirHeld(strToDir(t1.value));
            }
            if (t1.type == CommandToken::SIMUL) {

                std::stringstream ss(t1.value);
                std::string item;
                bool allPressed = true;
                while (std::getline(ss, item, '+')) {
                    item = trim(item);
                    if (!item.empty() && !input.isHeld(item)) {
                        allPressed = false;
                        break;
                    }
                }
                return allPressed;
            }
        }

        if (tokens.size() >= 3) {

            std::vector<const CommandToken*> dirTokens;
            const CommandToken* finalBtn = nullptr;
            for (const auto& t : tokens) {
                if (t.type == CommandToken::DIR || t.type == CommandToken::HOLD_DIR) {
                    dirTokens.push_back(&t);
                } else if (t.type == CommandToken::BUTTON || t.type == CommandToken::SIMUL) {
                    finalBtn = &t;
                    break;
                }
            }
            if (dirTokens.size() < 2 || !finalBtn) return false;

            int window = std::min(cmd.time, 60);

            int btnFrame = -1;
            auto checkBtnState = [&](const FrameInput& fi) -> bool {
                if (finalBtn->type == CommandToken::SIMUL) return false;
                char c = finalBtn->value[0];
                if (c == 'x') return fi.x; if (c == 'y') return fi.y;
                if (c == 'z') return fi.z; if (c == 'a') return fi.a;
                if (c == 'b') return fi.b; if (c == 'c') return fi.c;
                if (c == 's') return fi.s;
                return false;
            };
            for (int fb = 0; fb < window; fb++) {
                auto frame = input.getFrame(fb);
                if (checkBtnState(frame)) {
                    auto prev = input.getFrame(fb + 1);
                    if (!checkBtnState(prev)) {
                        btnFrame = fb;
                        break;
                    }
                }
            }

            if (btnFrame < 0 && finalBtn->type == CommandToken::SIMUL) {
                std::stringstream ss(finalBtn->value);
                std::string item;
                bool allHeld = true;
                while (std::getline(ss, item, '+')) {
                    item = trim(item);
                    if (!item.empty() && !input.isHeld(item)) { allHeld = false; break; }
                }
                if (allHeld) btnFrame = 0;
            }

            if (btnFrame < 0) return false;

            auto dirHist = compressDirHistory(input, window);

            if (!cmd.name.empty() && cmd.name[0] >= '6' && cmd.name[0] <= '9') {
                static int dbgFrames = 0;
                if (++dbgFrames % 60 == 1) {
                }
            }

            if (dirHist.size() >= dirTokens.size()) {
                for (size_t s = 0; s + dirTokens.size() <= dirHist.size(); s++) {
                    bool match = true;
                    for (size_t j = 0; j < dirTokens.size(); j++) {
                        DirInput actual = dirHist[s + j];
                        const std::string& expected = dirTokens[j]->value;
                        if (!dirMatches(actual, expected, facingRight)) {
                            match = false;
                            break;
                        }
                    }
                    if (match) return true;
                }
            }
        }

        return false;
    }

    void CmdParser::evaluate(const InputManager& input, bool facingRight) {

        for (auto& [name, frames] : m_buffer) {
            if (frames > 0) frames--;
        }

        m_active.clear();
        for (const auto& [name, def] : m_commands) {
            bool triggered = evaluateCommand(def, input, facingRight);
            if (triggered) {
                m_buffer[name] = def.time;
            }
            m_active[name] = (m_buffer[name] > 0);
        }
    }

    void CmdParser::resetBuffers() {
        m_buffer.clear();
        m_active.clear();
    }

    bool CmdParser::isActive(const std::string& name) const {
        auto it = m_active.find(name);
        if (it != m_active.end()) return it->second;

        return false;
    }

}
