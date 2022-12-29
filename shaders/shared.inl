#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

#define BlockFace_Left    0
#define BlockFace_Right   1
#define BlockFace_Bottom  2
#define BlockFace_Top     3
#define BlockFace_Back    4
#define BlockFace_Front   5
#define BlockFace_Cross_A 6
#define BlockFace_Cross_B 7

struct UnpackedFace {
    f32vec3 block_position;
    f32vec3 position;
    f32vec3 normal;
    f32vec2 uv;
    u32 block_id;
    u32 block_face;
    u32 texture_id;
    u32 vertex_id;
};

struct DrawVertexBuffer {
    daxa_u32 data[16 * 16 * 16 * 6];
};
DAXA_ENABLE_BUFFER_PTR(DrawVertexBuffer)

struct DrawPush {
    f32mat4x4 vp;
    f32vec3 chunk_pos;
    daxa_RWBufferPtr(DrawVertexBuffer) face_buffer;
    daxa_Image2DArrayf32 atlas_texture;
    daxa_SamplerId atlas_sampler;
};
