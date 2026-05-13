#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <memory>

namespace db {

class BitmapFont {
public:
    bool load(const std::string& directory);
    bool hasLoaded() const { return !m_charTextures.empty(); }
    void drawText(sf::RenderWindow& window, const std::string& text,
                  const sf::Vector2f& position, unsigned int charSize = 64,
                  const sf::Color& color = sf::Color::White,
                  const sf::Vector2f& origin = {0.f, 0.f}) const;

private:
    std::map<char, std::unique_ptr<sf::Texture>> m_charTextures;
    std::string m_directory;
    char fileNameToChar(const std::string& name) const;
};

} // namespace db