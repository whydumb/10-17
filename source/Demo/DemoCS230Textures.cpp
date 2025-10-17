/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "DemoCS230Textures.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/RenderingAPI.hpp"
#include "DemoTexturing.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Window.hpp"

#include <cmath>
#include <imgui.h>
#include <iostream>

void DemoCS230Textures::Load()
{
    auto&      texture_manager        = Engine::GetTextureManager();
    const auto background_image_paths = { "Assets/images/DemoCS230Textures/Planets.png", "Assets/images/DemoCS230Textures/Ships.png", "Assets/images/DemoCS230Textures/Foreground.png" };
    for (const auto& path : background_image_paths)
    {
        
        backgroundTextures.push_back(texture_manager.Load(path));
    }

    robotTexture = texture_manager.Load("Assets/images/DemoCS230Textures/Robot.png");
    catTexture   = texture_manager.Load("Assets/images/DemoCS230Textures/Cat.png");

    initializeRobotAnimations();
    initializeCatAnimations();

    CS200::RenderingAPI::SetClearColor(0x2E3440FF);
}

template <typename T, typename FLOAT = double>
static void ease_to_target(T& current, const T& target, FLOAT delta_time, FLOAT weight = 1.0)
{
    const auto easing = std::min(delta_time * weight, static_cast<FLOAT>(1.0));
    current += easing * (target - current);
}

void DemoCS230Textures::Update()
{
    const auto& environment = Engine::GetWindowEnvironment();
    updateAnimation(environment.DeltaTime);
    const auto     delta_time = static_cast<float>(environment.DeltaTime);
    constexpr auto weight     = 1.5f;
    ease_to_target(backgroundTintColor[0], targetBackgroundTintColor[0], delta_time, weight);
    ease_to_target(backgroundTintColor[1], targetBackgroundTintColor[1], delta_time, weight);
    ease_to_target(backgroundTintColor[2], targetBackgroundTintColor[2], delta_time, weight);
    ease_to_target(backgroundTintColor[3], targetBackgroundTintColor[3], delta_time, weight);
    ease_to_target(characterTintColor[0], targetCharacterTintColor[0], delta_time, weight);
    ease_to_target(characterTintColor[1], targetCharacterTintColor[1], delta_time, weight);
    ease_to_target(characterTintColor[2], targetCharacterTintColor[2], delta_time, weight);
    ease_to_target(characterTintColor[3], targetCharacterTintColor[3], delta_time, weight);
}

void DemoCS230Textures::Draw() const
{
    CS200::RenderingAPI::Clear();
    auto& renderer_2d = Engine::GetRenderer2D();
    renderer_2d.BeginScene(CS200::build_ndc_matrix(Engine::GetWindow().GetSize()));
    const auto background_tint = CS200::pack_color(backgroundTintColor);
    for (const auto& texture : backgroundTextures)
    {
        //std::cout << "Background Texture Path: " << texture->GetPath() << std::endl;
        texture->Draw(Math::TransformationMatrix{}, background_tint);
    }
    CS230::Texture*  currentTexture = (selectedCharacter == CharacterType::Robot) ? robotTexture : catTexture;
    //std::cout << "Character Texture Path: " << currentTexture->GetPath() << std::endl;
    const auto       middle_x       = Engine::GetWindowEnvironment().DisplaySize.x / 2.0;
    const auto       texel_base     = getCurrentFrameTexelPosition();
    const auto       frame_size     = getCurrentFrameSize();
    const auto       hot_spot       = getCurrentHotSpot();
    constexpr double floor_y        = 80.0;
    const auto       to_center      = Math::TranslationMatrix(Math::vec2{ static_cast<double>(-hot_spot.x), static_cast<double>(-hot_spot.y) });
    const auto       scale          = faceRight ? Math::ScaleMatrix({ 1.0, 1.0 }) : Math::ScaleMatrix({ -1.0, 1.0 });
    const auto       translate      = Math::TranslationMatrix(Math::vec2{ middle_x, floor_y });
    const auto       transform      = translate * scale * to_center;
    const auto       character_tint = CS200::pack_color(characterTintColor);
    currentTexture->Draw(transform, texel_base, frame_size, character_tint);
    renderer_2d.EndScene();
}

