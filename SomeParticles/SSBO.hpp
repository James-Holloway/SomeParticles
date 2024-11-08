#ifndef SSBO_HPP
#define SSBO_HPP

#include <vector>
#include <glad/glad.h>

class SSBO
{
public:
    unsigned int GLBuffer = 0;
    unsigned int Size = 0;
    const GLenum BufferUsageHint;

    explicit SSBO(GLenum bufferUsageHint = GL_DYNAMIC_DRAW);
    ~SSBO();

    void Bind() const;
    static void Unbind();

    void Update(const void *data, unsigned int size);

    template<typename T>
    void Update(const std::vector<T> &data)
    {
        Update(data.data(), data.size() * sizeof(T));
    }

    explicit operator unsigned int() const;
};

#endif //SSBO_HPP
