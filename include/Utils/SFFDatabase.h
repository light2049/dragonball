#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

namespace db {

    struct SpriteAxisData {
        int width = 0;
        int height = 0;
        int axisX = 0;
        int axisY = 0;
    };

    class SFFDatabase {
    public:
        SFFDatabase() = default;

        bool load(const std::string& filePath);

        // 按 (group, image) 查找轴数据
        const SpriteAxisData* lookup(int group, int image) const;

    private:
        // key = (group << 16) | image
        std::unordered_map<int, SpriteAxisData> m_data;
    };

} // namespace db
