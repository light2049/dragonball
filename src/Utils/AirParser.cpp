#include "Utils/AirParser.h"
#include "Utils/SFFDatabase.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace db {

// ==========================================
// 局部辅助函数 (仅在 cpp 文件内部可见，不会引起链接错误)
// ==========================================

namespace {
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    ClsnRect parseCoordinateLine(const std::string& line) {
        std::string content = line;
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) content = line.substr(eqPos + 1);

        std::istringstream iss(content);
        std::string token;
        int x1=0, y1=0, x2=0, y2=0;

        if (std::getline(iss, token, ',')) x1 = std::stoi(trim(token));
        if (std::getline(iss, token, ',')) y1 = std::stoi(trim(token));
        if (std::getline(iss, token, ',')) x2 = std::stoi(trim(token));
        if (std::getline(iss, token, ',')) y2 = std::stoi(trim(token));

        return {
            .topLeft = {std::min(x1, x2), std::min(y1, y2)},
            .bottomRight = {std::max(x1, x2), std::max(y1, y2)}
        };
    }

    std::vector<ClsnRect> readRectsFromFile(std::ifstream& file, int count) {
        std::vector<ClsnRect> rects;
        rects.reserve(count);
        int found = 0;
        std::string line;
        while (found < count && std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line.starts_with(";")) continue;
            if (line.starts_with("[") || (std::isdigit(line[0]) && line.find(',') != std::string::npos) || line[0] == '-') break;
            try {
                rects.push_back(parseCoordinateLine(line));
                found++;
            } catch (...) {}
        }
        return rects;
    }
}

// ==========================================
// AirParser 核心实现
// ==========================================

std::map<int, Animation> AirParser::parse(const std::string& filePath,
                                          const std::string& basePath,
                                          const std::string& prefix,
                                          SFFDatabase& sffDb) {
    // 加载 SFF 轴数据库 (懒加载，仅首次成功加载后不再重复)
    // 数据库文件在角色根目录 (basePath 是 Sprites/ 子目录，需要回退一层)
    std::string dbPath = basePath + "../sprite_database_" + prefix + ".txt";
    if (!sffDb.lookup(0, 1)) {
        sffDb.load(dbPath);
    }

    std::map<int, Animation> animations;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "[AirParser] Error: Cannot open " << filePath << "\n";
        return animations;
    }

    std::string line;
    int currentGroup = -1;

    std::vector<ClsnRect> defaultClsn2;
    std::vector<ClsnRect> nextClsn1;
    std::vector<ClsnRect> nextClsn2;

    int parsedFrameCount = 0;
    int parsedClsnCount = 0;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line.starts_with(";")) continue;

        // 1. Action 开始
        if (line.starts_with("[Begin Action ")) {
            size_t end = line.find(']', 14);
            if (end != std::string::npos) {
                try {
                    currentGroup = std::stoi(trim(line.substr(14, end - 14)));
                    animations[currentGroup] = Animation{currentGroup, {}, -1};
                    defaultClsn2.clear();
                    nextClsn1.clear();
                    nextClsn2.clear();
                } catch (...) {}
            }
            continue;
        }

        // 2. 循环标记
        if (line == "LoopStart" || line == "Loopstart") {
            if (currentGroup != -1 && animations.contains(currentGroup)) {
                animations[currentGroup].loopStartIndex = static_cast<int>(animations[currentGroup].frames.size());
            }
            continue;
        }

        // 3. 碰撞框定义
        if (line.starts_with("Clsn")) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                int count = 0;
                try { count = std::stoi(trim(line.substr(colonPos + 1))); } catch (...) { continue; }

                bool isDefault = (line.find("Default") != std::string::npos);
                bool isClsn1 = (line.starts_with("Clsn1"));
                bool isClsn2 = (line.starts_with("Clsn2"));

                if (count > 0) {
                    std::vector<ClsnRect> rects = readRectsFromFile(file, count);
                    parsedClsnCount += rects.size();

                    if (isDefault && isClsn2) defaultClsn2 = rects;
                    else if (isClsn1) nextClsn1 = rects;
                    else if (isClsn2) nextClsn2 = rects;
                }
            }
            continue;
        }

        // 4. 帧数据
        if (currentGroup != -1) {
            bool looksLikeFrame = false;
            if (!line.empty() && !line.starts_with("[")) {
                for (char c : line) { if (std::isspace(c)) continue; if (std::isdigit(c) || c == '-') looksLikeFrame = true; break; }
            }

            if (looksLikeFrame) {
                std::istringstream iss(line);
                std::string token;
                int g=0, n=0, ox=0, oy=0, dur=0;
                bool flip = false;
                BlendMode blend = BlendMode::Normal;

                try {
                    std::getline(iss, token, ','); g = std::stoi(trim(token));
                    std::getline(iss, token, ','); n = std::stoi(trim(token));
                    std::getline(iss, token, ','); ox = std::stoi(trim(token));
                    std::getline(iss, token, ','); oy = std::stoi(trim(token));
                    std::getline(iss, token, ','); dur = std::stoi(trim(token));

                    // x-scale / flip flags
                    std::getline(iss, token, ',');
                    if (token.find('H') != std::string::npos) flip = true;

                    // y-scale / vertical flip
                    std::getline(iss, token, ',');

                    // transparency mode: A=additive, S=subtractive
                    std::getline(iss, token, ',');
                    std::string blendStr = trim(token);
                    if (blendStr == "A") blend = BlendMode::Additive;
                    else if (blendStr == "S") blend = BlendMode::Subtractive;

                    AnimFrame frame {
                        .texturePath = basePath + prefix + "_" + std::to_string(g) + "-" + std::to_string(n) + ".png",
                        .duration = dur,
                        .offset = {ox, oy},
                        .flipX = flip,
                        .blendMode = blend,
                        .clsn1 = nextClsn1,
                        .clsn2 = nextClsn2
                    };

                    // 用 SFF 轴数据修正 offset 并存储 raw axis
                    if (auto* sffData = sffDb.lookup(g, n)) {
                        frame.offset.x = -sffData->axisX;
                        frame.offset.y = sffData->height - sffData->axisY;
                        frame.axisX = sffData->axisX;
                        frame.axisY = sffData->axisY;
                    }

                    if (frame.clsn2.empty() && !defaultClsn2.empty()) frame.clsn2 = defaultClsn2;

                    animations[currentGroup].frames.push_back(std::move(frame));
                    parsedFrameCount++;

                    nextClsn1.clear();
                    nextClsn2.clear();

                } catch (...) {}
            }
        }
    }

    std::cout << "[AirParser] Parsing Done. Frames: " << parsedFrameCount
              << " | With Clsn: " << parsedClsnCount << "\n";
    return animations;
}

} // namespace db