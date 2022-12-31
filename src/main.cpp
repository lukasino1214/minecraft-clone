#include "window.hpp"

#include <thread>
#include <iostream>
#include <cmath>
#include <daxa/utils/pipeline_manager.hpp>
#define GLM_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <FastNoise/FastNoise.h>

#define APPNAME "Daxa Template App"
#define APPNAME_PREFIX(x) ("[" APPNAME "] " x)

#define DAXA_SHADERLANG DAXA_SHADERLANG_GLSL

#include <daxa/utils/imgui.hpp>
#include <imgui_impl_glfw.h>

using namespace daxa::types;
#include "../shaders/shared.inl"

using Clock = std::chrono::high_resolution_clock;

#include "camera.hpp"
#include <cstring>
#include "chunk.hpp"
#include "textures.hpp"

struct App : AppWindow<App> {
    daxa::Context daxa_ctx = daxa::create_context({
        .enable_validation = true,
    });

    daxa::Device device = daxa_ctx.create_device({
        .debug_name = APPNAME_PREFIX("device"),
    });

    daxa::Swapchain swapchain = device.create_swapchain({
        .native_window = get_native_handle(glfw_window_ptr),
        .native_window_platform = get_native_platform(),
        .present_mode = daxa::PresentMode::DO_NOT_WAIT_FOR_VBLANK,
        .image_usage = daxa::ImageUsageFlagBits::TRANSFER_DST,
        .debug_name = APPNAME_PREFIX("swapchain"),
    });

