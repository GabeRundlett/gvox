#include "gpu.inl"

DAXA_USE_PUSH_CONSTANT(PreprocessPush)

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    u32 tri_i = gl_GlobalInvocationID.x;
    u32 vert_off = tri_i * 3;
    f32vec3 pos[3];
    pos[0] = (daxa_push_constant.modl_mat * f32vec4(VERTS(vert_off + 0).pos, 1)).xyz;
    pos[1] = (daxa_push_constant.modl_mat * f32vec4(VERTS(vert_off + 1).pos, 1)).xyz;
    pos[2] = (daxa_push_constant.modl_mat * f32vec4(VERTS(vert_off + 2).pos, 1)).xyz;

    f32vec3 del_a = pos[1] - pos[0];
    f32vec3 del_b = pos[2] - pos[0];
    f32vec3 nrm = normalize(cross(del_a, del_b));

    f32 dx = abs(dot(nrm, f32vec3(1, 0, 0)));
    f32 dy = abs(dot(nrm, f32vec3(0, 1, 0)));
    f32 dz = abs(dot(nrm, f32vec3(0, 0, 1)));

    u32 side = 0;
    if (dx > dy) {
        if (dx > dz) {
            side = 0;
        } else {
            side = 2;
        }
    } else {
        if (dy > dz) {
            side = 1;
        } else {
            side = 2;
        }
    }
    VERTS(vert_off + 0).rotation = side;
    VERTS(vert_off + 1).rotation = side;
    VERTS(vert_off + 2).rotation = side;

    switch (side) {
    case 0:
        VERTS(vert_off + 0).pos = pos[0].zyx;
        VERTS(vert_off + 1).pos = pos[1].zyx;
        VERTS(vert_off + 2).pos = pos[2].zyx;
        break;
    case 1:
        VERTS(vert_off + 0).pos = pos[0].xzy;
        VERTS(vert_off + 1).pos = pos[1].xzy;
        VERTS(vert_off + 2).pos = pos[2].xzy;
        break;
    case 2:
        VERTS(vert_off + 0).pos = pos[0].xyz;
        VERTS(vert_off + 1).pos = pos[1].xyz;
        VERTS(vert_off + 2).pos = pos[2].xyz;
        break;
    }

    NORMALS(tri_i) = -nrm;
}
