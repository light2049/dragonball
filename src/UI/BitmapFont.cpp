#include "UI/BitmapFont.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace db {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

char BitmapFont::fileNameToChar(const std::string& name) const {
    if (name.size() == 1) {
        char c = static_cast<char>(std::toupper(name[0]));
        if (std::isalnum(c)) return c;
    }
    // Symbol name mapping
    struct { const char* key; char value; } map[] = {
        {"Dot", '.'}, {"Comma", ','}, {"Colon", ':'}, {"Semicolon", ';'},
        {"Exclamation_Mark", '!'}, {"Question_Mark", '?'},
        {"Minus_Sign", '-'}, {"Plus_Sign", '+'}, {"Equals_Sign", '='},
        {"Forward_Sign", '/'}, {"Backslash_Sign", '\\'},
        {"Star_and_Multiplier", '*'},
        {"Percent_Sign", '%'}, {"Number_Sign", '#'},
        {"Dollar_Sign", '$'}, {"At_Sign", '@'},
        {"Left_Parenthesis", '('}, {"Right_Parenthesis", ')'},
        {"Left_Square_Bracket", '['}, {"Right_Square_Bracket", ']'},
        {"Left_Curly_Bracket", '{'}, {"Right_Curly_Bracket", '}'},
        {"Quotation_Mark_1", '"'}, {"Quotation_Mark_2", '\''},
        {"Vertical_Line", '|'}, {"Multiplier_Sign", 'x'},
        {"Space", ' '}, {"_", ' '},
    };
    std::string clean = name;
    // Remove extension if present
    auto dotPos = clean.rfind('.');
    if (dotPos != std::string::npos) clean = clean.substr(0, dotPos);

    for (const auto& m : map) {
        if (clean == m.key) return m.value;
    }
    return 0;
}

bool BitmapFont::load(const std::string& directory) {
    m_directory = directory;
    m_charTextures.clear();

    if (!std::filesystem::exists(directory)) {
        std::cerr << "[BitmapFont] Directory not found: " << directory << std::endl;
        return false;
    }

    int loaded = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (path.size() < 4) continue;
        std::string ext = path.substr(path.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".png") continue;

        std::string filename = entry.path().stem().string();
        char c = fileNameToChar(filename);
        if (c == 0) continue;

        auto tex = std::make_unique<sf::Texture>();
        if (!tex->loadFromFile(path)) {
            std::cerr << "[BitmapFont] Failed to load: " << filename << std::endl;
            continue;
        }
        m_charTextures[c] = std::move(tex);
        loaded++;
    }

    // Add space (invisible character)
    auto spaceTex = std::make_unique<sf::Texture>();
    if (spaceTex->resize({20, 1})) {
        m_charTextures[' '] = std::move(spaceTex);
    }

    std::cout << "[BitmapFont] Loaded " << loaded << " characters from " << directory << "\n";
    int dbgCnt = 0;
    for (const auto& [c, tex] : m_charTextures) {
        if (dbgCnt++ >= 10) break;
        std::cout << "[BitmapFont] '" << c << "' (" << (int)c << ") tex=" << tex->getSize().x << "x" << tex->getSize().y << std::endl;
    }
    return loaded > 0;
}

void BitmapFont::drawText(sf::RenderWindow& window, const std::string& text,
                          const sf::Vector2f& position, unsigned int charSize,
                          const sf::Color& color, const sf::Vector2f& origin) const {
    std::string upper = text;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    static bool firstDraw = true;
    if (firstDraw) {
        firstDraw = false;
        std::cout << "[BitmapFont] drawing \"" << text << "\" -> \"" << upper << "\" chars found: ";
        for (char c : upper) {
            auto it = m_charTextures.find(c);
            if (it != m_charTextures.end()) std::cout << c;
            else {
                auto it2 = m_charTextures.find('?');
                std::cout << (it2 != m_charTextures.end() ? '?' : '_');
            }
        }
        std::cout << " map size=" << m_charTextures.size() << "\n";
    }

    // Calculate total width for centering
    float totalWidth = 0.f;
    float maxHeight = 0.f;
    float spaceWidth = static_cast<float>(charSize) * 0.4f;
    for (char c : upper) {
        if (c == ' ') { totalWidth += spaceWidth; continue; }
        auto it = m_charTextures.find(c);
        if (it == m_charTextures.end()) it = m_charTextures.find('?');
        if (it == m_charTextures.end()) continue;
        auto size = it->second->getSize();
        float scale = static_cast<float>(charSize) / size.y;
        totalWidth += size.x * scale + 2.f;
        maxHeight = std::max(maxHeight, static_cast<float>(charSize));
    }

    // Apply origin offset
    float startX = position.x - totalWidth * origin.x;
    float startY = position.y - maxHeight * origin.y;

    // 标准空格宽度 (字符大小的 ~40%)

    float cursorX = startX;
    for (char c : upper) {
        auto it = m_charTextures.find(c);
        if (it == m_charTextures.end()) it = m_charTextures.find('?');
        if (it == m_charTextures.end()) {
            cursorX += spaceWidth;
            continue;
        }
        auto size = it->second->getSize();
        float scale = static_cast<float>(charSize) / size.y;
        float w = size.x * scale;
        float h = size.y * scale;

        // 空格用固定宽度
        if (c == ' ') {
            cursorX += spaceWidth;
            continue;
        }

        sf::Sprite sprite(*it->second);
        sprite.setScale({scale, scale});
        sprite.setPosition({cursorX, startY});
        sprite.setColor(color);
        window.draw(sprite);

        cursorX += w + 2.f;
    }
}

} // namespace db