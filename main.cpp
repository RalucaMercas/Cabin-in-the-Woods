#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>

gps::Window myWindow;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 lightRotation;

glm::vec3 lightDir;
glm::vec3 lightColor;
GLfloat lightAngle;

GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

gps::Camera myCamera(
	glm::vec3(01.73f, 15.8f, -56.76f),
	glm::vec3(-23.57f, 5.56f, -17.24f),
	glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 1.00f;

GLboolean pressedKeys[1024];

gps::Model3D merryGoRound;
gps::Model3D sceneModel;
float angle;

gps::Shader myBasicShader;
gps::Shader shadowMapShader;
gps::Shader rainShader;

bool rotateMerryGoRound = false;
bool enableMouseMovement = true;
bool wireframeMode = false;
bool polygonalMode = false;
bool smoothMode = true;
bool solidMode = true;

bool isPresentationMode = false;
float presentationTime = 0.0f; 

float deltaTime = 0.0f; 
float lastFrame = 0.0f; 

float sceneScale = 1.0f;

glm::vec3 pointLightColor = glm::vec3(1.0f, 0.76f, 0.3f);
float constantAttenuation = 1.0f;
float linearAttenuation = 0.07f;
float quadraticAttenuation = 0.017f;
GLint pointLightColorLoc;
GLint constantAttLoc, linearAttLoc, quadraticAttLoc;

const int NUM_POINT_LIGHTS = 10;
glm::vec3 pointLightPositions[NUM_POINT_LIGHTS] = {
	glm::vec3(-67.123f, 15.016f, 10.52f),
	glm::vec3(-69.237f, 15.168f, 3.5162f),
	glm::vec3(-40.411f, 14.862f, -6.9554f),
	glm::vec3(-43.887f, 15.242f, -12.823f),
	glm::vec3(-16.317f, 14.91f, -22.823f),
	glm::vec3(-20.053f, 15.075f, -28.865f),
	glm::vec3(11.404f, 15.858f, -41.502f),
	glm::vec3(7.6644f, 15.473f, -47.189f),
	glm::vec3(35.37f, 15.955f, -56.966f),
	glm::vec3(29.917f, 15.622f, -61.9f)
};
GLint pointLightPosLoc[NUM_POINT_LIGHTS];


GLenum glCheckError_(const char* file, int line){
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
		case GL_INVALID_ENUM:
			error = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "INVALID_VALUE";
			break;
			/*case GL_INVALID_OPERATION:
				error = "INVALID_OPERATION";
				break;*/
		case GL_OUT_OF_MEMORY:
			error = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(_FILE, __LINE_)


void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	sceneScale += yoffset * 0.1f; 
	sceneScale = glm::clamp(sceneScale, 0.5f, 1.0f);
}

struct CameraWaypoint {
	glm::vec3 position;
	glm::vec3 target;
};

std::vector<CameraWaypoint> cameraPath = {
	{ glm::vec3(0.0f, 15.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f) },
	{ glm::vec3(-54.0768f, 12.859f, 4.5984f), glm::vec3(-34.294f, 10.164f, -15.794f) },
	{ glm::vec3(-0.374318f, 10.1639f, -33.5343f), glm::vec3(-34.5723f, 9.66168f, -37.4496f) },
	{ glm::vec3(18.5457f, 12.8636f, 50.8034f), glm::vec3(-0.374318f, 10.1639f, -33.5343f) },
	{ glm::vec3(01.73f, 15.8f, -56.76f), glm::vec3(-23.57f, 5.56f, -17.24f) }	 
};

void windowResizeCallback(GLFWwindow* window, int width, int height) {

	glViewport(0, 0, width, height);
	WindowDimensions dim;
	dim.width = width;
	dim.height = height;
	myWindow.setWindowDimensions(dim);

	projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f);

	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}

static float yaw = -90.0f; 
static float pitch = 0.0f;


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;

			if (key == GLFW_KEY_R) {
				rotateMerryGoRound = !rotateMerryGoRound;
			}

			if (key == GLFW_KEY_C) {
				enableMouseMovement = !enableMouseMovement;
				if (enableMouseMovement) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}
				else {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
			}

			if (key == GLFW_KEY_I) {  
				wireframeMode = !wireframeMode;
				if (wireframeMode) {
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
			}
			else
				if (key == GLFW_KEY_O) {
					polygonalMode = !polygonalMode;
					if (polygonalMode) {
						glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
					}
				}
				else
					if (key == GLFW_KEY_P) {
						solidMode = true;
						wireframeMode = false;
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					}
			if (key == GLFW_KEY_M) {
				isPresentationMode = !isPresentationMode;
				presentationTime = 0.0f;
			}
		}
		else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}
	}
}

