#version 410 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
uniform mat4 projMtx;

out vec2 uv_frag;
out vec4 color_frag;

void main()
{
    uv_frag = uv;
    color_frag = color;
    gl_Position = projMtx * vec4(pos.xy, 0.0, 1.0);
}

