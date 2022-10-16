#include "magicavoxel.h"

#include <stdio.h>

GVoxPayload magicavoxel_create_payload(GVoxScene scene) {
    // TODO: Implement this
    GVoxPayload result = {0};
    printf("creating magicavoxel payload from the %lld nodes at %p\n", scene.node_n, (void *)scene.nodes);
    return result;
}

void magicavoxel_destroy_payload(GVoxPayload payload) {
    // TODO: Implement this
    printf("destroying magicavoxel payload at %p with size %lld\n", payload.data, payload.size);
}

GVoxScene magicavoxel_parse_payload(GVoxPayload payload) {
    // TODO: Implement this
    GVoxScene result = {0};
    printf("parsing magicavoxel payload at %p with size %lld\n", payload.data, payload.size);
    return result;
}
