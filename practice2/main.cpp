// #define WITHOUT_MTR
// #define USE_MTR


#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <string_view>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <unordered_map>




std::string to_string(std::string_view str)
{
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}


#ifdef WITHOUT_MTR
const char vertex_shader_source[] =
R"(#version 330 core

uniform float scale;
uniform float angle;

const vec2 VERTICES[3] = vec2[3](
    vec2(0.0, 1.0),
    vec2(-sqrt(0.75), -0.5),
    vec2( sqrt(0.75), -0.5)
);

vec2 center = (VERTICES[0] + VERTICES[1] + VERTICES[2]) / 3.0;

const vec3 COLORS[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

out vec3 color;

void main()
{
    vec2 position = VERTICES[gl_VertexID];

    position -= center;
    position *= scale;
    float x = position.x;
    float y = position.y;
    position.x = x * cos(angle) - y * sin(angle);
    position.y = x * sin(angle) + y * cos(angle);
    position += center;
    gl_Position = vec4(position, 0.0, 1.0);
    color = COLORS[gl_VertexID];
}
)";
#endif


#ifdef USE_MTR
const char vertex_shader_source[] =
    R"(#version 330 core

uniform mat4 transform;
uniform mat4 view;

const vec2 VERTICES[3] = vec2[3](
    vec2(0.0, 1.0),
    vec2(-sqrt(0.75), -0.5),
    vec2( sqrt(0.75), -0.5)
);

const vec3 COLORS[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

out vec3 color;

void main()
{
    vec2 position = VERTICES[gl_VertexID];
    gl_Position = view * transform * vec4(position, 0.0, 1.0);
    color = COLORS[gl_VertexID];
}
)";
#endif

const char fragment_shader_source[] =
R"(#version 330 core

in vec3 color;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0);
}
)";

GLuint create_shader(GLenum type, const char * source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    SDL_GL_SetSwapInterval(0);

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = create_program(vertex_shader, fragment_shader);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    std::unordered_map<SDL_Scancode, bool> key_down;

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    glUseProgram(program);

    float scale = 0.1;
    float time = 0.f;
    float x = 0, y = 0;

#ifdef WITHOUT_MTR
    GLint scaleLoc = glGetUniformLocation(program, "scale");
    glUniform1f(scaleLoc, scale);
    GLint angleLoc = glGetUniformLocation(program, "angle");
#endif

#ifdef USE_MTR
    GLint transformLoc = glGetUniformLocation(program, "transform");
    GLint viewLoc = glGetUniformLocation(program, "view");
#endif
    bool running = true;
    while (running)
    {
        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_WINDOWEVENT: switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                width = event.window.data1;
                height = event.window.data2;
                glViewport(0, 0, width, height);
                break;
            }
            break;
        case SDL_KEYDOWN:
            key_down[event.key.keysym.scancode] = true;
            break;
        case SDL_KEYUP:
            key_down[event.key.keysym.scancode] = false;
            break;
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
#ifdef WITHOUT_MTR
        glUniform1f(angleLoc, time);
#endif

#ifdef USE_MTR
        float transform[16] =
        {
        scale * cos(time),  scale * -sin(time),     0, x, // 1 строка
        scale * sin(time),  scale *  cos(time),     0, y, // 2 строка
                        0,                   0, scale, 0, // 3 строка
                        0,                   0,     0, 1, // 4 строка
        };

        float view[16] =
            {
            height / (float)width, 0, 0, 0, // 1 строка
                                0, 1, 0, 0, // 2 строка
                                0, 0, 1, 0, // 3 строка
                                0, 0, 0, 1, // 4 строка
        };

        glUniformMatrix4fv(transformLoc, 1, GL_TRUE, transform);
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, view);
#endif
        x = sin(time / 2.0) / 2.0;
        y = cos(time / 2.0) / 2.0;

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        SDL_GL_SwapWindow(window);
        // dt = 0.001;
        time += dt;
        // std::cout << dt << " ";
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
