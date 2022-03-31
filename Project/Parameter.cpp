#include "Parameter.h"

// Data
GLFWwindow*  g_Window          = NULL;
double       g_Time            = 0.0f;
bool         g_MousePressed[3] = {false, false, false};
float        g_MouseWheel      = 0.0f;
GLuint       g_FontTexture     = 0;
int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

double last_mouse_x = 0;
double last_mouse_y = 0;
bool   mouse_click  = false;

float     rotate_angle_x = 0;
float     rotate_angle_y = 0;
glm::vec3 obj_pos        = glm::vec3(0.0, 0.0, 0.0);
glm::vec3 camera_pos     = glm::vec3(0.0, 0.0, 1.0);
glm::vec3 camera_lookat  = glm::vec3(0.0, 0.0, 0.0);

glm::vec3 model_color = glm::vec3(0.8);