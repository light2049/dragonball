#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

namespace db {

    class ResourceManager {
    public:
        // 禁止拷贝和赋值
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // 获取单例
        static ResourceManager& getInstance() {
            static ResourceManager instance;
            return instance;
        }

        // 获取纹理（如果没加载过就自动加载）
        const sf::Texture& getTexture(const std::string& path) {
            // 检查缓存
            auto search = m_textures.find(path);
            if (search == m_textures.end()) {
                // 没找到，加载它
                std::unique_ptr<sf::Texture> texture = std::make_unique<sf::Texture>();
                if (!texture->loadFromFile(path)) {
                    throw std::runtime_error("无法加载图片: " + path);
                }
                m_textures[path] = std::move(texture);
            }
            return *m_textures[path];
        }

    private:
        ResourceManager() = default; // 私有构造函数（单例）

        // 缓存：路径 -> 纹理指针
        std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    };

} // namespace db