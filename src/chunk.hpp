#pragma once
#include <daxa/daxa.hpp>
using namespace daxa::types;
#include "../shaders/shared.inl"

#include <FastNoise/FastNoise.h>
#include <glm/glm.hpp>
#include <array>

enum struct BlockID : u32 {
    Air,
    Grass,
    Dirt,
    Stone,
    Cobblestone,
    Gravel,
    Sand,
    Water,
    TallGrass,
    Rose
};

struct Block {
    BlockID id = BlockID::Air;

    auto is_occluding() const -> bool {
        switch (this->id) {
            case BlockID::Air:
            case BlockID::Rose:
            case BlockID::TallGrass:
            case BlockID::Water: return false;
            default: return true;
        }
    }
    auto is_cross() const -> bool {
        switch (this->id) {
            case BlockID::Rose:
            case BlockID::TallGrass: return true;
            default: return false;
        }
    }
};

struct BlockFace {
    u32 data = 0;

    BlockFace(const glm::ivec3& pos, u32 side, u32 block_id) {
        data |= (static_cast<u32>(pos.x) & 0x1f) << 0;
        data |= (static_cast<u32>(pos.y) & 0x1f) << 5;
        data |= (static_cast<u32>(pos.z) & 0x1f) << 10;
        data |= (side & 0x7) << 15;
        data |= (static_cast<u32>(block_id) & 0x3fff) << 18;
    }
};

#define CUBE_VERTS 6
#define CUBE_VERTS_SIZE CUBE_VERTS * sizeof(BlockFace)
#define CHUNK_SIZE 32
#define CHUNK_VERTS CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * CUBE_VERTS
#define CHUNK_VERTS_SIZE CHUNK_VERTS * sizeof(BlockFace)

struct Chunk {
    struct ChunkNeighbours {
        Chunk* nx = nullptr;
        Chunk* px = nullptr;
        Chunk* ny = nullptr;
        Chunk* py = nullptr;
        Chunk* nz = nullptr;
        Chunk* pz = nullptr;
    };

