#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Core/InputManager.h"

namespace db {

    // 单个命令定义
    struct CommandDef {
        std::string name;              // 命令名, 如 "FF", "6000"
        std::string rawCommand;        // 原始命令字符串, 如 "F, F"
        int time = 15;                 // 输入缓冲时间 (ticks)
    };

    class CmdParser {
    public:
        // 加载 .cmd 文件
        void load(const std::string& path);

        // 每帧计算所有命令的激活状态
        void evaluate(const InputManager& input, bool facingRight = true);

        // 查询某命令是否激活 (帧缓存)
        bool isActive(const std::string& name) const;

        // 重置所有命令的缓冲 (当状态变更时调用，防止缓冲残留)
        void resetBuffers();

        // 获取解析出的命令定义
        const std::map<std::string, CommandDef>& getCommands() const { return m_commands; }

    private:
        // 解析命令模式字符串
        struct CommandToken {
            enum Type {
                DIR,       // 方向: F, B, D, U, DF, DB, UF, UB
                BUTTON,    // 按钮: x, y, z, a, b, c, s
                HOLD_DIR,  // 按住方向: /$F, /$B, /$D, /$U
                HOLD_BTN,  // 按住按钮: /x, /y, /z, /a, /b, /c, /s
                SIMUL,     // 同时按: x+y, a+b 等
            };
            Type type;
            std::string value;
            bool released = false;  // ~ 前缀: 方向释放检测
        };

        // 将命令字符串解析为 token 序列
        std::vector<CommandToken> tokenize(const std::string& cmdStr) const;

        // 评估单条命令
        bool evaluateCommand(const CommandDef& cmd, const InputManager& input, bool facingRight) const;

        std::map<std::string, CommandDef> m_commands;
        std::map<std::string, bool> m_active; // 每帧计算结果缓存
        std::map<std::string, int> m_buffer;  // 命令缓冲帧计数
    };

} // namespace db