void updatePresentation(float deltaTime) {
	static int currentWaypoint = 0;

	if (currentWaypoint >= cameraPath.size() - 1) {
		isPresentationMode = false;
		return;
	}

	presentationTime += deltaTime;
	const float transitionDuration = 3.0f;

	float t = glm::clamp(presentationTime / transitionDuration, 0.0f, 1.0f);

	glm::vec3 newPosition = glm::mix(cameraPath[currentWaypoint].position,
		cameraPath[currentWaypoint + 1].position, t);
	glm::vec3 newTarget = glm::mix(cameraPath[currentWaypoint].target,
		cameraPath[currentWaypoint + 1].target, t);

	myCamera = gps::Camera(newPosition, newTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	if (t >= 1.0f) {
		currentWaypoint++;
		presentationTime = 0.0f;
	}
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	static float lastX = 1024.0f / 2.0f; 
	static float lastY = 768.0f / 2.0f;
	static bool firstMouse = true;

	if (!enableMouseMovement) {
		firstMouse = true;
		return;
	}

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
		return;
	}

	float xOffset = xpos - lastX;
	float yOffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float mouseSensitivity = 0.1f;
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);

	view = myCamera.getViewMatrix();
	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

}

struct RainParticle {
	glm::vec3 position;
	glm::vec3 velocity;
};

const int NUM_PARTICLES = 5000;
std::vector<RainParticle> rainParticles;

void generateRainParticles() {
	for (int i = 0; i < NUM_PARTICLES; ++i) {
		RainParticle particle;
		particle.position = glm::vec3(
			(rand() % 300 - 150) / 1.0f, // X: Random (-150.0, 150.0)
			rand() % 100 + 100.0f,       // Y: Random height (100.0, 200.0)
			(rand() % 300 - 150) / 1.0f  // Z: Random (-150.0, 150.0)
		);
		particle.velocity = glm::vec3(0.0f, -9.8f, 0.0f);
		rainParticles.push_back(particle);
	}
}

GLuint rainVAO, rainVBO;

void initRainParticles() {
	glGenVertexArrays(1, &rainVAO);
	glGenBuffers(1, &rainVBO);

	glBindVertexArray(rainVAO);

	glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
	glBufferData(GL_ARRAY_BUFFER, rainParticles.size() * sizeof(RainParticle), rainParticles.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), (void*)offsetof(RainParticle, velocity));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void updateRainParticles(float deltaTime) {
	for (size_t i = 0; i < rainParticles.size(); ++i) {
		rainParticles[i].position += rainParticles[i].velocity * deltaTime;

		if (rainParticles[i].position.y < 0.0f) {
			rainParticles[i].position.y = rand() % 100 + 100.0f;
			rainParticles[i].position.x = (rand() % 300 - 150) / 1.0f;
			rainParticles[i].position.z = (rand() % 300 - 150) / 1.0f;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, rainParticles.size() * sizeof(RainParticle), rainParticles.data());
}

void renderRain() {
	rainShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "deltaTime"), deltaTime);

	glBindVertexArray(rainVAO);
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	glBindVertexArray(0);
}

glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
float fogDensity = 0.01f;
GLint fogColorLoc, fogDensityLoc;

void processMovement() {

	if (rotateMerryGoRound) {
		angle += 1.5f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}


	if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_LEFT_CONTROL]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_MINUS]) {
		lightAngle -= 1.0f;
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		lightDir = glm::vec3(lightRotation * glm::vec4(0.0f, 1.0f, 1.0f, 0.0f));
	}

	if (pressedKeys[GLFW_KEY_EQUAL]) {
		lightAngle += 1.0f;
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		lightDir = glm::vec3(lightRotation * glm::vec4(0.0f, 1.0f, 1.0f, 0.0f));
	}

	if (pressedKeys[GLFW_KEY_UP]) {
		fogDensity += 0.005f;
		myBasicShader.useShaderProgram(); 
		glUniform1f(fogDensityLoc, fogDensity);
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		fogDensity = std::max(0.0f, fogDensity - 0.005f);
		myBasicShader.useShaderProgram(); 
		glUniform1f(fogDensityLoc, fogDensity);
	}

}

