#version 410 core

uniform sampler2D tex;

in vec2 uv_frag;
in vec4 color_frag;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = color_frag * texture(tex, uv_frag.st);
    // fragColor = vec4(uv_frag.x, uv_frag.y, 0.0f, 1.0f);
}
