/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "CS200/NDC.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/GameState.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/Texture.hpp"
#include "OpenGL/VertexArray.hpp"
#include <array>
#include <vector>

class DemoTexturing : public CS230::GameState
{
public:
    void          Load() override;
    void          Unload() override;
    void          Update() override;
    void          Draw() const override;
    void          DrawImGui() override;
    gsl::czstring GetName() const override;

private:
    OpenGL::CompiledShader texturingCombineShader{};

    struct object
    {
        OpenGL::BufferHandle      positionBufferHandle{};
        OpenGL::BufferHandle      colorBufferHandle{};
        OpenGL::BufferHandle      texCoordBufferHandle{};
        OpenGL::BufferHandle      indexBufferHandle{};
        GLsizei                   indicesCount{};
        OpenGL::VertexArrayHandle modelHandle{};
    } quad{};

    constexpr static int QuadSize = 256;

    OpenGL::TextureHandle duckTextureHandle{};
    OpenGL::TextureHandle noiseTextureHandle{};
    OpenGL::TextureHandle logoTextureHandle{};

    enum class TextureType
    {
        Duck  = 0,
        Noise = 1,
        Logo  = 2
    };

    struct Settings
    {
        int                   FilteringIndex           = 0;
        std::array<int, 2>    WrapIndex                = { 0, 0 };
        float                 TexCoordScale            = 1.0f;
        float                 TargetTexCoordScale      = 1.0f;
        bool                  ModulateColor            = false;
        bool                  ApplyProceduralTexture   = false;
        float                 ProceduralTileSize       = 32.0f;
        float                 TargetProceduralTileSize = 32.0f;
        bool                  UseTexture               = true;
        TextureType           SelectedTexture          = TextureType::Duck;
        bool                  DoBlending               = false;
        bool                  AnimateProceduralTexture = false;
        static constexpr auto MAX_TILE_SIZE            = 256.0f;
        static constexpr auto MIN_TILE_SIZE            = 16.0f;
    } settings;


private:
    void loadShaders();
    void createQuadModel();
    void createNoiseTexture();
    void createLogoTexture();
    void imgui_pick_filtering();
    void imgui_pick_wrapping(const char* label, OpenGL::TextureCoordinate which_coord);
};
