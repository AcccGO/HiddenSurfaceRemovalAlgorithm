#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm.hpp>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

// Data
extern GLFWwindow*  g_Window;
extern double       g_Time;
extern bool         g_MousePressed[3];
extern float        g_MouseWheel;
extern GLuint       g_FontTexture;
extern int          g_ShaderHandle, g_VertHandle, g_FragHandle;
extern int          g_AttribLocationTex, g_AttribLocationProjMtx;
extern int          g_AttribLocationPosition, g_AttribLocationUV, g_AttribLocationColor;
extern unsigned int g_VboHandle, g_VaoHandle, g_ElementsHandle;

extern double last_mouse_x;
extern double last_mouse_y;
extern bool   mouse_click;

extern float     rotate_angle_x;
extern float     rotate_angle_y;
extern glm::vec3 obj_pos;
extern glm::vec3 camera_pos;
extern glm::vec3 camera_lookat;

extern glm::vec3 model_color;