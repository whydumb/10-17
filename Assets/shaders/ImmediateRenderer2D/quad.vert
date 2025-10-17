#version 300 es

/**
 * \file
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

// declare inputs, outputs, and uniforms as needed
layout(location = 0) in vec2 a_position;      // Vertex position in model space
layout(location = 1) in vec2 a_texCoord;      // Texture coordinate

uniform mat3 u_model;                         // 3x3 model transformation matrix
uniform float u_depth;                        // Depth (for z coordinate in clip space)
uniform mat3 u_textureTransform;              // Texture coordinate transformation

layout(std140) uniform Camera {
    mat3 u_viewProjection;                    // View-projection matrix from uniform block
};

out vec2 v_texCoord;                          // Pass to fragment shader

void main()
{
    // Ensure position is converted to vec3 with a zero z value
    vec3 model_pos = u_model * vec3(a_position, 1.0); // Use 0.0 for z
    vec3 ndc_pos = u_viewProjection * model_pos;
    gl_Position = vec4(ndc_pos.xy, 0.5, 1.0); // Ensure z is 0.5 for 2D rendering

    // Apply texture transformation
    vec3 texCoord = u_textureTransform * vec3(a_texCoord, 1.0);
    v_texCoord = texCoord.st;  // Correct the texCoord for fragment shader
}
