#version 410 core

in vec2 uv_frag;

uniform sampler2D tex;

out vec4 fragColor;

void main()
{
    fragColor = texture(tex, uv_frag);
    // fragColor = vec4(uv_frag.x, uv_frag.y, 0.0f, 1.0f);
}