void initOpenGLWindow() {
	myWindow.Create(1024, 768, "Cabin by the lake");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
	glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
	glfwSetScrollCallback(myWindow.getWindow(), [](GLFWwindow* window, double xoffset, double yoffset) {
		if (yoffset > 0) {
			sceneScale += 0.1f; 
		}
		else if (yoffset < 0) {
			sceneScale = std::max(0.1f, sceneScale - 0.1f);
		}
		std::cout << "Scene Scale: " << sceneScale << std::endl;
		});
	glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}


void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

GLuint shadowMapFBO;
GLuint depthMapTexture;

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

void initShadowMapping() {
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initModels() {
	merryGoRound.LoadModel("models/merry_go_round/merry_go_round.obj");
	sceneModel.LoadModel("models/scene/scena_completa.obj");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
	shadowMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
	rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

	fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
	fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");

	glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
	glUniform1f(fogDensityLoc, fogDensity);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
	glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f),
		(float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
		0.1f, 500.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), lightDir);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
		std::string posUniformName = "pointLightPositions[" + std::to_string(i) + "]";
		pointLightPosLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, posUniformName.c_str());
		glUniform3fv(pointLightPosLoc[i], 1, glm::value_ptr(pointLightPositions[i]));
	}

	pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
	constantAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "constantAttenuation");
	linearAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "linearAttenuation");
	quadraticAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "quadraticAttenuation");

	glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
	glUniform1f(constantAttLoc, constantAttenuation);
	glUniform1f(linearAttLoc, linearAttenuation);
	glUniform1f(quadraticAttLoc, quadraticAttenuation);
}


void renderObjects(gps::Shader shader, bool depthPass) {
	shader.useShaderProgram();
	glm::mat4 sceneScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(sceneScale));

	
	glm::vec3 merryGoRoundPosition = glm::vec3(-33.0f, 6.682f, -6.24f);
	glm::mat4 merryGoRoundModel = glm::mat4(1.0f);
	merryGoRoundModel = glm::translate(merryGoRoundModel, merryGoRoundPosition); 
	merryGoRoundModel = glm::rotate(merryGoRoundModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	merryGoRoundModel = glm::translate(merryGoRoundModel, -merryGoRoundPosition); 
	merryGoRoundModel = sceneScaleMatrix * merryGoRoundModel;

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(merryGoRoundModel));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * merryGoRoundModel));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	merryGoRound.Draw(shader);

	glm::mat4 sceneModelMatrix = glm::mat4(1.0f);
	sceneModelMatrix = glm::translate(sceneModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f)); 
	sceneModelMatrix = sceneScaleMatrix * sceneModelMatrix; 

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sceneModelMatrix));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * sceneModelMatrix));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	sceneModel.Draw(shader);
}


glm::mat4 computeLightSpaceTrMatrix() {
	const glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	constexpr GLfloat orthoLeft = -150.0f;
	constexpr GLfloat orthoRight = 150.0f;
	constexpr GLfloat orthoBottom = -150.0f;
	constexpr GLfloat orthoTop = 150.0f;
	constexpr GLfloat nearPlane = 0.1f;
	constexpr GLfloat farPlane = 500.0f;
	const glm::mat4 lightProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void renderScene() {
	shadowMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(shadowMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	renderObjects(shadowMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	myBasicShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);
	glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	renderObjects(myBasicShader, false);
}

void cleanup() {
	myWindow.Delete();
}

int main(int argc, const char* argv[]) {

	try {
		initOpenGLWindow();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
	initShadowMapping();
	setWindowCallbacks();
	generateRainParticles();
	initRainParticles();

	//glCheckError();
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (isPresentationMode) {
			updatePresentation(deltaTime);
		}
		else {
			processMovement();
		}

		updateRainParticles(deltaTime);
		renderScene();
		renderRain();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		//glCheckError();
	}

	cleanup();

	return 1;
}