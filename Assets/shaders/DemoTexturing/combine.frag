#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

in vec3 vColor;
in vec2 vTextureCoordinates;

layout(location = 0) out vec4 fFragClr;

uniform sampler2D uTex2d;
uniform float     uModulateColor;
uniform float     uProcTex;
uniform float     uTileSize;
uniform float     uUseImage;
uniform bool      uUseTextureAlpha;

const vec4 cyan = vec4(1.0, 0.0, 1.0, 1.0);
const vec4 teal = vec4(0.0, 0.68, 0.94, 1.0);

void main()
{
    vec4 interpolated_color = mix(vec4(1.0), vec4(vColor, 1.0), uModulateColor);
    vec4 procedural_color   = mix(vec4(1.0), (0.0 == mod(floor(gl_FragCoord.x / uTileSize) + floor(gl_FragCoord.y / uTileSize), 2.0)) ? cyan : teal, uProcTex);
    vec4 texture_color      = mix(vec4(1.0), texture(uTex2d, vTextureCoordinates), uUseImage);
    if (!uUseTextureAlpha)
    {
        texture_color.a = 1.0;
    }

    fFragClr = interpolated_color * procedural_color * texture_color;
}
