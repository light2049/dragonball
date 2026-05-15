#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace db {


    struct ClsnRect {
        sf::Vector2i topLeft;
        sf::Vector2i bottomRight;
        int getWidth() const { return std::abs(bottomRight.x - topLeft.x); }
        int getHeight() const { return std::abs(bottomRight.y - topLeft.y); }

        sf::FloatRect toLocalRect() const {
            float left = static_cast<float>(std::min(topLeft.x, bottomRight.x));
            float top = static_cast<float>(std::min(topLeft.y, bottomRight.y));
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

    class AirParser {
    public:
        static std::map<int, Animation> parse(const std::string& filePath,
                                              const std::string& basePath,
                                              const std::string& prefix,
                                              class SFFDatabase& sffDb);
    };

} // namespace db