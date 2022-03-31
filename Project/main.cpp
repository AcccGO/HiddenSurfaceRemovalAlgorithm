#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <imgui.h>
#include <stdio.h>.
#include <time.h>

#include "IntervalScannerLineZbuffer.h"
#include "Model.h"
#include "Parameter.h"
#include "ScannerLineZbuffer.h"
#include "imgui_impl_glfw_gl3.h"

using namespace scanner_line;

GLubyte* pixels;
// Setup window.
int window_size  = 600;  // It would stretch if it wasn't a square.
GLuint bg_tex_id = 0;

GLuint load_texture(unsigned char* f)
{
    assert(f);

    glBindTexture(GL_TEXTURE_2D, bg_tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_size, window_size, 0, GL_RGB, GL_UNSIGNED_BYTE, f);

    return bg_tex_id;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    if (mouse_click) {
        float ox = x - last_mouse_x;
        float oy = y - last_mouse_y;

        rotate_angle_y -= ox / 100;
        rotate_angle_x += oy / 100;

        camera_pos = glm::vec3(cos(rotate_angle_x) * sin(rotate_angle_y), sin(rotate_angle_x), cos(rotate_angle_x) * cos(rotate_angle_y));

        last_mouse_x = x;
        last_mouse_y = y;
    }
}

int main(int, char**)
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(window_size, window_size, "CG_Project_Autumn", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        return 0;
    }

    // Setup ImGui binding.
    ImGui_ImplGlfwGL3_Init(window, true);

    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Load model.
    model::Model model_bunny(0);        // bunny
    model::Model model_teapot(1, 0.2);  // teapot
    model::Model model_ball(2);         // ball
    model::Model model_deer(3, 0.8);    // al
    model::Model model_special(4);      // special model

    // Imgui options.
    model::Model* current_model = nullptr;
    current_model               = &model_ball;
    int current_al_op           = 0;  // 0 represents scan line Zbuffer, and 1 represents interval scan line.
    string al;
    string current_model_name    = "ball";
    unsigned char* result        = nullptr;
    ImVec4 clear_color           = ImColor(114, 144, 154);
    float size                   = 0.0f;
    int last_time                = 0;
    static ImTextureID im_tex_id = 0;
    // Scannner line.
    scanner_line_zbuffer::ScannerLineZbuffer slz(current_model, window_size, window_size);
    // Interval scanner line.
    interval_scanner_line_zbuffer::IntervalScannerLineZbuffer islz(current_model, window_size);

    // Main loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        slz.SetClearColor(glm::vec3(clear_color.x, clear_color.y, clear_color.z));
        islz.SetClearColor(glm::vec3(clear_color.x, clear_color.y, clear_color.z));

        if (current_al_op == 0) {
            al = "Current algorithm: scan line z_buffer.";

            last_time = slz.ScanModel(window_size, window_size);

            result = slz.frame_buffer;
        } else if (current_al_op == 1) {
            al = "Current algorithm: interval scan line.";

            last_time = islz.ScanModel(window_size, window_size);

            result = islz.frame_buffer;
        } else if (current_al_op == -1) {
            al        = "Current algorithm: none";
            result    = nullptr;
            last_time = 0;
        }

        ImGui::Text(al.c_str());
        string t = "Last scanning time was " + to_string(last_time) + " ms";
        ImGui::Text((t).c_str());

        ImGui::Text("Choose Algorithm: ");
        ImGui::SameLine();
        if (ImGui::Button("scan line z_buffer", ImVec2(150, 25))) {
            current_al_op = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("interval scan line", ImVec2(150, 25))) {
            current_al_op = 1;
        }

        ImGui::NewLine();
        string temp = "Current Model: " + current_model_name + ", Vertex:" + to_string(current_model->obj_vectexs.size()) + ", Face: " + to_string(current_model->obj_faces.size());
        ImGui::Text(temp.c_str());
        ImGui::Text("Choose Model: ");
        ImGui::SameLine();
        if (ImGui::Button("ball", ImVec2(80, 25))) {
            rotate_angle_x = 0;
            rotate_angle_y = 0;

            current_model_name = "ball";
            current_model      = &model_ball;
            slz.ResetModel(current_model);
            islz.ResetModel(current_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("teapot", ImVec2(80, 25))) {
            rotate_angle_x = 0;
            rotate_angle_y = 0;

            current_model_name = "teapot";
            current_model      = &model_teapot;
            slz.ResetModel(current_model);
            islz.ResetModel(current_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("deer", ImVec2(80, 25))) {
            rotate_angle_x = 0;
            rotate_angle_y = 0;

            current_model_name = "deer";
            current_model      = &model_deer;
            slz.ResetModel(current_model);
            islz.ResetModel(current_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("special", ImVec2(80, 25))) {
            rotate_angle_x = 0;
            rotate_angle_y = 0;

            current_model_name = "special";
            current_model      = &model_special;
            slz.ResetModel(current_model);
            islz.ResetModel(current_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("bunny", ImVec2(80, 25))) {
            rotate_angle_x = 0;
            rotate_angle_y = 0;

            current_model_name = "bunny";
            current_model      = &model_bunny;
            slz.ResetModel(current_model);
            islz.ResetModel(current_model);
        }

        ImGui::ColorEdit3("Model Color", (float*)&model_color);
        ImGui::ColorEdit3("Clear Color", (float*)&clear_color);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));

        if (result != nullptr) {
            bg_tex_id = load_texture(result);
            im_tex_id = (GLuint*)bg_tex_id;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("result", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            ImGui::Image(im_tex_id, ImGui::GetContentRegionAvail(), ImVec2(1, 1), ImVec2(0, 0));
            ImGui::End();
            ImGui::PopStyleVar();
        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
