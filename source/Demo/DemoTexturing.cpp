/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "DemoTexturing.hpp"

#include "CS200/Image.hpp"
#include "CS200/NDC.hpp"
#include "CS200/Renderer2DUtils.hpp"
#include "CS200/RenderingAPI.hpp"
#include "DemoCS230Textures.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Random.hpp"
#include "Engine/Window.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/GL.hpp"
#include <cmath>
#include <imgui.h>
#include <limits>
#include <numbers>
#include <stb_perlin.h>

void DemoTexturing::Load()
{
    loadShaders();
    createQuadModel();

    constexpr bool flip_image = true;
    const auto     duck_image = CS200::Image("Assets/images/DemoTexturing/duck.png", flip_image);
    duckTextureHandle         = OpenGL::CreateTextureFromImage(duck_image, OpenGL::Filtering::NearestPixel, OpenGL::Wrapping::Repeat);
    createNoiseTexture();
    createLogoTexture();
    CS200::RenderingAPI::SetClearColor(0x6495edff); // cornflower blue
}

void DemoTexturing::Unload()
{
    OpenGL::DestroyShader(texturingCombineShader);
    GL::DeleteTextures(1, &duckTextureHandle), duckTextureHandle                = 0;
    GL::DeleteTextures(1, &noiseTextureHandle), noiseTextureHandle              = 0;
    GL::DeleteTextures(1, &logoTextureHandle), logoTextureHandle                = 0;
    GL::DeleteBuffers(1, &quad.positionBufferHandle), quad.positionBufferHandle = 0;
    GL::DeleteBuffers(1, &quad.colorBufferHandle), quad.colorBufferHandle       = 0;
    GL::DeleteBuffers(1, &quad.texCoordBufferHandle), quad.texCoordBufferHandle = 0;
    GL::DeleteBuffers(1, &quad.indexBufferHandle), quad.indexBufferHandle       = 0;
    GL::DeleteVertexArrays(1, &quad.modelHandle), quad.modelHandle              = 0;

    // other demos assume blending is enabled
    GL::Enable(GL_BLEND);
    GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

template <typename T, typename FLOAT = double>
static void ease_to_target(T& current, const T& target, FLOAT delta_time, FLOAT weight = 1.0)
{
    const auto easing = std::min(delta_time * weight, static_cast<FLOAT>(1.0));
    current += easing * (target - current);
}

void DemoTexturing::Update()
{
    const auto& environment = Engine::GetWindowEnvironment();
    if (settings.AnimateProceduralTexture)
    {
        const auto      elapsed_time      = static_cast<float>(environment.ElapsedTime);
        constexpr float small_to_big_time = 30.0f; // seconds
        const float     theta             = elapsed_time / small_to_big_time;
        const float     t                 = (std::sin(std::numbers::pi_v<float> * theta - std::numbers::pi_v<float> / 2.0f) + 1.0f) / 2.0f;
        settings.TargetProceduralTileSize = (1 - t) * Settings::MIN_TILE_SIZE + t * (Settings::MAX_TILE_SIZE);
    }

    ease_to_target(settings.TexCoordScale, settings.TargetTexCoordScale, static_cast<float>(environment.DeltaTime), 1.5f);
    ease_to_target(settings.ProceduralTileSize, settings.TargetProceduralTileSize, static_cast<float>(environment.DeltaTime), 1.5f);
}

void DemoTexturing::Draw() const
{
    CS200::RenderingAPI::Clear();

    if (settings.DoBlending)
    {
        GL::Enable(GL_BLEND);
        GL::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        GL::Disable(GL_BLEND);
    }

    GL::UseProgram(texturingCombineShader.Shader);
    GL::ActiveTexture(GL_TEXTURE0);

    // Choose which texture to bind based on settings
    switch (settings.SelectedTexture)
    {
        case TextureType::Duck: GL::BindTexture(GL_TEXTURE_2D, duckTextureHandle); break;
        case TextureType::Noise: GL::BindTexture(GL_TEXTURE_2D, noiseTextureHandle); break;
        case TextureType::Logo: GL::BindTexture(GL_TEXTURE_2D, logoTextureHandle); break;
    }


    const auto& locations = texturingCombineShader.UniformLocations;
    GL::Uniform1i(locations.at("uTex2d"), 0);
    GL::Uniform1f(locations.at("uTexCoordScale"), static_cast<float>(settings.TexCoordScale));
    GL::Uniform1f(locations.at("uModulateColor"), static_cast<float>(settings.ModulateColor));
    GL::Uniform1f(locations.at("uProcTex"), static_cast<float>(settings.ApplyProceduralTexture));
    GL::Uniform1f(locations.at("uTileSize"), static_cast<float>(settings.ProceduralTileSize));
    GL::Uniform1f(locations.at("uUseImage"), static_cast<float>(settings.UseTexture));
    GL::Uniform1i(locations.at("uUseTextureAlpha"), settings.DoBlending);
    const auto window_size      = Engine::GetWindow().GetSize();
    const auto to_ndc_transform = CS200::build_ndc_matrix(window_size);
    const auto to_ndc_opengl    = CS200::Renderer2DUtils::to_opengl_mat3(to_ndc_transform);
    GL::UniformMatrix3fv(locations.at("uToNDC"), 1, GL_FALSE, to_ndc_opengl.data());
    const auto screen_size  = Engine::GetWindowEnvironment().DisplaySize;
    const auto model_matrix = CS200::Renderer2DUtils::to_opengl_mat3(Math::TranslationMatrix(screen_size * 0.5) * Math::ScaleMatrix(std::min(screen_size.x, screen_size.y)));
    GL::UniformMatrix3fv(locations.at("uModel"), 1, GL_FALSE, model_matrix.data());

    GL::BindVertexArray(quad.modelHandle);

    constexpr GLenum  primitive_pattern        = GL_TRIANGLES;
    constexpr GLenum  indices_type             = GL_UNSIGNED_BYTE;
    constexpr GLvoid* byte_offset_into_indices = nullptr;
    GL::DrawElements(primitive_pattern, quad.indicesCount, indices_type, byte_offset_into_indices);

    GL::BindTexture(GL_TEXTURE_2D, 0);
    GL::BindVertexArray(0);
    GL::UseProgram(0);
}

void DemoTexturing::DrawImGui()
{
    const auto timing = Engine::GetWindowEnvironment();

    ImGui::Begin("OpenGL Textures Demo");
    {
        ImGui::LabelText("FPS", "%d", timing.FPS);
        ImGui::SeparatorText("Effects");
        ImGui::Checkbox("Modulate Color", &settings.ModulateColor);
        ImGui::SameLine();
        ImGui::Checkbox("Procedural Texture", &settings.ApplyProceduralTexture);
        ImGui::SameLine();
        ImGui::Checkbox("Apply Texture", &settings.UseTexture);
        ImGui::Checkbox("Blending", &settings.DoBlending);

        // Texture selection dropdown
        const char* texture_items[] = { "Duck Texture", "Noise Texture", "DigiPen Logo" };
        const char* current_texture = texture_items[static_cast<int>(settings.SelectedTexture)];
        if (ImGui::BeginCombo("Texture Source", current_texture))
        {
            for (int n = 0; n < IM_ARRAYSIZE(texture_items); n++)
            {
                const bool is_selected = (static_cast<int>(settings.SelectedTexture) == n);
                if (ImGui::Selectable(texture_items[n], is_selected))
                {
                    settings.SelectedTexture = static_cast<TextureType>(n);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }


        if (settings.ApplyProceduralTexture)
        {
            ImGui::SeparatorText("Procedural Texture Controls");
            ImGui::Checkbox("Animate Procedural Size", &settings.AnimateProceduralTexture);

            if (ImGui::SliderFloat("Procedural Tile Size", &settings.TargetProceduralTileSize, Settings::MIN_TILE_SIZE, Settings::MAX_TILE_SIZE, "%2.1f"))
            {
                settings.AnimateProceduralTexture = false;
            }
        }
        if (settings.UseTexture)
        {
            ImGui::SeparatorText("Texture Filtering and Wrapping");

            imgui_pick_filtering();
            ImGui::SliderFloat("Tex Coord Scale", &settings.TargetTexCoordScale, 0.25f, 10.0f, "%2.1f");
            imgui_pick_wrapping("Wrapping for S Direction", OpenGL::TextureCoordinate::S);
            imgui_pick_wrapping("Wrapping for T Direction", OpenGL::TextureCoordinate::T);
        }
        
        ImGui::SeparatorText("Switch Demo");
        if (ImGui::Button("Switch to Demo CS230 Textures"))
        {
            Engine::GetGameStateManager().PopState();
            Engine::GetGameStateManager().PushState<DemoCS230Textures>();
        }
            
    }
    ImGui::End();
}

gsl::czstring DemoTexturing::GetName() const
{
    return "Demo OpenGL Texturing";
}

void DemoTexturing::loadShaders()
{
    using namespace std::filesystem;
    texturingCombineShader = OpenGL::CreateShader(path{ "Assets/shaders/DemoTexturing/combine.vert" }, path{ "Assets/shaders/DemoTexturing/combine.frag" });
}

void DemoTexturing::createQuadModel()
{
    struct position
    {
        float x, y;
    };

    struct texture_coordinate
    {
        float s, t;
    };

    struct color
    {
        unsigned char rgb[3];
    };

    constexpr std::array positions = {
        position{ -0.5f, -0.5f }, // bottom-left
        position{ -0.5f,  0.5f }, // top-left
        position{  0.5f,  0.5f }, // top-right
        position{  0.5f, -0.5f }  // bottom-right
    };
    constexpr std::array colors = {
        color{ 255,   0,   0 }, // bottom-left
        color{ 255,   0, 255 }, // top-left
        color{   0,   0, 255 }, // top-right
        color{   0, 255,   0 }  // bottom-right
    };
    constexpr std::array texture_coordinates = {
        texture_coordinate{ 0.0f, 0.0f }, // bottom-left
        texture_coordinate{ 0.0f, 1.0f }, // top-left
        texture_coordinate{ 1.0f, 1.0f }, // top-right
        texture_coordinate{ 1.0f, 0.0f }  // bottom-right
    };

    constexpr std::array<unsigned char, 6> indices = { 0, 3, 2, 0, 2, 1 };

    quad.positionBufferHandle = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ positions }));
    quad.colorBufferHandle    = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ colors }));
    quad.texCoordBufferHandle = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ texture_coordinates }));
    quad.indexBufferHandle    = OpenGL::CreateBuffer(OpenGL::BufferType::Indices, std::as_bytes(std::span{ indices }));
    quad.indicesCount         = static_cast<GLsizei>(indices.size());
    const auto layout         = {
        OpenGL::VertexBuffer{ quad.positionBufferHandle,             { OpenGL::Attribute::Float2 } },
        OpenGL::VertexBuffer{    quad.colorBufferHandle, { OpenGL::Attribute::UByte3ToNormalized } },
        OpenGL::VertexBuffer{ quad.texCoordBufferHandle,             { OpenGL::Attribute::Float2 } }
    };
    quad.modelHandle = OpenGL::CreateVertexArrayObject(layout, quad.indexBufferHandle);
}

