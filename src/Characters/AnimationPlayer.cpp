#include "Characters/AnimationPlayer.h"
#include "Core/ResourceManager.h"
#include <iostream>
#include <set>

namespace db {

    AnimationPlayer::AnimationPlayer()
        : m_sprite(nullptr), m_currentAnimation(nullptr), m_currentTime(0.0f), m_currentFrameIndex(0), m_facingRight(true) {}

    static std::set<int> s_loggedPlay;
    static std::set<int> s_loggedLoop;

    void AnimationPlayer::play(const Animation& anim) {
        int newId = anim.id;

        if (m_currentAnimation && m_currentAnimation->id == newId) {
            // 相同动画 ID 不重置，让动画继续播放
            return;
        }

        m_currentAnimation = &anim;
        m_currentFrameIndex = 0;
        m_currentTime = 0.0f;
        m_justLooped = false;
        updateFrame();

        if (!s_loggedPlay.contains(newId)) {
            std::cout << "[AnimationPlayer] Playing Anim ID: " << newId << std::endl;
            s_loggedPlay.insert(newId);
        }
    }

    int AnimationPlayer::getCurrentAnimId() const {
        if (!m_currentAnimation) return -1;
        return m_currentAnimation->id;
    }

    void AnimationPlayer::update(float dt) {
        if (!m_currentAnimation || m_currentAnimation->frames.empty()) return;

        m_currentTime += dt;
        float frameDurationInSeconds = m_currentAnimation->frames[m_currentFrameIndex].duration / 60.0f;

        if (m_currentTime >= frameDurationInSeconds) {
            m_currentTime -= frameDurationInSeconds;
            m_currentFrameIndex++;

            if (m_currentFrameIndex >= m_currentAnimation->frames.size()) {
                int loopIndex = m_currentAnimation->loopStartIndex;
                if (loopIndex < 0 || loopIndex >= m_currentAnimation->frames.size()) {
                    loopIndex = 0;
                }
                m_currentFrameIndex = loopIndex;
                m_justLooped = true;

                int animId = m_currentAnimation->id;
                if (!s_loggedLoop.contains(animId)) {
                    std::cout << "[AnimationPlayer] Looping Anim ID: " << animId << std::endl;
                    s_loggedLoop.insert(animId);
                }
            }
            updateFrame();
        }
    }

    const AnimFrame& AnimationPlayer::getCurrentFrame() const {
        if (!m_currentAnimation || m_currentAnimation->frames.empty()) {
            static const AnimFrame emptyFrame = {};
            return emptyFrame;
        }
        return m_currentAnimation->frames[m_currentFrameIndex];
    }

    void AnimationPlayer::updateFrame() {
        if (!m_currentAnimation || m_currentAnimation->frames.empty()) return;
        const auto& frame = m_currentAnimation->frames[m_currentFrameIndex];

        try {
            const auto& texture = ResourceManager::getInstance().getTexture(frame.texturePath);
            m_sprite = std::make_unique<sf::Sprite>(texture);
            m_sprite->setScale({ (m_facingRight ? 1.0f : -1.0f) * m_scaleX, m_scaleY });
        } catch (const std::exception& e) {
            std::cerr << "[AnimationPlayer] Texture load failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[AnimationPlayer] Texture load failed: unknown error" << std::endl;
        }
    }

    void AnimationPlayer::draw(sf::RenderWindow& window, const sf::Vector2f& position) const {
        if (!m_sprite) return;

        sf::Sprite tempSprite(*m_sprite);

        BlendMode blendMode = BlendMode::Normal;
        float axisX = 0.f, axisY = 0.f;
        if (m_currentAnimation && m_currentFrameIndex < m_currentAnimation->frames.size()) {
            const auto& frame = m_currentAnimation->frames[m_currentFrameIndex];
            axisX = static_cast<float>(frame.axisX);
            axisY = static_cast<float>(frame.axisY);
            blendMode = frame.blendMode;
        }

        // 轴对齐: 精灵的轴 (axisX, axisY) 对齐到 position，受缩放影响
        float renderScaleX = (m_facingRight ? 1.0f : -1.0f) * m_scaleX;
        float finalX = position.x - axisX * renderScaleX;
        float finalY = position.y - axisY * m_scaleY;

        tempSprite.setScale({ renderScaleX, m_scaleY });
        tempSprite.setPosition({finalX, finalY});

        sf::RenderStates states;
        if (blendMode == BlendMode::Additive) {
            states.blendMode = sf::BlendAdd;
        } else if (blendMode == BlendMode::Subtractive) {
            states.blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::ReverseSubtract);
        }
        window.draw(tempSprite, states);
    }

    void AnimationPlayer::drawWithAlpha(sf::RenderWindow& window, const sf::Vector2f& position, uint8_t alpha) const {
        if (!m_sprite) return;

        sf::Sprite tempSprite(*m_sprite);

        BlendMode blendMode = BlendMode::Normal;
        float axisX = 0.f, axisY = 0.f;
        if (m_currentAnimation && m_currentFrameIndex < m_currentAnimation->frames.size()) {
            const auto& frame = m_currentAnimation->frames[m_currentFrameIndex];
            axisX = static_cast<float>(frame.axisX);
            axisY = static_cast<float>(frame.axisY);
            blendMode = frame.blendMode;
        }

        // 轴对齐: 精灵的轴 (axisX, axisY) 对齐到 position，受缩放影响
        float renderScaleX = (m_facingRight ? 1.0f : -1.0f) * m_scaleX;
        float finalX = position.x - axisX * renderScaleX;
        float finalY = position.y - axisY * m_scaleY;

        sf::Color color = tempSprite.getColor();
        color.a = alpha;
        tempSprite.setColor(color);
        tempSprite.setScale({ renderScaleX, m_scaleY });
        tempSprite.setPosition({finalX, finalY});

        sf::RenderStates states;
        states.blendMode = sf::BlendAdd;
        window.draw(tempSprite, states);
    }

    std::optional<sf::Sprite> AnimationPlayer::cloneSprite() const {
        if (m_sprite) return std::optional<sf::Sprite>(*m_sprite);
        return std::nullopt;
    }

    void AnimationPlayer::setScale(float x, float y) {
        m_scaleX = x;
        m_scaleY = y;
        // Apply scale to current sprite if it exists
        if (m_sprite) {
            m_sprite->setScale({x, y});
        }
    }

    bool AnimationPlayer::isFacingRight() const { return m_facingRight; }
    void AnimationPlayer::setFacingRight(bool right) { m_facingRight = right; }

    int AnimationPlayer::getAnimTime() const {
        if (!m_currentAnimation || m_currentAnimation->frames.empty()) return 0;
        // 返回从当前帧到动画结束的剩余帧数
        // 0 表示已在最后一帧或已循环
        if (m_currentFrameIndex >= m_currentAnimation->frames.size() - 1) return 0;
        return static_cast<int>(m_currentAnimation->frames.size() - 1 - m_currentFrameIndex);
    }

    sf::Vector2f AnimationPlayer::getSpriteSize() const {
        if (m_sprite) {
            return m_sprite->getLocalBounds().size;
        }
        return {0.f, 0.f};
    }

} // namespace db