void DemoCS230Textures::DrawImGui()
{
    if (ImGui::Begin("CS230 Textures Demo"))
    {
         const auto timing = Engine::GetWindowEnvironment();
        ImGui::LabelText("FPS", "%d", timing.FPS);
        ImGui::SeparatorText("Tint Color Controls");
        ImGui::ColorEdit4("Background Tint", targetBackgroundTintColor.data());
        ImGui::ColorEdit4("Character Tint", targetCharacterTintColor.data());

        ImGui::SeparatorText("Character Selection");

        static const char* character_names[] = { "Robot", "Cat" };
        int                character_index   = static_cast<int>(selectedCharacter);

        if (ImGui::Combo("Character", &character_index, character_names, IM_ARRAYSIZE(character_names)))
        {
            selectedCharacter = static_cast<CharacterType>(character_index);
            currentFrameIndex = 0;
            animationTimer    = 0.0;
        }

        if (selectedCharacter == CharacterType::Robot)
        {
            ImGui::SeparatorText("Robot Animation Controls");

            static const char* robot_animation_names[] = { "None", "RobotDead", "RobotWalking", "RobotAngry" };
            int                current_anim_index      = static_cast<int>(currentRobotAnimation);

            if (ImGui::Combo("Animation", &current_anim_index, robot_animation_names, IM_ARRAYSIZE(robot_animation_names)))
            {
                currentRobotAnimation = static_cast<RobotAnimation>(current_anim_index);
                currentFrameIndex     = 0;
                animationTimer        = 0.0;
            }
        }
        else // Cat
        {
            ImGui::SeparatorText("Cat Animation Controls");

            static const char* cat_animation_names[] = { "CatIdle", "CatRunning", "CatJumping", "CatFalling" };
            int                current_anim_index    = static_cast<int>(currentCatAnimation);

            if (ImGui::Combo("Animation", &current_anim_index, cat_animation_names, IM_ARRAYSIZE(cat_animation_names)))
            {
                currentCatAnimation = static_cast<CatAnimation>(current_anim_index);
                currentFrameIndex   = 0;
                animationTimer      = 0.0;
            }
        }

        ImGui::SeparatorText("Direction Controls");

        // Direction control
        ImGui::BeginDisabled(!faceRight);
        if (ImGui::Button("Face Left", ImVec2(80, 0)))
        {
            faceRight = false;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(faceRight);
        if (ImGui::Button("Face Right", ImVec2(80, 0)))
        {
            faceRight = true;
        }
        ImGui::EndDisabled();

        ImGui::SeparatorText("Animation Info");
        const auto& animations = getCurrentAnimations();
        const int   anim_index = getCurrentAnimationIndex();
        const auto& anim       = animations[static_cast<size_t>(anim_index)];
        ImGui::Text("Current Animation: %s", anim.name.c_str());
        ImGui::Text("Frame Count: %zu", anim.frames.size());
        ImGui::Text("Current Frame Index: %d", currentFrameIndex);
        ImGui::Text("Loop Frame: %d", anim.loopFrame);
        const auto& currentFrame = anim.frames[static_cast<size_t>(currentFrameIndex)];
        ImGui::Text("Current Sprite Frame: %d", currentFrame.frameIndex);
        ImGui::Text("Frame Duration: %.2f seconds", currentFrame.duration);
        ImGui::Text("Animation Timer: %.2f seconds", animationTimer);
        const double totalDuration = getTotalAnimationDuration();
        const double elapsedTime   = getCurrentAnimationElapsedTime();
        const double progress      = elapsedTime / totalDuration;
        ImGui::Text("Animation Progress: %.1f%% (%.2f/%.2f seconds)", progress * 100.0, elapsedTime, totalDuration);
        ImGui::ProgressBar(static_cast<float>(progress), ImVec2(-1.0f, 0.0f));
        if (ImGui::Button("Reset Animation"))
        {
            currentFrameIndex = 0;
            animationTimer    = 0.0;
        }


        ImGui::SeparatorText("Hot Spot and Frame Size");
        const auto hot_spot   = getCurrentHotSpot();
        const auto frame_size = getCurrentFrameSize();
        ImGui::Text("Hot Spot: (%d, %d)", hot_spot.x, hot_spot.y);
        ImGui::Text("Frame Size: %dx%d", frame_size.x, frame_size.y);

        ImGui::SeparatorText("Switch Demo");
        if (ImGui::Button("Switch to Demo Texturing"))
        {
            Engine::GetGameStateManager().PopState();
            Engine::GetGameStateManager().PushState<DemoTexturing>();
        }
    }
    ImGui::End();
}

void DemoCS230Textures::Unload()
{
    auto& texture_manager = Engine::GetTextureManager();
    texture_manager.Unload();
    backgroundTextures.clear();
    robotTexture = nullptr;
    catTexture   = nullptr;
}

gsl::czstring DemoCS230Textures::GetName() const
{
    return "Demo CS230 Textures";
}

void DemoCS230Textures::initializeRobotAnimations()
{
    robotAnimations.clear();
    robotAnimations.resize(4); // 4 animations: None, RobotDead, RobotWalking, RobotAngry

    // Animation: None - PlayFrame 0 10
    robotAnimations[static_cast<int>(RobotAnimation::None)] = { "None", { { 0, 10.0 } }, 0 };

    // Animation: RobotDead - PlayFrame 2 5
    robotAnimations[static_cast<int>(RobotAnimation::RobotDead)] = { "RobotDead", { { 2, 5.0 } }, 0 };

    // Animation: RobotWalking - PlayFrame 0 0.2, PlayFrame 1 0.2
    robotAnimations[static_cast<int>(RobotAnimation::RobotWalking)] = {
        "RobotWalking", { { 0, 0.2 }, { 1, 0.2 } },
         0
    };

    // Animation: RobotAngry - PlayFrame 3 0.1, PlayFrame 4 0.1
    robotAnimations[static_cast<int>(RobotAnimation::RobotAngry)] = {
        "RobotAngry", { { 3, 0.1 }, { 4, 0.1 } },
         0
    };
}

void DemoCS230Textures::initializeCatAnimations()
{
    catAnimations.clear();
    catAnimations.resize(4); // 4 animations: CatIdle, CatRunning, CatJumping, CatFalling

    // Animation: CatIdle
    catAnimations[static_cast<int>(CatAnimation::CatIdle)] = {
        "CatIdle",
        { { 0, 6.0 }, { 1, 0.5 }, { 2, 0.1 }, { 1, 0.5 }, { 2, 0.1 }, { 1, 0.5 }, { 0, 3.0 }, { 1, 0.5 }, { 2, 0.3 }, { 1, 0.5 },
          { 2, 0.3 }, { 1, 0.5 }, { 0, 3.0 }, { 1, 0.5 }, { 2, 1.0 }, { 1, 0.3 }, { 2, 2.0 }, { 3, 0.4 }, { 4, 0.4 }, { 5, 0.4 } },
        16  // Loop back to frame 16
    };

    // Animation: CatRunning - PlayFrame 6 .1, PlayFrame 0 .1, PlayFrame 7 .1, PlayFrame 0 .1, Loop 0
    catAnimations[static_cast<int>(CatAnimation::CatRunning)] = {
        "CatRunning", { { 6, 0.1 }, { 0, 0.1 }, { 7, 0.1 }, { 0, 0.1 } },
         0
    };

    // Animation: CatJumping - PlayFrame 9 3, Loop 0
    catAnimations[static_cast<int>(CatAnimation::CatJumping)] = { "CatJumping", { { 9, 3.0 } }, 0 };

    // Animation: CatFalling - PlayFrame 8 3, Loop 0
    catAnimations[static_cast<int>(CatAnimation::CatFalling)] = { "CatFalling", { { 8, 3.0 } }, 0 };
}

void DemoCS230Textures::updateAnimation(double deltaTime)
{
    const auto& animations = getCurrentAnimations();
    const int   anim_index = getCurrentAnimationIndex();
    const auto& anim       = animations[static_cast<size_t>(anim_index)];
    animationTimer += deltaTime;
    const auto& currentFrame = anim.frames[static_cast<size_t>(currentFrameIndex)];
    if (animationTimer >= currentFrame.duration)
    {
        animationTimer    = 0.0;
        currentFrameIndex = (currentFrameIndex + 1);
        if (static_cast<size_t>(currentFrameIndex) >= anim.frames.size())
        {
            currentFrameIndex = anim.loopFrame;
        }
    }
}

int DemoCS230Textures::getCurrentSpriteFrame() const
{
    const auto& animations = getCurrentAnimations();
    const int   anim_index = getCurrentAnimationIndex();
    const auto& anim       = animations[static_cast<size_t>(anim_index)];
    return anim.frames[static_cast<size_t>(currentFrameIndex)].frameIndex;
}

Math::ivec2 DemoCS230Textures::getCurrentFrameTexelPosition() const
{
    const int spriteFrame = getCurrentSpriteFrame();
    if (selectedCharacter == CharacterType::Robot)
    {
        return Math::ivec2{ spriteFrame * ROBOT_FRAME_SIZE.x, 0 };
    }
    else
    {
        return CAT_FRAME_POSITIONS[static_cast<size_t>(spriteFrame)];
    }
}

Math::ivec2 DemoCS230Textures::getCurrentFrameSize() const
{
    return (selectedCharacter == CharacterType::Robot) ? ROBOT_FRAME_SIZE : CAT_FRAME_SIZE;
}

Math::ivec2 DemoCS230Textures::getCurrentHotSpot() const
{
    return (selectedCharacter == CharacterType::Robot) ? ROBOT_HOT_SPOT : CAT_HOT_SPOT;
}

double DemoCS230Textures::getTotalAnimationDuration() const
{
    const auto& animations    = getCurrentAnimations();
    const int   anim_index    = getCurrentAnimationIndex();
    const auto& anim          = animations[static_cast<size_t>(anim_index)];
    double      totalDuration = 0.0;
    for (const auto& frame : anim.frames)
    {
        totalDuration += frame.duration;
    }
    return totalDuration;
}

double DemoCS230Textures::getCurrentAnimationElapsedTime() const
{
    const auto& animations  = getCurrentAnimations();
    const int   anim_index  = getCurrentAnimationIndex();
    const auto& anim        = animations[static_cast<size_t>(anim_index)];
    double      elapsedTime = 0.0;
    for (int i = 0; i < currentFrameIndex && i < static_cast<int>(anim.frames.size()); ++i)
    {
        elapsedTime += anim.frames[static_cast<size_t>(i)].duration;
    }
    elapsedTime += animationTimer;
    return elapsedTime;
}

const std::vector<DemoCS230Textures::Animation>& DemoCS230Textures::getCurrentAnimations() const
{
    return (selectedCharacter == CharacterType::Robot) ? robotAnimations : catAnimations;
}

int DemoCS230Textures::getCurrentAnimationIndex() const
{
    return (selectedCharacter == CharacterType::Robot) ? static_cast<int>(currentRobotAnimation) : static_cast<int>(currentCatAnimation);
}
