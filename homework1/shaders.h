const char grid_vertex_shader_source[] =
    R"(#version 330 core

uniform mat4 view;
uniform float max_value;

layout (location = 0) in vec2 in_position;
layout (location = 1) in float value;

out vec4 color;

void main()
{
    float _val = value;
    gl_Position = view * vec4(in_position, 0.0, 1.0);
    if (value < 0) {
        float val = -max(value, -max_value) / max_value;
        color = vec4(1 - val, 1 - val, 1.0, 1.0);
    }
    else {
        float val = min(value, max_value) / max_value;
        color = vec4(1.0, 1 - val, 1 - val, 1.0);
    }
}
)";

const char grid_fragment_shader_source[] =
    R"(#version 330 core

in vec4 color;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = color;
}
)";


const char isoline_vertex_shader_source[] =
    R"(#version 330 core
uniform mat4 view;

layout (location = 0) in vec2 in_position;

void main()
{
    gl_Position = view * vec4(in_position, 0.0, 1.0);
}
)";

const char isoline_fragment_shader_source[] =
    R"(#version 330 core

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
}
)";
