//
// Created by lukas on 19.12.21.
//

#include "Chunk.h"

bool randomBool() {
    return 0 + (rand() % (1 - 0 + 1)) == 1;
}

float VOXEL_FACE_UP[] = {-0.5f,  0.5f, -0.5f,                   // +y up
                         0.5f,  0.5f, -0.5f,
                         0.5f,  0.5f,  0.5f,
                         0.5f,  0.5f,  0.5f,
                         -0.5f,  0.5f,  0.5f,
                         -0.5f,  0.5f, -0.5f}; //+y
float VOXEL_FACE_DOWN[] = {-0.5f, -0.5f, -0.5f,                      // -y down
                           0.5f, -0.5f, -0.5f,
                           0.5f, -0.5f,  0.5f,
                           0.5f, -0.5f,  0.5f,
                           -0.5f, -0.5f,  0.5f,
                           -0.5f, -0.5f, -0.5f}; //-y
float VOXEL_FACE_RIGHT[] = {-0.5f, -0.5f, -0.5f,                      //right -z
                            0.5f, -0.5f, -0.5f,
                            0.5f,  0.5f, -0.5f,
                            0.5f,  0.5f, -0.5f,
                            -0.5f,  0.5f, -0.5f,
                            -0.5f, -0.5f, -0.5f}; // -z
float VOXEL_FACE_LEFT[] = {-0.5f, -0.5f,  0.5f,                     // +z left
                           0.5f, -0.5f,  0.5f,
                           0.5f,  0.5f,  0.5f,
                           0.5f,  0.5f,  0.5f,
                           -0.5f,  0.5f,  0.5f,
                           -0.5f, -0.5f,  0.5f}; // +z
float VOXEL_FACE_FRONT[] = {0.5f,  0.5f,  0.5f,                               // +x front
                            0.5f,  0.5f, -0.5f,
                            0.5f, -0.5f, -0.5f,
                            0.5f, -0.5f, -0.5f,
                            0.5f, -0.5f,  0.5f,
                            0.5f,  0.5f,  0.5f}; // +x
float VOXEL_FACE_BACK[] = {-0.5f,  0.5f,  0.5f,                      // -x back
                           -0.5f,  0.5f, -0.5f,
                           -0.5f, -0.5f, -0.5f,
                           -0.5f, -0.5f, -0.5f,
                           -0.5f, -0.5f,  0.5f,
                           -0.5f,  0.5f,  0.5f}; // -x

Chunk::Chunk() {
    GenerateChunk();
    GenerateMesh();
}

void Chunk::GenerateChunk() {
    for(int x = 0; x <= 15; x++) {
        for(int y = 0; y <= 15; y++) {
            for(int z = 0; z <= 15; z++) {
                VoxelData[x][y][z] = randomBool();
            }
        }
    }
}

void Chunk::GenerateMesh() {

    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_UP, VOXEL_FACE_UP + 18);
    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_DOWN, VOXEL_FACE_DOWN + 18);
    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_BACK, VOXEL_FACE_BACK + 18);
    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_FRONT, VOXEL_FACE_FRONT + 18);
    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_RIGHT, VOXEL_FACE_RIGHT + 18);
    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_LEFT, VOXEL_FACE_LEFT + 18);
    for(int x = 0; x <= 15; x++) {
        for(int y = 0; y <= 15; y++) {
            for(int z = 0; z <= 15; z++) {
                /*if(!GetVoxel({x-1, y, z})) {
                    float VOXEL_FACE_BACK[] = {-0.5f,  0.5f,  0.5f-0.5f,  0.5f, -0.5f-0.5f, -0.5f, -0.5f-0.5f, -0.5f, -0.5f-0.5f, -0.5f,  0.5f-0.5f,  0.5f,  0.5f}; // -x
                    for(int i = 0; i < 6; i++) {

                        VOXEL_FACE_BACK[0 + 3*i] += 0.0f;
                        VOXEL_FACE_BACK[1 + 3*i] += 1.0f;
                        VOXEL_FACE_BACK[2 + 3*i] += 0.0f;
                    }
                    VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_BACK, VOXEL_FACE_BACK + 18);
                }*/
                /*if(!GetVoxel({x+1, y, z})) {
                    VoxelRenderData.push_back();
                }
                if(!GetVoxel({x, y-1, z})) {
                    VoxelRenderData.push_back();
                }
                if(!GetVoxel({x, y+1, z})) {
                    VoxelRenderData.push_back();
                }
                if(!GetVoxel({x, y, z-1})) {
                    VoxelRenderData.push_back();
                }
                if(!GetVoxel({x, y, z+1})) {
                    VoxelRenderData.push_back();
                }*/
                /*VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_UP, VOXEL_FACE_UP + 18);
                VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_DOWN, VOXEL_FACE_DOWN + 18);
                VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_BACK, VOXEL_FACE_BACK + 18);
                VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_FRONT, VOXEL_FACE_FRONT + 18);
                VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_RIGHT, VOXEL_FACE_RIGHT + 18);
                VoxelRenderData.insert(VoxelRenderData.end(), VOXEL_FACE_LEFT, VOXEL_FACE_LEFT + 18);*/
            }
        }
    }
}

bool Chunk::GetVoxel(const glm::ivec3 &Position) {
    return -1 < Position.x < 16 && -1 < Position.y < 16 && -1 < Position.z < 16 ? VoxelData[Position.x][Position.y][Position.z] : false;
}
