/**
 * \file
 * \author Rudy Castan
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "VertexArray.hpp"
#include "GL.hpp"

namespace OpenGL
{
    /**
     * \brief Creates and configures a Vertex Array Object (VAO) with multiple vertex buffers and optional index buffer

     * \param vertices An initializer list of VertexBuffer objects, each containing:
     *                 - buffer_handle: The OpenGL buffer handle containing vertex data
     *                 - buffer_layout: Description of how the data is organized (attributes, stride, offset)
     * \param index_buffer Optional index buffer handle for indexed rendering (0 if not used)
     *
     * \return VertexArrayHandle The OpenGL handle to the created VAO
     *
     * \note The VAO will be unbound (set to 0) before returning to avoid affecting subsequent OpenGL state
     * \note Each vertex attribute will be assigned sequential attribute indices starting from 0
     */
    VertexArrayHandle CreateVertexArrayObject(std::initializer_list<VertexBuffer> vertices, BufferHandle index_buffer)
    {
        // PSEUDO CODE for CreateVertexArrayObject:
        // 1. Create a new Vertex Array Object (VAO)
        // 2. Bind the VAO to make it active
        // 3. For each vertex buffer:
        //    a. Bind the buffer as GL_ARRAY_BUFFER
        //    b. Calculate the stride (total bytes per vertex)
        //    c. For each attribute in the buffer layout:
        //       - Enable the vertex attribute array
        //       - Set up the vertex attribute pointer (regular or integer)
        //       - Set the vertex attribute divisor for instancing
        // 4. If an index buffer is provided, bind it as GL_ELEMENT_ARRAY_BUFFER
        // 5. Unbind the VAO (bind 0) 
        // 6. Return the VAO handle

        VertexArrayHandle vao{};

        GL::GenVertexArrays(1, &vao);

        GL::BindVertexArray(vao);

        GLuint attribute_index = 0;

        for(auto& each : vertices)
        {
            GL::BindBuffer(GL_ARRAY_BUFFER, each.Handle);

            GLsizei stride = 0;
            for (const auto& attr_type : each.Layout.Attributes) {
                if(attr_type != Attribute::None) stride += attr_type.SizeBytes;
            }

            GLintptr offset = static_cast<GLintptr>(each.Layout.BufferStartingByteOffset);

            for(Attribute::Type attr_type : each.Layout.Attributes)
            {
                if(attr_type == Attribute::None) continue;

                GL::EnableVertexAttribArray(attribute_index);

                const GLenum    gl_type         = attr_type.GLType;
                const GLint     component_count = attr_type.ComponentCount;
                const GLboolean normalized      = attr_type.Normalize;
                const bool      is_integer      = attr_type.IntAttribute;
                const GLuint    divisor         = attr_type.Divisor;

                if(is_integer == true){
                    GL::VertexAttribIPointer(attribute_index, component_count, gl_type, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
                }
                else{
                    GL::VertexAttribPointer(attribute_index, component_count, gl_type, normalized, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
                }
                GL::VertexAttribDivisor(attribute_index, divisor);
                ++attribute_index;
                offset+=attr_type.SizeBytes;
            }
        }

        if(index_buffer != 0){
            GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        }

        GL::BindVertexArray(0);

        return vao;
    }

    VertexArrayHandle CreateVertexArrayObject(VertexBuffer vertices, BufferHandle index_buffer)
    {
        return CreateVertexArrayObject({ vertices }, index_buffer);
    }

}
