#include "SSBO.hpp"

SSBO::SSBO(const GLenum bufferUsageHint) : BufferUsageHint(bufferUsageHint)
{
    glGenBuffers(1, &GLBuffer);
}

SSBO::~SSBO()
{
    glDeleteBuffers(1, &GLBuffer);
}

void SSBO::Bind() const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, GLBuffer);
}

void SSBO::Unbind()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SSBO::Update(const void *data, const unsigned int size)
{
    Bind();

    if (Size != size)
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, BufferUsageHint);
    }
    else
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    }
    Size = size;

    Unbind();
}

SSBO::operator unsigned int() const
{
    return GLBuffer;
}
