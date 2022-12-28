#pragma once
#include <daxa/daxa.hpp>
using namespace daxa::types;
#include "../shaders/shared.inl"

#include <FastNoise/FastNoise.h>
#include <glm/glm.hpp>
#include <array>

enum struct BlockType : u32 {
    Air,
    Solid,
};

struct BlockFace {
    u32 data = 0;

    BlockFace(const glm::ivec3& pos, u32 side, u32 texture_id) {
        data |= (static_cast<u32>(pos.x) & 0xf) << 0;
        data |= (static_cast<u32>(pos.y) & 0xf) << 4;
        data |= (static_cast<u32>(pos.z) & 0xf) << 8;
        data |= (side & 0x7) << 12;
        data |= (texture_id & 0xff) << 15;
    }
};

static constexpr inline u32 CUBE_VERTS = 36;
static constexpr inline u32 CUBE_VERTS_SIZE = CUBE_VERTS * sizeof(DrawVertex);
static constexpr inline u32 CHUNK_SIZE = 16;
static constexpr inline u32 CHUNK_VERTS = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * CUBE_VERTS;
static constexpr inline u32 CHUNK_VERTS_SIZE = CHUNK_VERTS * sizeof(DrawVertex);

struct Chunk {
    Chunk(daxa::Device& device, FastNoise::SmartNode<> generator, const glm::ivec3 pos) : device{device}, pos{pos} {
        std::vector<float> noiseOutput(16 * 16 * 16);
        generator->GenUniformGrid3D(noiseOutput.data(), 16 * pos.z, 16 * pos.y, 16 * pos.x, 16, 16, 16, 0.05f, 1337);

        int index = 0;
        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    if(noiseOutput[index++] <= 0.0f) {
                        voxel_data[x][y][z] = BlockType::Solid;

                    } else {
                        voxel_data[x][y][z] = BlockType::Air;
                    }
                }
            }
        }

        std::vector<DrawVertex> vertices = {};
        vertices.reserve(CHUNK_VERTS);

        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    if(voxel_data[x][y][z] != BlockType::Solid) { continue; }
                    f32 f_x = static_cast<f32>(x);
                    f32 f_y = static_cast<f32>(y);
                    f32 f_z = static_cast<f32>(z);

                    f32 r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    f32 g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    f32 b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

                    f32vec3 col = {r, g, b};
                    glm::ivec3 voxel_pos = { x, y, z };

                    std::vector<DrawVertex> block_verts = {};

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, -1 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                        };

                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, +1 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                        };
                        
                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ -1, 0, 0 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                        };

                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ +1, 0, 0 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                        };

                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, -1, 0 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x, -0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                        };

                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, +1, 0 }) == BlockType::Air) {
                        std::vector<DrawVertex> face = {
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 1.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{ 0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 1.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y,  0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 0.0f }},
                            DrawVertex {{-0.5f + f_x,  0.5f + f_y, -0.5f + f_z},      { col.x, col.y, col.z },        { 0.0f, 1.0f }},
                        };

                        block_verts.insert(block_verts.end(), face.begin(), face.end());
                    }

                    vertices.insert(vertices.end(), block_verts.begin(), block_verts.end());
                }
            }
        }

        std::cout << vertices.size() << std::endl;
        chunk_size = vertices.size();
        renderable = chunk_size != 0 ? true : false;

        this->face_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = CHUNK_VERTS_SIZE
        });


        daxa::BufferId staging_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = CHUNK_VERTS_SIZE
        });

        auto * buffer_ptr = device.get_host_address_as<DrawVertex>(staging_buffer);

        std::memcpy(buffer_ptr, vertices.data(), CHUNK_VERTS_SIZE);

        daxa::CommandList command_list = device.create_command_list({.debug_name = "my command list"});

        command_list.copy_buffer_to_buffer({
            .src_buffer = staging_buffer,
            .dst_buffer = face_buffer,
            .size = CHUNK_VERTS_SIZE
        });

        command_list.complete();
        device.submit_commands({
            .command_lists = {std::move(command_list)},
        });
        device.wait_idle();
        device.destroy_buffer(staging_buffer);
    }
    ~Chunk() {
        device.destroy_buffer(face_buffer);
    }

    void draw(daxa::CommandList& cmd_list, DrawPush& push) {
        if(this->renderable) {
            push.chunk_pos = { static_cast<f32>(pos.x * 16), static_cast<f32>(pos.y * 16), static_cast<f32>(pos.z * 16) };
            push.face_buffer = device.get_device_address(face_buffer);
            cmd_list.push_constant(push);
            cmd_list.draw({.vertex_count = chunk_size});
        }
    }

    auto get_voxel(const glm::ivec3& p) -> BlockType {
        if(p.x < 0 || p.x >= CHUNK_SIZE) {
            return BlockType::Air;
        }

        if(p.y < 0 || p.y >= CHUNK_SIZE) {
            return BlockType::Air;
        }

        if(p.z < 0 || p.z >= CHUNK_SIZE) {
            return BlockType::Air;
        }

        return voxel_data[p.x][p.y][p.z];
    }

    daxa::BufferId face_buffer;
    daxa::Device& device;
    glm::ivec3 pos;
    bool renderable = false;
    u32 chunk_size;
    std::array<std::array<std::array<BlockType, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> voxel_data;
};