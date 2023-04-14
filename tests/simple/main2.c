#include <gvox/gvox.h>
#include <gvox/io_adapters/file.h>

#include <stdio.h>

int main(void) {
    GvoxContainer image_container;
    {
        GvoxContainerCreateInfo create_info;
        create_info.type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO;
        create_info.next = NULL;
        create_info.config = NULL;
        GvoxResult result = gvox_get_standard_container_description("image", &create_info.description);
        if (result != GVOX_SUCCESS) {
            printf("Failed to find standard container 'image'\n");
            return -1;
        }
        result = gvox_create_container(&create_info, &image_container);
        if (result != GVOX_SUCCESS) {
            printf("Failed to create 'image' container\n");
            return -1;
        }
    }

    GvoxContainer text_container;
    {
        GvoxContainerCreateInfo create_info;
        create_info.type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO;
        create_info.next = NULL;
        create_info.config = NULL;
        GvoxResult result = gvox_get_standard_container_description("colored_text", &create_info.description);
        if (result != GVOX_SUCCESS) {
            gvox_destroy_container(image_container);
            printf("Failed to find standard container 'colored_text'\n");
            return -1;
        }
        result = gvox_create_container(&create_info, &text_container);
        if (result != GVOX_SUCCESS) {
            gvox_destroy_container(image_container);
            printf("Failed to create 'colored_text' container\n");
            return -1;
        }
    }

    {
        GvoxIoAdapter grass_file_adapter;
        {
            GvoxFileIoAdapterConfig config;
            config.filepath = "assets/grass.png";
            config.open_mode = GVOX_FILE_IO_ADAPTER_OPEN_MODE_INPUT;
            GvoxIoAdapterCreateInfo create_info;
            create_info.type = GVOX_STRUCT_TYPE_IO_ADAPTER_CREATE_INFO;
            create_info.next = NULL;
            create_info.config = &config;
            GvoxResult result = gvox_get_standard_io_adapter_description("file", &create_info.description);
            if (result != GVOX_SUCCESS) {
                printf("Failed to find standard adapter 'file'\n");
                return -1;
            }
            result = gvox_create_io_adapter(&create_info, &grass_file_adapter);
            if (result != GVOX_SUCCESS) {
                gvox_destroy_container(text_container);
                gvox_destroy_container(image_container);
                printf("Failed to create grass 'file' adapter\n");
                return -1;
            }
        }

        GvoxIoAdapter rock_file_adapter;
        {
            GvoxFileIoAdapterConfig config;
            config.filepath = "assets/rock.png";
            config.open_mode = GVOX_FILE_IO_ADAPTER_OPEN_MODE_INPUT;
            GvoxIoAdapterCreateInfo create_info;
            create_info.type = GVOX_STRUCT_TYPE_IO_ADAPTER_CREATE_INFO;
            create_info.next = NULL;
            create_info.config = &config;
            GvoxResult result = gvox_get_standard_io_adapter_description("file", &create_info.description);
            if (result != GVOX_SUCCESS) {
                printf("Failed to find standard adapter 'file'\n");
                return -1;
            }
            result = gvox_create_io_adapter(&create_info, &rock_file_adapter);
            if (result != GVOX_SUCCESS) {
                gvox_destroy_io_adapter(grass_file_adapter);
                gvox_destroy_container(text_container);
                gvox_destroy_container(image_container);
                printf("Failed to create rock 'file' adapter\n");
                return -1;
            }
        }

        GvoxIoAdapter debug_file_adapter;
        {
            GvoxFileIoAdapterConfig config;
            config.filepath = "assets/debug.png";
            config.open_mode = GVOX_FILE_IO_ADAPTER_OPEN_MODE_INPUT;
            GvoxIoAdapterCreateInfo create_info;
            create_info.type = GVOX_STRUCT_TYPE_IO_ADAPTER_CREATE_INFO;
            create_info.next = NULL;
            create_info.config = &config;
            GvoxResult result = gvox_get_standard_io_adapter_description("file", &create_info.description);
            if (result != GVOX_SUCCESS) {
                printf("Failed to find standard adapter 'file'\n");
                return -1;
            }
            result = gvox_create_io_adapter(&create_info, &debug_file_adapter);
            if (result != GVOX_SUCCESS) {
                gvox_destroy_io_adapter(rock_file_adapter);
                gvox_destroy_io_adapter(grass_file_adapter);
                gvox_destroy_container(text_container);
                gvox_destroy_container(image_container);
                printf("Failed to create rock 'file' adapter\n");
                return -1;
            }
        }

        // {
        //     GvoxParseInfo parse_info;
        //     parse_info.type = GVOX_STRUCT_TYPE_PARSE_INFO;
        //     parse_info.next = NULL;
        //     parse_info.src = grass_file_adapter;
        //     parse_info.dst = image_container;
        //     parse_info.transform = gvox_identity_transform(2);
        //     gvox_scale(&parse_info.transform, (GvoxScale){{3, 3, 0, 0}});
        //     GvoxResult result = gvox_parse(&parse_info);
        //     if (result != GVOX_SUCCESS) {
        //         gvox_destroy_io_adapter(debug_file_adapter);
        //         gvox_destroy_io_adapter(rock_file_adapter);
        //         gvox_destroy_io_adapter(grass_file_adapter);
        //         gvox_destroy_container(text_container);
        //         gvox_destroy_container(image_container);
        //         printf("Failed to parse the input data to the 'image' container\n");
        //         return -1;
        //     }
        // }

        // {
        //     GvoxParseInfo parse_info;
        //     parse_info.type = GVOX_STRUCT_TYPE_PARSE_INFO;
        //     parse_info.next = NULL;
        //     parse_info.src = debug_file_adapter;
        //     parse_info.dst = image_container;
        //     parse_info.transform = gvox_identity_transform(2);
        //     // gvox_translate(&parse_info.transform, (GvoxTranslation){{8, 8, 0, 0}});
        //     gvox_scale(&parse_info.transform, (GvoxScale){{2, 2, 0, 0}});
        //     gvox_rotate(&parse_info.transform, (GvoxRotation){{1.57079f, 0, 0, 0}});
        //     GvoxResult result = gvox_parse(&parse_info);
        //     if (result != GVOX_SUCCESS) {
        //         gvox_destroy_io_adapter(debug_file_adapter);
        //         gvox_destroy_io_adapter(rock_file_adapter);
        //         gvox_destroy_io_adapter(grass_file_adapter);
        //         gvox_destroy_container(text_container);
        //         gvox_destroy_container(image_container);
        //         printf("Failed to parse the input data to the 'image' container\n");
        //         return -1;
        //     }
        // }

        {
            GvoxParseInfo parse_info;
            parse_info.type = GVOX_STRUCT_TYPE_PARSE_INFO;
            parse_info.next = NULL;
            parse_info.src = rock_file_adapter;
            parse_info.dst = image_container;
            parse_info.transform = gvox_identity_transform(2);
            gvox_translate(&parse_info.transform, (GvoxTranslation){{8, 0, 0, 0}});
            GvoxResult result = gvox_parse(&parse_info);
            if (result != GVOX_SUCCESS) {
                gvox_destroy_io_adapter(debug_file_adapter);
                gvox_destroy_io_adapter(rock_file_adapter);
                gvox_destroy_io_adapter(grass_file_adapter);
                gvox_destroy_container(text_container);
                gvox_destroy_container(image_container);
                printf("Failed to parse the input data to the 'image' container\n");
                return -1;
            }
        }

        gvox_destroy_io_adapter(debug_file_adapter);
        gvox_destroy_io_adapter(grass_file_adapter);
        gvox_destroy_io_adapter(rock_file_adapter);
    }

    {
        GvoxBlitInfo blit_info;
        blit_info.type = GVOX_STRUCT_TYPE_BLIT_INFO;
        blit_info.next = NULL;
        blit_info.src = image_container;
        blit_info.dst = text_container;
        GvoxResult result = gvox_blit(&blit_info);
        if (result != GVOX_SUCCESS) {
            gvox_destroy_container(text_container);
            gvox_destroy_container(image_container);
            printf("Failed to blit from the 'image' container to the 'colored_text' container\n");
            return -1;
        }
    }

    {
        GvoxSerializeInfo serialize_info;
        serialize_info.type = GVOX_STRUCT_TYPE_SERIALIZE_INFO;
        serialize_info.next = NULL;
        serialize_info.src = text_container;
        // serialize_info.dst = byte_buffer_adapter;
        GvoxResult result = gvox_serialize(&serialize_info);
        if (result != GVOX_SUCCESS) {
            gvox_destroy_container(text_container);
            gvox_destroy_container(image_container);
            printf("Failed to serialize the data from the 'colored_text' container\n");
            return -1;
        }
    }

    gvox_destroy_container(text_container);
    gvox_destroy_container(image_container);
    return 0;
}
