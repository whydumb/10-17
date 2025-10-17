/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Path.hpp"

#include <SDL.h>
#include <optional>

namespace
{
    std::optional<std::filesystem::path> try_get_asset_path(const std::filesystem::path& starting_directory)
    {
        namespace fs                 = std::filesystem;
        fs::path       assets_parent = fs::absolute(starting_directory);
        const fs::path root          = assets_parent.root_path();

        // note that the do while loop is needed because on the web platform the starting folder is the root and that's also where the assets folder should be
        do
        {
            const fs::path assets_folder = assets_parent / "Assets";
            if (fs::is_directory(assets_folder))
            {
                return assets_parent;
            }
            // if assets not found then try looking the in parent folder
            assets_parent = assets_parent.parent_path();
        } while (assets_parent != root);

        return std::nullopt;
    }
}

namespace assets
{

    std::filesystem::path get_base_path()
    {
        namespace fs                  = std::filesystem;
        static fs::path assets_folder = []()
        {
            auto result = try_get_asset_path(fs::current_path());
            if (result)
                return result.value();
            // try from the exe path rather than the current working directory
            const auto base_path = SDL_GetBasePath();
            result               = try_get_asset_path(base_path);
            SDL_free(base_path);
            if (result)
                return result.value();
            throw std::runtime_error{ "Failed to find Assets folder in parent folders" };
        }();
        return assets_folder;
    }

    std::filesystem::path locate_asset(const std::filesystem::path& asset_path)
    {
        auto asset_filepath = asset_path;
        if (!std::filesystem::exists(asset_filepath))
        {
            // try prepending the asset directory path
            asset_filepath = get_base_path() / asset_filepath;
            if (!std::filesystem::exists(asset_filepath))
            {
                throw std::runtime_error("Failed to locate asset: " + asset_path.string());
            }
        }
        return asset_filepath;
    }
}
