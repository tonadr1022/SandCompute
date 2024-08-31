#version 460 core

layout(local_size_x = WORK_GROUP_X, local_size_y = WORK_GROUP_Y, local_size_z = 1) in;

layout(r32ui, binding = 0) uniform uimage2D img_input;
layout(r32ui, binding = 1) uniform uimage2D img_output;

uniform int grid_size_x;
uniform int grid_size_y;
uniform int modification_count = 0;

const int MAT_None = 0;
const int MAT_Sand = 1;
const int MAT_Water = 2;

uint Pack(int material_type) {
    return material_type;
}

uint get_data(ivec2 pos, ivec2 offset);

const ivec2 up = ivec2(0, 1);
const ivec2 down = ivec2(0, -1);
const ivec2 right = ivec2(1, 0);
const ivec2 left = ivec2(-1, 0);
const ivec2 none = ivec2(0, 0);

struct Cell {
    int material;
};

uint SHAPE_Circle = 0;
uint SHAPE_Square = 1;
struct Modification {
    ivec2 pos;
    float radius;
    uint shape;
    int material;
};

layout(std430, binding = 0) readonly buffer ModBuffer {
    Modification modifications[];
};

Cell new_cell(uint data);

Cell simulate(ivec2 pos);

bool is_inside_circle(ivec2 pos, ivec2 circle_pos, float radius) {
    return (pos.x - circle_pos.x) * (pos.x - circle_pos.x) + (pos.y - circle_pos.y) * (pos.y - circle_pos.y) < radius;
}

void set_cell(ivec2 pos, Cell cell) {
    imageStore(img_output, pos, ivec4(Pack(cell.material), 0, 0, 0));
}

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= grid_size_x || pos.y >= grid_size_y || pos.x < 0 || pos.y < 0) {
        return;
    }

    for (int i = 0; i < modification_count; i++) {
        if (modifications[i].shape == SHAPE_Circle) {
            if (is_inside_circle(pos, modifications[i].pos, modifications[i].radius)) {
                Cell c = new_cell(modifications[i].material);
                set_cell(pos, c);
                return;
            }
        } else {}
    }

    Cell cell = simulate(pos);
    set_cell(pos, cell);
}

Cell simulate(ivec2 pos) {
    Cell cell = new_cell(get_data(none, pos));
    if (pos.y < grid_size_y - 1) {
        Cell cell_above = new_cell(get_data(up, pos));
        if (cell_above.material == MAT_Sand && cell.material == MAT_None) {
            cell.material = MAT_Sand;
            return cell;
        }
    }
    if (pos.y > 0) {
        Cell cell_below = new_cell(get_data(down, pos));
        if (cell_below.material == MAT_None && cell.material != MAT_None) {
            cell.material = MAT_None;
            return cell;
        }
    }
    return cell;
}

uint get_data(ivec2 pos, ivec2 offset) {
    return imageLoad(img_input, pos + offset).r;
}

Cell new_cell(uint data) {
    return Cell(data);
}
