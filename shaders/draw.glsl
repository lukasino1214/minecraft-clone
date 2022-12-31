#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawPush)

#define PI 3.14159265

u32 rand_hash(u32 x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
u32 rand_hash(u32vec2 v) { return rand_hash(v.x ^ rand_hash(v.y)); }
u32 rand_hash(u32vec3 v) {
    return rand_hash(v.x ^ rand_hash(v.y) ^ rand_hash(v.z));
}
u32 rand_hash(u32vec4 v) {
    return rand_hash(v.x ^ rand_hash(v.y) ^ rand_hash(v.z) ^ rand_hash(v.w));
}
f32 rand_float_construct(u32 m) {
    const u32 ieee_mantissa = 0x007FFFFFu;
    const u32 ieee_one = 0x3F800000u;
    m &= ieee_mantissa;
    m |= ieee_one;
    f32 f = uintBitsToFloat(m);
    return f - 1.0;
}
f32 rand(f32 x) { return rand_float_construct(rand_hash(floatBitsToUint(x))); }
f32 rand(f32vec2 v) { return rand_float_construct(rand_hash(floatBitsToUint(v))); }
f32 rand(f32vec3 v) { return rand_float_construct(rand_hash(floatBitsToUint(v))); }
f32 rand(f32vec4 v) { return rand_float_construct(rand_hash(floatBitsToUint(v))); }

const f32vec2 instance_offsets[6] = {
    f32vec2(+0.0, +1.0),
    f32vec2(+1.0, +1.0),
    f32vec2(+0.0, +0.0),
    f32vec2(+1.0, +1.0),
    f32vec2(+1.0, +0.0),
    f32vec2(+0.0, +0.0),
};
const f32vec3 cross_instance_positions[6] = {
    f32vec3(+0.0 + 0.2, +0.2, +1.0 - 0.2),
    f32vec3(+1.0 - 0.2, +0.2, +0.0 + 0.2),
    f32vec3(+0.0 + 0.2, +1.0, +1.0 - 0.2),
    f32vec3(+1.0 - 0.2, +0.2, +0.0 + 0.2),
    f32vec3(+1.0 - 0.2, +1.0, +0.0 + 0.2),
    f32vec3(+0.0 + 0.2, +1.0, +1.0 - 0.2),
};

void correct_pos(inout UnpackedFace face, u32 face_id) {
    f32vec3 cross_uv = cross_instance_positions[face.vertex_id];
    f32vec2 rnd = f32vec2(
        face.block_position.x * 3.165 + face.block_position.y * 5.967 + face.block_position.z * 7.830,
        face.block_position.x * 1.715 + face.block_position.y * 6.683 + face.block_position.z * 3.096);
    rnd = f32vec2(rand(rnd.x) - 0.5, rand(rnd.y) - 0.5);
    f32vec2 cross_rnd = rnd * 0.4;
    switch (face.block_face) {
        case BlockFace_Left: face.position += f32vec3(1.0, face.uv.x, face.uv.y), face.normal = f32vec3(+1.0, +0.0, +0.0); break;
        case BlockFace_Right: face.position += f32vec3(0.0, 1.0 - face.uv.x, face.uv.y), face.normal = f32vec3(-1.0, +0.0, +0.0); break;
        case BlockFace_Bottom: face.position += f32vec3(1.0 - face.uv.x, 1.0, face.uv.y), face.normal = f32vec3(+0.0, +1.0, +0.0); break;
        case BlockFace_Top: face.position += f32vec3(face.uv.x, 0.0, face.uv.y), face.normal = f32vec3(+0.0, -1.0, +0.0); break;
        case BlockFace_Back: face.position += f32vec3(face.uv.x, face.uv.y, 1.0), face.normal = f32vec3(+0.0, +0.0, +1.0); break;
        case BlockFace_Front: face.position += f32vec3(1.0 - face.uv.x, face.uv.y, 0.0), face.normal = f32vec3(+0.0, +0.0, -1.0); break;
        case BlockFace_Cross_A:
            {
                if (face_id % 2 == 0) {
                    face.position += f32vec3(cross_uv.x, 1.0 - cross_uv.y, cross_uv.z);
                } else {
                    face.position += f32vec3(1.0 - cross_uv.x, 1.0 - cross_uv.y, 1.0 - cross_uv.z);
                }
                face.position.xz += cross_rnd;
                face.normal = f32vec3(+0.0, 1.0, +0.0);
            }
            break;
        case BlockFace_Cross_B:
            {
                if (face_id % 2 == 0) {
                    face.position += f32vec3(cross_uv.x, 1.0 - cross_uv.y, 1.0 - cross_uv.z);
                } else {
                    face.position += f32vec3(1.0 - cross_uv.x, 1.0 - cross_uv.y, cross_uv.z);
                }
                face.position.xz += cross_rnd;
                face.normal = f32vec3(+0.0, 1.0, +0.0);
            }
            break;
    }
}

void correct_uv(inout UnpackedFace face, u32 face_id) {
    switch (face.block_face) {
        case BlockFace_Left: face.uv = f32vec2(1.0, 0.0) + f32vec2(-face.uv.y, +face.uv.x); break;
        case BlockFace_Right: face.uv = f32vec2(0.0, 1.0) + f32vec2(+face.uv.y, -face.uv.x); break;
        case BlockFace_Bottom: face.uv = f32vec2(0.0, 1.0) + f32vec2(+face.uv.x, -face.uv.y); break;
        case BlockFace_Top: face.uv = f32vec2(0.0, 1.0) + f32vec2(+face.uv.x, -face.uv.y); break;
        case BlockFace_Back: face.uv = f32vec2(1.0, 0.0) + f32vec2(-face.uv.x, +face.uv.y); break;
        case BlockFace_Front: face.uv = f32vec2(1.0, 0.0) + f32vec2(-face.uv.x, +face.uv.y); break;
        case BlockFace_Cross_A:
            {
                if (face_id % 2 == 0) {
                    face.uv = f32vec2(1.0, 0.0) + f32vec2(-face.uv.x, face.uv.y);
                } else {
                    face.uv = f32vec2(0.0, 0.0) + f32vec2(+face.uv.x, face.uv.y);
                }
            }
            break;
        case BlockFace_Cross_B:
            {
                if (face_id % 2 == 0) {
                    face.uv = f32vec2(1.0, 0.0) + f32vec2(-face.uv.x, face.uv.y);
                } else {
                    face.uv = f32vec2(0.0, 0.0) + f32vec2(+face.uv.x, face.uv.y);
                }
            }
            break;
    }

    if (face.texture_id == TextureID_Grass_Top || face.texture_id == TextureID_Dirt || face.texture_id == BlockID_Cobblestone || face.texture_id == TextureID_Stone || face.texture_id == TextureID_Gravel) {
        u32 i = u32(rand(face.block_position) * 8);
        switch (i) {
            case 0: face.uv = f32vec2(0 + face.uv.x, 0 + face.uv.y); break;
            case 1: face.uv = f32vec2(1 - face.uv.x, 0 + face.uv.y); break;
            case 2: face.uv = f32vec2(1 - face.uv.x, 1 - face.uv.y); break;
            case 3: face.uv = f32vec2(0 + face.uv.x, 1 - face.uv.y); break;
            case 4: face.uv = f32vec2(0 + face.uv.y, 0 + face.uv.x); break;
            case 5: face.uv = f32vec2(1 - face.uv.y, 0 + face.uv.x); break;
            case 6: face.uv = f32vec2(1 - face.uv.y, 1 - face.uv.x); break;
            case 7: face.uv = f32vec2(0 + face.uv.y, 1 - face.uv.x); break;
        }
    }
}

u32 tile_texture_index(u32 block_id, u32 face) {
    switch(block_id) {
        case BlockID_Air: return TextureID_Air;
        case BlockID_Grass: 
            switch(face) {
                case BlockFace_Back:
                case BlockFace_Front:
                case BlockFace_Left:
                case BlockFace_Right: return TextureID_Grass_Side;
                case BlockFace_Bottom: return TextureID_Grass_Top;
                case BlockFace_Top: return TextureID_Dirt;
                default: return 0;
            }
        case BlockID_Dirt: return TextureID_Dirt;
        case BlockID_Stone: return TextureID_Stone;
        case BlockID_Cobblestone: return TextureID_Cobblestone;
        case BlockID_Gravel: return TextureID_Gravel;
        case BlockID_Sand: return TextureID_Sand;
        case BlockID_Water: return TextureID_Water;
        case BlockID_Tallgrass: return TextureID_Tallgrass;
        case BlockID_Rose: return TextureID_Rose;
        default: return 0;
    }
}

UnpackedFace get_vertex(u32 vert_i) {
    u32 data_index = vert_i / 6;
    u32 data_instance = vert_i - data_index * 6;
    u32 vert_data = deref(daxa_push_constant.face_buffer).data[data_index];
    UnpackedFace result;
    result.block_position = f32vec3((vert_data >> 0) & 0x1f, (vert_data >> 5) & 0x1f, (vert_data >> 10) & 0x1f);
    result.position = result.block_position;
    result.uv = instance_offsets[data_instance];
    result.block_id = (vert_data >> 18) & 0x3fff;
    result.block_face = (vert_data >> 15) & 0x7;
    result.vertex_id = data_instance;
    correct_pos(result, data_index);
    result.texture_id = tile_texture_index(result.block_id, result.block_face);
    correct_uv(result, data_index);
    return result;
}


#if defined(DRAW_VERT)

layout(location = 0) out f32vec3 v_normal;
layout(location = 1) out f32vec3 v_uv;

void main() {
    UnpackedFace vertex = get_vertex(gl_VertexIndex);

    gl_Position = daxa_push_constant.vp * f32vec4(vertex.position.xyz + daxa_push_constant.chunk_pos, 1);
    v_uv = f32vec3(vertex.uv, vertex.texture_id);
    v_normal = vertex.normal;
}

#elif defined(DRAW_FRAG)

layout(location = 0) in f32vec3 v_normal;
layout(location = 1) in f32vec3 v_uv;

layout(location = 0) out f32vec4 color;

void main() {
    f32vec4 tex_col = texture(daxa_push_constant.atlas_texture, daxa_push_constant.atlas_sampler, v_uv);
    f32vec3 col = tex_col.rgb;
    col *= max(dot(normalize(v_normal), normalize(f32vec3(4, 5, 2))) * 0.5 + 0.5, 0.0);

    if(tex_col.a < 0.1) { discard; }

    color = f32vec4(col, 1);
}

#endif