/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "GameState.hpp"
#include <memory>
#include <vector>

namespace CS230
{
    class GameStateManager
    {
    public:
        template <typename STATE>
        void PushState();
        void PopState();
        void Update();
        void Draw();
        void DrawImGui();

        [[nodiscard]] bool HasGameEnded() const
        {
            return mGameStateStack.empty();
        }

        void Clear();

    private:
        std::vector<std::unique_ptr<GameState>> mGameStateStack;
        std::vector<std::unique_ptr<GameState>> mToClear;
    };

    template <typename STATE>
    void GameStateManager::PushState()
    {
        using namespace std::literals;
        mGameStateStack.push_back(std::make_unique<STATE>());
        const auto& state = mGameStateStack.back();
        Engine::GetLogger().LogEvent("Entering state "s + state->GetName());
        state->Load();
    }
}
