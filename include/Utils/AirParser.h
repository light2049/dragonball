#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace db {

    // ==========================================
    // 数据结构定义
    // ==========================================

    struct ClsnRect {
        sf::Vector2i topLeft;
        sf::Vector2i bottomRight;
        int getWidth() const { return std::abs(bottomRight.x - topLeft.x); }
        int getHeight() const { return std::abs(bottomRight.y - topLeft.y); }

        sf::FloatRect toLocalRect() const {
            float left = static_cast<float>(std::min(topLeft.x, bottomRight.x));
            float top = static_cast<float>(std::min(topLeft.y, bottomRight.y));
            // SFML 3 语法：使用 {位置向量, 大小向量}
            return sf::FloatRect({left, top}, {static_cast<float>(getWidth()), static_cast<float>(getHeight())});
        }
    };

    enum class BlendMode : uint8_t {
        Normal,     // 标准 alpha 混合 (默认)
        Additive,   // 加色混合 (A)
        Subtractive // 减色混合 (S)
    };

    struct AnimFrame {
        std::string texturePath;
        int duration;
        sf::Vector2i offset;
        int axisX = 0;     // SFF 轴 X (纹理像素)
        int axisY = 0;     // SFF 轴 Y (纹理像素)
        bool flipX;
        BlendMode blendMode = BlendMode::Normal;
        std::vector<ClsnRect> clsn1;
        std::vector<ClsnRect> clsn2;
        bool hasHitbox() const { return !clsn2.empty(); }
        bool hasHurtbox() const { return !clsn1.empty(); }
    };

    struct Animation {
        int id;
        std::vector<AnimFrame> frames;
        int loopStartIndex;

        const AnimFrame& getFrame(size_t index) const {
            if (frames.empty()) { static const AnimFrame e = {}; return e; }
            return frames[index % frames.size()];
        }
        size_t getFrameCount() const { return frames.size(); }
    };

    // ==========================================
    // 解析器类 (仅暴露 parse 接口)
    // ==========================================
    class AirParser {
    public:
        static std::map<int, Animation> parse(const std::string& filePath,
                                              const std::string& basePath,
                                              const std::string& prefix);
    };

} // namespace db