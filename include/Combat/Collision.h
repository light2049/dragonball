#pragma once
#include <SFML/Graphics.hpp>

namespace db {

    // ✅ SFML 3 适配：使用 .position 和 .size
    inline bool checkCollision(const sf::FloatRect& rectA, const sf::FloatRect& rectB) {
        return rectA.position.x < rectB.position.x + rectB.size.x &&
               rectA.position.x + rectA.size.x > rectB.position.x &&
               rectA.position.y < rectB.position.y + rectB.size.y &&
               rectA.position.y + rectA.size.y > rectB.position.y;
    }

} // namespace db