#include "Core/Game.h"
#include <iostream>

int main() {
    try {
        std::cout << "🐉 Dragon Ball 2D Fighter - Phase 1 Init..." << std::endl;

        db::Game game;
        game.run();

        std::cout << "👋 Game Exited Successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
        return 1;
    }
}
