#pragma once
#include "Utils/AirParser.h"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>

namespace db {

    // 单帧绘制覆盖 (由 AngleDraw / Trans 控制器每帧设置)
    struct DrawOverrides {
        float scaleX = 1.f;
        float scaleY = 1.f;
        uint8_t alpha = 255;
        uint8_t hitFlash = 0;      // 受击闪白亮度 (0=无, 255=纯白)
        bool useAdditiveBlend = false;  // 由 trans=add / addalpha 设置
    };

    class AnimationPlayer {
    public:
        AnimationPlayer();
        void play(const Animation& anim);
        void setFacingRight(bool right);
        void update(float dt);
        void tickAnimTime();  // 帧开始时递减 AnimTime (CNS 执行前)
        void draw(sf::RenderWindow& window, const sf::Vector2f& position, const DrawOverrides* overrides = nullptr) const;
        void drawWithAlpha(sf::RenderWindow& window, const sf::Vector2f& position, uint8_t alpha) const;
        std::optional<sf::Sprite> cloneSprite() const;
        void setScale(float x, float y);
        bool isFacingRight() const;
        const AnimFrame& getCurrentFrame() const;
        // 计算动画总 tick 数
        int getTotalAnimTicks() const;

        // Phase 7 新增：获取当前播放的动画 ID
        int getCurrentAnimId() const;
        // 获取当前精灵的尺寸 (用于计算中心点)
        sf::Vector2f getSpriteSize() const;
        int getCurrentFrameIndex() const { return static_cast<int>(m_currentFrameIndex); }

        // AnimElem: 当前动画帧序号 (1-based), 每次换帧递增
        int getCurrentAnimElem() const { return static_cast<int>(m_currentFrameIndex) + 1; }

        // AnimTime: 动画剩余帧数 (0 = 已播完/循环)
        // AnimTime: 剩余 tick 数 (递减, 到 0 后不再变化)
        int getAnimTime() const { return m_animTimeRemaining; }

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
        int m_animTimeRemaining = 0;  // AnimTime: 递减 tick 数
    };

} // namespace db