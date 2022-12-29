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

static constexpr inline u32 CUBE_VERTS = 6;
static constexpr inline u32 CUBE_VERTS_SIZE = CUBE_VERTS * sizeof(BlockFace);
static constexpr inline u32 CHUNK_SIZE = 16;
static constexpr inline u32 CHUNK_VERTS = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * CUBE_VERTS;
static constexpr inline u32 CHUNK_VERTS_SIZE = CHUNK_VERTS * sizeof(BlockFace);

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

        this->face_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = CHUNK_VERTS_SIZE
        });


        daxa::BufferId staging_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = CHUNK_VERTS_SIZE
        });

        auto * buffer_ptr = device.get_host_address_as<BlockFace>(staging_buffer);

        u32 texture_id = 0;

        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    if(voxel_data[x][y][z] != BlockType::Solid) { continue; }

                    glm::ivec3 voxel_pos = { x, y, z };


                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, -1 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 5, texture_id), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, +1 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 4, texture_id), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ -1, 0, 0 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 1, texture_id), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ +1, 0, 0 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 0, texture_id), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, -1, 0 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 3, texture_id), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, +1, 0 }) == BlockType::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 2, texture_id), buffer_ptr++, chunk_size++;
                    }
                }
            }
        }

        std::cout << chunk_size << std::endl;
        renderable = chunk_size != 0 ? true : false;

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
            cmd_list.draw({.vertex_count = chunk_size * 6});
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
    u32 chunk_size = 0;
    std::array<std::array<std::array<BlockType, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> voxel_data;
};