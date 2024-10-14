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
#include <vector>
#include <map>
#include <cmath>

#include "obj_parser.hpp"
#include "stb_image.h"


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

const char vertex_shader_source[] =
R"(#version 330 core

uniform mat4 viewmodel;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

out vec3 normal;
out vec2 texcoord;

void main()
{
    gl_Position = projection * viewmodel * vec4(in_position, 1.0);
    normal = mat3(viewmodel) * in_normal;
    texcoord = in_texcoord;
}
)";

const char fragment_shader_source[] =
R"(#version 330 core

uniform sampler2D sampler;
uniform float t;

in vec3 normal;
in vec2 texcoord;

layout (location = 0) out vec4 out_color;

void main()
{
    float lightness = 0.5 + 0.5 * dot(normalize(normal), normalize(vec3(1.0, 2.0, 3.0)));
    // vec3 albedo = vec3(texcoord, 0.0);
    vec4 albedo = texture(sampler, texcoord + vec2(t, t));
    out_color = vec4(lightness * albedo.xyz, albedo.w);
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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 5",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    GLuint viewmodel_location = glGetUniformLocation(program, "viewmodel");
    GLuint projection_location = glGetUniformLocation(program, "projection");

    std::string project_root = PROJECT_ROOT;
    std::string cow_texture_path = project_root + "/cow.png";
    obj_data cow = parse_obj(project_root + "/cow.obj");

    GLuint vao, vbo, ebo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, cow.vertices.size() * sizeof(obj_data::vertex), cow.vertices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cow.indices.size() * sizeof(std::uint32_t), cow.indices.data(), GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(sizeof(obj_data::vertex::position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(sizeof(obj_data::vertex::position) + sizeof(obj_data::vertex::normal)) );
    glEnableVertexAttribArray(2);


    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    size_t size = 512;
    std::vector<std::uint32_t> pixels(size * size);
    for(size_t i = 0; i < size; ++i) for(size_t j = 0; j < size; ++j)
            pixels[i * size + j] = (i + j) % 2 == 0 ? 0xFFFFFFFFu : 0xFF000000u ;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    size_t size1 = 256;
    std::vector<std::uint32_t> pixelsR(size1 * size1, 0xFFFF0000u);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size1, size1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsR.data());

    size_t size2 = 128;
    std::vector<std::uint32_t> pixelsG(size2 * size2, 0xFF00FF00u);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA8, size2, size2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsG.data());

    size_t size3 = 64;
    std::vector<std::uint32_t> pixelsB(size3 * size3, 0xFF0000FFu);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA8, size3, size3, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsB.data());

    GLuint new_texture;
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, new_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int x, y, channels;
    stbi_uc *img = stbi_load(cow_texture_path.data(), &x, &y, &channels, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);
    glGenerateMipmap(GL_TEXTURE_2D);

    GLuint time_location = glGetUniformLocation(program, "t");

    GLuint texture_location = glGetUniformLocation(program, "sampler");

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    float angle_y = M_PI;
    float offset_z = -2.f;

    std::map<SDL_Keycode, bool> button_down;

    int taskType = 0;

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
            button_down[event.key.keysym.sym] = true;
            if (event.key.keysym.sym == SDLK_SPACE)
            {
               taskType = (taskType + 1 ) % 3;
            }
            break;
        case SDL_KEYUP:
            button_down[event.key.keysym.sym] = false;
            break;
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        if (button_down[SDLK_UP]) offset_z -= 4.f * dt;
        if (button_down[SDLK_DOWN]) offset_z += 4.f * dt;
        if (button_down[SDLK_LEFT]) angle_y += 4.f * dt;
        if (button_down[SDLK_RIGHT]) angle_y -= 4.f * dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float near = 0.1f;
        float far = 100.f;
        float top = near;
        float right = (top * width) / height;

        float viewmodel[16] =
        {
            std::cos(angle_y), 0.f, -std::sin(angle_y), 0.f,
            0.f, 1.f, 0.f, 0.f,
            std::sin(angle_y), 0.f, std::cos(angle_y), offset_z,
            0.f, 0.f, 0.f, 1.f,
        };

        float projection[16] =
        {
            near / right, 0.f, 0.f, 0.f,
            0.f, near / top, 0.f, 0.f,
            0.f, 0.f, - (far + near) / (far - near), - 2.f * far * near / (far - near),
            0.f, 0.f, -1.f, 0.f,
        };

        glUseProgram(program);
        glUniformMatrix4fv(viewmodel_location, 1, GL_TRUE, viewmodel);
        glUniformMatrix4fv(projection_location, 1, GL_TRUE, projection);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture);

        if(taskType == 0)
            glUniform1i(texture_location, 0);
        else if(taskType == 1)
        {
            glUniform1i(texture_location, 1);
            glUniform1f(time_location, 0.f);
        }
        else if(taskType == 2)
        {
            glUniform1i(texture_location, 1);
            glUniform1f(time_location, time);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, cow.indices.size(), GL_UNSIGNED_INT, (void*)(0));

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
