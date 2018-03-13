// This example is heavily based on the tutorial at https://open.gl

// OpenGL Helpers to reduce the clutter
#include "Helpers.h"

// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>

// Linear Algebra Library
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

// Timer
#include <chrono>
#include <iostream>

// VertexBufferObject wrapper
VertexBufferObject VBO;

// Contains the vertex positions
Eigen::MatrixXf V(2,1);

Eigen::Matrix4f view(4,4);

// Save color of triangle in between translations
Eigen::Matrix3f savedColors(3,3);

// Save position of triangle for keyframe animation, start and end
Eigen::MatrixXf savedEndTrianglePosition(2,3);
Eigen::MatrixXf savedStartTrianglePosition(2,3);

// Tracks the number of vertices when adding triangles to the scene
int vColumnIndex = 0;

// Tracks the rightmost column in V of the triangle being dragged
int triangleToTranslate = -1;

// Tracks the vertex to be colored in colorMode
int vertexToColor = -1;

// Different interaction modes
bool insertionMode, translationMode, deleteMode, colorMode, keyFrameMode = false;

// In process of dragging a triangle or having the vertex follow the cursor
bool drawingRadius, currentlySeeking = false;
bool addingVelocity = false;
bool drawWithGravity = false;
bool pauseSim = false;
int cycle = 0;

// Tracks the stage of insertion mode, looping back to zero after 3
int insertionModeClickCount = 0;

// Tracks the stage of keyFrame moveBack()
int moveBackClickCount = 0;

// Track previous cursor position
Eigen::Vector2d lastCursorPosition(2);

float aspect_ratio;
float x_offset, y_offset = 0;

Eigen::Vector4f mouse;
Eigen::Vector2f newVelocity;

int render = 0;
float renderSpeed = 1;

float circles[20] = {-1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,
                     -1.0, -1.0,};

float velocity[20] = {0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,
                      0.0,0.0,};

float radius[10]   = {0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      0.0,};
int newSphereClickCount = 0;

void update_with_gravity(float c[], int length)
{
    for (int i = 0; i < length; i++)             // update each x
    {
        float towardsOne = 1 - circles[i];
        float frac = towardsOne/7500;
        velocity[i] += frac;
        c[i] += velocity[i]/renderSpeed;
    }
}