    daxa::PipelineManager pipeline_manager = daxa::PipelineManager({
        .device = device,
        .shader_compile_options = {
            .root_paths = {
                "/shaders",
                "../shaders",
                "../../shaders",
                "../../../shaders",
                "shaders",
                DAXA_SHADER_INCLUDE_DIR,
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .debug_name = APPNAME_PREFIX("pipeline_manager"),
    });

    daxa::ImGuiRenderer imgui_renderer = create_imgui_renderer();
    auto create_imgui_renderer() -> daxa::ImGuiRenderer
    {
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(glfw_window_ptr, true);
        return daxa::ImGuiRenderer({
            .device = device,
            .format = swapchain.get_format(),
        });
    }

    Clock::time_point start = Clock::now(), prev_time = start;
    f32 elapsed_s = 1.0f;

    daxa::ImageId swapchain_image;

    std::shared_ptr<daxa::RasterPipeline> raster_pipeline = pipeline_manager.add_raster_pipeline({
        .vertex_shader_info = {.source = daxa::ShaderFile{"draw.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_VERT"}}}},
        .fragment_shader_info = {.source = daxa::ShaderFile{"draw.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"DRAW_FRAG"}}}},
        .color_attachments = {{.format = swapchain.get_format()}},
        .depth_test = {
            .depth_attachment_format = daxa::Format::D24_UNORM_S8_UINT,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .raster = {
            .face_culling = daxa::FaceCullFlagBits::NONE
        },
        .push_constant_size = sizeof(DrawPush),
        .debug_name = APPNAME_PREFIX("raster_pipeline"),
    }).value();

    ControlledCamera3D camera;

    daxa::ImageId depth_image = device.create_image({
        .dimensions = 2,
        .format = daxa::Format::D24_UNORM_S8_UINT,
        .aspect = daxa::ImageAspectFlagBits::DEPTH | daxa::ImageAspectFlagBits::STENCIL,
        .size = { size_x, size_y, 1 },
        .mip_level_count = 1,
        .array_layer_count = 1,
        .sample_count = 1,
        .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY
    });

    bool paused = true;

    std::unique_ptr<Textures> textures = std::make_unique<Textures>(device);
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunks;

    App() : AppWindow<App>("Minecraft Clone To Bully Shit Out Of Meerkat 😈") {}
    ~App() {
        device.wait_idle();
        device.collect_garbage();
        ImGui_ImplGlfw_Shutdown();
        device.destroy_image(depth_image);
    }

    bool update() {
        glfwPollEvents();
        if (glfwWindowShouldClose(glfw_window_ptr)) {
            return true;
        }

        if (!minimized) {
            on_update();
        } else {
            using namespace std::literals;
            std::this_thread::sleep_for(1ms);
        }

        return false;
    }

    void on_update() {
        auto now = Clock::now();
        elapsed_s = std::chrono::duration<f32>(now - prev_time).count();
        prev_time = now;

        ui_update();

        camera.camera.set_pos(camera.pos);
        camera.camera.set_rot(camera.rot.x, camera.rot.y);
        camera.update(elapsed_s);

        swapchain_image = swapchain.acquire_next_image();
        if (swapchain_image.is_empty())
            return;
        
        daxa::CommandList command_list = device.create_command_list({.debug_name = "my command list"});

        command_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::COLOR_ATTACHMENT_OUTPUT_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_id = this->swapchain_image,
        });

        command_list.pipeline_barrier_image_transition({
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::DEPTH | daxa::ImageAspectFlagBits::STENCIL},
            .image_id = this->depth_image,
        });


        command_list.begin_renderpass({
            .color_attachments = {{
                .image_view = this->swapchain_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = std::array<f32, 4>{0.2f, 0.4f, 1.0f, 1.0f},
            }},
            .depth_attachment = {{
                .image_view = this->depth_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = daxa::DepthValue{1.0f, 0},
            }},
            .render_area = {.x = 0, .y = 0, .width = size_x, .height = size_y},
        });

        command_list.set_pipeline(*raster_pipeline);

        glm::mat4 vp = camera.camera.get_vp();

        DrawPush push;
        push.vp = *reinterpret_cast<const f32mat4x4*>(&vp);
        push.atlas_texture = textures->atlas_texture_array.default_view();
        push.atlas_sampler = textures->atlas_sampler;

        for(auto& [pos, chunk] : chunks) {
            chunk->draw(command_list, push);
        }

        command_list.end_renderpass();
        imgui_renderer.record_commands(ImGui::GetDrawData(), command_list, swapchain_image, size_x, size_y);

        command_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::ALL_GRAPHICS_READ_WRITE,
            .before_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .after_layout = daxa::ImageLayout::PRESENT_SRC,
            .image_id = swapchain_image,
        });


        command_list.complete();

        device.submit_commands({
            .command_lists = {std::move(command_list)},
            .wait_binary_semaphores = {swapchain.get_acquire_semaphore()},
            .signal_binary_semaphores = {swapchain.get_present_semaphore()},
            .signal_timeline_semaphores = {{swapchain.get_gpu_timeline_semaphore(), swapchain.get_cpu_timeline_value()}},
        });

        device.present_frame({
            .wait_binary_semaphores = {swapchain.get_present_semaphore()},
            .swapchain = swapchain,
        });
    }

    auto to_chunk_position(const glm::ivec3& position) -> glm::ivec3 {
        return glm::ivec3{
            position.x <= 0 ? -position.x / CHUNK_SIZE : -position.x / CHUNK_SIZE - 1,
            position.y <= 0 ? -position.y / CHUNK_SIZE : -position.y / CHUNK_SIZE - 1, 
            position.z <= 0 ? -position.z / CHUNK_SIZE : -position.z / CHUNK_SIZE - 1
        };
    }

    auto to_chunk_position(const glm::vec3& position) -> glm::ivec3 {
        i32 x = static_cast<i32>(position.x);        
        i32 y = static_cast<i32>(position.y);
        i32 z = static_cast<i32>(position.z);        

        return to_chunk_position(glm::ivec3{x, y, z});
    }

    auto to_local_voxel_postion(const glm::vec3& position) -> glm::ivec3 {
        i32 x = static_cast<i32>(position.x);        
        i32 y = static_cast<i32>(position.y);
        i32 z = static_cast<i32>(position.z);        

        return to_local_voxel_postion(glm::ivec3{x, y, z});
    }

    auto to_local_voxel_postion(const glm::ivec3& position) -> glm::ivec3 {
        return glm::abs(glm::ivec3{ 
            position.x <= 0 ? position.x % CHUNK_SIZE : (CHUNK_SIZE - 1) - position.x % CHUNK_SIZE, 
            position.y <= 0 ? position.y % CHUNK_SIZE : (CHUNK_SIZE - 1) - position.y % CHUNK_SIZE, 
            position.z <= 0 ? position.z % CHUNK_SIZE : (CHUNK_SIZE - 1) - position.z % CHUNK_SIZE 
        });
    }

    void print_pos(const std::string& str, const glm::ivec3& pos) {
        std::cout << str << " x: " << pos.x << " y: " << pos.y << " z: " << pos.z << std::endl;
    }

    void print_pos(const std::string& str, const glm::vec3& pos) {
        std::cout << str << " x: " << pos.x << " y: " << pos.y << " z: " << pos.z << std::endl;
    }

    // position, rotation, range
    auto raycast_voxels(const glm::vec3& start_point, const glm::vec3& direction, f32 range) -> std::vector<glm::ivec3> {
        glm::vec3 normalized_direction = glm::normalize(direction);
        glm::vec3 end_point = start_point + direction * range;
        print_pos("end_point", end_point);
        glm::ivec3 start_voxel = glm::ivec3{
            static_cast<i32>(start_point.x), 
            static_cast<i32>(start_point.y), 
            static_cast<i32>(start_point.z)
        };

        i32 stepX = (normalized_direction.x > 0) ? 1 : ((normalized_direction.x < 0) ? -1 : 0);
        i32 stepY = (normalized_direction.y > 0) ? 1 : ((normalized_direction.y < 0) ? -1 : 0);
        i32 stepZ = (normalized_direction.z > 0) ? 1 : ((normalized_direction.z < 0) ? -1 : 0);

        float tDeltaX = (stepX != 0) ? fmin(stepX / (end_point.x - start_point.x), FLT_MAX) : FLT_MAX;
        float tDeltaY = (stepY != 0) ? fmin(stepY / (end_point.y - start_point.y), FLT_MAX) : FLT_MAX;
        float tDeltaZ = (stepZ != 0) ? fmin(stepZ / (end_point.z - start_point.z), FLT_MAX) : FLT_MAX;

        float tMaxX = (stepX > 0.0f) ? tDeltaX * (1.0f - start_point.x + start_voxel.x) : tDeltaX * (start_point.x - start_voxel.x);
        float tMaxY = (stepY > 0.0f) ? tDeltaY * (1.0f - start_point.y + start_voxel.y) : tDeltaY * (start_point.y - start_voxel.y);
        float tMaxZ = (stepZ > 0.0f) ? tDeltaZ * (1.0f - start_point.z + start_voxel.z) : tDeltaZ * (start_point.z - start_voxel.z);
        
        glm::ivec3 currentVoxel = start_voxel;
        std::vector<glm::ivec3> intersected;
        intersected.push_back(start_voxel);

        // sanity check to prevent leak
        while (intersected.size() < range * 6) {
            if (tMaxX < tMaxY) {
                if (tMaxX < tMaxZ) {
                    currentVoxel.x += stepX;
                    tMaxX += tDeltaX;
                }
                else {
                    currentVoxel.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
            else {
                if (tMaxY < tMaxZ) {
                    currentVoxel.y += stepY;
                    tMaxY += tDeltaY;
                }
                else {
                    currentVoxel.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
            if (tMaxX > 1 && tMaxY > 1 && tMaxZ > 1)
                break;
            intersected.push_back(currentVoxel);
        }
        return intersected;
    }

    void on_mouse_button(i32 key, i32 action) {
        glm::vec3 camera_pos = camera.pos;
        glm::ivec3 chunk_pos = to_chunk_position(camera_pos);
        glm::ivec3 block_pos = to_local_voxel_postion(camera_pos);

        //if(!paused) {
            if(key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                print_pos("start point: ", camera_pos);
                std::vector<glm::ivec3> voxels = raycast_voxels(camera_pos, camera.camera.forward_vector(), 8.0f);
                for(auto& pos : voxels) {
                    print_pos("voxel: ", pos);
                    chunk_pos = to_chunk_position(pos);
                    block_pos = to_local_voxel_postion(pos);

                    if(chunks.find(chunk_pos) != chunks.end()) {
                        auto& chunk = chunks.at(chunk_pos);

                        if(chunk->voxel_data[block_pos.x][block_pos.y][block_pos.z] != BlockID::Air) {
                            chunk->voxel_data[block_pos.x][block_pos.y][block_pos.z] = BlockID::Air;

                            Chunk::ChunkNeighbours neighbours {
                                .nx = chunks.find(chunk_pos + glm::ivec3{ -1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ -1, 0, 0 }).get() : nullptr,
                                .px = chunks.find(chunk_pos + glm::ivec3{ +1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ +1, 0, 0 }).get() : nullptr,
                                .ny = chunks.find(chunk_pos + glm::ivec3{ 0, -1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, -1, 0 }).get() : nullptr,
                                .py = chunks.find(chunk_pos + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, +1, 0 }).get() : nullptr,
                                .nz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, -1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, -1 }).get() : nullptr,
                                .pz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, +1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, +1 }).get() : nullptr,
                            };

                            chunk->generate_mesh(neighbours);
                            return;
                        }
                    }
                }
            }

            if(key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                print_pos("start point: ", camera_pos);

                if(chunks.find(chunk_pos) != chunks.end()) {
                    auto& chunk = chunks.at(chunk_pos);
                    std::cout << "place block" << std::endl;

                    chunk->voxel_data[block_pos.x][block_pos.y][block_pos.z] = BlockID::Sand;

                    Chunk::ChunkNeighbours neighbours {
                        .nx = chunks.find(chunk_pos + glm::ivec3{ -1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ -1, 0, 0 }).get() : nullptr,
                        .px = chunks.find(chunk_pos + glm::ivec3{ +1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ +1, 0, 0 }).get() : nullptr,
                        .ny = chunks.find(chunk_pos + glm::ivec3{ 0, -1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, -1, 0 }).get() : nullptr,
                        .py = chunks.find(chunk_pos + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, +1, 0 }).get() : nullptr,
                        .nz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, -1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, -1 }).get() : nullptr,
                        .pz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, +1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, +1 }).get() : nullptr,
                    };

                    chunk->generate_mesh(neighbours);
                }
            }
        //}
    }

    void on_mouse_move(f32 x, f32 y) {
        if (!paused) {
            f32 center_x = static_cast<f32>(size_x / 2);
            f32 center_y = static_cast<f32>(size_y / 2);
            auto offset = f32vec2{x - center_x, center_y - y};
            camera.on_mouse_move(offset.x, offset.y);
            glfwSetCursorPos(glfw_window_ptr, static_cast<f64>(center_x), static_cast<f64>(center_y));
        }
    }

    void on_key(i32 key, i32 action) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            toggle_pause();
        }

        if (!paused) {
            camera.on_key(key, action);
        }
    }

    void on_resize(u32 sx, u32 sy) {
        minimized = (sx == 0 || sy == 0);
        if (!minimized)
        {
            swapchain.resize();
            size_x = swapchain.get_surface_extent().x;
            size_y = swapchain.get_surface_extent().y;

            device.destroy_image(depth_image);
            depth_image = device.create_image({
                .dimensions = 2,
                .format = daxa::Format::D24_UNORM_S8_UINT,
                .aspect = daxa::ImageAspectFlagBits::DEPTH | daxa::ImageAspectFlagBits::STENCIL,
                .size = { size_x, size_y, 1 },
                .mip_level_count = 1,
                .array_layer_count = 1,
                .sample_count = 1,
                .usage = daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
                .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY
            });

            camera.camera.resize(size_x, size_y);

            on_update();
        }
    }

    void toggle_pause() {
        glfwSetCursorPos(glfw_window_ptr, static_cast<f64>(size_x / 2), static_cast<f64>(size_y / 2));
        glfwSetInputMode(glfw_window_ptr, GLFW_CURSOR, paused ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        glfwSetInputMode(glfw_window_ptr, GLFW_RAW_MOUSE_MOTION, paused);
        paused = !paused;
    }

    void ui_update() {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Test");
        ImGui::Text("%f", 1.0f / elapsed_s);
        ImGui::End();
        ImGui::Render();
    }

    void setup() {
        auto OpenSimplex = FastNoise::New<FastNoise::OpenSimplex2>();
        auto FractalFBm = FastNoise::New<FastNoise::FractalFBm>();
        FractalFBm->SetSource(OpenSimplex);
        FractalFBm->SetGain(0.280f);
        FractalFBm->SetOctaveCount(4);
        FractalFBm->SetLacunarity(4.0f);
        auto DomainScale = FastNoise::New<FastNoise::DomainScale>();
        DomainScale->SetSource(FractalFBm);
        DomainScale->SetScale(0.86f);
        auto PosationOutput = FastNoise::New<FastNoise::PositionOutput>();
        PosationOutput->Set<FastNoise::Dim::Y>(6.72f);
        auto add = FastNoise::New<FastNoise::Add>();
        add->SetLHS(DomainScale);
        add->SetRHS(PosationOutput);

        static constexpr i32 world_size_x = 1;
        static constexpr i32 world_size_y = 1;
        static constexpr i32 world_size_z = 1;

        u32 chunk_amount = 0;
    
        for(i32 y = world_size_y; y >= -world_size_y; y--) {
            for(i32 x = -world_size_x; x <= world_size_x; x++) {
                for(i32 z = -world_size_z; z <= world_size_z; z++) {
                    chunk_amount++;
                    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(device, glm::ivec3{x, y, z});
                    Chunk* upper_chunk = chunks.find(glm::ivec3{x, y, z} + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(glm::ivec3{x, y, z} + glm::ivec3{ 0, +1, 0 }).get() : nullptr;
                    chunk->generate_terrain(add, upper_chunk);
                    this->chunks.insert(std::make_pair(glm::ivec3{x, y, z}, std::move(chunk)));
                }
            }
        }

        u32 vertices = 0;

        for(i32 x = -world_size_x; x <= world_size_x; x++) {
            for(i32 y = -world_size_y; y <= world_size_y; y++) {
                for(i32 z = -world_size_z; z <= world_size_z; z++) {
                    glm::ivec3 chunk_pos = {x, y, z};
                    if(chunks.find(chunk_pos) != chunks.end()) {
                        auto& chunk = chunks.at(chunk_pos);

                        Chunk::ChunkNeighbours neighbours {
                            .nx = chunks.find(chunk_pos + glm::ivec3{ -1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ -1, 0, 0 }).get() : nullptr,
                            .px = chunks.find(chunk_pos + glm::ivec3{ +1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ +1, 0, 0 }).get() : nullptr,
                            .ny = chunks.find(chunk_pos + glm::ivec3{ 0, -1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, -1, 0 }).get() : nullptr,
                            .py = chunks.find(chunk_pos + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, +1, 0 }).get() : nullptr,
                            .nz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, -1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, -1 }).get() : nullptr,
                            .pz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, +1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, +1 }).get() : nullptr,
                        };

                        chunk->generate_mesh(neighbours);
                        vertices += chunk->chunk_size;
                    }
                }
            }
        }

        std::cout << "amount of chunks: " << chunk_amount << std::endl;
        std::cout << "amount of vertices: " << vertices << std::endl;
        std::cout << "done!" << std::endl;

        /*for(i32 y = world_size_y; y >= -world_size_y; y--) {
            std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(device, glm::ivec3{0, y, 0});
            Chunk* upper_chunk = chunks.find(glm::ivec3{0, y, 0} + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(glm::ivec3{0, y, 0} + glm::ivec3{ 0, +1, 0 }).get() : nullptr;
            chunk->generate_terrain(add, upper_chunk);
            this->chunks.insert(std::make_pair(glm::ivec3{0, y, 0}, std::move(chunk)));
        }

        u32 vertices = 0;

        for(i32 y = -world_size_y; y <= world_size_y; y++) {
            glm::ivec3 chunk_pos = {0, y, 0};
            if(chunks.find(chunk_pos) != chunks.end()) {
                auto& chunk = chunks.at(chunk_pos);

                Chunk::ChunkNeighbours neighbours {
                    .nx = chunks.find(chunk_pos + glm::ivec3{ -1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ -1, 0, 0 }).get() : nullptr,
                    .px = chunks.find(chunk_pos + glm::ivec3{ +1, 0, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ +1, 0, 0 }).get() : nullptr,
                    .ny = chunks.find(chunk_pos + glm::ivec3{ 0, -1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, -1, 0 }).get() : nullptr,
                    .py = chunks.find(chunk_pos + glm::ivec3{ 0, +1, 0 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, +1, 0 }).get() : nullptr,
                    .nz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, -1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, -1 }).get() : nullptr,
                    .pz = chunks.find(chunk_pos + glm::ivec3{ 0, 0, +1 }) != chunks.end() ? chunks.at(chunk_pos + glm::ivec3{ 0, 0, +1 }).get() : nullptr,
                };

                chunk->generate_mesh(neighbours);
                vertices += chunk->chunk_size;
            }
        }*/

        camera.camera.resize(size_x, size_y);
    }
};

int main() {
    App app = {};
    app.setup();
    while (true) {
        if (app.update())
            break;
    }
}