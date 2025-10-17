#version 300 es
precision         mediump float;
precision mediump sampler2D;

/**
 * \file
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

// declare inputs and outputs

in vec2 v_texCoord;                   // Interpolated texture coordinate from vertex shader

uniform sampler2D u_texture;         // Texture sampler
uniform vec4 u_tint;                 // Tint color (RGBA)

out vec4 fragColor;                  // Output color

void main()
{
    vec4 texColor = texture(u_texture, v_texCoord);
    vec4 finalColor = texColor * u_tint;

    if (finalColor.a == 0.0) 
        discard;

    fragColor = finalColor;
}
