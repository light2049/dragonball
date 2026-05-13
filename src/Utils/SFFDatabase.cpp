#include "Utils/SFFDatabase.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace db {

    bool SFFDatabase::load(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[SFFDatabase] Cannot open: " << filePath << std::endl;
            return false;
        }

        m_data.clear();
        std::string line;
        int loadedCount = 0;

        while (std::getline(file, line)) {

            if (line.empty()) continue;

            try {
                std::istringstream iss(line);
                std::string token;

                if (!std::getline(iss, token, '\t')) continue;
                int group = std::stoi(token);

                if (!std::getline(iss, token, '\t')) continue;
                int image = std::stoi(token);

                if (!std::getline(iss, token, '\t')) continue;
                int width = std::stoi(token);

                if (!std::getline(iss, token, '\t')) continue;
                int height = std::stoi(token);

                if (!std::getline(iss, token, '\t')) continue;
                int axisX = std::stoi(token);

                if (!std::getline(iss, token, '\t')) continue;
                int axisY = std::stoi(token);

                int key = (group << 16) | (image & 0xFFFF);
                m_data[key] = {width, height, axisX, axisY};
                loadedCount++;

            } catch (...) {

            }
        }

        std::cout << "[SFFDatabase] Loaded " << loadedCount << " sprites from " << filePath << std::endl;
        return loadedCount > 0;
    }

    const SpriteAxisData* SFFDatabase::lookup(int group, int image) const {
        int key = (group << 16) | (image & 0xFFFF);
        auto it = m_data.find(key);
        if (it != m_data.end()) {
            return &it->second;
        }

        if (group == 0 && image == 6) {
            std::cout << "[SFFDatabase] LOOKUP MISS (0,6): key=" << key << " map_size=" << m_data.size() << std::endl;
        }
        return nullptr;
    }

}
