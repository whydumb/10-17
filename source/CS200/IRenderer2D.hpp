/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Vec2.hpp"
#include "OpenGL/Texture.hpp"
#include "RGBA.hpp"

namespace Math
{
    class TransformationMatrix;
}

namespace CS200
{
    
    class IRenderer2D
    {
    public:
        
        virtual ~IRenderer2D() = default;

        virtual void Init() = 0;

      
        virtual void Shutdown() = 0;

     
        virtual void BeginScene(const Math::TransformationMatrix& view_projection) = 0;

     
        virtual void EndScene() = 0;

        virtual void DrawQuad(
            const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl = Math::vec2{ 0.0, 0.0 }, Math::vec2 texture_coord_tr = Math::vec2{ 1.0, 1.0 },
            CS200::RGBA tintColor = CS200::WHITE) = 0;
    };

}
