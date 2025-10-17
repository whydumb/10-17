/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Texture.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/ImmediateRenderer2D.hpp"
#include "CS200/Image.hpp"
#include "Engine.hpp"
#include "stb_image.h"
#include "OpenGL/GL.hpp"
#include <iostream>
#include "Engine/Logger.hpp"
namespace CS230
{

    Texture::Texture(const std::filesystem::path& file_name)
    {
        CS200::Image image(file_name, true);
        size.x = image.GetSize().x;
        size.y = image.GetSize().y;
        textureHandle = OpenGL::CreateTextureFromImage(image);
    }

    Texture::Texture(OpenGL::TextureHandle given_texture, Math::ivec2 the_size)
        : textureHandle(given_texture), size(the_size)
    {
    }

    Texture::~Texture()
    {
        if (textureHandle) {
            GLuint tex = textureHandle;
            GL::DeleteTextures(1, &tex);
            textureHandle = 0;
        }
    }

    Texture::Texture(Texture&& temporary) noexcept
        : textureHandle(temporary.textureHandle), size(temporary.size)
    {
        temporary.textureHandle = 0;
        temporary.size = {0, 0};
    }

    Texture& Texture::operator=(Texture&& temporary) noexcept
    {
        if (this != &temporary) {
            std::swap(textureHandle, temporary.textureHandle);
            std::swap(size, temporary.size);
        }
        return *this;
    }

    Math::ivec2 Texture::GetSize() const
    {
        return size;
    }

    void Texture::Draw(const Math::TransformationMatrix& display_matrix, unsigned int color)
    {
       
        CS200::IRenderer2D& renderer = Engine::Instance().GetRenderer2D();
        Math::vec2 size_temp = static_cast<Math::vec2>(this->GetSize());
        const Math::TransformationMatrix transform = display_matrix * Math::ScaleMatrix(size_temp);

        
        renderer.DrawQuad(transform, this->GetHandle(), {0, 0}, {1, 1}, color);
    }

    /**
         * \brief Draw a rectangular region of the texture (sprite sheet support)
         * \param display_matrix Transformation matrix for positioning, scaling, and rotation
         * \param texel_position Top-left corner position in pixel coordinates within the texture
         * \param frame_size Size of the region to draw in pixels
         * \param color RGBA color value for tinting the texture (default: white/no tint)
         *
         * Renders a specific rectangular region of the texture, enabling sprite sheet
         * functionality, texture atlases, and animation frame rendering. This method
         * is essential for efficient graphics where multiple sprites or animation
         * frames are packed into a single texture file.
         *
         * Sprite Sheet Applications:
         * - Character animation frames stored in a grid layout
         * - UI element collections (buttons, icons, decorative elements)
         * - Tile sets for 2D game environments
         * - Font glyph rendering from character atlases
         * - Particle effect textures with multiple variations
         *
         * Coordinate System:
         * The texel_position uses pixel coordinates with (0,0) at the top-left
         * of the texture, following standard image coordinate conventions. The
         * method automatically converts these to the appropriate OpenGL texture
         * coordinates for rendering.
         *
         * Performance Benefits:
         * Using sprite sheets reduces texture binding overhead and improves
         * rendering performance by allowing multiple related graphics to be
         * stored in a single texture object, enabling more efficient batching.
         *
         * The transformation matrix affects the final rendered size and position,
         * while frame_size determines which portion of the texture is sampled.
         */
    void Texture::Draw(const Math::TransformationMatrix& display_matrix,
                   Math::ivec2 texel_position, Math::ivec2 frame_size,
                   unsigned int color)
    {
        if (textureHandle == 0 || size.x == 0 || size.y == 0)
            return;

        Math::vec2 bottom_left = {
            static_cast<double>(texel_position.x) / static_cast<double>(size.x),  
            static_cast<double>(size.y - (texel_position.y + frame_size.y)) / static_cast<double>(size.y) 
        };

        Math::vec2 top_right = {
            static_cast<double>(texel_position.x + frame_size.x) / static_cast<double>(size.x),  
            static_cast<double>(size.y - texel_position.y) / static_cast<double>(size.y)  
        };

        CS200::RGBA tintColor = static_cast<CS200::RGBA>(color);
        CS200::IRenderer2D& renderer = Engine::Instance().GetRenderer2D();
        renderer.DrawQuad(display_matrix, textureHandle, bottom_left, top_right, tintColor);
    }

    
}


