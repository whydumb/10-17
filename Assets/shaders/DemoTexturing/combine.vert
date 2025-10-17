#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec3 aVertexColor;
layout(location = 2) in vec2 aVertexTextureCoordinates;

out vec3 vColor;
out vec2 vTextureCoordinates;

uniform float uTexCoordScale;

uniform mat3 uToNDC;
uniform mat3 uModel;

void main()
{
    vec3 ndc_position   = uToNDC * uModel * vec3(aVertexPosition, 1.0);
    gl_Position         = vec4(ndc_position.xy, 0.0, 1.0);
    vColor              = aVertexColor;
    vTextureCoordinates = aVertexTextureCoordinates * uTexCoordScale;
}
