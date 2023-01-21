#include "gpu.inl"

DAXA_USE_PUSH_CONSTANT(RasterPush)

#if RASTER_VERT

layout(location = 0) out f32vec2 v_tex;
layout(location = 1) out u32 v_rotation;

void main() {
    Vertex vert = VERTS(gl_VertexIndex);
    gl_Position = f32vec4(vert.pos, 1);
    gl_Position.xyz = gl_Position.xyz * f32vec3(1, 1, 0.5) + f32vec3(0, 0, 0.5);
    v_tex = vert.tex;
    v_rotation = vert.rotation;
}

#elif RASTER_FRAG

layout(location = 0) in f32vec2 v_tex;
layout(location = 1) flat in u32 v_rotation;

void main() {
    f32vec3 p = gl_FragCoord.xyz;
    p.z *= INPUT.size.z;
    switch (v_rotation) {
    case 0: p = p.zyx; break;
    case 1: p = p.xzy; break;
    case 2: break;
    }
    u32 o_index = u32(p.x) + u32(p.y) * INPUT.size.x + u32(p.z) * INPUT.size.x * INPUT.size.y;
    f32vec4 tex0_col = texture(daxa_push_constant.texture_id, daxa_push_constant.texture_sampler, v_tex);
    u32 r = u32(tex0_col.r * 255);
    u32 g = u32(tex0_col.g * 255);
    u32 b = u32(tex0_col.b * 255);
    u32 i = 1;
    u32 u32_voxel = 0;
    u32_voxel = u32_voxel | (r << 0x00);
    u32_voxel = u32_voxel | (g << 0x08);
    u32_voxel = u32_voxel | (b << 0x10);
    u32_voxel = u32_voxel | (i << 0x18);
    atomicExchange(VOXELS(o_index), u32_voxel);
}

#endif
