#version 460 core

in vec2 TexCoord;

out vec4 o_Color;

uniform usampler2D tex;

const vec3 MaterialToColor[3] = {
        vec3(0),
        vec3(1, 1, 0),
        vec3(0, 0, 1),
    };

void main() {
    uvec4 texel = texture(tex, TexCoord);
    uint value = texel.r;
    // uint material_type = bitfieldExtract(value, 4, 4);
    uint material_type = value;

    o_Color = vec4(MaterialToColor[material_type], 1.0);
}