void update_without_gravity(float c[], int length)
{
    for (int i = 0; i < length; i++)
    {
        c[i] += velocity[i]/renderSpeed;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    Eigen::Vector4f p_screen((float)xpos,(float)(height-1-ypos),0,1);

    if (cycle == 1)
    {
        Eigen::Vector2f currentMousePosition;
        currentMousePosition[0] = p_screen[0]*2/width;
        currentMousePosition[1] = p_screen[1]*2/height;
        float deltaX = currentMousePosition[0] - circles[newSphereClickCount-2];
        float deltaY = currentMousePosition[1] - circles[newSphereClickCount-1];

        radius[newSphereClickCount/2 - 1] = sqrtf((deltaX*deltaX)+(deltaY*deltaY));
    }

    if (cycle == 2)
    {
        Eigen::Vector2f currentMousePosition;
        currentMousePosition[0] = p_screen[0]*2/width;
        currentMousePosition[1] = p_screen[1]*2/height;
        float deltaX = currentMousePosition[0] - circles[newSphereClickCount-2];
        float deltaY = currentMousePosition[1] - circles[newSphereClickCount-1];

        newVelocity[0] = deltaX/100;
        newVelocity[1] = deltaY/100;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    Eigen::Vector4f p_screen((float)xpos,(float)(height-1-ypos),0,1);
    Eigen::Vector4f p_canonical((p_screen[0]/width)*2-1,(p_screen[1]/height)*2-1,0,1);
    Eigen::Vector4f p_world = view.inverse()*p_canonical;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (newSphereClickCount >= 20) newSphereClickCount = 0;
        if (cycle == 0) {
            std::cout << "press:" << newSphereClickCount << std::endl;
            velocity[newSphereClickCount] = 0;
            velocity[newSphereClickCount + 1] = 0;
            circles[newSphereClickCount++] = (float) (2 * xpos / width);
            circles[newSphereClickCount++] = (float) (2 * (1 - ypos / height));
            radius[newSphereClickCount / 2 - 1] = 0.0;
            drawingRadius = true;
            cycle = 1;
        } else if (cycle == 2) {
            velocity[newSphereClickCount-2] = newVelocity[0];
            velocity[newSphereClickCount-1] = newVelocity[1];
            addingVelocity = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        drawingRadius = false;
        addingVelocity = true;
        if (cycle == 2) cycle = 0;
        if (cycle == 1) cycle = 2;

    }
}

void resetCircles()
{
    for (int i = 0; i < 20; i++)
    {
        circles[i] = -1.0;
    }
}

void resetVelocities()
{
    for (int i = 0; i < 20; i++)
    {
        velocity[i] = 0.0;
    }
}

void resetRadii()
{
    for (int i = 0; i < 10; i++)
    {
        radius[i] = 0.0;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    switch (key)
    {
        case GLFW_KEY_R:
            if (action == GLFW_PRESS)
            {
                resetCircles();
                resetVelocities();
                resetRadii();
                newSphereClickCount = 0;
                cycle = 0;
            }
            break;
        case GLFW_KEY_W:
            if (action == GLFW_PRESS)
            {
                drawWithGravity = !drawWithGravity;
            }
            break;
        case GLFW_KEY_P:
            if (action == GLFW_PRESS)
            {
                pauseSim = !pauseSim;
            }
            break;
        case GLFW_KEY_E:
            if (action == GLFW_PRESS)
            {
                if (render == 0) {
                    render = 1;
                } else {
                    render = 0;
                }
            }
            break;
        case GLFW_KEY_1:
            if (action == GLFW_PRESS)
            {
                renderSpeed = 1;
            }
            break;
        case GLFW_KEY_2:
            if (action == GLFW_PRESS)
            {
                renderSpeed = .75;
            }
            break;
        case GLFW_KEY_3:
            if (action == GLFW_PRESS)
            {
                renderSpeed = .5;
            }
            break;
        case GLFW_KEY_4:
            if (action == GLFW_PRESS)
            {
                renderSpeed = .25;
            }
            break;
        default:
            break;
    }
    VBO.update(V);
}

int main()
{
    GLFWwindow* window;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 640, "Fragment Shader AF", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Initialize the VAO
    // A Vertex Array Object (or VAO) is an object that describes how the vertex
    // attributes are stored in a Vertex Buffer Object (or VBO). This means that
    // the VAO is not the actual object storing the vertex data,
    // but the descriptor of the vertex data.
    VertexArrayObject VAO;
    VAO.init();
    VAO.bind();

    // Initialize the VBO with the vertices data
    // A VBO is a data container that lives in the GPU memory
    VBO.init();

    V.resize(2,6);
    V.col(0) << -1, 1;
    V.col(1) <<  1, 1;
    V.col(2) << -1,-1;
    V.col(3) <<  1, 1;
    V.col(4) <<  1,-1;
    V.col(5) << -1,-1;

    VBO.update(V);

    // Initialize the OpenGL Program
    // A program controls the OpenGL pipeline and it must contains
    // at least a vertex shader and a fragment shader to be valid
    Program program;
    const GLchar* vertex_shader =
            "#version 150 core\n"
                    "in vec2 position;"
                    "in vec3 color;"
                    "out vec3 f_color;"
                    "uniform mat4 view;"
                    "void main()"
                    "{"
                    "    gl_Position = view * vec4(position, 0.0, 1.0);"
                    "    f_color = color;"
                    "}";
    const GLchar* fragment_shader =
            "#version 150 core\n"
                    "out vec4 outColor;"
                    "uniform vec2 u_resolution;"
                    "uniform vec2 circles[10];"
                    "uniform float radii[10];"
                    "uniform int render;"

                    "struct circ"
                    "{"
                    "   float radius;"
                    "   vec2  pos;"
                    "};"

                    "circ c1  = circ(radii[0],vec2(circles[0][0],circles[0][1]));"
                    "circ c2  = circ(radii[1],vec2(circles[1][0],circles[1][1]));"
                    "circ c3  = circ(radii[2],vec2(circles[2][0],circles[2][1]));"
                    "circ c4  = circ(radii[3],vec2(circles[3][0],circles[3][1]));"
                    "circ c5  = circ(radii[4],vec2(circles[4][0],circles[4][1]));"
                    "circ c6  = circ(radii[5],vec2(circles[5][0],circles[5][1]));"
                    "circ c7  = circ(radii[6],vec2(circles[6][0],circles[6][1]));"
                    "circ c8  = circ(radii[7],vec2(circles[7][0],circles[7][1]));"
                    "circ c9  = circ(radii[8],vec2(circles[8][0],circles[8][1]));"
                    "circ c10 = circ(radii[9],vec2(circles[9][0],circles[9][1]));"

                    "circ circleArray[10] = circ[10](c1,c2,c3,c4,c5,c6,c7,c8,c9,c10);"

                    "float drawCircle(in vec2 _st, in circ _c){"
                    "   return 1.0-smoothstep(_c.radius-(_c.radius*0.95),"
                    "                         _c.radius+(_c.radius*0.2),"
                    "                         distance(_st,vec2(_c.pos)));"
                    "}"

                    "float smin(float a, float b)"
                    "{"
                    "   float k = 1;"
                    "   float h = clamp(0.5 + 0.5*(a-b)/k, 0.0, 1.0);"
                    "   return mix(a,b,h) - k*h*(1.0-h);"
                    "}"

                    "vec3 color;"
                    "void main()"
                    "{"
                    "   vec2 st = gl_FragCoord.xy/u_resolution;"
                    ""
                    "   if (render == 0) {"
                    "       for(int i=0;i<10;i++)"
                    "       {"
                    "           color = max(color,vec3(drawCircle(st,circleArray[i])));"
                    "       }"
                    "   }"
                    "   if (render == 1) {"
                    "       for (int i=0;i<10;i++)"
                    "       {"
                    "           color += vec3(drawCircle(st,circleArray[i]));"
                    "       }"
                    "   }"
                    "   outColor = vec4(color[0]*.75, color[1]*.75, color[2],1.0);"
                    "}";

    // Compile the two shaders and upload the binary to the GPU
    // Note that we have to explicitly specify that the output "slot" called outColor
    // is the one that we want in the fragment buffer (and thus on screen)
    program.init(vertex_shader,fragment_shader,"outColor");
    program.bind();

    // The vertex shader wants the position of the vertices as an input.
    // The following line connects the VBO we defined above with the position "slot"
    // in the vertex shader
    program.bindVertexAttribArray("position",VBO);

    // Save the current time
    auto t_start = std::chrono::high_resolution_clock::now();

    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Register cursor position callback
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Get size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    aspect_ratio = float(height)/float(width);

    view <<
         aspect_ratio,0, 0, 0,
            0,           aspect_ratio, 0, 0,
            0,           0, 1, 0,
            0,           0, 0, 1;



    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    { VAO.bind();
        program.bind();
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        glUniform3f(program.uniform("triangleColor"), (float)(sin(time * 4.0f) + 1.0f) / 2.0f, 0.0f, 0.0f);
        glUniform1f(program.uniform("u_time"), time);
        glUniformMatrix4fv(program.uniform("view"), 1, GL_FALSE, view.data());
        glUniform2f(program.uniform("u_resolution"), 640,640);
        glUniform4f(program.uniform("mouse_coords"), mouse[0], mouse[1], mouse[2], mouse[3]);
        glUniform1i(program.uniform("render"), render);
        glUniform2fv(program.uniform("circles"),10,circles);
        glUniform1fv(program.uniform("radii"),  10,radius);

        if (!pauseSim) {
            if (drawWithGravity) {
                update_with_gravity(circles, 20);
            } else {
                update_without_gravity(circles, 20);
            }
        }

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Deallocate opengl memory
    program.free();
    VAO.free();
    VBO.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}
