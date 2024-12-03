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
#include <assert.h>
#include <unordered_map>

#include "shaders.h"

const std::uint32_t PRIMITIVE_RESTART_INDEX = 2392392392;
float VAL_LIM = 2.5;
int stepsCount = 1;

float scale = 10;

int gridW = 200, gridH = 200; // количество элементов в сетке


struct point2d
{
    point2d(){}
    point2d(float x_, float y_): x(x_), y(y_){}
    point2d(double x_, double y_): x(x_), y(y_){}
    float x;
    float y;
};



void calcOneIsoline(const std::vector<point2d> & points,
                    const std::vector<float> & values,
                    int w, int h,
                    const float threeshold,
                    std::vector<std::pair<point2d, point2d>> * ret)
{
    assert(points.size() == values.size());
    assert(points.size() == (w + 1) * (h + 1));

    std::vector<bool> moreThreeshold;
    moreThreeshold.reserve(values.size());
    for(const float & val : values)
        moreThreeshold.push_back(val > threeshold);


    std::vector<u_char> configs;
    configs.reserve((w - 1) * (h - 1));
    for(int i = 0; i < w; ++i)
    {
        for(int j = 0; j < h; ++j)
        {
            u_char currConf = 0;
            if(moreThreeshold[i * (h + 1) + j])
                currConf |= 0b0001;
            if(moreThreeshold[i * (h + 1) + j + 1])
                currConf |= 0b0010;
            if(moreThreeshold[(i + 1) * (h + 1) + j + 1])
                currConf |= 0b0100;
            if(moreThreeshold[(i + 1) * (h + 1) + j])
                currConf |= 0b1000;
            configs.push_back(currConf);
        }
    }

    for(int i = 0; i < w; ++i)
    {
        for(int j = 0; j < h; ++j)
        {
            u_char currConf = configs[i * h + j];

            // float shiftX = 1.f / (float) w / 2.f;
            // float shiftY = 1.f / (float) h / 2.f;
            // point2d currCentre = {(float) i / float(w) + shiftX, (float) j / float(h) + shiftY};

            // point2d left  = {currCentre.x, currCentre.y - shiftY};
            // point2d right = {currCentre.x, currCentre.y + shiftY};

            // point2d up   = {currCentre.x - shiftX, currCentre.y};
            // point2d down = {currCentre.x + shiftX, currCentre.y};

            double upLeftV    = values[ i      * (h + 1) + j    ];
            double upRightV   = values[ i      * (h + 1) + j + 1];
            double downLeftV  = values[(i + 1) * (h + 1) + j    ];
            double downRightV = values[(i + 1) * (h + 1) + j + 1];

            double x0 = points[i * (h + 1) + j].x;
            double y0 = points[i * (h + 1) + j].y;

            double x1 = points[(i + 1) * (h + 1) + j + 1].x;
            double y1 = points[(i + 1) * (h + 1) + j + 1].y;


            double c3 = (upLeftV - threeshold) / (threeshold - upRightV);
            double c4 = (downLeftV - threeshold) / (threeshold - downRightV);

            double c1 = (upLeftV - threeshold) / (threeshold - downLeftV);
            double c2 = (upRightV - threeshold) / (threeshold - downRightV);

            c1 = std::abs(c1);
            c2 = std::abs(c2);
            c3 = std::abs(c3);
            c4 = std::abs(c4);

            point2d left ((x0 + c1 * x1) / (c1 + 1.f), y0);
            if(std::isinf(c1) || std::isnan(c1))
                left = point2d(x0, y0);
            if(c1 == -1)
                left = point2d(x0, y0);

            point2d right ((x0 + c2  * x1 ) / (c2 + 1.f), y1);
            if(std::isinf(c2) || std::isnan(c2))
                right = point2d(x0, y1);
            if(c2 == -1)
                right = point2d(x0, y1);


            point2d up  (x0, (y0 + c3 * y1 ) / (1.f + c3));
            if(std::isinf(c3) || std::isnan(c3))
                up = point2d (x0, y0);
            if(c3 == -1)
                right = point2d(x0, y1);

            point2d down (x1, (y0 + c4 * y1 ) / (1.f + c4));
            if(std::isinf(c4) || std::isnan(c4))
                down = point2d(x1, y0);
            if(c4 == -1)
                right = point2d(x1, y1);

            // 16
            if(currConf == 0b0000 || currConf == 0b1111)
                continue;
            // 14
            if(currConf == 0b0001)
                ret->push_back(std::make_pair(left, up));
            if(currConf == 0b0010)
                ret->push_back(std::make_pair(up, right));
            if(currConf == 0b0100)
                ret->push_back(std::make_pair(right, down));
            if(currConf == 0b1000)
                ret->push_back(std::make_pair(down, left));
            // 10
            if(currConf == 0b1110)
                ret->push_back(std::make_pair(left, up));
            if(currConf == 0b1101)
                ret->push_back(std::make_pair(up, right));
            if(currConf == 0b1011)
                ret->push_back(std::make_pair(right, down));
            if(currConf == 0b0111)
                ret->push_back(std::make_pair(down, left));
            // 6
            if(currConf == 0b0101)
            {
                ret->push_back(std::make_pair(left, up));
                ret->push_back(std::make_pair(right, down));
            }
            if(currConf == 0b1010)
            {
                ret->push_back(std::make_pair(up, right));
                ret->push_back(std::make_pair(down, left));
            }
            // 4
            if(currConf == 0b0011)
                ret->push_back(std::make_pair(left, right));
            if(currConf == 0b0110)
                ret->push_back(std::make_pair(up, down));
            if(currConf == 0b1100)
                ret->push_back(std::make_pair(right, left));
            if(currConf == 0b1001)
                ret->push_back(std::make_pair(down, up));
        }
    }
}

