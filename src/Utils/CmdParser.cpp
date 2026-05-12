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

    // 方向匹配 (含 facing-relative 和 4-way 容差)
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

    // 方向历史去重压缩: 去除连续重复, 只保留变化点
    static std::vector<DirInput> compressDirHistory(const InputManager& input, int maxFrames) {
        std::vector<DirInput> hist;
        DirInput last = DirInput::NONE;
        for (int i = 0; i < maxFrames; i++) {
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
                // 去掉尾部分号注释
                size_t semicolon = value.find(';');
                if (semicolon != std::string::npos) value = value.substr(0, semicolon);
                value = trim(value);
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

            // 检查是否带 ~ (释放方向)
            if (part[0] == '~') {
                std::string rest = part.substr(1);
                // 可选: ~30 蓄力数字前缀, 先忽略数字部分
                while (!rest.empty() && std::isdigit(rest[0])) rest = rest.substr(1);
                if (rest.empty()) continue;
                // 检查 $ 4-way
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

    bool CmdParser::evaluateCommand(const CommandDef& cmd, const InputManager& input, bool facingRight) const {
        auto tokens = tokenize(cmd.rawCommand);
        if (tokens.empty()) return false;

        // 特殊情况：双方向轻击 (FF, BB)
        // "F, F" 格式的 2 个方向 token 序列 → 用双轻击检测
        if (tokens.size() == 2 &&
            tokens[0].type == CommandToken::DIR &&
            tokens[1].type == CommandToken::DIR &&
            tokens[0].value == tokens[1].value) {
            DirInput dir = strToDir(tokens[0].value);
            // F/B 是相对方向: F=面朝方向(向前), B=面朝反方向(向后)
            // 需要翻译成物理方向再检查双击
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
                    // 4-way + facing-relative: F = 面朝方向(向前)
                    // 面向右时 F=F/UF/DF, 面向左时 F=B/UB/DB
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
                    // 检查当前帧 + 历史帧, 4-way + facing-relative
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

        // 多 token 方向序列匹配 (≥3 tokens, 如 ~D,DF,F,a)
        if (tokens.size() >= 3) {
            // 1. 提取方向 token 和最终按钮
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

            // 2. 获取压缩方向历史
            auto hist = compressDirHistory(input, std::min(cmd.time, 60));

            // 3. 检查按钮是否在当前帧/最近帧按下
            bool btnPressed = false;
            if (finalBtn->type == CommandToken::BUTTON) {
                for (int fb = 0; fb <= std::min(cmd.time, 10); fb++) {
                    if (input.justPressed(finalBtn->value[0])) { btnPressed = true; break; }
                }
            } else if (finalBtn->type == CommandToken::SIMUL) {
                std::stringstream ss(finalBtn->value);
                std::string item;
                bool allHeld = true;
                while (std::getline(ss, item, '+')) {
                    item = trim(item);
                    if (!item.empty() && !input.isHeld(item)) { allHeld = false; break; }
                }
                btnPressed = allHeld;
            }
            if (!btnPressed) return false;

            // 4. 在历史中搜索方向序列
            if (hist.size() >= dirTokens.size()) {
                for (size_t s = 0; s + dirTokens.size() <= hist.size(); s++) {
                    bool match = true;
                    for (size_t j = 0; j < dirTokens.size(); j++) {
                        DirInput actual = hist[s + j];
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
        // 递减所有缓冲计数
        for (auto& [name, frames] : m_buffer) {
            if (frames > 0) frames--;
        }

        m_active.clear();
        for (const auto& [name, def] : m_commands) {
            bool triggered = evaluateCommand(def, input, facingRight);
            if (triggered) {
                m_buffer[name] = def.time;  // 触发后缓冲 time 帧
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

        // 对于没有解析过的命令，检查是否是基本命令
        // (hold_x, holdfwd, x, y 等由 InputManager::isHeld 处理)
        return false;
    }

} // namespace db
