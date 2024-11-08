#version 450
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_NV_shader_atomic_int64 : require

// Packing: R21 G22 B21 (high to low)
const vec3 packedMax = vec3((1 << 21) - 1, (1 << 22) - 1, (1 << 21) - 1);
const uvec3 packingOffsets = uvec3(21 + 22, 21, 0);
const uvec3 packingMasks = uvec3(0x1FFFFF, 0x3FFFFF, 0x1FFFFF);

// Scales the particle color output by this value
uniform float outputScalar;

uniform vec3 ColdColor;
uniform vec3 HotColor;

uniform ivec2 RenderTextureDimensions;

layout(std430, binding = 0) restrict readonly buffer PixelBufferSSBO
{
    int64_t PixelBuffer[];
};

layout (location = 0) in vec2 uv;
out vec4 outFragColor;

vec3 unpack(ivec2 coord)
{
    if (coord.x < 0 || coord.x >= RenderTextureDimensions.x || coord.y < 0 || coord.y >= RenderTextureDimensions.y)
    {
        return vec3(0.0);
    }

    int index = (coord.y * RenderTextureDimensions.x) + coord.x;
    uint64_t packedRGB = uint64_t(PixelBuffer[index]);

    uvec3 uintRGB = uvec3(uint(packedRGB >> packingOffsets.r) & packingMasks.r, uint(packedRGB >> packingOffsets.g) & packingMasks.g, uint(packedRGB >> packingOffsets.b) & packingMasks.b);

    return (uintRGB / packedMax);
}

void main()
{
    ivec2 pixelCoord = ivec2(uv * vec2(RenderTextureDimensions));

    vec3 col = unpack(pixelCoord) * outputScalar;
    if (col.x > 0.0 && col.y > 0.0 && col.z > 0.0)
    {
        col = mix(ColdColor, HotColor, col) * max(col.x, max(col.y, col.z));
    }

    outFragColor = vec4(col, 1.0);
}