struct HashPoint
{
    size_t operator()(const point2d & p) const noexcept
    {
        return std::hash<float>()(p.x) ^ std::hash<float>()(p.y);
    }
};

struct EqualPoint
{
    bool operator()(const point2d & left, const point2d & right) const noexcept
    {
        return left.x == right.x && left.y == right.y;
    }
};

void matchPoints(const std::vector<std::pair<point2d, point2d>> & pairPoints,
                 std::vector<point2d> * normalIsoPoints,
                 std::vector<uint32_t> * isoIndeces)
{
    normalIsoPoints->clear();
    isoIndeces->clear();
    std::unordered_map<point2d, int, HashPoint, EqualPoint> mapPoints;
    for(std::pair<point2d, point2d> pp : pairPoints)
    {
        for(const point2d & p : {pp.first, pp.second})
        {
            if(mapPoints.contains(p))
            {
                isoIndeces->push_back(mapPoints.at(p));
            }
            else
            {
                normalIsoPoints->push_back(p);
                mapPoints.insert(std::make_pair(p, normalIsoPoints->size() - 1));
                isoIndeces->push_back(mapPoints.at(p));
            }
        }
    }
}

float f1(float x, float y, float time)
{
    // return sin(y * 3.1415f * 2.f);

    return sin(10.f * x + time * 1.2f) * cos(y + time * 2.1) +
           cos(time * 1.3f) * cos(5.f * (x + y) + time / 1.2f) +
           sin(time / 1.5f)  * cos(5.f * (x - y + time / 2.f)) +
           cos(time * 1.5f)  * sin(5.f * (x + y + time / 2.1f)) +
           sin(time * 1.4f)   * sin(x * 3.14f * 2.f + time * 2.f) +
           cos(time / 3.f)   * sin(y * 3.14f * 2.f + time / 4.f) +
           sin(time / 1.6f)  * cos(3.5f * (x - y + time / 2.3f)) +
           cos(time / 1.7f)  * sin(3.5f * sqrt(x * x + y * y + time * time / 2.4f)) +
           sin(sqrt(2.f * y * x) + time * 1.8) * cos(sqrt(1.5f * y * x)+ time * 1.9) ;
}


void calcValuesGrid(const std::vector<point2d> & grid, float time, std::vector<float> * values)
{
    values->clear();
    values->reserve(grid.size());
    for(const point2d & p : grid)
        values->push_back(f1(p.x, p.y, time));
}

void calcGrid(int w, int h, std::vector<point2d> * grid)
{
    grid->clear();
    grid->reserve(w * h);
    for(int i = 0; i < w + 1; ++i) for(int j = 0; j < h + 1; ++j)
        {
            point2d p;
            p.x = (float) i / float(w) * scale;
            p.y = (float) j / float(h) * scale;
            grid->push_back(p);
        }
}

void calcIndeces(int w, int h, std::vector<uint32_t>* indices)
{
    // 0, 1, 2, 3
    // 4, 5, 6, 7
    // ind 0, 4, 1, 5, 2, 6, 3, 7, restart...

    indices->clear();
    indices->reserve(w * (h * 2 + 1) );

    for(int i = 0; i < w - 1 + 1; ++i)
    {
        for(int j = 0; j < h + 1; ++j)
        {
            indices->push_back(i * (h + 1) + j);
            indices->push_back((i + 1) * (h + 1) + j);
        }
        indices->push_back(PRIMITIVE_RESTART_INDEX);
    }
}



