#define STB_IMAGE_IMPLEMENTATION
#define _USE_MATH_DEFINES
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader_s.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "camera.h"
#include <math.h>
#include <time.h>

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;
const int pointLightCount = 3;
const int cubeStaticCount = 15;
const int rec = 4; 

// cameras
Camera cameraWorld(glm::vec3(-2.0f, 9.0f, 30.0f), glm::vec3(0.0f, 1.0f, 0.0f), -83.0f, -20.0f);
Camera cameraFollowing(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f);
Camera cameraObject(glm::vec3(0.0f, 0.8f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
int usedCamera = 1; // 1 - world, 2 - following, 3 - object
float yaw = -90.0f;
float beginYaw = -90.0f;
int currMove = 0; // 0 - A->B, 1 - B->C, 2 - C->D, 3 - D->A, 4 - inbetween (waiting for proper time)
float lastTime = 0.0f;

// shaders
Shader* shaderObjInUse; // pointer to used shader
Shader* shaderGrdInUse;
Shader* shaderSphereInUse;
Shader* objectTexture;
Shader* objectPhong;
Shader* objectGouraud;
Shader* groundTexture;
Shader* groundPhong;
Shader* groundGouraud;
Shader* spherePhong;
Shader* sphereGouraud;

// timing and movement
float deltaTime = 0.0f;
float lastFrame = 0.0f;
clock_t daynightTimer;
glm::vec3 currentPos; // for moving object
glm::vec3 currentCamPos; // for object's camera
float rotated = 0.0f;
bool restartTime = true; // nie restartuje czasu, oznacza restartowanie rotacji obiektu

// functions
unsigned int loadTexture(char const* path);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void processInput(GLFWwindow* window);
void set_directional_light(Shader shader);
void set_point_light(Shader shader, glm::vec3* position);
void set_spotlights(Shader shader, glm::vec3* positions);
void set_flashlight(Shader shader, glm::vec3 position);
std::vector<float> calcVertex(float x, float y, float z);
std::vector<float> getMiddlePoint(float x1, float y1, float z1, float x2, float y2, float z2);

int main()
{
	// initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GK zadanie 4", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	//glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Shader objectTextureShader("normmapshader.vs", "normmapshaderfrag.vs");
	Shader objectPhongShader("colorphong.vs", "colorphongfrag.vs");
	Shader objectGouraudShader("colorgouraud.vs", "colorgouraudfrag.vs");
	Shader groundTextureShader("normmapshader.vs", "normmapshaderfrag.vs");
	Shader groundPhongShader("colorphong.vs", "colorphongfrag.vs");
	Shader groundGouraudShader("colorgouraud.vs", "colorgouraudfrag.vs");
	Shader lightSourceShader("basicshader.vs", "basicshaderfrag.vs");
	Shader spherePhongShader("colorphong.vs", "colorphongfrag.vs");
	Shader sphereGouraudShader("colorgouraud.vs", "colorgouraudfrag.vs");

	// vertices
	//----------------------------
	// ground
	// positions
	glm::vec3 pos1(-0.5f, 0.0f, -0.5f);
	glm::vec3 pos2(0.5f, 0.0f, -0.5f);
	glm::vec3 pos3(0.5f, 0.0f, 0.5f);
	glm::vec3 pos4(-0.5f, 0.0f, 0.5f);
	// texture coordinates
	glm::vec2 uv1(0.0f, 1.0f);
	glm::vec2 uv2(1.0f, 1.0f);
	glm::vec2 uv3(1.0f, 0.0f);
	glm::vec2 uv4(0.0f, 0.0f);
	// normal vector
	glm::vec3 nm(0.0f, 1.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent1, bitangent1;
	glm::vec3 tangent2, bitangent2;

	// triangle 1
	// ----------
	glm::vec3 edge1 = pos2 - pos1;
	glm::vec3 edge2 = pos3 - pos1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos3 - pos1;
	edge2 = pos4 - pos1;
	deltaUV1 = uv3 - uv1;
	deltaUV2 = uv4 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	float groundVertices[] = {
		// positions            // normal         // texcoords  // tangent                          // bitangent
		pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
		pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
		pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

		pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
		pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
		pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
	};
	//----------------------------
	// cube
	//----------------------------
	// side1
	// positions
	glm::vec3 pos11(-0.5f, -0.5f, -0.5f);
	glm::vec3 pos12(0.5f, -0.5f, -0.5f);
	glm::vec3 pos13(0.5f, 0.5f, -0.5f);
	glm::vec3 pos14(-0.5f, 0.5f, -0.5f);
	// texture coordinates
	glm::vec2 uv11(0.0f, 0.0f);
	glm::vec2 uv12(1.0f, 0.0f);
	glm::vec2 uv13(1.0f, 1.0f);
	glm::vec2 uv14(0.0f, 1.0f);
	// normal vector
	glm::vec3 nm1(0.0f, 0.0f, -1.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent11, bitangent11;
	glm::vec3 tangent12, bitangent12;

	// triangle 1
	// ----------
	edge1 = pos12 - pos11;
	edge2 = pos13 - pos11;
	deltaUV1 = uv12 - uv11;
	deltaUV2 = uv13 - uv11;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent11.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent11.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent11.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent11.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent11.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent11.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos13 - pos11;
	edge2 = pos14 - pos11;
	deltaUV1 = uv13 - uv11;
	deltaUV2 = uv14 - uv11;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent12.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent12.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent12.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent12.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent12.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent12.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	// side2
	// positions
	glm::vec3 pos21(-0.5f, -0.5f, 0.5f);
	glm::vec3 pos22(0.5f, -0.5f, 0.5f);
	glm::vec3 pos23(0.5f, 0.5f, 0.5f);
	glm::vec3 pos24(-0.5f, 0.5f, 0.5f);
	// texture coordinates
	glm::vec2 uv21(0.0f, 0.0f);
	glm::vec2 uv22(1.0f, 0.0f);
	glm::vec2 uv23(1.0f, 1.0f);
	glm::vec2 uv24(0.0f, 1.0f);
	// normal vector
	glm::vec3 nm2(0.0f, 0.0f, 1.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent21, bitangent21;
	glm::vec3 tangent22, bitangent22;

	// triangle 1
	// ----------
	edge1 = pos22 - pos21;
	edge2 = pos23 - pos21;
	deltaUV1 = uv22 - uv21;
	deltaUV2 = uv23 - uv21;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent21.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent21.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent21.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent21.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent21.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent21.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos23 - pos21;
	edge2 = pos24 - pos21;
	deltaUV1 = uv23 - uv21;
	deltaUV2 = uv24 - uv21;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent22.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent22.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent22.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent22.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent22.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent22.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	// side3
	// positions
	glm::vec3 pos31(-0.5f, 0.5f, 0.5f);
	glm::vec3 pos32(-0.5f, 0.5f, -0.5f);
	glm::vec3 pos33(-0.5f, -0.5f, -0.5f);
	glm::vec3 pos34(-0.5f, -0.5f, 0.5f);
	// texture coordinates
	glm::vec2 uv31(1.0f, 0.0f);
	glm::vec2 uv32(1.0f, 1.0f);
	glm::vec2 uv33(0.0f, 1.0f);
	glm::vec2 uv34(0.0f, 0.0f);
	// normal vector
	glm::vec3 nm3(-1.0f, 0.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent31, bitangent31;
	glm::vec3 tangent32, bitangent32;

	// triangle 1
	// ----------
	edge1 = pos32 - pos31;
	edge2 = pos33 - pos31;
	deltaUV1 = uv32 - uv31;
	deltaUV2 = uv33 - uv31;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent31.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent31.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent31.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent31.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent31.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent31.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos33 - pos31;
	edge2 = pos34 - pos31;
	deltaUV1 = uv33 - uv31;
	deltaUV2 = uv34 - uv31;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent32.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent32.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent32.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent32.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent32.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent32.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	// side4
	// positions
	glm::vec3 pos41(0.5f, 0.5f, 0.5f);
	glm::vec3 pos42(0.5f, 0.5f, -0.5f);
	glm::vec3 pos43(0.5f, -0.5f, -0.5f);
	glm::vec3 pos44(0.5f, -0.5f, 0.5f);
	// texture coordinates
	glm::vec2 uv41(1.0f, 0.0f);
	glm::vec2 uv42(1.0f, 1.0f);
	glm::vec2 uv43(0.0f, 1.0f);
	glm::vec2 uv44(0.0f, 0.0f);
	// normal vector
	glm::vec3 nm4(1.0f, 0.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent41, bitangent41;
	glm::vec3 tangent42, bitangent42;

	// triangle 1
	// ----------
	edge1 = pos42 - pos41;
	edge2 = pos43 - pos41;
	deltaUV1 = uv42 - uv41;
	deltaUV2 = uv43 - uv41;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent41.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent41.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent41.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent41.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent41.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent41.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos43 - pos41;
	edge2 = pos44 - pos41;
	deltaUV1 = uv43 - uv41;
	deltaUV2 = uv44 - uv41;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent42.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent42.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent42.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent42.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent42.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent42.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	// side5
	// positions
	glm::vec3 pos51(-0.5f, -0.5f, -0.5f);
	glm::vec3 pos52(0.5f, -0.5f, -0.5f);
	glm::vec3 pos53(0.5f, -0.5f, 0.5f);
	glm::vec3 pos54(-0.5f, -0.5f, 0.5f);
	// texture coordinates
	glm::vec2 uv51(0.0f, 1.0f);
	glm::vec2 uv52(1.0f, 1.0f);
	glm::vec2 uv53(1.0f, 0.0f);
	glm::vec2 uv54(0.0f, 0.0f);
	// normal vector
	glm::vec3 nm5(0.0f, -1.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent51, bitangent51;
	glm::vec3 tangent52, bitangent52;

	// triangle 1
	// ----------
	edge1 = pos52 - pos51;
	edge2 = pos53 - pos51;
	deltaUV1 = uv52 - uv51;
	deltaUV2 = uv53 - uv51;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent51.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent51.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent51.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent51.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent51.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent51.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos53 - pos51;
	edge2 = pos54 - pos51;
	deltaUV1 = uv53 - uv51;
	deltaUV2 = uv54 - uv51;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent52.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent52.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent52.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent52.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent52.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent52.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	// side6
	// positions
	glm::vec3 pos61(-0.5f, 0.5f, -0.5f);
	glm::vec3 pos62(0.5f, 0.5f, -0.5f);
	glm::vec3 pos63(0.5f, 0.5f, 0.5f);
	glm::vec3 pos64(-0.5f, 0.5f, 0.5f);
	// texture coordinates
	glm::vec2 uv61(0.0f, 1.0f);
	glm::vec2 uv62(1.0f, 1.0f);
	glm::vec2 uv63(1.0f, 0.0f);
	glm::vec2 uv64(0.0f, 0.0f);
	// normal vector
	glm::vec3 nm6(0.0f, 1.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent61, bitangent61;
	glm::vec3 tangent62, bitangent62;

	// triangle 1
	// ----------
	edge1 = pos62 - pos61;
	edge2 = pos63 - pos61;
	deltaUV1 = uv62 - uv61;
	deltaUV2 = uv63 - uv61;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent61.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent61.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent61.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent61.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent61.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent61.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

	// triangle 2
	// ----------
	edge1 = pos63 - pos61;
	edge2 = pos64 - pos61;
	deltaUV1 = uv63 - uv61;
	deltaUV2 = uv64 - uv61;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent62.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent62.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent62.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	bitangent62.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent62.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent62.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


	float cubeVertices[] = {
		// positions				// normals			  // texcoords	  // tangent                          // bitangent
		pos11.x, pos11.y, pos11.z,  nm1.x, nm1.y, nm1.z,  uv11.x, uv11.y, tangent11.x, tangent11.y, tangent11.z, bitangent11.x, bitangent11.y, bitangent11.z,
		pos12.x, pos12.y, pos12.z,  nm1.x, nm1.y, nm1.z,  uv12.x, uv12.y, tangent11.x, tangent11.y, tangent11.z, bitangent11.x, bitangent11.y, bitangent11.z,
		pos13.x, pos13.y, pos13.z,  nm1.x, nm1.y, nm1.z,  uv13.x, uv13.y, tangent11.x, tangent11.y, tangent11.z, bitangent11.x, bitangent11.y, bitangent11.z,
		pos11.x, pos11.y, pos11.z,  nm1.x, nm1.y, nm1.z,  uv11.x, uv11.y, tangent12.x, tangent12.y, tangent12.z, bitangent12.x, bitangent12.y, bitangent12.z,
		pos13.x, pos13.y, pos13.z,  nm1.x, nm1.y, nm1.z,  uv13.x, uv13.y, tangent12.x, tangent12.y, tangent12.z, bitangent12.x, bitangent12.y, bitangent12.z,
		pos14.x, pos14.y, pos14.z,  nm1.x, nm1.y, nm1.z,  uv14.x, uv14.y, tangent12.x, tangent12.y, tangent12.z, bitangent12.x, bitangent12.y, bitangent12.z,

		pos21.x, pos21.y, pos21.z,  nm2.x, nm2.y, nm2.z,  uv21.x, uv21.y, tangent21.x, tangent21.y, tangent21.z, bitangent21.x, bitangent21.y, bitangent21.z,
		pos22.x, pos22.y, pos22.z,  nm2.x, nm2.y, nm2.z,  uv22.x, uv22.y, tangent21.x, tangent21.y, tangent21.z, bitangent21.x, bitangent21.y, bitangent21.z,
		pos23.x, pos23.y, pos23.z,  nm2.x, nm2.y, nm2.z,  uv23.x, uv23.y, tangent21.x, tangent21.y, tangent21.z, bitangent21.x, bitangent21.y, bitangent21.z,
		pos21.x, pos21.y, pos21.z,  nm2.x, nm2.y, nm2.z,  uv21.x, uv21.y, tangent22.x, tangent22.y, tangent22.z, bitangent22.x, bitangent22.y, bitangent22.z,
		pos23.x, pos23.y, pos23.z,  nm2.x, nm2.y, nm2.z,  uv23.x, uv23.y, tangent22.x, tangent22.y, tangent22.z, bitangent22.x, bitangent22.y, bitangent22.z,
		pos24.x, pos24.y, pos24.z,  nm2.x, nm2.y, nm2.z,  uv24.x, uv24.y, tangent22.x, tangent22.y, tangent22.z, bitangent22.x, bitangent22.y, bitangent22.z,

		pos31.x, pos31.y, pos31.z,  nm3.x, nm3.y, nm3.z,  uv31.x, uv31.y, tangent31.x, tangent31.y, tangent31.z, bitangent31.x, bitangent31.y, bitangent31.z,
		pos32.x, pos32.y, pos32.z,  nm3.x, nm3.y, nm3.z,  uv32.x, uv32.y, tangent31.x, tangent31.y, tangent31.z, bitangent31.x, bitangent31.y, bitangent31.z,
		pos33.x, pos33.y, pos33.z,  nm3.x, nm3.y, nm3.z,  uv33.x, uv33.y, tangent31.x, tangent31.y, tangent31.z, bitangent31.x, bitangent31.y, bitangent31.z,
		pos31.x, pos31.y, pos31.z,  nm3.x, nm3.y, nm3.z,  uv31.x, uv31.y, tangent32.x, tangent32.y, tangent32.z, bitangent32.x, bitangent32.y, bitangent32.z,
		pos33.x, pos33.y, pos33.z,  nm3.x, nm3.y, nm3.z,  uv33.x, uv33.y, tangent32.x, tangent32.y, tangent32.z, bitangent32.x, bitangent32.y, bitangent32.z,
		pos34.x, pos34.y, pos34.z,  nm3.x, nm3.y, nm3.z,  uv34.x, uv34.y, tangent32.x, tangent32.y, tangent32.z, bitangent32.x, bitangent32.y, bitangent32.z,

		pos41.x, pos41.y, pos41.z,  nm4.x, nm4.y, nm4.z,  uv41.x, uv41.y, tangent41.x, tangent41.y, tangent41.z, bitangent41.x, bitangent41.y, bitangent41.z,
		pos42.x, pos42.y, pos42.z,  nm4.x, nm4.y, nm4.z,  uv42.x, uv42.y, tangent41.x, tangent41.y, tangent41.z, bitangent41.x, bitangent41.y, bitangent41.z,
		pos43.x, pos43.y, pos43.z,  nm4.x, nm4.y, nm4.z,  uv43.x, uv43.y, tangent41.x, tangent41.y, tangent41.z, bitangent41.x, bitangent41.y, bitangent41.z,
		pos41.x, pos41.y, pos41.z,  nm4.x, nm4.y, nm4.z,  uv41.x, uv41.y, tangent42.x, tangent42.y, tangent42.z, bitangent42.x, bitangent42.y, bitangent42.z,
		pos43.x, pos43.y, pos43.z,  nm4.x, nm4.y, nm4.z,  uv43.x, uv43.y, tangent42.x, tangent42.y, tangent42.z, bitangent42.x, bitangent42.y, bitangent42.z,
		pos44.x, pos44.y, pos44.z,  nm4.x, nm4.y, nm4.z,  uv44.x, uv44.y, tangent42.x, tangent42.y, tangent42.z, bitangent42.x, bitangent42.y, bitangent42.z,

		pos51.x, pos51.y, pos51.z,  nm5.x, nm5.y, nm5.z,  uv51.x, uv51.y, tangent51.x, tangent51.y, tangent51.z, bitangent51.x, bitangent51.y, bitangent51.z,
		pos52.x, pos52.y, pos52.z,  nm5.x, nm5.y, nm5.z,  uv52.x, uv52.y, tangent51.x, tangent51.y, tangent51.z, bitangent51.x, bitangent51.y, bitangent51.z,
		pos53.x, pos53.y, pos53.z,  nm5.x, nm5.y, nm5.z,  uv53.x, uv53.y, tangent51.x, tangent51.y, tangent51.z, bitangent51.x, bitangent51.y, bitangent51.z,
		pos51.x, pos51.y, pos51.z,  nm5.x, nm5.y, nm5.z,  uv51.x, uv51.y, tangent52.x, tangent52.y, tangent52.z, bitangent52.x, bitangent52.y, bitangent52.z,
		pos53.x, pos53.y, pos53.z,  nm5.x, nm5.y, nm5.z,  uv53.x, uv53.y, tangent52.x, tangent52.y, tangent52.z, bitangent52.x, bitangent52.y, bitangent52.z,
		pos54.x, pos54.y, pos54.z,  nm5.x, nm5.y, nm5.z,  uv54.x, uv54.y, tangent52.x, tangent52.y, tangent52.z, bitangent52.x, bitangent52.y, bitangent52.z,

		pos61.x, pos61.y, pos61.z,  nm6.x, nm6.y, nm6.z,  uv61.x, uv61.y, tangent61.x, tangent61.y, tangent61.z, bitangent61.x, bitangent61.y, bitangent61.z,
		pos62.x, pos62.y, pos62.z,  nm6.x, nm6.y, nm6.z,  uv62.x, uv62.y, tangent61.x, tangent61.y, tangent61.z, bitangent61.x, bitangent61.y, bitangent61.z,
		pos63.x, pos63.y, pos63.z,  nm6.x, nm6.y, nm6.z,  uv63.x, uv63.y, tangent61.x, tangent61.y, tangent61.z, bitangent61.x, bitangent61.y, bitangent61.z,
		pos61.x, pos61.y, pos61.z,  nm6.x, nm6.y, nm6.z,  uv61.x, uv61.y, tangent62.x, tangent62.y, tangent62.z, bitangent62.x, bitangent62.y, bitangent62.z,
		pos63.x, pos63.y, pos63.z,  nm6.x, nm6.y, nm6.z,  uv63.x, uv63.y, tangent62.x, tangent62.y, tangent62.z, bitangent62.x, bitangent62.y, bitangent62.z,
		pos64.x, pos64.y, pos64.z,  nm6.x, nm6.y, nm6.z,  uv64.x, uv64.y, tangent62.x, tangent62.y, tangent62.z, bitangent62.x, bitangent62.y, bitangent62.z,
	};

	glm::vec3 cubePositions[] = {
		glm::vec3(7.0f, 0.5f, 8.5f),
		glm::vec3(0.0f, 0.5f, 0.0f),
		glm::vec3(-12.0f, 0.5f, 0.0f),
		glm::vec3(-12.0f, 0.5f, -1.0f),
		glm::vec3(-12.0f, 0.5f, 1.0f),
		glm::vec3(-12.0f, 1.5f, -1.0f),
		glm::vec3(-12.0f, 1.5f, 1.0f),
		glm::vec3(-10.0f, 0.5f, -17.0f),
		glm::vec3(-10.0f, 1.5f, -17.0f),
		glm::vec3(-9.0f, 0.5f, -17.0f),
		glm::vec3(19.5f, 0.5f, -5.0f),
		glm::vec3(19.5f, 0.5f, -4.0f),
		glm::vec3(19.5f, 0.5f, -3.0f),
		glm::vec3(19.5f, 0.5f, -2.0f),
		glm::vec3(19.5f, 0.5f, -1.0f)
	};
	currentPos = glm::vec3(-7.0f, 0.5f, 7.0f);
	currentCamPos = glm::vec3(-7.0f, 0.8f, 6.5f);

	glm::vec3 pointLightPositions[] = {
		glm::vec3(17.0f, 0.1f, -4.0f),
		glm::vec3(6.0f, 0.1f, 9.0f),
		glm::vec3(-10.8f, 0.1f, 0.5f)
	};
	glm::vec3 spotlightsPositions[] = {
		glm::vec3(-8.5f, 3.0f, -17.0f),
		glm::vec3(8.5f, 4.5f, 7.5f)
	};

	//---------------------------------
	// sphere
	float t = (1.0f + sqrt(5.0f)) / 2.0f;
	std::vector<float> vertices;

	// 12 base vertices
	std::vector<float> temp = calcVertex(-1.0f, t, 0.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(1.0f, t, 0.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(-1.0f, -t, 0.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(1.0f, -t, 0.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);

	temp = calcVertex(0.0f, -1.0f, t);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(0.0f, 1.0f, t);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(0.0f, -1.0f, -t);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(0.0f, 1.0f, -t);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);

	temp = calcVertex(t, 0.0f, -1.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(t, 0.0f, 1.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(-t, 0.0f, -1.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);
	temp = calcVertex(-t, 0.0f, 1.0f);
	vertices.push_back(temp[0]); vertices.push_back(temp[1]); vertices.push_back(temp[2]);

	// 20 base triangles - 9 floats per triangle
	// i-ty vertice : vertices[i*3 / i*3 + 1 / i*3 + 2]
	std::vector<float> facesT;
	facesT.push_back(vertices[0]); facesT.push_back(vertices[1]); facesT.push_back(vertices[2]);
	facesT.push_back(vertices[11*3]); facesT.push_back(vertices[11*3 + 1]); facesT.push_back(vertices[11*3+2]);
	facesT.push_back(vertices[5 * 3]); facesT.push_back(vertices[5 * 3 + 1]); facesT.push_back(vertices[5 * 3 + 2]);

	facesT.push_back(vertices[0 * 3]); facesT.push_back(vertices[0 * 3 + 1]); facesT.push_back(vertices[0 * 3 + 2]);
	facesT.push_back(vertices[5 * 3]); facesT.push_back(vertices[5 * 3 + 1]); facesT.push_back(vertices[5 * 3 + 2]);
	facesT.push_back(vertices[1 * 3]); facesT.push_back(vertices[1 * 3 + 1]); facesT.push_back(vertices[1 * 3 + 2]);

	facesT.push_back(vertices[0 * 3]); facesT.push_back(vertices[0 * 3 + 1]); facesT.push_back(vertices[0 * 3 + 2]);
	facesT.push_back(vertices[1 * 3]); facesT.push_back(vertices[1 * 3 + 1]); facesT.push_back(vertices[1 * 3 + 2]);
	facesT.push_back(vertices[7 * 3]); facesT.push_back(vertices[7 * 3 + 1]); facesT.push_back(vertices[7 * 3 + 2]);

	facesT.push_back(vertices[0 * 3]); facesT.push_back(vertices[0 * 3 + 1]); facesT.push_back(vertices[0 * 3 + 2]);
	facesT.push_back(vertices[7 * 3]); facesT.push_back(vertices[7 * 3 + 1]); facesT.push_back(vertices[7 * 3 + 2]);
	facesT.push_back(vertices[10 * 3]); facesT.push_back(vertices[10 * 3 + 1]); facesT.push_back(vertices[10 * 3 + 2]);

	facesT.push_back(vertices[0 * 3]); facesT.push_back(vertices[0 * 3 + 1]); facesT.push_back(vertices[0 * 3 + 2]);
	facesT.push_back(vertices[10 * 3]); facesT.push_back(vertices[10 * 3 + 1]); facesT.push_back(vertices[10 * 3 + 2]);
	facesT.push_back(vertices[11 * 3]); facesT.push_back(vertices[11 * 3 + 1]); facesT.push_back(vertices[11 * 3 + 2]);

	facesT.push_back(vertices[1 * 3]); facesT.push_back(vertices[1 * 3 + 1]); facesT.push_back(vertices[1 * 3 + 2]);
	facesT.push_back(vertices[5 * 3]); facesT.push_back(vertices[5 * 3 + 1]); facesT.push_back(vertices[5 * 3 + 2]);
	facesT.push_back(vertices[9 * 3]); facesT.push_back(vertices[9 * 3 + 1]); facesT.push_back(vertices[9 * 3 + 2]);

	facesT.push_back(vertices[5 * 3]); facesT.push_back(vertices[5 * 3 + 1]); facesT.push_back(vertices[5 * 3 + 2]);
	facesT.push_back(vertices[11 * 3]); facesT.push_back(vertices[11 * 3 + 1]); facesT.push_back(vertices[11 * 3 + 2]);
	facesT.push_back(vertices[4 * 3]); facesT.push_back(vertices[4 * 3 + 1]); facesT.push_back(vertices[4 * 3 + 2]);

	facesT.push_back(vertices[11 * 3]); facesT.push_back(vertices[11 * 3 + 1]); facesT.push_back(vertices[11 * 3 + 2]);
	facesT.push_back(vertices[10 * 3]); facesT.push_back(vertices[10 * 3 + 1]); facesT.push_back(vertices[10 * 3 + 2]);
	facesT.push_back(vertices[2 * 3]); facesT.push_back(vertices[2 * 3 + 1]); facesT.push_back(vertices[2 * 3 + 2]);

	facesT.push_back(vertices[10 * 3]); facesT.push_back(vertices[10 * 3 + 1]); facesT.push_back(vertices[10 * 3 + 2]);
	facesT.push_back(vertices[7 * 3]); facesT.push_back(vertices[7 * 3 + 1]); facesT.push_back(vertices[7 * 3 + 2]);
	facesT.push_back(vertices[6 * 3]); facesT.push_back(vertices[6 * 3 + 1]); facesT.push_back(vertices[6 * 3 + 2]);

	facesT.push_back(vertices[7 * 3]); facesT.push_back(vertices[7 * 3 + 1]); facesT.push_back(vertices[7 * 3 + 2]);
	facesT.push_back(vertices[1 * 3]); facesT.push_back(vertices[1 * 3 + 1]); facesT.push_back(vertices[1 * 3 + 2]);
	facesT.push_back(vertices[8 * 3]); facesT.push_back(vertices[8 * 3 + 1]); facesT.push_back(vertices[8 * 3 + 2]);

	facesT.push_back(vertices[3 * 3]); facesT.push_back(vertices[3 * 3 + 1]); facesT.push_back(vertices[3 * 3 + 2]);
	facesT.push_back(vertices[9 * 3]); facesT.push_back(vertices[9 * 3 + 1]); facesT.push_back(vertices[9 * 3 + 2]);
	facesT.push_back(vertices[4 * 3]); facesT.push_back(vertices[4 * 3 + 1]); facesT.push_back(vertices[4 * 3 + 2]);

	facesT.push_back(vertices[3 * 3]); facesT.push_back(vertices[3 * 3 + 1]); facesT.push_back(vertices[3 * 3 + 2]);
	facesT.push_back(vertices[4 * 3]); facesT.push_back(vertices[4 * 3 + 1]); facesT.push_back(vertices[4 * 3 + 2]);
	facesT.push_back(vertices[2 * 3]); facesT.push_back(vertices[2 * 3 + 1]); facesT.push_back(vertices[2 * 3 + 2]);

	facesT.push_back(vertices[3 * 3]); facesT.push_back(vertices[3 * 3 + 1]); facesT.push_back(vertices[3 * 3 + 2]);
	facesT.push_back(vertices[2 * 3]); facesT.push_back(vertices[2 * 3 + 1]); facesT.push_back(vertices[2 * 3 + 2]);
	facesT.push_back(vertices[6 * 3]); facesT.push_back(vertices[6 * 3 + 1]); facesT.push_back(vertices[6 * 3 + 2]);

	facesT.push_back(vertices[3 * 3]); facesT.push_back(vertices[3 * 3 + 1]); facesT.push_back(vertices[3 * 3 + 2]);
	facesT.push_back(vertices[6 * 3]); facesT.push_back(vertices[6 * 3 + 1]); facesT.push_back(vertices[6 * 3 + 2]);
	facesT.push_back(vertices[8 * 3]); facesT.push_back(vertices[8 * 3 + 1]); facesT.push_back(vertices[8 * 3 + 2]);

	facesT.push_back(vertices[3 * 3]); facesT.push_back(vertices[3 * 3 + 1]); facesT.push_back(vertices[3 * 3 + 2]);
	facesT.push_back(vertices[8 * 3]); facesT.push_back(vertices[8 * 3 + 1]); facesT.push_back(vertices[8 * 3 + 2]);
	facesT.push_back(vertices[9 * 3]); facesT.push_back(vertices[9 * 3 + 1]); facesT.push_back(vertices[9 * 3 + 2]);

	facesT.push_back(vertices[4 * 3]); facesT.push_back(vertices[4 * 3 + 1]); facesT.push_back(vertices[4 * 3 + 2]);
	facesT.push_back(vertices[9 * 3]); facesT.push_back(vertices[9 * 3 + 1]); facesT.push_back(vertices[9 * 3 + 2]);
	facesT.push_back(vertices[5 * 3]); facesT.push_back(vertices[5 * 3 + 1]); facesT.push_back(vertices[5 * 3 + 2]);

	facesT.push_back(vertices[2 * 3]); facesT.push_back(vertices[2 * 3 + 1]); facesT.push_back(vertices[2 * 3 + 2]);
	facesT.push_back(vertices[4 * 3]); facesT.push_back(vertices[4 * 3 + 1]); facesT.push_back(vertices[4 * 3 + 2]);
	facesT.push_back(vertices[11 * 3]); facesT.push_back(vertices[11 * 3 + 1]); facesT.push_back(vertices[11 * 3 + 2]);

	facesT.push_back(vertices[6 * 3]); facesT.push_back(vertices[6 * 3 + 1]); facesT.push_back(vertices[6 * 3 + 2]);
	facesT.push_back(vertices[2 * 3]); facesT.push_back(vertices[2 * 3 + 1]); facesT.push_back(vertices[2 * 3 + 2]);
	facesT.push_back(vertices[10 * 3]); facesT.push_back(vertices[10 * 3 + 1]); facesT.push_back(vertices[10 * 3 + 2]);

	facesT.push_back(vertices[8 * 3]); facesT.push_back(vertices[8 * 3 + 1]); facesT.push_back(vertices[8 * 3 + 2]);
	facesT.push_back(vertices[6 * 3]); facesT.push_back(vertices[6 * 3 + 1]); facesT.push_back(vertices[6 * 3 + 2]);
	facesT.push_back(vertices[7 * 3]); facesT.push_back(vertices[7 * 3 + 1]); facesT.push_back(vertices[7 * 3 + 2]);

	facesT.push_back(vertices[9 * 3]); facesT.push_back(vertices[9 * 3 + 1]); facesT.push_back(vertices[9 * 3 + 2]);
	facesT.push_back(vertices[8 * 3]); facesT.push_back(vertices[8 * 3 + 1]); facesT.push_back(vertices[8 * 3 + 2]);
	facesT.push_back(vertices[1 * 3]); facesT.push_back(vertices[1 * 3 + 1]); facesT.push_back(vertices[1 * 3 + 2]);

	// subdivide triangles
	// rec = 4;
	for (int i = 0; i < rec; i++)
	{
		std::vector<float> facesT2;
		int currCount = facesT.size() / 9;
		// 20 ---> 20 * 4 ---> 20 * 4 * 4
		for (int j = 0; j < currCount; j++)
		{
			// j - triangle index
			// vertex 0 of triangle j: facesT[j*9 / j*9 + 1 / j*9 + 2]
			// vertex 1 of triangle j: facesT[j*9 + 3 / j*9 + 4 / j*9 + 5]
			// vertex 2 of triangle j: facesT[j*9 + 6 / j*9 + 7 / j*9 + 8]
			std::vector<float> vecA = getMiddlePoint(facesT[j*9], facesT[j*9+1], facesT[j*9+2], facesT[j * 9 + 3], facesT[j * 9 + 4], facesT[j * 9 + 5]);
			std::vector<float> vecB = getMiddlePoint(facesT[j * 9 +3], facesT[j * 9 + 4], facesT[j * 9 + 5], facesT[j * 9 + 6], facesT[j * 9 + 7], facesT[j * 9 + 8]);
			std::vector<float> vecC = getMiddlePoint(facesT[j * 9 +6], facesT[j * 9 + 7], facesT[j * 9 + 8], facesT[j * 9], facesT[j * 9 + 1], facesT[j * 9 + 2]);
			
			// we add 4 new triangles to facesT2
			facesT2.push_back(facesT[j*9]); facesT2.push_back(facesT[j * 9 + 1]); facesT2.push_back(facesT[j * 9 + 2]);
			facesT2.push_back(vecA[0]); facesT2.push_back(vecA[1]); facesT2.push_back(vecA[2]);
			facesT2.push_back(vecC[0]); facesT2.push_back(vecC[1]); facesT2.push_back(vecC[2]);

			facesT2.push_back(facesT[j * 9 + 3]); facesT2.push_back(facesT[j * 9 + 4]); facesT2.push_back(facesT[j * 9 + 5]);
			facesT2.push_back(vecB[0]); facesT2.push_back(vecB[1]); facesT2.push_back(vecB[2]);
			facesT2.push_back(vecA[0]); facesT2.push_back(vecA[1]); facesT2.push_back(vecA[2]);
			
			facesT2.push_back(facesT[j * 9 + 6]); facesT2.push_back(facesT[j * 9 + 7]); facesT2.push_back(facesT[j * 9 + 8]);
			facesT2.push_back(vecC[0]); facesT2.push_back(vecC[1]); facesT2.push_back(vecC[2]);
			facesT2.push_back(vecB[0]); facesT2.push_back(vecB[1]); facesT2.push_back(vecB[2]);

			facesT2.push_back(vecA[0]); facesT2.push_back(vecA[1]); facesT2.push_back(vecA[2]);
			facesT2.push_back(vecB[0]); facesT2.push_back(vecB[1]); facesT2.push_back(vecB[2]);
			facesT2.push_back(vecC[0]); facesT2.push_back(vecC[1]); facesT2.push_back(vecC[2]);
		}
		facesT.clear();
		facesT.insert(facesT.end(), facesT2.begin(), facesT2.end());
	}

	// w facesT mamy positions
	// calculate normals
	int currTriCount = facesT.size() / 9;
	std::vector<float> norm;
	for (int i = 0; i < currTriCount; i++)
	{
		// i - indeks trojkata dla ktorego liczymy wektor normalny
		// facesT[i*9/ +1/ +2]
		// facesT[ +3/ +4/ +5]
		// facesT[ +6/ +7/ +8]
		glm::vec3 b = glm::vec3(facesT[i*9], facesT[i*9+1], facesT[i*9+2]);
		glm::vec3 r = glm::vec3(facesT[i * 9 + 3], facesT[i * 9 + 4], facesT[i * 9 + 5]);
		glm::vec3 s = glm::vec3(facesT[i * 9 + 6], facesT[i * 9 + 7], facesT[i * 9 + 8]);
		glm::vec3 n = glm::cross(r - b, s - b);
		norm.push_back(n.x);
		norm.push_back(n.y);
		norm.push_back(n.z);
	}

	// currTriCount = 1280 for rec = 3
	// currTriCount = 5120 for rec = 4
	float sphereVertices[5120 * 3 * 6];
	for (int i = 0; i < currTriCount; i++)
	{
		sphereVertices[i * 18] = facesT[i * 9]; sphereVertices[i * 18 + 1] = facesT[i * 9 + 1]; sphereVertices[i * 18 + 2] = facesT[i * 9 + 2]; sphereVertices[i * 18 + 3] = norm[i*3]; sphereVertices[i * 18 + 4] = norm[i*3 + 1]; sphereVertices[i * 18 + 5] = norm[i*3 + 2];
		sphereVertices[i * 18 + 6] = facesT[i * 9 + 3]; sphereVertices[i * 18 + 7] = facesT[i * 9 + 4]; sphereVertices[i * 18 + 8] = facesT[i * 9 + 5]; sphereVertices[i * 18 + 9] = norm[i*3]; sphereVertices[i * 18 + 10] = norm[i*3 + 1]; sphereVertices[i * 18 + 11] = norm[i*3 + 2];
		sphereVertices[i * 18 + 12] = facesT[i * 9 + 6]; sphereVertices[i * 18 + 13] = facesT[i * 9 + 7]; sphereVertices[i * 18 + 14] = facesT[i * 9 + 8]; sphereVertices[i * 18 + 15] = norm[i*3]; sphereVertices[i * 18 + 16] = norm[i*3 + 1]; sphereVertices[i * 18 + 17] = norm[i*3 + 2];
	}

	//---------------------------------

	// sphere VAO/VBO
	unsigned int sphereVAO, sphereVBO;
	glGenVertexArrays(1, &sphereVAO);
	glGenBuffers(1, &sphereVBO);
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertices), &sphereVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// ground VAO/VBO
	unsigned int groundVAO = 0;
	unsigned int groundVBO;
	glGenVertexArrays(1, &groundVAO);
	glGenBuffers(1, &groundVBO);
	glBindVertexArray(groundVAO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), &groundVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));

	// vertice buffer & attributes objects
	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	// tangent
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
	// bitangent
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
	glEnableVertexAttribArray(4);

	// light VAO/VBO
	unsigned int lVAO;
	glGenVertexArrays(1, &lVAO);
	glBindVertexArray(lVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// loading textures
	unsigned int diffuseCubeMap = loadTexture("resources/textures/metalplatesfull.jpg");
	unsigned int normalCubeMap = loadTexture("resources/textures/metalplates_normal.jpg");
	unsigned int diffuseGroundMap = loadTexture("resources/textures/brickwall.jpg");
	unsigned int normalGroundMap = loadTexture("resources/textures/brickwall_normal.jpg");

	// setting textures
	objectTextureShader.use();
	objectTextureShader.setInt("material.diffuseMap", 0);
	objectTextureShader.setInt("material.normalMap", 1);
	objectTextureShader.setFloat("material.shininess", 32.0f);
	objectTextureShader.setBool("blinn", true);
	groundTextureShader.use();
	groundTextureShader.setInt("material.diffuseMap", 2);
	groundTextureShader.setInt("material.normalMap", 3);
	groundTextureShader.setFloat("material.shininess", 20.0f);
	groundTextureShader.setBool("blinn", true);

	// setting colors/materials
	objectPhongShader.use(); // jade
	objectPhongShader.setVec3("material.ambient", 0.135f, 0.2225f, 0.1575f);
	objectPhongShader.setVec3("material.diffuse", 0.54f, 0.89f, 0.63f);
	objectPhongShader.setVec3("material.specular", 0.316228f, 0.316228f, 0.316228f);
	objectPhongShader.setFloat("material.shininess", 0.1f * 128.0f);
	objectPhongShader.setBool("blinn", true);

	groundPhongShader.use(); // black rubber
	groundPhongShader.setVec3("material.ambient", 0.02f, 0.02f, 0.02f);
	groundPhongShader.setVec3("material.diffuse", 0.1f, 0.1f, 0.1f);
	groundPhongShader.setVec3("material.specular", 0.1f, 0.1f, 0.1f);
	groundPhongShader.setFloat("material.shininess", 0.078125f * 128.0f);
	groundPhongShader.setBool("blinn", true);

	spherePhongShader.use(); // pearl
	spherePhongShader.setVec3("material.ambient", 0.25f, 0.20725f, 0.20725f);
	spherePhongShader.setVec3("material.diffuse", 0.9f, 0.829f, 0.829f);
	spherePhongShader.setVec3("material.specular", 0.296648f, 0.296648f, 0.296648f);
	spherePhongShader.setFloat("material.shininess", 0.088f * 128.0f);
	spherePhongShader.setBool("blinn", true);

	objectGouraudShader.use();
	objectGouraudShader.setVec3("material.ambient", 0.135f, 0.2225f, 0.1575f);
	objectGouraudShader.setVec3("material.diffuse", 0.54f, 0.89f, 0.63f);
	objectGouraudShader.setVec3("material.specular", 0.316228f, 0.316228f, 0.316228f);
	objectGouraudShader.setFloat("material.shininess", 0.1f * 128.0f);
	objectGouraudShader.setBool("blinn", true);

	groundGouraudShader.use();
	groundGouraudShader.setVec3("material.ambient", 0.02f, 0.02f, 0.02f);
	groundGouraudShader.setVec3("material.diffuse", 0.1f, 0.1f, 0.1f);
	groundGouraudShader.setVec3("material.specular", 0.1f, 0.1f, 0.1f);
	groundGouraudShader.setFloat("material.shininess", 0.078125f * 128.0f);
	groundGouraudShader.setBool("blinn", true);

	sphereGouraudShader.use(); 
	sphereGouraudShader.setVec3("material.ambient", 0.25f, 0.20725f, 0.20725f);
	sphereGouraudShader.setVec3("material.diffuse", 0.9f, 0.829f, 0.829f);
	sphereGouraudShader.setVec3("material.specular", 0.296648f, 0.296648f, 0.296648f);
	sphereGouraudShader.setFloat("material.shininess", 0.088f * 128.0f);
	sphereGouraudShader.setBool("blinn", true);

	objectPhong = &objectPhongShader;
	objectGouraud = &objectGouraudShader;
	objectTexture = &objectTextureShader;
	groundPhong = &groundPhongShader;
	groundGouraud = &groundGouraudShader;
	groundTexture = &groundTextureShader;
	spherePhong = &spherePhongShader;
	sphereGouraud = &sphereGouraudShader;

	shaderObjInUse = objectPhong;
	shaderGrdInUse = groundPhong;
	shaderSphereInUse = spherePhong;

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		// rendering commands and set "sky" color
		float var = 0.3f * sin(glfwGetTime() / 4.0f) +0.4f;
		glm::vec3 sky = glm::vec3(var, var, var);
		glClearColor(var, var, var, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightSourceShader.use();
		lightSourceShader.setVec3("skyColor", sky);
		objectPhongShader.use();
		objectPhongShader.setVec3("skyColor", sky);
		groundPhongShader.use();
		groundPhongShader.setVec3("skyColor", sky);
		objectGouraudShader.use();
		objectGouraudShader.setVec3("skyColor", sky);
		groundGouraudShader.use();
		groundGouraudShader.setVec3("skyColor", sky);
		objectTextureShader.use();
		objectTextureShader.setVec3("skyColor", sky);
		groundTextureShader.use();
		groundTextureShader.setVec3("skyColor", sky);

		// binding textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseCubeMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalCubeMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, diffuseGroundMap);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, normalGroundMap);

		(*shaderObjInUse).use();
		set_directional_light(*shaderObjInUse);
		set_point_light(*shaderObjInUse, pointLightPositions);
		set_spotlights(*shaderObjInUse, spotlightsPositions); 

		(*shaderGrdInUse).use();
		set_directional_light(*shaderGrdInUse);
		set_point_light(*shaderGrdInUse, pointLightPositions);
		set_spotlights(*shaderGrdInUse, spotlightsPositions);

		(*shaderSphereInUse).use();
		set_directional_light(*shaderSphereInUse);
		set_point_light(*shaderSphereInUse, pointLightPositions);
		set_spotlights(*shaderSphereInUse, spotlightsPositions);

		glm::mat4 projection = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 modelGround = glm::mat4(1.0f);
		glm::mat4 invmodel = glm::mat4(1.0f); //inverse(model)
		glm::mat4 movingModel = glm::mat4(1.0f);
		glm::mat4 modelCube = glm::mat4(1.0f);

		// update moving object
		float currX = currentPos.x, currZ = currentPos.z;

		if (currentPos.x == -7.0f && currentPos.z > -7.0f && currentPos.z <= 7.0f && currMove == 0) // A->B
		{
			currZ = 7.0f * sin(glfwGetTime() / 3 + M_PI / 2);
			currentCamPos.z = currZ - 0.5f;
		}
		else if (currentPos.x == -7.0f && currentPos.z == -7.0f && -rotated < 1.5f && currMove == 0) // rotate in B
		{
			//if (restartTime)
			//{
				//rotated = 0.0f;
				//restartTime = false;
			//}
			//else
			//{
				// rotate right
				rotated = 1.5f * sin(glfwGetTime());
				// flashlight x and z
				yaw = beginYaw - glm::degrees(rotated);
				cameraObject.Yaw = yaw;
				currentCamPos.x = currentPos.x - 0.5*sin(glfwGetTime());
				currentCamPos.z = currentPos.z - 0.5f - 0.5*sin(glfwGetTime());
				cameraObject.updateCameraVectors();
			//}
		}
		else if (currentPos.x == -7.0f && currentPos.z == -7.0f && currMove == 0) // before B->C
		{
			// update flashlight and camera
			currentCamPos.x = currentPos.x + 0.5f;
			currentCamPos.z = currentPos.z;
			cameraObject.updateCameraVectors();
			//restartTime = true;
			beginYaw = 0.0f;
			rotated = 0.0f;
			currMove = 4;
		}
		else if (currentPos.x == -7.0f && currentPos.z == -7.0f && currMove == 4)
		{
			// waiting for proper glfwTime
			if (sin(glfwGetTime() / 3.0f + M_PI / 2) > 0.99f) // good enough
				currMove = 1;
		}
		else if (currentPos.z == -7.0f && currentPos.x >= -7.0f && currentPos.x < 7.0f && currMove == 1) // B->C
		{
			currX = -7.0f * sin(glfwGetTime() / 3 + M_PI / 2);
			currentCamPos.x = currX + 0.5f;
		}
		else if (currentPos.z == -7.0f && currentPos.x == 7.0f && -rotated < 1.5f && currMove == 1) // rotate in C
		{
			//if (restartTime)
			//{
				//glfwSetTime(0.0f);
				//rotated = 0.0f;
				//restartTime = false;
			//}
			//else
			//{
				// rotate right
				rotated = 1.5f * sin(glfwGetTime());
				// flashlight x and z
				yaw = beginYaw - glm::degrees(rotated);
				cameraObject.Yaw = yaw;
				currentCamPos.x = currentPos.x + 0.5f + 0.5*sin(glfwGetTime());
				currentCamPos.z = currentPos.z - 0.5*sin(glfwGetTime());
				cameraObject.updateCameraVectors();
			//}
		}
		else if (currentPos.x == 7.0f && currentPos.z == -7.0f && currMove == 1) // before C->D
		{
			// update flashlight and camera
			currentCamPos.x = currentPos.x;
			currentCamPos.z = currentPos.z + 0.5f;
			cameraObject.updateCameraVectors();
			//restartTime = true;
			beginYaw = 90.0f;
			rotated = 0.0f;
			currMove = 4;
		}
		else if (currentPos.x == 7.0f && currentPos.z == -7.0f && currMove == 4)
		{
			// waiting for proper glfwTime
			if (sin(glfwGetTime() / 3.0f + M_PI / 2) > 0.99f)
				currMove = 2;
		}
		else if (currentPos.x == 7.0f && currentPos.z >= -7.0f && currentPos.z < 7.0f && currMove == 2) // C->D
		{
			currZ = -7.0f * sin(glfwGetTime() / 3 + M_PI / 2);
			currentCamPos.z = currZ + 0.5f;
		}
		else if (currentPos.x == 7.0f && currentPos.z == 7.0f && -rotated < 1.5f && currMove == 2) // rotate in D
		{
			//if (restartTime)
			//{
				//rotated = 0.0f;
				//restartTime = false;
			//}
			//else
			//{
				// rotate right
				rotated = 1.5f * sin(glfwGetTime());
				// flashlight x and z
				yaw = beginYaw - glm::degrees(rotated);
				cameraObject.Yaw = yaw;
				currentCamPos.x = currentPos.x + 0.5*sin(glfwGetTime());
				currentCamPos.z = currentPos.z + 0.5f + 0.5*sin(glfwGetTime());
				cameraObject.updateCameraVectors();
			//}
		}
		else if (currentPos.x == 7.0f && currentPos.z == 7.0f && currMove == 2) // before D->A
		{
			// update flashlight and camera
			currentCamPos.x = currentPos.x - 0.5f;
			currentCamPos.z = currentPos.z;
			cameraObject.updateCameraVectors();
			//restartTime = true;
			beginYaw = 180.0f;
			rotated = 0.0f;
			currMove = 4;
		}
		else if (currentPos.x == 7.0f && currentPos.z == 7.0f && currMove == 4)
		{
			// waiting for proper glfwTime
			if (sin(glfwGetTime() / 3.0f + M_PI / 2) > 0.99f)
				currMove = 3;
		}
		else if (currentPos.z == 7.0f && currentPos.x > -7.0f && currentPos.x <= 7.0f && currMove == 3) // D->A
		{
			currX = 7.0f * sin(glfwGetTime() / 3 + M_PI / 2);
			currentCamPos.x = currX - 0.5f;
		}
		else if (currentPos.z == 7.0f && currentPos.x == -7.0f && -rotated < 1.5f && currMove == 3) // rotate in A
		{
			//if (restartTime)
			//{
				//rotated = 0.0f;
				//restartTime = false;
			//}
			//else
			//{
				// rotate right
				rotated = 1.5f * sin(glfwGetTime());
				// flashlight x and z
				yaw = beginYaw - glm::degrees(rotated);
				cameraObject.Yaw = yaw;
				currentCamPos.x = currentPos.x - 0.5f - 0.5*sin(glfwGetTime());
				currentCamPos.z = currentPos.z + 0.5*sin(glfwGetTime());
				cameraObject.updateCameraVectors();
			//}
		}
		else if (currentPos.x == -7.0f && currentPos.z == 7.0f && currMove == 3) // before A->B
		{
			// update flashlight and camera
			currentCamPos.x = currentPos.x;
			currentCamPos.z = currentPos.z - 0.5f;
			cameraObject.updateCameraVectors();
			//restartTime = true;
			beginYaw = -90.0f;
			rotated = 0.0f;
			currMove = 4;
		}
		else if (currentPos.x == -7.0f && currentPos.z == 7.0f && currMove == 4)
		{
			// waiting for proper glfwTime
			if (sin(glfwGetTime() / 3.0f + M_PI / 2) > 0.99f)
				currMove = 0;
		}


		currentPos = glm::vec3(currX, currentPos.y, currZ);
		glm::vec3 flashPos = glm::vec3(currentCamPos.x, 0.5f, currentCamPos.z);
		cameraFollowing.Position = glm::vec3(currX, 4.5f, currZ + 5.5f);
		cameraObject.Position = currentCamPos;

		// flashlight
		(*shaderObjInUse).use();
		set_flashlight(*shaderObjInUse, glm::vec3(currX, 0.5f, currZ));
		(*shaderGrdInUse).use();
		set_flashlight(*shaderGrdInUse, glm::vec3(currX, 0.5f, currZ));
		(*shaderSphereInUse).use();
		set_flashlight(*shaderSphereInUse, glm::vec3(currX, 0.5f, currZ));

		if (usedCamera == 1)
		{
			(*shaderObjInUse).use();
			(*shaderObjInUse).setVec3("viewPos", cameraWorld.Position);
			(*shaderGrdInUse).use();
			(*shaderGrdInUse).setVec3("viewPos", cameraWorld.Position);
			projection = glm::perspective(glm::radians(cameraWorld.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			view = cameraWorld.GetViewMatrix();
		}
		if (usedCamera == 2)
		{
			(*shaderObjInUse).use();
			(*shaderObjInUse).setVec3("viewPos", cameraFollowing.Position);
			(*shaderGrdInUse).use();
			(*shaderGrdInUse).setVec3("viewPos", cameraFollowing.Position);
			projection = glm::perspective(glm::radians(cameraFollowing.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			view = cameraFollowing.GetViewMatrix();
		}
		if (usedCamera == 3)
		{
			(*shaderObjInUse).use();
			(*shaderObjInUse).setVec3("viewPos", cameraObject.Position);
			(*shaderGrdInUse).use();
			(*shaderGrdInUse).setVec3("viewPos", cameraObject.Position);
			projection = glm::perspective(glm::radians(cameraObject.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			view = cameraObject.GetViewMatrix();
		}

		// draw ground
		(*shaderGrdInUse).use();
		(*shaderGrdInUse).setMat4("projection", projection);
		(*shaderGrdInUse).setMat4("view", view);
		for (int i = -20; i < 21; i+=4)
		{
			for (int j = -20; j < 21; j+=4)
			{
				modelGround = glm::mat4(1.0f);
				modelGround = glm::translate(modelGround, glm::vec3(i, 0, j));
				modelGround = glm::scale(modelGround, glm::vec3(4.0f, 4.0f, 4.0f));
				(*shaderGrdInUse).setMat4("model", modelGround);
				(*shaderGrdInUse).setMat4("invmodel", inverse(modelGround));

				glBindVertexArray(groundVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		// draw point lights objects
		lightSourceShader.use();
		lightSourceShader.setVec3("color", glm::vec3(1.0f));
		lightSourceShader.setMat4("projection", projection);
		lightSourceShader.setMat4("view", view);

		glBindVertexArray(lVAO);
		glm::mat4 lightModel = glm::mat4(1.0f);
		for (int i = 0; i < pointLightCount; i++)
		{
			lightModel = glm::mat4(1.0f);
			lightModel = glm::translate(lightModel, pointLightPositions[i]);
			lightModel = glm::scale(lightModel, glm::vec3(0.2f));
			lightSourceShader.setMat4("model", lightModel);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		lightModel = glm::mat4(1.0f);
		lightModel = glm::translate(lightModel, glm::vec3(currentCamPos.x, 0.5f, currentCamPos.z));
		lightModel = glm::scale(lightModel, glm::vec3(0.2f));
		lightSourceShader.setMat4("model", lightModel);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//draw spotlights
		for (int i = 0; i < 2; i++)
		{
			lightModel = glm::mat4(1.0f);
			lightModel = glm::translate(lightModel, spotlightsPositions[i]);
			lightModel = glm::scale(lightModel, glm::vec3(0.5f, 0.2f, 0.5f));
			lightSourceShader.setMat4("model", lightModel);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// draw boxes
		(*shaderObjInUse).use();
		(*shaderObjInUse).setMat4("projection", projection);
		(*shaderObjInUse).setMat4("view", view);
		glBindVertexArray(cubeVAO);

		// moving cube
		movingModel = glm::translate(movingModel, currentPos);
		movingModel = glm::rotate(movingModel, rotated, glm::vec3(0.0f, 1.0f, 0.0f));
		(*shaderObjInUse).setMat4("model", movingModel);
		(*shaderObjInUse).setMat4("invmodel", inverse(movingModel));
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// static cubes
		for (unsigned int i = 0; i < cubeStaticCount; i++)
		{
			modelCube = glm::translate(modelCube, cubePositions[i]);
			(*shaderObjInUse).setMat4("model", modelCube);
			(*shaderObjInUse).setMat4("invmodel", inverse(modelCube));
			glDrawArrays(GL_TRIANGLES, 0, 36);
			modelCube = glm::translate(modelCube, -cubePositions[i]);
		}

		// draw sphere
		(*shaderSphereInUse).use();
		(*shaderSphereInUse).setMat4("projection", projection);
		(*shaderSphereInUse).setMat4("view", view);
		glBindVertexArray(sphereVAO);
		glm::mat4 sphereModel = glm::mat4(1.0f);
		// pierwsza kula
		sphereModel = glm::translate(sphereModel, glm::vec3(-7.3f, 0.7f, -17.0f));
		sphereModel = glm::scale(sphereModel, glm::vec3(0.7f, 0.7f, 0.7f));
		(*shaderSphereInUse).setMat4("model", sphereModel);
		(*shaderSphereInUse).setMat4("invmodel", inverse(sphereModel));
		glDrawArrays(GL_TRIANGLES, 0, 3*currTriCount);
		// druga kula
		sphereModel = glm::mat4(1.0f);
		sphereModel = glm::translate(sphereModel, glm::vec3(-7.3f, 1.7f, -17.0f));
		sphereModel = glm::scale(sphereModel, glm::vec3(0.5f, 0.5f, 0.5f));
		(*shaderSphereInUse).setMat4("model", sphereModel);
		(*shaderSphereInUse).setMat4("invmodel", inverse(sphereModel));
		glDrawArrays(GL_TRIANGLES, 0, 3 * currTriCount);
		// trzecia kula
		sphereModel = glm::mat4(1.0f);
		sphereModel = glm::translate(sphereModel, glm::vec3(-7.3f, 2.45f, -17.0f));
		sphereModel = glm::scale(sphereModel, glm::vec3(0.35f, 0.35f, 0.35f));
		(*shaderSphereInUse).setMat4("model", sphereModel);
		(*shaderSphereInUse).setMat4("invmodel", inverse(sphereModel));
		glDrawArrays(GL_TRIANGLES, 0, 3 * currTriCount);

		// check and call events and swap the buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &groundVAO);
	glDeleteBuffers(1, &groundVBO);
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		usedCamera = 1;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		usedCamera = 2;
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		usedCamera = 3;
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	{
		shaderObjInUse = objectTexture;
		shaderGrdInUse = groundTexture;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		shaderObjInUse = objectPhong;
		shaderGrdInUse = groundPhong;
		shaderSphereInUse = spherePhong;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		shaderObjInUse = objectGouraud;
		shaderGrdInUse = groundGouraud;
		shaderSphereInUse = sphereGouraud;
	}
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
	{
		(*shaderObjInUse).setBool("blinn", true);
		(*shaderGrdInUse).setBool("blinn", true);
		(*shaderSphereInUse).setBool("blinn", true);
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
	{
		(*shaderObjInUse).setBool("blinn", false);
		(*shaderGrdInUse).setBool("blinn", false);
		(*shaderSphereInUse).setBool("blinn", false);
	}
	if (usedCamera == 1)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraWorld.ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraWorld.ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraWorld.ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraWorld.ProcessKeyboard(RIGHT, deltaTime);
	}
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	if (usedCamera == 3)
		cameraObject.ProcessMouseMovement(xoffset, yoffset);
	if (usedCamera == 1)
		cameraWorld.ProcessMouseMovement(xoffset, yoffset);
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void set_directional_light(Shader shader)
{
	float var = 0.3f * sin(glfwGetTime() / 4.0f) + 0.4f;
	shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	shader.setVec3("dirLight.ambient", var, var, var); // brighter/darker
	shader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
	shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
}

void set_point_light(Shader shader, glm::vec3* positions)
{
	// point light 1
	shader.setVec3("pointLights[0].position", positions[0]);
	shader.setVec3("pointLights[0].ambient", 0.1f, 0.1f, 0.1f);
	shader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
	shader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("pointLights[0].constant", 1.0f);
	shader.setFloat("pointLights[0].linear", 0.09f);
	shader.setFloat("pointLights[0].quadratic", 0.032f);
	// point light 2
	shader.setVec3("pointLights[1].position", positions[1]);
	shader.setVec3("pointLights[1].ambient", 0.3f, 0.3f, 0.3f);
	shader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
	shader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("pointLights[1].constant", 1.0f);
	shader.setFloat("pointLights[1].linear", 0.09f);
	shader.setFloat("pointLights[1].quadratic", 0.032f);
	// point light 3
	shader.setVec3("pointLights[2].position", positions[2]);
	shader.setVec3("pointLights[2].ambient", 0.3f, 0.3f, 0.3f);
	shader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
	shader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("pointLights[2].constant", 1.0f);
	shader.setFloat("pointLights[2].linear", 0.09f);
	shader.setFloat("pointLights[2].quadratic", 0.032f);
}

void set_spotlights(Shader shader, glm::vec3* positions)
{
	glm::vec3 front; // yaw = 10.0f?, pitch = 45.0f?
	float yaww = 0.0f;
	float pitch = -90.0f;
	if (shaderObjInUse == objectTexture)
	{
		yaww = -90.0f;
		pitch = 0.0f;
	}
	front.x = cos(glm::radians(yaww)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaww)) * cos(glm::radians(pitch));
	shader.setVec3("spotLights[1].position", positions[0]);
	shader.setVec3("spotLights[1].direction", glm::normalize(front));
	shader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
	shader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
	shader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("spotLights[1].constant", 1.0f);
	shader.setFloat("spotLights[1].linear", 0.09f);
	shader.setFloat("spotLights[1].quadratic", 0.032f);
	shader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(17.5f)));
	shader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(21.0f)));
	//////
	shader.setVec3("spotLights[2].position", positions[1]);
	shader.setVec3("spotLights[2].direction", glm::normalize(front));
	shader.setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
	shader.setVec3("spotLights[2].diffuse", 1.0f, 1.0f, 1.0f);
	shader.setVec3("spotLights[2].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("spotLights[2].constant", 1.0f);
	shader.setFloat("spotLights[2].linear", 0.09f);
	shader.setFloat("spotLights[2].quadratic", 0.032f);
	shader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(15.5f)));
	shader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(17.5f)));
}

void set_flashlight(Shader shader, glm::vec3 position)
{

	if (usedCamera == 3)
	{
		shader.setVec3("spotLights[0].position", cameraObject.Position);
		/*if (shaderObjInUse == objectTexture)
		{
			glm::vec3 newFront; // camera yaw <-> pitch
			float yaww = cameraObject.Pitch;
			float pitch = -cameraObject.Yaw;
			newFront.x = cos(glm::radians(yaww)) * cos(pitch);
			newFront.y = sin(glm::radians(pitch));
			newFront.z = sin(glm::radians(yaww)) * cos(pitch);
			shader.setVec3("spotLights[0].direction", normalize(newFront));
		}
		else*/
			shader.setVec3("spotLights[0].direction", cameraObject.Front);
	}
	else
	{
		// calculate direction
		glm::vec3 front;
		float yaww = yaw;
		float pitch = 0.0f;
		if (shaderObjInUse == objectTexture)
		{
			pitch = -yaw;
			yaww = 0.0f;
		}
		front.x = cos(glm::radians(yaww)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaww)) * cos(glm::radians(pitch));
		shader.setVec3("spotLights[0].position", position);
		shader.setVec3("spotLights[0].direction", glm::normalize(front));
	}

	shader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
	shader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
	shader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("spotLights[0].constant", 1.0f);
	shader.setFloat("spotLights[0].linear", 0.09f);
	shader.setFloat("spotLights[0].quadratic", 0.032f);
	shader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
	shader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));
}

std::vector<float> calcVertex(float x, float y, float z)
{
	float length = sqrt(x*x + y*y + z*z);
	std::vector<float> result;
	result.push_back(x / length);
	result.push_back(y / length);
	result.push_back(z / length);
	return result;
}

std::vector<float> getMiddlePoint(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float midX = (x1 + x2) / 2.0f;
	float midY = (y1 + y2) / 2.0f;
	float midZ = (z1 + z2) / 2.0f;
	float length = sqrt(midX * midX + midY * midY + midZ * midZ);
	std::vector<float> result;
	result.push_back(midX / length);
	result.push_back(midY / length);
	result.push_back(midZ / length);
	return result;
}