void DemoTexturing::imgui_pick_filtering()
{
    constexpr const char*       items[]             = { "NearestPixel", "Linear" };
    constexpr OpenGL::Filtering types[]             = { OpenGL::Filtering::NearestPixel, OpenGL::Filtering::Linear };
    const char* const           combo_preview_value = items[settings.FilteringIndex];
    if (ImGui::BeginCombo("Texture Filtering", combo_preview_value, 0))
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            const bool is_selected = (settings.FilteringIndex == n);
            if (ImGui::Selectable(items[n], is_selected))
            {
                settings.FilteringIndex = n;
                OpenGL::SetFiltering(duckTextureHandle, types[n]);
                OpenGL::SetFiltering(noiseTextureHandle, types[n]);
                OpenGL::SetFiltering(logoTextureHandle, types[n]);
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void DemoTexturing::imgui_pick_wrapping(const char* label, OpenGL::TextureCoordinate which_coord)
{
    constexpr const char*      items[]             = { "Repeat", "ClampToEdge", "MirroredRepeat" };
    constexpr OpenGL::Wrapping types[]             = { OpenGL::Wrapping::Repeat, OpenGL::Wrapping::ClampToEdge, OpenGL::Wrapping::MirroredRepeat };
    const char* const          combo_preview_value = items[settings.WrapIndex[which_coord]];

    if (ImGui::BeginCombo(label, combo_preview_value, 0))
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            const bool is_selected = (settings.WrapIndex[which_coord] == n);
            if (ImGui::Selectable(items[n], is_selected))
            {
                settings.WrapIndex[which_coord] = n;
                OpenGL::SetWrapping(duckTextureHandle, types[n], which_coord);
                OpenGL::SetWrapping(noiseTextureHandle, types[n], which_coord);
                OpenGL::SetWrapping(logoTextureHandle, types[n], which_coord);
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void DemoTexturing::createNoiseTexture()
{
    // Create an empty RGBA texture using CreateRGBATexture
    constexpr Math::ivec2 texture_size{ 128, 128 };
    noiseTextureHandle = OpenGL::CreateRGBATexture(texture_size, OpenGL::Filtering::NearestPixel, OpenGL::Wrapping::Repeat);

    std::vector<CS200::RGBA> pixels(128 * 128);

    constexpr static float NoiseLacunarity = 2.0f;
    constexpr static float NoiseGain       = 0.5f;
    constexpr static int   NoiseLayers     = 5;
    constexpr float        NoiseFrequency  = 0.1f;

    for (int y = 0; y < texture_size.y; ++y)
    {
        for (int x = 0; x < texture_size.x; ++x)
        {
            // Normalize coordinates and apply noise scale
            const float fx = static_cast<float>(x) * NoiseFrequency;
            const float fy = static_cast<float>(y) * NoiseFrequency;

            // Generate noise value
            const float raw_noise        = stb_perlin_fbm_noise3(fx, fy, 0.0f, NoiseLacunarity, NoiseGain, NoiseLayers);
            const float noise            = std::clamp(raw_noise, -1.0f, 1.0f);
            const float normalized_noise = (noise + 1.0f) * 0.5f;

            const auto         noise_value   = static_cast<std::uint8_t>(normalized_noise * 255.0f);
            const std::uint8_t red_channel   = 255;
            const std::uint8_t green_channel = 255 - noise_value;
            const std::uint8_t blue_channel  = noise_value / 2;

            const float        brightness    = 0.299f * red_channel + 0.587f * green_channel + 0.114f * blue_channel;
            const std::uint8_t alpha_channel = static_cast<std::uint8_t>(brightness);

            const CS200::RGBA pixel_color = (static_cast<std::uint32_t>(red_channel) << 0) | (static_cast<std::uint32_t>(green_channel) << 8) | (static_cast<std::uint32_t>(blue_channel) << 16) |
                                            (static_cast<std::uint32_t>(alpha_channel) << 24);

            const auto index = static_cast<size_t>(y * texture_size.x + x);
            pixels[index]    = pixel_color;
        }
    }

    // Upload the pixel data to the texture
    GL::BindTexture(GL_TEXTURE_2D, noiseTextureHandle);
    GL::TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_size.x, texture_size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    GL::BindTexture(GL_TEXTURE_2D, 0);
}

void DemoTexturing::createLogoTexture()
{
    constexpr unsigned                  W      = 128;
    constexpr unsigned                  H      = 128;
    // generated from https://www.digipen.edu/themes/custom/digipen_future/favicon.ico
    constexpr std::array<unsigned, 847> colors = {
        0x0,        0x12617BA,  0x12D15BE,  0x12E16BE,  0x12F16BE,  0x12F17BE,  0x13017BF,  0x1331AC1,  0x1331AC4,  0x1341AC1,  0x1351BC2,  0x1351CC3,  0x1361CC2,  0x1361CC3,  0x1381DC4,  0x1391CC3,
        0x13A1EC4,  0x13A1FC6,  0x13A1FC8,  0x13B19C4,  0x13B1FC4,  0x13B1FC5,  0x13C20C6,  0x13C20C7,  0x13E21C6,  0x13F22C6,  0x13F22C7,  0x13F22CB,  0x14022C7,  0x14022C8,  0x14022C9,  0x22816BC,
        0x22B13BF,  0x22E16BE,  0x22F16BE,  0x22F17BE,  0x22F17BF,  0x2331AC1,  0x2341AC1,  0x2341BC1,  0x2351BC2,  0x2361CC2,  0x2361CC3,  0x2381DC4,  0x2391EC3,  0x2391EC4,  0x23A1EC4,  0x23A1EC5,
        0x23A1EC6,  0x23B1FC5,  0x23C1FC5,  0x23E21C7,  0x23E22C7,  0x23F22C6,  0x23F22C7,  0x32E16BE,  0x32E16BF,  0x32F16BE,  0x32F17BE,  0x32F17BF,  0x33117BE,  0x33319C1,  0x3331AC1,  0x3341AC1,
        0x3351BC2,  0x3361BC2,  0x3361CC2,  0x3371CC3,  0x3381DC4,  0x3391EC4,  0x33A1EC4,  0x33A1EC5,  0x33A1FC5,  0x33B1EC4,  0x33B1FC5,  0x33C20C5,  0x33C20C6,  0x33E21C7,  0x33E22C7,  0x33F22C6,
        0x33F22C7,  0x34022C8,  0x42C12BE,  0x42E16BE,  0x4361CC3,  0x4371CC3,  0x4381DC3,  0x4381DC4,  0x4391DC4,  0x43F22C7,  0x62D14BE,  0x8351CC1,  0xA2E15BE,  0xA3319C1,  0xA341AC1,  0xA391CC3,
        0xC351DC2,  0xC3D1EC4,  0xD3A1DC3,  0xD3D21C6,  0xE341AC0,  0x10381DC3, 0x122E17BD, 0x123E21C6, 0x132E17BF, 0x132F17BF, 0x133E21C5, 0x142D15BE, 0x143218C0, 0x143E21C5, 0x153219C0, 0x153B1EC4,
        0x16331AC1, 0x173F22C7, 0x192E16BE, 0x193F22C7, 0x1E2E16BE, 0x1E371CC2, 0x1E3D1FC6, 0x1F3319C1, 0x1F331AC1, 0x1F3B1FC5, 0x1F3C1FC5, 0x1F3E21C7, 0x20381CC4, 0x223017BF, 0x223E21C7, 0x232D16BE,
        0x233E21C7, 0x243218C0, 0x25351BC2, 0x262E15BD, 0x273B1EC5, 0x29341AC1, 0x2C3A1EC4, 0x2C3E22C6, 0x2D3E22C7, 0x302D16BE, 0x312E16BE, 0x322E16BE, 0x323B1FC5, 0x33371CC3, 0x333D1FC6, 0x343F22C7,
        0x35381CC3, 0x353E21C7, 0x362D16BE, 0x362F17BF, 0x382F17BF, 0x383218C0, 0x383F22C7, 0x39351BC2, 0x3A3219C0, 0x3A351AC1, 0x3B351AC1, 0x3D361CC2, 0x3E2E16BE, 0x402E16BE, 0x40391EC4, 0x423A1EC5,
        0x423F22C7, 0x43371CC3, 0x433A1FC5, 0x44381DC3, 0x452F17BF, 0x473018BF, 0x473D20C6, 0x483118C0, 0x483C20C6, 0x483E21C7, 0x4A361BC2, 0x4A3F22C7, 0x4B3319C1, 0x4B3F22C7, 0x4C3E21C7, 0x4D3F22C7,
        0x4F381DC3, 0x50391EC4, 0x512F16BE, 0x523018BF, 0x543118C0, 0x543F22C7, 0x573D20C6, 0x58341AC1, 0x583C20C5, 0x583E21C7, 0x5B3F22C6, 0x5C3F22C7, 0x5D3E21C7, 0x5D3F22C6, 0x5F2F17BE, 0x5F3118BF,
        0x603118C0, 0x623017BF, 0x643B1FC5, 0x663219C0, 0x66331AC1, 0x672E16BE, 0x673D20C6, 0x683118BF, 0x683D21C6, 0x693118BF, 0x693118C0, 0x6C3F22C7, 0x723B1FC5, 0x753C20C6, 0x76361CC3, 0x763D1FC6,
        0x763D21C6, 0x772E16BE, 0x783D21C5, 0x783D21C6, 0x79391DC4, 0x793E21C7, 0x813D21C6, 0x822E16BE, 0x823D21C6, 0x83351BC2, 0x833D21C6, 0x842F17BF, 0x873319C1, 0x883F22C7, 0x8A341AC1, 0x8A3A1EC4,
        0x8D3017BF, 0x8E2E16BE, 0x8F3219C0, 0x953C20C6, 0x963B1FC5, 0x963F22C7, 0x9A2E16BE, 0x9B361BC2, 0x9C3E21C7, 0x9D2E16BE, 0x9E3F22C7, 0xA0391EC4, 0xA03C20C6, 0xA12E16BE, 0xA23E21C7, 0xA72E16BE,
        0xA7351BC2, 0xA8341AC1, 0xA8371CC3, 0xA9341AC1, 0xAC2F16BE, 0xAC2F17BE, 0xAD391DC4, 0xAD3F22C7, 0xAF3A1EC5, 0xB03A1EC5, 0xB23017BF, 0xB2331AC1, 0xB23A1FC5, 0xB3341AC1, 0xB53219C0, 0xB5351BC2,
        0xB93F22C7, 0xBA3F22C7, 0xBB3A1FC5, 0xBB3F22C7, 0xBD2F17BF, 0xBE3B1FC5, 0xBF3319C1, 0xBF3B1FC5, 0xC13F22C7, 0xC22E16BE, 0xC32E16BE, 0xC33E21C7, 0xC43C20C6, 0xC43E22C7, 0xC63E21C7, 0xCC3C20C6,
        0xCF3017BF, 0xCF371CC3, 0xD03E21C7, 0xD12E16BE, 0xD22E16BE, 0xD23219C0, 0xD2381DC4, 0xD9361BC2, 0xDD2F16BE, 0xDD361CC2, 0xDD3F22C7, 0xDE2E16BE, 0xDE3A1EC4, 0xDE3C20C6, 0xE03E21C7, 0xE1391EC4,
        0xE3341AC1, 0xE62F17BF, 0xE63F22C7, 0xE72F17BE, 0xE73017BF, 0xE8371CC3, 0xE93219C0, 0xE93319C0, 0xE93F22C7, 0xEA331AC1, 0xEA381DC4, 0xEB3B1FC5, 0xEE2E16BE, 0xEE3F22C7, 0xF02F17BE, 0xF22E16BE,
        0xF23C1FC5, 0xF23C20C6, 0xF23D20C6, 0xF33E21C7, 0xF33F22C7, 0xF42E16BE, 0xF4341AC1, 0xF52E16BE, 0xF53E21C7, 0xF63017BF, 0xF73218C0, 0xF7371CC3, 0xF8351BC2, 0xF8381DC3, 0xF9341AC1, 0xFA2E16BE,
        0xFA3B1FC5, 0xFB2E16BE, 0xFB361BC2, 0xFB361CC2, 0xFB371CC3, 0xFB381DC4, 0xFB391EC4, 0xFB3A1EC4, 0xFB3F22C7, 0xFC2E16BE, 0xFC2F16BE, 0xFC2F17BE, 0xFC2F17BF, 0xFC3017BF, 0xFC3118BF, 0xFC3118C0,
        0xFC3219C0, 0xFC3319C1, 0xFC331AC1, 0xFC341AC1, 0xFC351BC2, 0xFC361CC3, 0xFC371CC3, 0xFC371DC3, 0xFC381DC3, 0xFC391DC4, 0xFC391EC4, 0xFC3A1EC4, 0xFC3A1EC5, 0xFC3A1FC5, 0xFC3B1FC5, 0xFC3C1FC5,
        0xFC3C20C6, 0xFC3D20C6, 0xFC3D21C6, 0xFC3E21C7, 0xFC3E22C7, 0xFC3F22C7, 0xFD2F17BE, 0xFD3017BF, 0xFD3118BF, 0xFD3118C0, 0xFD3219C0, 0xFD3319C1, 0xFD341AC1, 0xFD361BC2, 0xFD391EC4, 0xFD3C1FC5,
        0xFD3C20C5, 0xFD3C20C6, 0xFD3D20C6, 0xFD3D21C6, 0xFD3E21C7, 0xFD3F22C7, 0xFE2E16BE, 0xFE2F16BE, 0xFE2F17BE, 0xFE2F17BF, 0xFE3017BF, 0xFE3018BF, 0xFE3118BF, 0xFE3218C0, 0xFE3219C0, 0xFE3319C1,
        0xFE331AC1, 0xFE341AC1, 0xFE351BC2, 0xFE361BC2, 0xFE371CC3, 0xFE371DC3, 0xFE391DC4, 0xFE3A1EC4, 0xFE3A1EC5, 0xFE3A1FC5, 0xFE3B1FC5, 0xFE3C20C6, 0xFE3D20C6, 0xFE3D21C6, 0xFE3E21C7, 0xFE3E22C7,
        0xFE3F22C7, 0xFF0000BC, 0xFF0000BD, 0xFF0000BE, 0xFF0000BF, 0xFF0000C0, 0xFF0000C1, 0xFF0000C2, 0xFF0200C0, 0xFF0600BF, 0xFF0600C1, 0xFF0700BF, 0xFF0800C1, 0xFF0900C2, 0xFF0D00C2, 0xFF0F00C2,
        0xFF1500BF, 0xFF1600C0, 0xFF1600C1, 0xFF1700C1, 0xFF1700C2, 0xFF1800C1, 0xFF1900C0, 0xFF1900C3, 0xFF1B00BF, 0xFF1B00C3, 0xFF1C00C1, 0xFF1C00C2, 0xFF1D00C2, 0xFF1E00C2, 0xFF1F00C0, 0xFF1F00C1,
        0xFF2100C0, 0xFF2100C2, 0xFF2100C3, 0xFF2200C1, 0xFF2300C0, 0xFF2300C1, 0xFF2300C3, 0xFF2400C0, 0xFF2400C1, 0xFF2400C2, 0xFF2400C3, 0xFF2400C4, 0xFF2500C0, 0xFF2600C1, 0xFF2600C3, 0xFF2700C1,
        0xFF2700C2, 0xFF2800C1, 0xFF2800C2, 0xFF2900C1, 0xFF2900C2, 0xFF2A00C0, 0xFF2A00C2, 0xFF2A00C3, 0xFF2B00C2, 0xFF2B00C3, 0xFF2C00C2, 0xFF2C00C3, 0xFF2D00C2, 0xFF2D00C3, 0xFF2E00C3, 0xFF2E00C4,
        0xFF2E03C3, 0xFF2E0AC2, 0xFF2E16BE, 0xFF2F0CC0, 0xFF2F16BE, 0xFF2F17BE, 0xFF2F17BF, 0xFF3000C4, 0xFF300EC1, 0xFF3010C1, 0xFF3011C1, 0xFF3017BF, 0xFF3018BF, 0xFF3103C3, 0xFF310DC2, 0xFF3111C1,
        0xFF3118BF, 0xFF3118C0, 0xFF320AC3, 0xFF320BC4, 0xFF320CC3, 0xFF3218C0, 0xFF3219C0, 0xFF330DC3, 0xFF3318C1, 0xFF3319C0, 0xFF3319C1, 0xFF331AC1, 0xFF340CC3, 0xFF340EC3, 0xFF340FC3, 0xFF340FC4,
        0xFF3411C3, 0xFF3416C2, 0xFF3419C1, 0xFF341AC1, 0xFF341AC2, 0xFF341BC1, 0xFF350FC4, 0xFF3510C4, 0xFF3513C4, 0xFF351AC2, 0xFF351BC1, 0xFF351BC2, 0xFF351BC3, 0xFF351CC1, 0xFF351CC2, 0xFF351DC1,
        0xFF351EC1, 0xFF3611C4, 0xFF3612C4, 0xFF361AC3, 0xFF361BC2, 0xFF361BC3, 0xFF361CC2, 0xFF361CC3, 0xFF361DC1, 0xFF361DC2, 0xFF361EC1, 0xFF361EC2, 0xFF371AC4, 0xFF371BC3, 0xFF371CC3, 0xFF371DC2,
        0xFF371DC3, 0xFF371EC2, 0xFF371EC3, 0xFF371FC2, 0xFF3720C1, 0xFF3720C2, 0xFF3818C6, 0xFF381BC4, 0xFF381CC3, 0xFF381CC4, 0xFF381DC3, 0xFF381DC4, 0xFF381EC2, 0xFF381FC2, 0xFF381FC3, 0xFF3822C1,
        0xFF3822C3, 0xFF391BC4, 0xFF391CC4, 0xFF391DC4, 0xFF391DC5, 0xFF391EC4, 0xFF391FC3, 0xFF391FC4, 0xFF3920C2, 0xFF3920C3, 0xFF3920C4, 0xFF3922C2, 0xFF3923C1, 0xFF3923C2, 0xFF3A1EC4, 0xFF3A1EC5,
        0xFF3A1FC4, 0xFF3A1FC5, 0xFF3A20C5, 0xFF3A21C3, 0xFF3A21C4, 0xFF3A23C2, 0xFF3A23C3, 0xFF3A24C1, 0xFF3A24C2, 0xFF3A25C2, 0xFF3A26C2, 0xFF3B1EC5, 0xFF3B1FC4, 0xFF3B1FC5, 0xFF3B20C4, 0xFF3B20C5,
        0xFF3B21C4, 0xFF3B21C5, 0xFF3B23C4, 0xFF3B24C3, 0xFF3B25C3, 0xFF3B26C2, 0xFF3B26C3, 0xFF3B27C2, 0xFF3C1CC5, 0xFF3C1FC5, 0xFF3C20C5, 0xFF3C20C6, 0xFF3C21C5, 0xFF3C22C5, 0xFF3C23C4, 0xFF3C23C5,
        0xFF3C24C3, 0xFF3C24C4, 0xFF3C25C4, 0xFF3C26C4, 0xFF3C27C2, 0xFF3C27C3, 0xFF3C28C2, 0xFF3C28C3, 0xFF3C29C2, 0xFF3D20C6, 0xFF3D21C6, 0xFF3D22C5, 0xFF3D23C5, 0xFF3D24C4, 0xFF3D24C5, 0xFF3D25C4,
        0xFF3D25C5, 0xFF3D26C4, 0xFF3D26C5, 0xFF3D27C3, 0xFF3D27C4, 0xFF3D27C5, 0xFF3D28C3, 0xFF3D29C2, 0xFF3D29C3, 0xFF3D2AC2, 0xFF3D2AC3, 0xFF3E21C7, 0xFF3E22C7, 0xFF3E23C5, 0xFF3E25C5, 0xFF3E26C5,
        0xFF3E27C4, 0xFF3E27C5, 0xFF3E28C4, 0xFF3E28C5, 0xFF3E29C3, 0xFF3E29C4, 0xFF3E2AC3, 0xFF3E2AC4, 0xFF3E2BC2, 0xFF3E2BC3, 0xFF3F22C7, 0xFF3F27C4, 0xFF3F27C5, 0xFF3F28C5, 0xFF3F28C6, 0xFF3F29C4,
        0xFF3F29C5, 0xFF3F2AC4, 0xFF3F2AC5, 0xFF3F2BC3, 0xFF4027C5, 0xFF4029C5, 0xFF402AC5, 0xFF402AC6, 0xFF402BC4, 0xFF402EC4, 0xFF412AC5, 0xFF412AC6, 0xFF412BC5, 0xFF412BC6, 0xFF412CC4, 0xFF412CC5,
        0xFF412CC6, 0xFF412EC4, 0xFF422BC6, 0xFF422CC6, 0xFF422DC6, 0xFF432EC6, 0xFF4532C5, 0xFF4535C5, 0xFF4838C6, 0xFF4937C7, 0xFF493CC4, 0xFF4D3CC7, 0xFF5041C9, 0xFF5042C7, 0xFF5444C8, 0xFF5647C8,
        0xFF5749C9, 0xFF584AC9, 0xFF5B50C9, 0xFF5C4FC9, 0xFF5C51C9, 0xFF5D51C9, 0xFF5E52C9, 0xFF6056C9, 0xFF6359CA, 0xFF645BCB, 0xFF6960CB, 0xFF6C63CB, 0xFF6D65CD, 0xFF6E64CE, 0xFF6F65CE, 0xFF6F69CB,
        0xFF706BCA, 0xFF706CCA, 0xFF7168CF, 0xFF716CCA, 0xFF716CCB, 0xFF716CCC, 0xFF7269CF, 0xFF726CCC, 0xFF726DCA, 0xFF726DCC, 0xFF736ACE, 0xFF746DCD, 0xFF756ECD, 0xFF756ECE, 0xFF756ECF, 0xFF756FCF,
        0xFF766CD0, 0xFF766ECF, 0xFF766FCF, 0xFF766FD0, 0xFF7670D0, 0xFF7670D1, 0xFF776FD0, 0xFF7770CE, 0xFF7770D1, 0xFF7771CF, 0xFF7972D1, 0xFF7A72D1, 0xFF7C75D1, 0xFF7F76D3, 0xFF7F78D2, 0xFF8680D3,
        0xFF8682D1, 0xFF8781D3, 0xFF8B85D3, 0xFF8C86D4, 0xFF8D87D4, 0xFF8E89D6, 0xFF8F89D5, 0xFF8F8AD5, 0xFF9490D6, 0xFF9790DA, 0xFF9791D9, 0xFF9792D8, 0xFF9B95DB, 0xFF9B96D8, 0xFF9E9ADA, 0xFFA09CDA,
        0xFFA19DDB, 0xFFA29EDA, 0xFFA29EDB, 0xFFA29EDC, 0xFFA39FDC, 0xFFA4A0DB, 0xFFA5A0DD, 0xFFA5A1DD, 0xFFA5A2DD, 0xFFA6A1DD, 0xFFA6A2DD, 0xFFA6A3DC, 0xFFA8A4DD, 0xFFAAA6DF, 0xFFAAA7DF, 0xFFACA8DF,
        0xFFB1ADE0, 0xFFB1ADE1, 0xFFB2AEE0, 0xFFB2AEE1, 0xFFB6B2E2, 0xFFB6B3E2, 0xFFB7B4E2, 0xFFB7B5E2, 0xFFB8B6E2, 0xFFB9B6E4, 0xFFBAB7E4, 0xFFBEBBE5, 0xFFBEBCE5, 0xFFBFBCE5, 0xFFBFBCE6, 0xFFC0BDE5,
        0xFFC0BDE6, 0xFFC0BEE5, 0xFFC1BFE6, 0xFFC2BFE7, 0xFFC4C2E7, 0xFFC5C2E7, 0xFFC6C4E8, 0xFFC9C6EA, 0xFFCAC8E9, 0xFFCCCAEA, 0xFFCDCCEA, 0xFFCECCEA, 0xFFCECCEB, 0xFFCECCEC, 0xFFD0CEEC, 0xFFD1CFED,
        0xFFD1D0EB, 0xFFD3D1ED, 0xFFD4D1ED, 0xFFD5D3EE, 0xFFD6D4EE, 0xFFD6D4EF, 0xFFD7D5EF, 0xFFD7D6EE, 0xFFD8D6EF, 0xFFD9D7EF, 0xFFD9D7F0, 0xFFDAD8F0, 0xFFDBD9F0, 0xFFDCDAF1, 0xFFDDDBF1, 0xFFDEDCF2,
        0xFFE0DEF2, 0xFFE0DEF3, 0xFFE1DFF2, 0xFFE1DFF3, 0xFFE1E0F3, 0xFFE2E0F3, 0xFFE3E2F4, 0xFFE5E4F4, 0xFFE6E4F5, 0xFFE6E5F4, 0xFFE6E5F5, 0xFFE8E6F6, 0xFFE8E7F5, 0xFFE8E7F6, 0xFFE9E8F6, 0xFFEAE8F7,
        0xFFEBE9F7, 0xFFECEBF7, 0xFFEDECF8, 0xFFEEEDF8, 0xFFEFEEF8, 0xFFF1F0F9, 0xFFF1F0FA, 0xFFF3F2FA, 0xFFF3F3FA, 0xFFF4F3FB, 0xFFF6F6FB, 0xFFF7F7FB, 0xFFF7F7FC, 0xFFF8F7FB, 0xFFF8F8FC, 0xFFF8F8FD,
        0xFFF9F9FD, 0xFFFAFAFD, 0xFFFBFAFE, 0xFFFBFBFD, 0xFFFBFBFE, 0xFFFCFBFE, 0xFFFCFCFE, 0xFFFDFDFE, 0xFFFDFDFF, 0xFFFEFDFE, 0xFFFEFDFF, 0xFFFEFEFE, 0xFFFEFEFF, 0xFFFFFEFF, 0xFFFFFFFF
    };
    constexpr std::array<unsigned short, W * H> indices = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   24,  51,  77,  51,  0,   0,   0,   0,   0,   0,   99,  123, 145, 169, 185, 200, 208, 211, 218, 216, 211, 207, 198, 182, 166, 142, 118, 97,  0,   0,   0,
        0,   0,   0,   76,  75,  50,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   52,  77,  0,   0,   0,   0,   73,  126, 174, 213, 238, 270, 286, 307, 619, 619, 602, 602, 602,
        602, 602, 602, 601, 601, 601, 601, 601, 396, 306, 285, 268, 236, 205, 168, 121, 19,  0,   0,   0,   22,  74,  49,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   26,  78,  27,  0,   0,   0,   128, 188,
        232, 274, 307, 619, 619, 619, 619, 619, 619, 619, 371, 371, 354, 354, 354, 354, 353, 353, 353, 353, 370, 396, 601, 587, 587, 587, 587, 587, 587, 305, 271, 227, 184, 122, 0,   0,   0,   23,
        74,  21,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   54,  80,  0,   0,   0,   136, 213, 267, 312, 619, 619, 619, 619, 398, 355, 355, 372, 372, 398, 397, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 395, 395,
        369, 369, 352, 352, 395, 587, 587, 586, 585, 304, 261, 204, 132, 0,   0,   0,   74,  49,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  29,  0,   0,   115, 203, 269, 372, 619, 619, 619, 372, 355, 372, 398, 619, 619, 619, 619, 619, 602, 602,
        602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 395, 368, 351, 367, 573, 573, 573, 320, 263, 194, 111, 0,   0,   17,  72,  0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   0,   0,   150, 234, 301, 620, 619, 619,
        355, 372, 398, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 394,
        350, 350, 573, 573, 573, 299, 228, 140, 0,   0,   18,  71,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   80,  28,  0,   0,   173, 259, 634, 634, 620, 355, 372, 398, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587,
        587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 394, 350, 350, 573, 573, 394, 252, 162, 0,   0,   47,  46,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   25,  80,  0,   0,   171, 264, 634, 634, 634, 356, 398, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602,
        602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 394, 350, 561, 561, 561,
        258, 159, 0,   0,   70,  16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   0,   143, 256, 634, 634, 400, 357, 399, 619,
        619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 393, 349, 393, 559, 559, 248, 134, 0,   0,   69,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   54,  30,  0,   106, 229, 634, 634, 373, 357, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601,
        587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 348, 392, 559, 347, 223, 98,  0,   45,  44,  0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   89,  0,   0,   189, 290, 634, 634, 357, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,
        602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561,
        561, 561, 559, 559, 559, 559, 347, 558, 558, 284, 177, 0,   0,   69,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  80,  0,   113, 247, 634, 634, 328, 400, 634, 634, 620,
        619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 391, 327, 549, 549, 235, 101, 0,   88,  14,  0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   53,  0,   0,   175, 296, 634, 373, 373, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601,
        601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 366,
        366, 549, 287, 158, 0,   20,  43,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   79,  0,   0,   221, 634, 634, 328, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,
        619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561,
        561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 326, 549, 549, 212, 0,   0,   68,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   89,  0,   103, 257, 634, 400, 373, 634, 634, 634, 634,
        620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585,
        585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 346, 547, 547, 246, 95,  0,   87,  0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   89,  0,   135, 282, 634, 328, 400, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601,
        601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549,
        549, 549, 549, 549, 549, 549, 547, 390, 345, 547, 278, 124, 0,   86,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   160, 308, 634, 328, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,
        619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573,
        561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 325, 539, 298, 144, 0,   86,  0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   81,  0,   181, 634, 634, 357, 634, 634, 634, 634,
        634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586,
        585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539,
        325, 538, 317, 163, 0,   86,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   81,  0,   186, 634, 634, 357, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601,
        601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558,
        549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 538, 344, 538, 538, 176, 0,   86,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   186, 634, 634, 373, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,
        619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573,
        573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 538, 538, 538, 344, 538, 538, 176, 0,   86,  0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   89,  0,   181, 634, 634, 373, 634, 634, 634,
        634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587,
        586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539,
        539, 539, 538, 538, 538, 538, 538, 344, 538, 389, 161, 0,   85,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   89,  0,   160, 634, 634, 373, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601,
        601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558,
        558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 343, 526, 315, 141, 0,   85,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   79,  0,   135, 308, 634, 357, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619,
        619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 585, 644, 658, 653, 658, 658, 622, 571, 573, 588, 659, 659,
        659, 659, 659, 656, 656, 656, 653, 653, 653, 652, 652, 652, 652, 650, 645, 625, 590, 560, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526, 342,
        526, 293, 117, 0,   67,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   53,  0,   103, 282, 634, 357, 634, 634,
        634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587,
        587, 585, 621, 434, 406, 406, 406, 406, 446, 603, 588, 503, 406, 406, 406, 406, 406, 406, 406, 406, 406, 406, 405, 405, 405, 405, 405, 405, 406, 427, 484, 626, 655, 610, 558, 547, 547, 547,
        539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526, 526, 526, 324, 526, 273, 91,  0,   42,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   24,  0,   0,   257, 634, 328, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601,
        601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 621, 471, 717, 732, 729, 729, 732, 704, 471, 514, 672, 742, 746, 743, 744, 744, 743, 743, 743, 743, 743, 743, 743,
        743, 743, 740, 730, 726, 705, 667, 406, 404, 433, 624, 645, 551, 539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526, 526, 526, 526, 526, 324, 526, 242, 0,   15,  13,  0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   221, 634, 328, 634, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619,
        619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 586, 658, 406, 783, 846, 846, 846, 846, 771, 406,
        459, 714, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 838, 821, 796, 753, 686, 406, 405, 594, 613, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526,
        526, 526, 526, 526, 526, 526, 342, 526, 206, 0,   84,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   89,  0,   175, 634, 400, 400,
        634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587,
        587, 587, 586, 585, 586, 658, 407, 780, 846, 839, 839, 846, 766, 406, 461, 710, 836, 846, 843, 843, 843, 843, 843, 843, 843, 843, 843, 843, 843, 843, 843, 844, 846, 846, 846, 846, 846, 834,
        792, 725, 413, 421, 654, 550, 536, 538, 538, 538, 538, 528, 526, 526, 526, 526, 526, 526, 526, 526, 388, 519, 519, 155, 0,   66,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   54,  0,   113, 296, 634, 373, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601,
        601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 575, 658, 407, 781, 846, 841, 843, 846, 766, 406, 461, 712, 838, 846, 845, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 843, 843, 846, 846, 846, 794, 702, 404, 592, 592, 526, 538, 538, 528, 526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 341, 518, 281,
        96,  0,   12,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   30,  0,   247, 634, 328, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619,
        619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 575, 658, 407, 781, 846, 843,
        843, 846, 766, 406, 459, 709, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 846, 837, 754, 408, 478, 624, 525, 528,
        526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518, 323, 518, 231, 0,   41,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   187, 634,
        373, 634, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587,
        587, 587, 587, 586, 585, 585, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 709, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 839, 846, 846, 785, 546, 451, 626, 517, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518, 518, 518, 387, 516, 170, 0,   65,  0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   25,  0,   109, 290, 634, 373, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602,
        601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 709, 838, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 843, 846, 800, 662, 454, 595, 517, 526, 526, 526, 526, 526, 526, 519, 519, 518,
        518, 518, 516, 365, 516, 279, 94,  0,   10,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   229, 634, 328, 634, 634, 634, 620, 619, 619, 619, 619, 619, 619,
        619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 575, 658,
        407, 781, 846, 843, 843, 846, 766, 406, 459, 709, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839,
        846, 798, 464, 497, 563, 526, 526, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 322, 507, 217, 0,   64,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,
        143, 634, 634, 400, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587,
        587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 709, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839, 846, 776, 404, 612, 528, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 386, 507, 316, 130, 0,   40,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   28,  0,   256, 634, 357, 634, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602,
        602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 709,
        838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 736, 404, 648, 525, 526, 519, 519,
        518, 518, 518, 516, 516, 516, 516, 507, 507, 340, 507, 240, 0,   40,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   171, 634, 373, 634, 634, 620, 619, 619, 619, 619, 619,
        619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573,
        573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 844, 846, 825, 673, 454, 566, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 386, 507, 507, 151, 0,   64,  0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   264, 634, 357, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587,
        587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846, 766, 406, 459, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839, 846, 773, 404, 631, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507,
        340, 507, 255, 0,   11,  0,   0,   0,   0,   0,   0,   0,   0,   0,   80,  0,   173, 634, 400, 634, 620, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602,
        602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 575, 658, 407, 781, 846, 843, 843, 846,
        766, 406, 458, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 831, 681,
        435, 568, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 386, 507, 507, 154, 0,   40,  0,   0,   0,   0,   0,   0,   0,   0,   29,  0,   259, 634, 357, 620, 619, 619, 619, 619,
        619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573,
        573, 573, 573, 573, 573, 573, 575, 653, 407, 781, 846, 843, 843, 846, 766, 405, 458, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 758, 402, 633, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 340, 499, 243, 0,   39,  0,   0,   0,   0,
        0,   0,   0,   54,  0,   150, 634, 634, 399, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587,
        587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 575, 653, 407, 781, 846, 843, 843, 846, 765, 405, 458, 708, 838, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 809, 465, 518, 541, 516, 516, 516, 507, 507, 507, 507,
        507, 507, 507, 507, 507, 385, 499, 318, 133, 0,   38,  0,   0,   0,   0,   0,   0,   80,  0,   234, 634, 356, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602,
        602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 562, 653, 407, 781,
        846, 843, 843, 846, 765, 405, 458, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 837, 700, 417, 579, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 339, 499, 222, 0,   63,  0,   0,   0,   0,   0,   26,  0,   115, 301, 620, 398, 619, 619,
        619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573,
        573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 562, 653, 407, 781, 846, 843, 843, 846, 765, 405, 456, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 737, 403, 628, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 364, 499, 288, 100,
        0,   9,   0,   0,   0,   0,   78,  0,   203, 620, 355, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587,
        587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 562, 651, 407, 781, 846, 843, 843, 846, 765, 405, 454, 708, 838, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 769, 402, 628, 507, 507, 507,
        507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 364, 499, 183, 0,   63,  0,   0,   0,   0,   27,  0,   269, 619, 372, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602,
        602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561,
        561, 651, 406, 781, 846, 843, 843, 846, 765, 405, 454, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 843, 846, 784, 403, 597, 510, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 339, 499, 253, 0,   38,  0,   0,   0,   52,  0,   136, 372, 619,
        398, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 561, 651, 406, 780, 846, 843, 843, 846, 765, 405, 454, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 789, 405, 570, 518, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499,
        499, 385, 499, 310, 119, 0,   37,  0,   0,   77,  0,   213, 619, 355, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587,
        587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 561, 650, 405, 780, 846, 843, 843, 846, 765, 405,
        454, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 790, 405,
        569, 518, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 339, 491, 196, 0,   62,  0,   24,  0,   0,   267, 619, 372, 619, 619, 619, 619, 619, 619, 619, 619, 619, 602,
        602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561,
        561, 559, 559, 559, 561, 650, 405, 780, 846, 843, 843, 846, 765, 405, 454, 708, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 787, 404, 581, 518, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 338, 491, 251, 0,   37,  0,   51,  0,
        128, 312, 619, 398, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 560, 650, 405, 780, 846, 843, 843, 846, 764, 405, 454, 707, 838, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 778, 403, 599, 510, 507, 507, 507, 499, 499, 499, 499, 499,
        499, 499, 499, 499, 499, 491, 384, 491, 297, 112, 0,   7,   77,  0,   188, 619, 372, 619, 619, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601,
        587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 560, 650, 405, 780, 846, 843,
        843, 846, 765, 405, 454, 707, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        843, 846, 757, 402, 618, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 384, 490, 172, 0,   61,  51,  0,   232, 619, 355, 619, 619, 619, 619, 619, 619, 619,
        602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561,
        561, 561, 559, 559, 559, 559, 558, 558, 560, 650, 405, 780, 846, 843, 843, 846, 765, 405, 454, 707, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 728, 403, 599, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 337, 490, 220,
        0,   61,  0,   73,  274, 619, 372, 619, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 560, 646, 405, 780, 846, 843, 843, 846, 764, 405, 454, 707, 838, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 826, 679, 439, 555, 507, 499, 499, 499, 499,
        499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 363, 490, 262, 0,   8,   0,   126, 307, 619, 398, 619, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601,
        601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 560, 646,
        405, 780, 846, 843, 843, 846, 765, 405, 454, 707, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 843, 846, 793, 404, 568, 521, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 383, 490, 295, 110, 0,   0,   174, 619, 398, 619, 619, 619, 619,
        619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561,
        561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 551, 646, 405, 780, 846, 843, 843, 846, 765, 405, 452, 707, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 843, 846, 735, 403, 616, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490,
        490, 383, 489, 486, 152, 0,   0,   213, 619, 355, 619, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585,
        585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 551, 646, 405, 780, 846, 843, 843, 846, 765, 405, 452, 707,
        838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 818, 657, 473, 531, 499, 499,
        499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 336, 486, 195, 0,   0,   238, 619, 355, 619, 619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601,
        601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549,
        549, 549, 551, 646, 405, 780, 846, 843, 843, 846, 765, 405, 452, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 843, 846, 748, 403, 617, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 336, 486, 226, 0,   0,   270, 619, 372,
        619, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573,
        561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 551, 646, 405, 780, 846, 843, 843, 846, 765, 405, 452, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839, 846, 805, 440, 511, 520, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490,
        490, 490, 490, 489, 486, 486, 336, 486, 254, 0,   99,  286, 619, 372, 619, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586,
        585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 551, 646, 405, 780, 846, 843, 843, 846,
        765, 404, 451, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 836, 711, 409, 581,
        499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 362, 486, 277, 60,  123, 307, 619, 398, 602, 602, 602, 602, 602, 602, 601, 601, 601, 601,
        601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558,
        549, 549, 549, 549, 549, 549, 551, 645, 405, 780, 846, 843, 843, 846, 765, 404, 451, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 844, 846, 846, 741, 402, 632, 498, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 382, 486, 294, 108,
        145, 619, 619, 397, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573,
        573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 549, 645, 405, 780, 846, 843, 843, 846, 765, 404, 451, 706, 838, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 846, 760, 402, 596, 501, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491,
        490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 382, 486, 314, 129, 169, 619, 371, 602, 602, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587,
        586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 549, 645, 405, 780,
        846, 843, 843, 846, 765, 404, 451, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839, 846, 846, 759, 402,
        556, 520, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 382, 485, 485, 149, 185, 602, 371, 602, 602, 602, 602, 601, 601, 601,
        601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558,
        558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 549, 645, 405, 780, 846, 843, 843, 846, 765, 404, 451, 706, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 844, 843, 846, 836, 738, 402, 567, 522, 498, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486,
        485, 381, 481, 167, 200, 602, 354, 602, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573,
        573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 549, 640, 405, 780, 846, 843, 843, 846, 765, 404, 451, 706, 838, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 843, 843, 846, 846, 804, 701, 402, 598, 511, 498, 499, 499, 499, 499, 499, 499, 491, 491, 491,
        491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 361, 481, 180, 208, 602, 354, 602, 602, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587,
        587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 575, 588, 604, 607, 605, 607, 607, 607, 594, 594, 594, 594, 594, 593, 593, 593, 605, 593,
        551, 640, 405, 780, 846, 843, 843, 846, 765, 404, 449, 703, 836, 846, 843, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 843, 843, 843, 844, 846, 846, 846, 817, 747, 437,
        411, 632, 501, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 361, 481, 192, 211, 602, 354, 602, 601, 601,
        601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 577, 625, 653, 656, 608, 535, 477, 459,
        459, 459, 457, 457, 457, 457, 457, 457, 457, 457, 457, 457, 455, 455, 524, 642, 404, 780, 846, 843, 843, 846, 765, 404, 445, 713, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 828, 793, 734, 649, 403, 520, 570, 490, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486,
        486, 485, 485, 481, 481, 335, 481, 202, 218, 602, 354, 601, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573,
        573, 573, 573, 561, 591, 656, 622, 442, 406, 406, 425, 584, 670, 678, 677, 677, 677, 677, 677, 677, 677, 677, 677, 677, 677, 676, 676, 674, 640, 624, 404, 780, 846, 843, 843, 846, 765, 404,
        460, 684, 808, 814, 813, 813, 813, 813, 813, 813, 813, 813, 813, 813, 813, 813, 813, 811, 807, 791, 767, 733, 680, 404, 403, 479, 615, 511, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491,
        491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 335, 481, 199, 214, 602, 353, 601, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587,
        587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 589, 659, 504, 406, 423, 685, 742, 775, 802, 815, 823, 829, 827, 827, 827, 827, 827, 827, 827, 827, 827, 827,
        827, 827, 830, 824, 687, 458, 405, 780, 846, 843, 843, 846, 765, 403, 641, 525, 513, 502, 502, 502, 502, 495, 495, 494, 494, 494, 493, 492, 492, 492, 487, 482, 431, 404, 402, 403, 432, 568,
        616, 531, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 335, 480, 199, 210, 601,
        353, 601, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 571, 647, 562, 406, 665, 742, 799, 833, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 839, 697, 454, 405, 780, 846, 843, 843, 846, 765, 403, 641, 526, 517, 517, 517, 508, 508, 507, 507, 507,
        507, 505, 505, 505, 505, 500, 500, 500, 533, 582, 630, 614, 557, 521, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486,
        486, 486, 485, 485, 481, 481, 481, 481, 481, 334, 480, 201, 207, 601, 353, 601, 601, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573,
        573, 573, 573, 575, 661, 420, 463, 750, 819, 846, 846, 846, 843, 843, 843, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 844, 846, 833, 693, 454, 405, 780, 846, 843,
        843, 846, 765, 403, 641, 528, 550, 550, 542, 542, 542, 541, 541, 541, 541, 540, 540, 540, 529, 529, 529, 529, 523, 518, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491,
        491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 360, 480, 191, 198, 601, 353, 601, 601, 601, 587, 587, 587, 587, 587, 587,
        587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 571, 589, 653, 406, 715, 806, 846, 846, 843, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 835, 695, 454, 405, 780, 846, 843, 843, 846, 765, 403, 641, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507,
        507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 360,
        480, 179, 182, 601, 370, 601, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 571, 604, 637, 406, 745, 837, 846, 843, 844, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 454, 405, 780, 846, 843, 843, 846, 765, 403, 641, 526, 526, 519, 519, 518,
        518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486,
        486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 380, 476, 165, 166, 601, 396, 601, 587, 587, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573,
        573, 573, 573, 573, 603, 636, 406, 761, 846, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 454,
        405, 780, 846, 843, 843, 846, 765, 403, 641, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499,
        491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 380, 476, 476, 147, 142, 396, 601, 395, 587, 587, 587, 587,
        587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 575, 651, 406, 762, 846, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 765, 403, 641, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507,
        507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480,
        480, 480, 379, 476, 313, 125, 118, 306, 587, 395, 587, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 571, 661, 406, 749, 846, 846, 844, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 765, 403, 631, 519,
        518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486,
        486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 379, 475, 292, 105, 97,  285, 587, 369, 587, 587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573,
        573, 573, 573, 573, 573, 571, 651, 415, 718, 840, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 765, 403, 631, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499,
        499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 359, 475, 272, 32,  0,   268, 587, 369,
        587, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 589, 548, 534, 810, 846, 841, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 764, 403, 630, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507,
        507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480,
        480, 480, 480, 476, 476, 475, 333, 475, 250, 0,   0,   236, 587, 352, 587, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 660, 405, 753, 846, 839,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846,
        764, 402, 630, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486,
        486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 333, 475, 224, 0,   0,   205, 587, 352, 587, 587, 587, 586, 585, 585, 573, 573, 573, 573,
        573, 573, 573, 573, 573, 573, 573, 561, 606, 483, 668, 822, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 764, 402, 630, 518, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499,
        499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 359, 475, 193, 0,
        0,   168, 587, 395, 587, 587, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 659, 406, 751, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 764, 402, 630, 516, 516, 516, 507, 507, 507, 507,
        507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481,
        480, 480, 480, 480, 476, 476, 475, 475, 475, 378, 475, 475, 148, 0,   0,   121, 305, 587, 395, 586, 585, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 577, 591, 443,
        804, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780,
        846, 843, 843, 846, 764, 402, 628, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486,
        486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 378, 475, 289, 104, 0,   0,   19,  271, 587, 368, 585, 585, 573, 573, 573,
        573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 625, 438, 690, 833, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 452, 405, 780, 846, 843, 843, 846, 764, 402, 628, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499,
        499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 359,
        470, 260, 0,   6,   76,  0,   227, 586, 351, 585, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 653, 406, 739, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 695, 450, 405, 780, 846, 843, 843, 846, 764, 402, 628, 507, 507, 507,
        507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481,
        481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 332, 470, 219, 0,   59,  75,  0,   184, 585, 367, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561,
        559, 658, 406, 772, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835,
        693, 448, 404, 780, 846, 843, 843, 846, 764, 402, 628, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489,
        486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 377, 470, 164, 0,   59,  50,  0,   122, 304, 573, 394,
        573, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 573, 638, 407, 788, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 693, 447, 404, 779, 846, 843, 843, 846, 764, 402, 616, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499,
        499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475,
        475, 470, 377, 470, 291, 102, 0,   5,   0,   22,  0,   261, 573, 350, 573, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 575, 604, 438, 801, 846, 843, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 693, 447, 404, 780, 846, 843, 843, 846, 764, 402,
        616, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481,
        481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 332, 469, 245, 0,   35,  0,   0,   74,  0,   204, 573, 350, 573, 573, 573, 573, 573, 573, 573, 561, 561, 561,
        561, 559, 559, 559, 588, 545, 544, 812, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 835, 692, 447, 404, 780, 846, 843, 843, 846, 764, 402, 616, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490,
        489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 331, 469, 190, 0,   58,  0,   0,   49,
        0,   132, 320, 573, 394, 573, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 576, 574, 459, 806, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 447, 404, 780, 846, 843, 843, 846, 764, 402, 616, 507, 507, 507, 507, 499, 499, 499, 499, 499,
        499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475,
        475, 475, 470, 470, 470, 376, 469, 302, 114, 0,   34,  0,   0,   0,   23,  0,   263, 573, 350, 573, 573, 573, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 572, 635, 414, 794, 846, 843,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 447, 404, 780, 846, 843,
        843, 846, 764, 402, 616, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481,
        481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 331, 469, 244, 0,   35,  0,   0,   0,   0,   74,  0,   194, 573, 350, 573, 573, 573, 561, 561,
        561, 561, 559, 559, 559, 559, 558, 558, 560, 650, 405, 777, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 835, 692, 447, 404, 780, 846, 843, 843, 846, 764, 402, 616, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490,
        490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 358, 468, 178, 0,   57,
        0,   0,   0,   0,   21,  0,   111, 299, 573, 394, 573, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 655, 405, 752, 846, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 447, 404, 780, 846, 843, 843, 846, 764, 402, 616, 507, 499, 499, 499, 499,
        499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475,
        475, 475, 475, 470, 470, 470, 469, 469, 358, 468, 280, 92,  0,   4,   0,   0,   0,   0,   0,   74,  0,   228, 573, 350, 561, 561, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 637,
        412, 721, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 447,
        404, 780, 846, 843, 843, 846, 764, 402, 628, 501, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481,
        481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 330, 468, 215, 0,   57,  0,   0,   0,   0,   0,   0,   49,  0,   140, 394, 561,
        393, 561, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 578, 496, 664, 819, 846, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 764, 402, 615, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490,
        490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 375, 468, 311,
        127, 0,   33,  0,   0,   0,   0,   0,   0,   0,   17,  0,   252, 561, 349, 561, 559, 559, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 652, 405, 774, 846, 843, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 764, 402, 615, 499,
        499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475,
        475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 330, 466, 237, 0,   33,  0,   0,   0,   0,   0,   0,   0,   0,   72,  0,   162, 561, 393, 559, 559, 559, 559, 558, 558, 558, 558, 549,
        549, 549, 549, 549, 627, 418, 716, 844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 763, 401, 615, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485,
        481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 375, 466, 374, 139, 0,   33,  0,   0,   0,   0,   0,   0,   0,   0,
        0,   18,  0,   258, 559, 348, 559, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 551, 640, 406, 786, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 763, 401, 615, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490,
        490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468,
        329, 466, 239, 0,   36,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   71,  0,   159, 559, 392, 559, 558, 558, 558, 558, 549, 549, 549, 549, 549, 549, 549, 547, 623, 429, 694, 836, 846,
        844, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846,
        763, 401, 615, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475,
        475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 374, 466, 374, 137, 0,   33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   48,  0,   249, 559, 347, 558, 558, 558,
        549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 655, 405, 756, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 763, 401, 615, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485,
        485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 321, 466, 233, 0,   55,  0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   46,  0,   134, 347, 558, 391, 558, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 554, 564, 428, 795, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 780, 846, 843, 843, 846, 763, 401, 615, 499, 499, 499, 491, 491, 491, 491,
        490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468,
        468, 466, 374, 466, 303, 116, 0,   33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   70,  0,   223, 558, 327, 549, 549, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539,
        537, 609, 454, 669, 814, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 404, 778,
        846, 843, 843, 846, 763, 401, 615, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476,
        475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 329, 466, 209, 0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   16,  0,   98,
        284, 549, 366, 549, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 526, 639, 426, 675, 816, 846, 839, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 445, 403, 778, 846, 843, 843, 846, 763, 401, 615, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486,
        485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 329, 466, 275, 82,  0,   2,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   69,  0,   177, 549, 366, 549, 549, 549, 549, 547, 547, 547, 547, 539, 539, 539, 538, 538, 525, 648, 422, 671, 800, 846, 846, 843,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835, 692, 444, 403, 778, 846, 843, 843, 846, 763, 401, 600, 491, 491, 491,
        491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469,
        468, 468, 466, 466, 374, 466, 466, 157, 0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   45,  0,   235, 549, 326, 549, 549, 547, 547, 547, 547,
        539, 539, 539, 538, 538, 538, 538, 525, 641, 445, 462, 770, 846, 846, 844, 843, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 835,
        691, 439, 403, 778, 846, 843, 843, 846, 763, 401, 600, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476,
        476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 321, 466, 225, 0,   56,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   44,  0,   101, 287, 549, 346, 547, 547, 547, 547, 539, 539, 539, 538, 538, 538, 538, 538, 538, 525, 612, 515, 404, 719, 803, 846, 846, 846, 843, 843, 844, 844, 846, 846, 846,
        846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 835, 689, 439, 403, 778, 846, 839, 841, 846, 763, 401, 600, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486,
        486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 329, 466, 276, 90,  0,   3,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   69,  0,   158, 549, 547, 390, 547, 547, 539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 519,
        553, 641, 405, 441, 731, 797, 838, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 844, 846, 835, 688, 439, 403, 778, 846, 839, 839, 846, 763, 401,
        600, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469,
        469, 468, 468, 466, 466, 466, 374, 466, 329, 138, 0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   88,  0,   212, 547, 345,
        547, 539, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526, 526, 526, 526, 626, 553, 404, 410, 698, 755, 793, 820, 832, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846, 846,
        846, 846, 846, 842, 696, 436, 403, 782, 846, 846, 846, 846, 768, 401, 600, 491, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480,
        476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 329, 466, 197, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   14,  20,  0,   246, 547, 325, 539, 539, 538, 538, 538, 538, 538, 538, 538, 528, 526, 526, 526, 526, 526, 526, 542, 629, 579, 419, 403, 404, 663,
        682, 720, 724, 723, 723, 723, 723, 723, 723, 723, 723, 723, 723, 723, 723, 723, 723, 722, 666, 474, 453, 699, 726, 722, 722, 727, 683, 444, 512, 490, 490, 490, 489, 486, 486, 486, 486, 486,
        486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 230, 0,   36,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   43,  0,   95,  278, 539, 325, 538, 538, 538, 538, 538, 538, 538, 528,
        526, 526, 526, 526, 526, 526, 526, 526, 526, 530, 580, 643, 596, 472, 430, 404, 403, 403, 403, 403, 403, 403, 403, 403, 403, 403, 403, 403, 402, 402, 402, 402, 467, 506, 520, 416, 402, 402,
        402, 402, 424, 511, 488, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469,
        469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 266, 31,  0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   68,  0,   124, 298, 538, 344, 538, 538, 538, 538, 538, 528, 526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518, 518, 527, 552, 565, 597, 611, 597, 597, 597, 597, 597, 597, 597,
        597, 597, 583, 581, 583, 581, 583, 581, 509, 499, 498, 543, 583, 570, 570, 583, 532, 488, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480,
        480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 283, 107, 0,   33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   87,  0,   144, 317, 538, 344, 538, 538, 538, 528, 526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518,
        518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486,
        486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 329, 466, 300, 131,
        0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   86,  0,   163, 538, 538, 344, 538,
        528, 526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499,
        491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470,
        469, 469, 469, 468, 468, 466, 466, 466, 466, 329, 466, 309, 146, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   86,  0,   176, 538, 538, 343, 526, 526, 526, 526, 526, 526, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507,
        507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480,
        480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 329, 466, 319, 156, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   86,  0,   176, 389, 526, 342, 526, 526, 526, 526, 526, 526, 526, 519, 519,
        518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486,
        486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 309,
        156, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        86,  0,   161, 315, 526, 324, 526, 526, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499,
        499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470,
        470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 300, 146, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   86,  0,   141, 293, 526, 324, 526, 526, 526, 519, 519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507,
        507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480,
        480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 466, 321, 466, 283, 131, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   85,  0,   117, 273, 526, 342, 388, 519,
        519, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486,
        486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 374, 329, 466,
        265, 107, 0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   85,  0,   91,  242, 526, 519, 341, 518, 518, 518, 516, 516, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499,
        499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470,
        470, 470, 469, 469, 469, 468, 468, 466, 466, 466, 329, 466, 466, 230, 1,   0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   67,  0,   0,   206, 519, 518, 323, 518, 516, 516, 516, 516, 507, 507, 507, 507,
        507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481,
        480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 374, 321, 466, 329, 197, 0,   0,   33,  0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   42,
        15,  0,   155, 281, 518, 387, 365, 516, 516, 507, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486,
        486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 466, 466, 329, 466, 466,
        276, 138, 0,   36,  3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   13,  84,  0,   96,  231, 516, 516, 322, 386, 507, 507, 507, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499,
        499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475,
        470, 470, 470, 469, 469, 469, 468, 468, 466, 374, 329, 466, 466, 225, 90,  0,   83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   66,  0,   0,   170, 279, 507, 507, 340,
        386, 507, 507, 507, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481,
        481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 468, 374, 321, 466, 466, 275, 157, 0,   0,   55,  0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   12,  41,  0,   93,  217, 316, 507, 507, 340, 386, 507, 507, 499, 499, 499, 499, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489,
        486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 469, 468, 375, 329, 466, 466, 303,
        209, 82,  0,   56,  3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   65,  0,   0,   130, 240, 507, 507, 507, 340, 385, 499, 499, 499, 499, 499,
        499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475,
        475, 470, 470, 470, 469, 469, 469, 375, 330, 466, 466, 374, 233, 116, 0,   0,   55,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        10,  64,  0,   0,   151, 255, 507, 499, 499, 339, 364, 499, 499, 499, 499, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481,
        481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 469, 469, 358, 330, 468, 466, 374, 239, 137, 0,   0,   55,  2,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   40,  40,  0,   0,   153, 241, 318, 499, 499, 364, 339, 385, 499, 499, 491, 491, 491, 491, 490, 490, 490, 490,
        489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475, 475, 475, 470, 470, 470, 376, 331, 358, 468, 468, 311, 237, 139,
        0,   0,   55,  33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   64,  11,  0,   0,   133, 222,
        288, 499, 499, 499, 339, 338, 384, 491, 491, 490, 490, 490, 490, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 475,
        475, 475, 470, 377, 332, 331, 469, 469, 468, 280, 215, 127, 0,   0,   36,  33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   40,  39,  0,   0,   100, 183, 253, 310, 491, 491, 491, 384, 337, 363, 383, 383, 489, 486, 486, 486, 486, 486, 486, 486, 485, 485, 481, 481,
        481, 481, 481, 480, 480, 480, 480, 476, 476, 475, 475, 475, 378, 378, 359, 332, 377, 470, 469, 469, 302, 244, 178, 92,  0,   0,   33,  33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   38,  63,  0,   0,   0,   120, 196, 251, 297, 490, 490, 490,
        490, 489, 336, 336, 336, 362, 382, 382, 382, 485, 485, 481, 481, 481, 481, 481, 480, 480, 480, 380, 379, 379, 359, 333, 333, 359, 475, 475, 470, 470, 470, 291, 245, 190, 114, 0,   0,   0,
        57,  33,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   9,   63,  38,  0,   0,   0,   112, 172, 220, 262, 295, 486, 486, 486, 486, 486, 486, 486, 485, 381, 361, 361, 335, 335, 335, 334, 360, 360, 380, 476, 476, 475, 475, 475,
        475, 475, 475, 289, 260, 219, 164, 102, 0,   0,   0,   35,  57,  4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   37,  62,  37,  0,   0,   0,   0,   110, 152, 195, 226, 254, 277, 294, 314, 485, 481,
        481, 481, 481, 481, 480, 480, 480, 480, 476, 476, 313, 292, 272, 250, 224, 193, 148, 104, 0,   0,   0,   0,   35,  58,  34,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        7,   61,  61,  8,   0,   0,   0,   0,   0,   60,  108, 129, 149, 167, 180, 192, 202, 199, 199, 201, 191, 179, 165, 147, 125, 105, 32,  0,   0,   0,   0,   0,   6,   59,  59,  5,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0
    };

    std::vector<CS200::RGBA> rgba_colors;
    rgba_colors.reserve(indices.size());
    for (const auto index : indices)
    {
        rgba_colors.push_back(colors[index]);
    }

    logoTextureHandle = OpenGL::CreateTextureFromMemory({ W, H }, std::span{ rgba_colors }, OpenGL::Filtering::NearestPixel, OpenGL::Wrapping::Repeat);
}
