//
// Created by lukas on 19.12.21.
//

#ifndef MINECRAFT_CLONE_CHUNK_H
#define MINECRAFT_CLONE_CHUNK_H

#include <vector>

#include <glm/glm.hpp>

class Chunk {
public:
    Chunk();

    bool GetVoxel(const glm::ivec3 &Position);
    auto GetMesh() { return VoxelRenderData; }
private:
    void GenerateChunk();
    void GenerateMesh();

    bool VoxelData[16][16][16] = {};
    std::vector<float> VoxelRenderData;
};


#endif //MINECRAFT_CLONE_CHUNK_H
