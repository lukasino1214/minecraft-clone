#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

struct DrawVertex {
    f32vec3 position;
    f32vec3 color;
    f32vec2 uv;
};

DAXA_ENABLE_BUFFER_PTR(DrawVertex)

struct DrawPush {
    f32mat4x4 vp;
    f32vec3 chunk_pos;
    daxa_RWBufferPtr(DrawVertex) face_buffer;
    daxa_Image2DArrayf32 atlas_texture;
    daxa_SamplerId atlas_sampler;
};
