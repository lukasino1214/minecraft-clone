#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawPush)

#define VERTEX deref(daxa_push_constant.face_buffer[gl_VertexIndex])

#if defined(DRAW_VERT)

layout(location = 0) out f32vec3 v_color;
layout(location = 1) out f32vec3 v_tex_uv;

void main() {
    gl_Position = daxa_push_constant.vp * f32vec4(VERTEX.position.xyz + daxa_push_constant.chunk_pos, 1);
    v_color = VERTEX.color;
    v_tex_uv = f32vec3(VERTEX.uv, 0);
}

#elif defined(DRAW_FRAG)
layout(location = 0) out f32vec4 out_color;
layout(location = 0) in f32vec3 v_color;
layout(location = 1) in f32vec3 v_tex_uv;

void main() {
    f32vec3 col = texture(daxa_push_constant.atlas_texture, daxa_push_constant.atlas_sampler, v_tex_uv).xyz;
    out_color = vec4(col * v_color, 1.0);
}

#endif