    Chunk(daxa::Device& device, const glm::ivec3 pos) : device{device}, pos{pos} {
        this->face_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
            .size = CHUNK_VERTS_SIZE
        });

        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    voxel_data[x][y][z] = BlockID::Air;
                }
            }
        }
    }
    ~Chunk() {
        device.destroy_buffer(face_buffer);
    }

    void generate_terrain(const FastNoise::SmartNode<>& generator, Chunk* upper_chunk = nullptr) {
        std::vector<f32> noiseOutput(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
        generator->GenUniformGrid3D(noiseOutput.data(), CHUNK_SIZE * pos.z, CHUNK_SIZE * pos.y, CHUNK_SIZE * pos.x, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 0.05f, 1337);

        int index = 0;
        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    if(noiseOutput[index++] <= 0.0f) {
                        voxel_data[x][y][z] = BlockID::Stone;

                    } else {
                        voxel_data[x][y][z] = BlockID::Air;
                    }
                }
            }
        }

        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 z = 0; z < CHUNK_SIZE; z++) {
                for(i32 y = CHUNK_SIZE-1; y >= 0; y--) {
                    if (get_voxel(glm::ivec3{x, y, z}, { .py = upper_chunk }) == BlockID::Stone) {
                        u32 above_i;
                        for (above_i = 0; above_i < 6; ++above_i) {
                            if (get_voxel(glm::ivec3{x, y + above_i + 1, z}, { .py = upper_chunk }) == BlockID::Air)
                                break;
                        }
                        switch (rand() % 8)
                        {
                        case 0: voxel_data[x][y][z] = BlockID::Gravel; break;
                        case 1: voxel_data[x][y][z] = BlockID::Cobblestone; break;
                        default: break;
                        }
                        if (above_i == 0)
                            voxel_data[x][y][z] = BlockID::Grass;
                        else if (above_i < 4)
                            voxel_data[x][y][z] = BlockID::Dirt;
                    } else if (get_voxel(glm::ivec3{x, y, z}, { .py = upper_chunk }) == BlockID::Air) {
                        u32 below_i;
                        for (below_i = 0; below_i < 6; ++below_i)
                        {
                            if (get_voxel(glm::ivec3{x, y - (below_i + 1), z}, { .py = upper_chunk }) == BlockID::Stone)
                                break;
                        }
                        if (below_i == 0)
                        {
                            i32 r = rand() % 100;
                            if (r < 50)
                            {
                                switch (r)
                                {
                                case 0: voxel_data[x][y][z] = BlockID::Rose; break;
                                default:
                                    voxel_data[x][y][z] = BlockID::TallGrass;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void generate_mesh(const ChunkNeighbours& neighbors) {
        chunk_size = 0;
        renderable = false;
        daxa::BufferId staging_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = CHUNK_VERTS_SIZE
        });

        auto* buffer_ptr = device.get_host_address_as<BlockFace>(staging_buffer);

        for(u32 x = 0; x < CHUNK_SIZE; x++) {
            for(u32 y = 0; y < CHUNK_SIZE; y++) {
                for(u32 z = 0; z < CHUNK_SIZE; z++) {
                    if(voxel_data[x][y][z] == BlockID::Air) { continue; }

                    glm::ivec3 voxel_pos = { x, y, z };


                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, -1 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 5, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, 0, +1 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 4, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ -1, 0, 0 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 1, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ +1, 0, 0 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 0, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, -1, 0 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 3, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }

                    if(get_voxel(voxel_pos + glm::ivec3{ 0, +1, 0 }, neighbors) == BlockID::Air) {
                        *buffer_ptr = BlockFace(voxel_pos, 2, static_cast<u32>(voxel_data[x][y][z])), buffer_ptr++, chunk_size++;
                    }
                }
            }
        }

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

    void draw(daxa::CommandList& cmd_list, DrawPush& push) {
        if(this->renderable) {
            push.chunk_pos = { static_cast<f32>(pos.x * CHUNK_SIZE), static_cast<f32>(pos.y * CHUNK_SIZE), static_cast<f32>(pos.z * CHUNK_SIZE) };
            push.face_buffer = device.get_device_address(face_buffer);
            cmd_list.push_constant(push);
            cmd_list.draw({.vertex_count = chunk_size * 6});
        }
    }

    auto get_voxel(const glm::ivec3& p, const ChunkNeighbours& neighbours) -> BlockID {
        if(p.x < 0) {
            if(neighbours.nx == nullptr) { return BlockID::Air; }
            return neighbours.nx->voxel_data[CHUNK_SIZE+p.x][p.y][p.z];
        }

        if(p.x >= CHUNK_SIZE) {
            if(neighbours.px == nullptr) { return BlockID::Air; }
            return neighbours.px->voxel_data[CHUNK_SIZE-p.x][p.y][p.z];
        }

        if(p.y < 0) {
            if(neighbours.ny == nullptr) { return BlockID::Air; }
            return neighbours.ny->voxel_data[p.x][CHUNK_SIZE+p.y][p.z];
        }

        if(p.y >= CHUNK_SIZE) {
            if(neighbours.py == nullptr) { return BlockID::Air; }
            return neighbours.py->voxel_data[p.x][CHUNK_SIZE-p.y][p.z];
        }

        if(p.z < 0) {
            if(neighbours.nz == nullptr) { return BlockID::Air; }
            return neighbours.nz->voxel_data[p.x][p.y][CHUNK_SIZE+p.z];
        }

        if(p.z >= CHUNK_SIZE) {
            if(neighbours.pz == nullptr) { return BlockID::Air; }
            return neighbours.pz->voxel_data[p.x][p.y][CHUNK_SIZE-p.z];
        }

        return voxel_data[p.x][p.y][p.z];
    }

    void set_voxel(const glm::ivec3& p, BlockID block_id, const ChunkNeighbours& neighbours) {
        if(p.x < 0) {
            if(neighbours.nx == nullptr) { return; }
            std::cout << "nx "  << p.x << std::endl;
            neighbours.nx->voxel_data[CHUNK_SIZE+p.x][p.y][p.z] = block_id;
            return;
        }

        if(p.x >= CHUNK_SIZE) {
            if(neighbours.px == nullptr) { return; }
            std::cout << "px "  << p.x << std::endl;
            neighbours.px->voxel_data[CHUNK_SIZE-p.x][p.y][p.z] = block_id;
            return;
        }

        if(p.y < 0) {
            if(neighbours.ny == nullptr) { return; }
            std::cout << "ny " << p.y << std::endl;
            neighbours.ny->voxel_data[p.x][CHUNK_SIZE+p.y][p.z] = block_id;
            return;
        }

        if(p.y >= CHUNK_SIZE) {
            if(neighbours.py == nullptr) { return; }
            std::cout << "py "  << p.y << std::endl;
            neighbours.py->voxel_data[p.x][CHUNK_SIZE-p.y][p.z] = block_id;
            return;
        }

        if(p.z < 0) {
            if(neighbours.nz == nullptr) { return; }
            std::cout << "nz "  << p.z << std::endl;
            neighbours.nz->voxel_data[p.x][p.y][CHUNK_SIZE+p.z] = block_id;
            return;
        }

        if(p.z >= CHUNK_SIZE) {
            if(neighbours.pz == nullptr) { return; }
            std::cout << "pz "  << p.z << std::endl;
            neighbours.pz->voxel_data[p.x][p.y][CHUNK_SIZE-p.z] = block_id;
            return;
        }

        voxel_data[p.x][p.y][p.z] = block_id;
    }

    daxa::BufferId face_buffer;
    daxa::Device device;
    glm::ivec3 pos;
    bool renderable = false;
    u32 chunk_size = 0;
    std::array<std::array<std::array<BlockID, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> voxel_data;
};