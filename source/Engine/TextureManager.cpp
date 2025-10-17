/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "TextureManager.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include "OpenGL/GL.hpp"
#include "Texture.hpp"
#include <iostream>

namespace CS230
{
    Texture* TextureManager::Load(const std::filesystem::path& file_name) {
        try {
            auto it = textures.find(file_name);
            if (it == textures.end()) {
                
                Engine::GetLogger().LogEvent("Load Texture: " + file_name.generic_string());
                Texture* tex = new Texture(file_name);
                textures[file_name] = tex;
                return tex;
            }
            return it->second;
        }
        catch (const std::exception& e) {
            Engine::GetLogger().LogEvent("Failed to load texture: " + std::string(e.what()));
            return nullptr;
        }
}


    void TextureManager::Unload() {
        Engine::GetLogger().LogEvent("Unload Textures");

        for (auto it = textures.begin(); it != textures.end(); ) {
            if (protected_paths.contains(it->first)) {
                ++it;
                continue;  
            }

            delete it->second;
            it = textures.erase(it);
        }

        for (auto it = rendered_textures.begin(); it != rendered_textures.end(); ) {
            delete *it;
            it = rendered_textures.erase(it);
        }
    }

}
