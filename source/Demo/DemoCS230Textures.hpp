/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once

#include "Engine/GameState.hpp"
#include "Engine/Vec2.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace CS230
{
    class Texture;
}

class DemoCS230Textures : public CS230::GameState
{
public:
    void          Load() override;
    void          Update() override;
    void          Unload() override;
    void          Draw() const override;
    void          DrawImGui() override;
    gsl::czstring GetName() const override;

private:

    struct AnimationFrame
    {
        int    frameIndex{};
        double duration{}; // in seconds
    };

    struct Animation
    {
        std::string                 name{};
        std::vector<AnimationFrame> frames{};
        int                         loopFrame{ 0 };
    };

    enum class RobotAnimation
    {
        None         = 0,
        RobotDead    = 1,
        RobotWalking = 2,
        RobotAngry   = 3
    };

    enum class CatAnimation
    {
        CatIdle    = 0,
        CatRunning = 1,
        CatJumping = 2,
        CatFalling = 3
    };

    enum class CharacterType
    {
        Robot = 0,
        Cat   = 1
    };

    // Robot sprite sheet info
    static constexpr Math::ivec2 ROBOT_HOT_SPOT{ 30, 20 };
    static constexpr Math::ivec2 ROBOT_FRAME_SIZE{ 63, 127 };
    static constexpr int         ROBOT_NUM_FRAMES = 5;

    // Cat sprite sheet info
    static constexpr Math::ivec2 CAT_HOT_SPOT{ 65, 15 };
    static constexpr Math::ivec2 CAT_FRAME_SIZE{ 128, 128 };
    static constexpr int         CAT_NUM_FRAMES = 10;

    // Cat frame positions in texture (texel coordinates)
    static constexpr std::array<Math::ivec2, CAT_NUM_FRAMES> CAT_FRAME_POSITIONS = {
        {
         { 0, 0 },     // Frame 0
            { 128, 0 },   // Frame 1
            { 256, 0 },   // Frame 2
            { 384, 0 },   // Frame 3
            { 512, 0 },   // Frame 4
            { 0, 128 },   // Frame 5
            { 128, 128 }, // Frame 6
            { 256, 128 }, // Frame 7
            { 384, 128 }, // Frame 8
            { 512, 128 }  // Frame 9
        }
    };

    std::vector<CS230::Texture*> backgroundTextures;
    CS230::Texture*              robotTexture              = nullptr;
    CS230::Texture*              catTexture                = nullptr;
    std::array<float, 4>         backgroundTintColor       = { 0.1f, 0.2f, 0.3f, 0.4f };
    std::array<float, 4>         characterTintColor        = { 0.4f, 0.3f, 0.2f, 0.1f };
    std::array<float, 4>         targetBackgroundTintColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4>         targetCharacterTintColor  = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Character selection and animation state
    CharacterType          selectedCharacter = CharacterType::Cat;
    std::vector<Animation> robotAnimations;
    std::vector<Animation> catAnimations;
    RobotAnimation         currentRobotAnimation = RobotAnimation::None;
    CatAnimation           currentCatAnimation   = CatAnimation::CatIdle;
    int                    currentFrameIndex     = 0;
    double                 animationTimer        = 0.0;
    bool                   faceRight             = true;

private:
    void                          initializeRobotAnimations();
    void                          initializeCatAnimations();
    void                          updateAnimation(double deltaTime);
    int                           getCurrentSpriteFrame() const;
    Math::ivec2                   getCurrentFrameTexelPosition() const;
    Math::ivec2                   getCurrentFrameSize() const;
    Math::ivec2                   getCurrentHotSpot() const;
    double                        getTotalAnimationDuration() const;
    double                        getCurrentAnimationElapsedTime() const;
    const std::vector<Animation>& getCurrentAnimations() const;
    int                           getCurrentAnimationIndex() const;
};
