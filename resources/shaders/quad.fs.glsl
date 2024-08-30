#version 460 core

in vec2 TexCoord;

out vec4 o_Color;

uniform sampler2D tex;

void main() {
    o_Color = vec4(texture(tex, TexCoord).rgb, 1.0);
}
