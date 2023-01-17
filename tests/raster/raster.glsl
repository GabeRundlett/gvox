#include "gpu.inl"

DAXA_USE_PUSH_CONSTANT(RasterPush)

#if RASTER_VERT

layout(location = 0) out f32vec3 v_col;

void main() {
    Vertex vert = VERTS(gl_VertexIndex);
    gl_Position = f32vec4(vert.pos, 1);
    v_col = vert.col;
    gl_Position.z = gl_Position.z * INPUT.size.z - daxa_push_constant.out_z;
}

#elif RASTER_FRAG

layout(location = 0) in f32vec3 v_col;

void main() {
    u32 o_index = u32(gl_FragCoord.x) + u32(gl_FragCoord.y) * INPUT.size.x + daxa_push_constant.out_z * INPUT.size.x * INPUT.size.y;
    Voxel result;
    result.col = v_col;
    VOXELS(o_index) = result;
}

#endif
