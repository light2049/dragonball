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

    // 方向字符串 → DirInput
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
                // 允许同一命令名有多个模式 (如 recovery 有 x+y, y+z, x+z 等)
                // 用 name+index 作为 key 会导致查询复杂，这里只覆盖
                // 更好: 用 vector 存多模式
                // 我们直接用 name 作 key，已有定义则追加
                if (m_commands.find(current.name) != m_commands.end()) {
                    // 同一命令已有定义，追加新定义用 name + "_altN"
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
            // 跳过 BOM
            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }

            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start);

            // 注释
            if (line[0] == ';') continue;

            std::string lowerLine = toLower(line);

            // [Command] 块开始
            if (lowerLine.starts_with("[command]")) {
                finalizeCommand();
                inCommand = true;
                continue;
            }

            // 其他块结束当前命令解析
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
                // 去掉引号
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

        // 按逗号分割 (方向序列/按键+按键 用逗号分隔)
        std::vector<std::string> parts;
        size_t pos = 0;
        while ((pos = str.find(',')) != std::string::npos) {
            parts.push_back(trim(str.substr(0, pos)));
            str = trim(str.substr(pos + 1));
        }
        if (!str.empty()) parts.push_back(trim(str));

        for (const auto& part : parts) {
            CommandToken token;

            // 检查是否是同时按键 (如 "x+y")
            if (part.find('+') != std::string::npos) {
                token.type = CommandToken::SIMUL;
                token.value = part;
                tokens.push_back(token);
                continue;
            }

            // 检查是否带 / (hold)
            if (part[0] == '/') {
                std::string rest = part.substr(1);
                // 如果后面是 $ 表示 4-way 方向
                if (rest[0] == '$' && rest.size() > 1) {
                    token.type = CommandToken::HOLD_DIR;
                    token.value = rest.substr(1);
                } else {
                    // 检查是否是方向键还是按钮
                    // M.U.G.E.N 规范: 方向大写(F/B/D/U), 按钮小写(a/b/c/x/y/z/s)
                    std::string upper = rest;
                    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                    if ((upper == "F" || upper == "B" || upper == "D" || upper == "U" ||
                         upper == "DF" || upper == "DB" || upper == "UF" || upper == "UB") &&
                        rest == upper) {  // 只有原字符串是大写时才视为方向
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

            // 检查是否带 $ (4-way 方向)
            if (part[0] == '$' && part.size() > 1) {
                token.type = CommandToken::DIR;
                token.value = part.substr(1);
                std::transform(token.value.begin(), token.value.end(), token.value.begin(), ::toupper);
                tokens.push_back(token);
                continue;
            }

            // 普通方向或按钮
            // M.U.G.E.N 规范: 方向大写(F/B/D/U), 按钮小写(a/b/c/x/y/z/s)
            std::string upper = part;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if ((upper == "F" || upper == "B" || upper == "D" || upper == "U" ||
                 upper == "DF" || upper == "DB" || upper == "UF" || upper == "UB") &&
                part == upper) {  // 只有原字符串是大写时才视为方向
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

    bool CmdParser::evaluateCommand(const CommandDef& cmd, const InputManager& input) const {
        auto tokens = tokenize(cmd.rawCommand);
        if (tokens.empty()) return false;

        // 特殊情况：双方向轻击 (FF, BB)
        // "F, F" 格式的 2 个方向 token 序列 → 用双轻击检测
        if (tokens.size() == 2 &&
            tokens[0].type == CommandToken::DIR &&
            tokens[1].type == CommandToken::DIR &&
            tokens[0].value == tokens[1].value) {
            DirInput dir = strToDir(tokens[0].value);
            if (dir != DirInput::NONE) {
                return input.doubleTap(dir, cmd.time);
            }
        }

        // 单 token 模式: 直接检查当前状态
        if (tokens.size() == 1) {
            const auto& t = tokens[0];
            switch (t.type) {
                case CommandToken::BUTTON: {
                    // M.U.G.E.N 规范: 不带 / 的按钮是"按下检测"(justPressed)
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
                    // 4-way: F = F|UF|DF, etc.
                    if (t.value == "F") return input.isDirHeld(DirInput::F) || input.isDirHeld(DirInput::UF) || input.isDirHeld(DirInput::DF);
                    if (t.value == "B") return input.isDirHeld(DirInput::B) || input.isDirHeld(DirInput::UB) || input.isDirHeld(DirInput::DB);
                    if (t.value == "D") return input.isDirHeld(DirInput::D) || input.isDirHeld(DirInput::DF) || input.isDirHeld(DirInput::DB);
                    if (t.value == "U") return input.isDirHeld(DirInput::U) || input.isDirHeld(DirInput::UF) || input.isDirHeld(DirInput::UB);
                    return input.isDirHeld(dir);
                }
                case CommandToken::HOLD_BTN:
                    return input.isHeld("hold_" + t.value);
                case CommandToken::SIMUL: {
                    // "x+y" → x && y
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

        // 多 token 模式 (如 "F, x" = 方向F + 按键x)
        if (tokens.size() == 2) {
            // 模式: [HOLD_DIR/HOLD_BTN/DIR, BUTTON] = 按住 + 按键
            bool firstHeld = false;

            // 检查第一个 token
            const auto& t0 = tokens[0];
            switch (t0.type) {
                case CommandToken::DIR: {
                    // 检查当前帧 + 历史帧 (M.U.G.E.N 标准: 方向在 command.time 帧内出现过即匹配)
                    // 同时使用 4-way 匹配 (F 匹配 F|UF|DF)
                    DirInput targetDir = strToDir(t0.value);
                    auto dirMatch = [&](DirInput d) -> bool {
                        if (d == targetDir) return true;
                        if (t0.value == "F") return d == DirInput::UF || d == DirInput::DF;
                        if (t0.value == "B") return d == DirInput::UB || d == DirInput::DB;
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
                    // 映射方向到 hold 名称
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

            // 检查第二个 token (通常是按钮)
            const auto& t1 = tokens[1];
            if (t1.type == CommandToken::BUTTON) {
                return input.justPressed(t1.value[0]);
            }
            if (t1.type == CommandToken::DIR) {
                return input.isDirHeld(strToDir(t1.value));
            }
            if (t1.type == CommandToken::SIMUL) {
                // 同时按键（如 F 状态下按 x+y）
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

        return false;
    }

    void CmdParser::evaluate(const InputManager& input) {
        // 递减所有缓冲计数
        for (auto& [name, frames] : m_buffer) {
            if (frames > 0) frames--;
        }

        m_active.clear();
        for (const auto& [name, def] : m_commands) {
            bool triggered = evaluateCommand(def, input);
            if (triggered) {
                m_buffer[name] = def.time;  // 触发后缓冲 time 帧
            }
            m_active[name] = (m_buffer[name] > 0);
        }
    }

    bool CmdParser::isActive(const std::string& name) const {
        auto it = m_active.find(name);
        if (it != m_active.end()) return it->second;

        // 对于没有解析过的命令，检查是否是基本命令
        // (hold_x, holdfwd, x, y 等由 InputManager::isHeld 处理)
        return false;
    }

} // namespace db