void calcScreenGrid(int screenWidth, int screenHeight, int funcW, int funcH, std::vector<point2d> * vec)
{
    vec->clear();
    vec->resize((funcW + 1) * (funcH + 1));

    int limit = std::min(screenWidth, screenHeight);
    float dx = (float) std::max(screenWidth - limit, 0) / 2.0f;
    float dy = (float) std::max(screenHeight - limit, 0) / 2.0f;

    for (int i = 0; i < funcW + 1 ; ++i)
    {
        for (int j = 0; j < funcH + 1; ++j)
        {
            point2d p;
            p.x = float(limit) * (float) i / (float) funcW + dx;
            p.y = float(limit) * (float) j / (float) funcH + dy;
            (*vec)[i * (funcH + 1) + j] = p;
        }
    }
}


void calcScreenIsopoints(int screenWidth, int screenHeight, const std::vector<point2d> & points,  std::vector<point2d> * vec)
{
    vec->clear();
    vec->reserve(points.size());

    int limit = std::min(screenWidth, screenHeight);
    float dx = (float) std::max(screenWidth - limit, 0) / 2.0f;
    float dy = (float) std::max(screenHeight - limit, 0) / 2.0f;

    for(const point2d & p : points)
    {
        point2d newP;
        newP.x = float(limit) * p.x / scale + dx;
        newP.y = float(limit) * p.y / scale + dy;
        vec->push_back(newP);
    }
}



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

    SDL_Window * window = SDL_CreateWindow("Graphics course homework 1",
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

    SDL_GL_SetSwapInterval(0);

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(1.f, 1.f, 1.f, 0.f);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);

    GLuint grid_vertex_shader = create_shader(GL_VERTEX_SHADER, grid_vertex_shader_source);
    GLuint grid_fragment_shader = create_shader(GL_FRAGMENT_SHADER, grid_fragment_shader_source);
    GLuint grid_program = create_program(grid_vertex_shader, grid_fragment_shader);

    GLint view_location = glGetUniformLocation(grid_program, "view");
    GLint value_limit = glGetUniformLocation(grid_program, "max_value");

    std::vector<point2d> grid, screenGrid;
    std::vector<float> gridValues;
    std::vector<uint32_t> gridIndeces;

    calcGrid(gridW, gridH, &grid);
    calcValuesGrid(grid, time, &gridValues);
    calcIndeces(gridW, gridH, &gridIndeces);
    calcScreenGrid(width, height, gridW, gridH, &screenGrid);

    GLuint grid_vao, grid_pos_vbo, grid_val_vbo, grid_ebo;
    glGenVertexArrays(1, &grid_vao);
    glGenBuffers(1, &grid_pos_vbo);
    glGenBuffers(1, &grid_val_vbo);
    glGenBuffers(1, &grid_ebo);
    {
        glBindVertexArray(grid_vao);

        glBindBuffer(GL_ARRAY_BUFFER, grid_pos_vbo);
        glBufferData(GL_ARRAY_BUFFER, screenGrid.size() * sizeof(point2d), screenGrid.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));


        glBindBuffer(GL_ARRAY_BUFFER, grid_val_vbo);
        glBufferData(GL_ARRAY_BUFFER, gridValues.size() * sizeof(float), gridValues.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndeces.size() * sizeof(std::uint32_t), gridIndeces.data(), GL_DYNAMIC_DRAW);
    }

    GLuint isoline_vertex_shader = create_shader(GL_VERTEX_SHADER, isoline_vertex_shader_source);
    GLuint isoline_fragment_shader = create_shader(GL_FRAGMENT_SHADER, isoline_fragment_shader_source);
    GLuint isoline_program = create_program(isoline_vertex_shader, isoline_fragment_shader);
    GLint view_location_iso = glGetUniformLocation(isoline_program, "view");


    std::vector<std::pair<point2d, point2d>> isolinesPair;
    std::vector<point2d> normalIsoPoints;
    std::vector<uint32_t> isoIndeces;
    std::vector<point2d> screenIsoPoints;

    isolinesPair.clear();
    for(int i = -1; i <= 1; ++i)
        calcOneIsoline(grid, gridValues, gridW, gridH, i, &isolinesPair);

    matchPoints(isolinesPair, &normalIsoPoints, &isoIndeces);
    calcScreenIsopoints(width, height, normalIsoPoints, &screenIsoPoints);

    GLuint isolines_vao, isolines_pos_vbo, isolines_ebo;
    glGenVertexArrays(1, &isolines_vao);
    glGenBuffers(1, &isolines_pos_vbo);
    glGenBuffers(1, &isolines_ebo);
    {
        glBindVertexArray(isolines_vao);

        glBindBuffer(GL_ARRAY_BUFFER, isolines_pos_vbo);
        glBufferData(GL_ARRAY_BUFFER, screenIsoPoints.size() * sizeof(point2d), screenIsoPoints.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, isolines_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, isoIndeces.size() * sizeof(std::uint32_t), isoIndeces.data(), GL_STREAM_DRAW);
    }


    bool running = true;
    bool needUpdatePos = false;
    bool needUpdateCalcGrid = false;
    bool pause = false;

    while (running)
    {
        glClear(GL_COLOR_BUFFER_BIT);

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        if(!pause)
            time += dt;

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
                needUpdatePos = true;
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {

            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {

            }
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_LEFT)
            {
                stepsCount = std::max(1, stepsCount - 1);
            }
            else if (event.key.keysym.sym == SDLK_RIGHT)
            {
                stepsCount = std::min(10, stepsCount + 1);
            }
            else if (event.key.keysym.sym == SDLK_UP)
            {
                ++gridW;
                ++gridH;
                needUpdateCalcGrid = true;
            }
            else if (event.key.keysym.sym == SDLK_DOWN)
            {
                gridW = std::max(5, gridW - 1);
                gridH = std::max(5, gridH - 1);
                needUpdateCalcGrid = true;
            }
            else if (event.key.keysym.sym == SDLK_SPACE)
            {
                pause = !pause;
            }
            else if (event.key.keysym.sym == SDLK_MINUS)
            {
                needUpdateCalcGrid = true;
                scale += 0.1;
            }
            else if (event.key.keysym.sym == SDLK_EQUALS)
            {
                needUpdateCalcGrid = true;
                scale = std::max(0.5, scale - 0.1);
            }
            break;
        }

        if (!running)
            break;

        float view[16] =
            {
            2.f / (float)width, 0.f, 0.f, -1.f,
            0.f, -2.f / (float)height, 0.f, 1.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };

        if (needUpdateCalcGrid)
        {
            needUpdateCalcGrid = false;

            calcGrid(gridW, gridH, &grid);
            calcIndeces(gridW, gridH, &gridIndeces);
            calcScreenGrid(width, height, gridW, gridH, &screenGrid);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndeces.size() * sizeof(std::uint32_t), gridIndeces.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, grid_pos_vbo);
            glBufferData(GL_ARRAY_BUFFER, screenGrid.size() * sizeof(point2d), screenGrid.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, grid_val_vbo);
            glBufferData(GL_ARRAY_BUFFER, gridValues.size() * sizeof(float), gridValues.data(), GL_STREAM_DRAW);
        }

        if (needUpdatePos)
        {
            needUpdatePos = false;
            calcScreenGrid(width, height, gridW, gridH, &screenGrid);

            glBindBuffer(GL_ARRAY_BUFFER, grid_pos_vbo);
            glBufferData(GL_ARRAY_BUFFER, screenGrid.size() * sizeof(point2d), screenGrid.data(), GL_DYNAMIC_DRAW);
        }

        calcValuesGrid(grid, time, &gridValues);

        glBindBuffer(GL_ARRAY_BUFFER, grid_val_vbo);
        glBufferData(GL_ARRAY_BUFFER, gridValues.size() * sizeof(float), gridValues.data(), GL_STREAM_DRAW);


        glUseProgram(grid_program);
        glBindVertexArray(grid_vao);
        glLineWidth(1.0f);
        glDrawElements(GL_TRIANGLE_STRIP, gridIndeces.size(), GL_UNSIGNED_INT, (void*) (0));

        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);
        glUniform1f(value_limit, VAL_LIM);


        glUseProgram(isoline_program);
        glBindVertexArray(isolines_vao);

        isolinesPair.clear();
        for(float i = -VAL_LIM + VAL_LIM / stepsCount / 2.0; i <= VAL_LIM - VAL_LIM / stepsCount / 2.0; i += VAL_LIM / stepsCount)
            calcOneIsoline(grid, gridValues, gridW, gridH, i, &isolinesPair);

        matchPoints(isolinesPair, &normalIsoPoints, &isoIndeces);
        calcScreenIsopoints(width, height, normalIsoPoints, &screenIsoPoints);

        glBindBuffer(GL_ARRAY_BUFFER, isolines_pos_vbo);
        glBufferData(GL_ARRAY_BUFFER, screenIsoPoints.size() * sizeof(point2d), screenIsoPoints.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, isolines_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, isoIndeces.size() * sizeof(std::uint32_t), isoIndeces.data(), GL_STREAM_DRAW);


        glLineWidth(2.0f);
        glDrawElements(GL_LINES, isoIndeces.size(), GL_UNSIGNED_INT, (void*) (0));
        glUniformMatrix4fv(view_location_iso, 1, GL_TRUE, view);


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
