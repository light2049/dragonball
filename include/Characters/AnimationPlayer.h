#pragma once
#include "Utils/AirParser.h"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>

namespace db {

    class AnimationPlayer {
    public:
        AnimationPlayer();
        void play(const Animation& anim);
        void setFacingRight(bool right);
        void update(float dt);
        void draw(sf::RenderWindow& window, const sf::Vector2f& position) const;
        void drawWithAlpha(sf::RenderWindow& window, const sf::Vector2f& position, uint8_t alpha) const;
        std::optional<sf::Sprite> cloneSprite() const;
        void setScale(float x, float y);
        bool isFacingRight() const;
        const AnimFrame& getCurrentFrame() const;

        // Phase 7 新增：获取当前播放的动画 ID
        int getCurrentAnimId() const;
        // 获取当前精灵的尺寸 (用于计算中心点)
        sf::Vector2f getSpriteSize() const;
        int getCurrentFrameIndex() const { return static_cast<int>(m_currentFrameIndex); }

        // AnimElem: 当前动画帧序号 (1-based), 每次换帧递增
        int getCurrentAnimElem() const { return static_cast<int>(m_currentFrameIndex) + 1; }

        // AnimTime: 动画剩余帧数 (0 = 已播完/循环)
        int getAnimTime() const;

        // 是否刚刚完成一次循环 (用于检测动画播完)
        bool hasJustLooped() const { return m_justLooped; }
        void clearLoopFlag() { m_justLooped = false; }

    private:
        void updateFrame();
        std::unique_ptr<sf::Sprite> m_sprite;
        const Animation* m_currentAnimation = nullptr;
        size_t m_currentFrameIndex = 0;
        float m_currentTime = 0.0f;
        bool m_facingRight = true;
        bool m_justLooped = false;
        float m_scaleX = 1.f, m_scaleY = 1.f;
    };

} // namespace db