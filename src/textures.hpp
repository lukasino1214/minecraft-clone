#pragma once

#include <daxa/daxa.hpp>
using namespace daxa::types;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static const std::vector<std::string> texture_names = {
    "air",
    "grass-top",
    "grass-side",
    "dirt",
    "stone",
    "cobblestone",
    "gravel",
    "sand",
    "water",
    "tallgrass",
    "rose"
};

struct Textures {
    Textures(daxa::Device& device) : device{device} {
        this->atlas_texture_array = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .size = {16, 16, 1},
            .mip_level_count = 4,
            .array_layer_count = static_cast<u32>(texture_names.size()),
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_SRC | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = APPNAME_PREFIX("atlas_texture_array"),
        });

        this->atlas_sampler = device.create_sampler({
            .magnification_filter = daxa::Filter::NEAREST,
            .minification_filter = daxa::Filter::LINEAR,
            .min_lod = 0,
            .max_lod = 0,
            .debug_name = APPNAME_PREFIX("atlas_sampler"),
        });

        daxa::BufferId staging_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(texture_names.size() * 16 * 16 * 4 * sizeof(u8)),
        });

        auto cmd_list = device.create_command_list({});

        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_READ,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_slice = {
                .base_mip_level = 0,
                .level_count = 4,
                .base_array_layer = 0,
                .layer_count = static_cast<u32>(texture_names.size())
            },
            .image_id = atlas_texture_array,
        });

        for(u32 i = 0; i < texture_names.size(); i++) {
            stbi_set_flip_vertically_on_load(1);
            std::string path = "textures/" + texture_names[i] + ".png";

            i32 size_x = 0;
            i32 size_y = 0;
            i32 num_channels = 0;
            u8* data = stbi_load(path.c_str(), &size_x, &size_y, &num_channels, 4);
            if(data == nullptr) {
                throw std::runtime_error("Textures couldn't be found");
            }

            usize offset = i * 16 * 16 * 4;

            auto staging_buffer_ptr = device.get_host_address_as<u8>(staging_buffer);
            for (usize j = 0; j < 16 * 16; j++) {
                usize const data_i = j * 4;
                staging_buffer_ptr[offset + data_i + 0] = data[data_i + 0];
                staging_buffer_ptr[offset + data_i + 1] = data[data_i + 1];
                staging_buffer_ptr[offset + data_i + 2] = data[data_i + 2];
                staging_buffer_ptr[offset + data_i + 3] = data[data_i + 3];
            }
            cmd_list.copy_buffer_to_image({
                .buffer = staging_buffer,
                .buffer_offset = i * 16 * 16 * 4 * sizeof(u8),
                .image = atlas_texture_array,
                .image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .image_slice = {
                    .mip_level = 0,
                    .base_array_layer = static_cast<u32>(i),
                    .layer_count = 1,
                },
                .image_offset = {0, 0, 0},
                .image_extent = {16, 16, 1},
            });
        }

        // mipmapping

        auto image_info = device.info_image(atlas_texture_array);

        std::array<i32, 3> mip_size = {
            static_cast<i32>(16),
            static_cast<i32>(16),
            static_cast<i32>(1),
        };
        for (u32 i = 0; i < 4 - 1; ++i) {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .after_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = static_cast<u32>(texture_names.size()),
                },
                .image_id = atlas_texture_array,
            });
            cmd_list.pipeline_barrier_image_transition({
                .waiting_pipeline_access = daxa::AccessConsts::BLIT_READ,
                .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i + 1,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = static_cast<u32>(texture_names.size()),
                },
                .image_id = atlas_texture_array,
            });
            std::array<i32, 3> next_mip_size = {
                std::max<i32>(1, mip_size[0] / 2),
                std::max<i32>(1, mip_size[1] / 2),
                std::max<i32>(1, mip_size[2] / 2),
            };
            cmd_list.blit_image_to_image({
                .src_image = atlas_texture_array,
                .src_image_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .dst_image = atlas_texture_array,
                .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .src_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i,
                    .base_array_layer = 0,
                    .layer_count = static_cast<u32>(texture_names.size()),
                },
                .src_offsets = {{{0, 0, 0}, {mip_size[0], mip_size[1], mip_size[2]}}},
                .dst_slice = {
                    .image_aspect = image_info.aspect,
                    .mip_level = i + 1,
                    .base_array_layer = 0,
                    .layer_count = static_cast<u32>(texture_names.size()),
                },
                .dst_offsets = {{{0, 0, 0}, {next_mip_size[0], next_mip_size[1], next_mip_size[2]}}},
                .filter = daxa::Filter::LINEAR,
            });
            mip_size = next_mip_size;
        }
        for (u32 i = 0; i < 4 - 1; ++i)
        {
            cmd_list.pipeline_barrier_image_transition({
                .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
                .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
                .before_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
                .image_slice = {
                    .image_aspect = image_info.aspect,
                    .base_mip_level = i,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = static_cast<u32>(texture_names.size()),
                },
                .image_id = atlas_texture_array,
            });
        }
        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::TRANSFER_READ_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::READ_WRITE,
            .before_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .image_slice = {
                .image_aspect = image_info.aspect,
                .base_mip_level = 4 - 1,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = static_cast<u32>(texture_names.size()),
            },
            .image_id = atlas_texture_array,
        });


        cmd_list.complete();
        device.submit_commands({
            .command_lists = {std::move(cmd_list)},
        });
        device.wait_idle();
        device.destroy_buffer(staging_buffer);
    }

    ~Textures() {
        this->device.destroy_image(this->atlas_texture_array);
        this->device.destroy_sampler(this->atlas_sampler);
    }

    daxa::ImageId atlas_texture_array;
    daxa::SamplerId atlas_sampler;
    daxa::Device& device;
}; 