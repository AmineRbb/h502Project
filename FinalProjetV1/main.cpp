#include<iostream>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_inverse.hpp>

#include <irrKlang.h>
using namespace irrklang;
#pragma comment(lib, "irrKlang.lib")

#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"
#include "shader.h"
#include "object.h"


const int width = 1200;
const int height = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void loadCubemapFace(const char* file, const GLenum& targetCube);
Object createObject(const char* path, const glm::vec3& translation, const glm::vec3& scale, Shader& shader);
GLuint loadTexture(const char* path, GLenum textureUnit);

Camera camera(glm::vec3(1.0, 0.0, -6.0), glm::vec3(0.0, 1.0, 0.0), 90.0);

 
int main(int argc, char* argv[])
{

	ISoundEngine* SoundEngine = createIrrKlangDevice();
	if (!SoundEngine)
	{
		throw std::runtime_error("SONNDENGINE ERROR\n");
	}
	SoundEngine->play2D("musics/itsMeMario!.mp3", false);


	//Boilerplate
	//Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	//Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Project H502 Amine RABBOUCH", NULL, NULL);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);


	const std::string sourceV = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coord; \n"
		"out vec2 v_tex; \n"
		"in vec3 normal; \n"
		"uniform int textureType;\n"
		"out vec3 v_diffuse; \n"

		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"uniform vec3 u_light_pos; \n"

		" void main(){ \n"

		"   v_tex = tex_coord;\n"
		"   if (textureType == 0) {\n"  // Vérifiez le type de texture (1 pour l'herbe)
		"       v_tex = tex_coord * 10.0;\n" // Divisez les coordonnées de texture par 5.0 pour l'herbe
		"   }\n"

		"vec4 frag_coord = M*vec4(position, 1.0);"
		"gl_Position = P*V*frag_coord;\n"
		//3. transform correctly the normals
		"vec3 norm = vec3(itM * vec4(normal, 1.0)); \n"
		//3. use Gouraud : compute the diffuse light with the normals at the vertices
		"vec3 L = normalize(u_light_pos - frag_coord.xyz); \n"
		"float diffusion = max(0.0, dot(norm, L)); \n"
		"v_diffuse = vec3(diffusion);\n" //same component in every direction
		"}\n";
	const std::string sourceF = "#version 330 core\n"
		"out vec4 FragColor;"
		"precision mediump float; \n"
		"in vec3 v_diffuse; \n"
		"uniform sampler2D ourTexture; \n"
		"in vec2 v_tex; \n"

		"void main() { \n"
		"   vec4 texColor = texture(ourTexture, v_tex); \n"
		"   vec3 finalColor = texColor.rgb;\n" //* v_diffuse; \n"
		"   FragColor = vec4(finalColor, texColor.a); \n"
		"} \n";


	Shader shader(sourceV, sourceF);

	const std::string sourceVCubeMap = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coords; \n"
		"in vec3 normal; \n"
		
		//only P and V are necessary
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"

		"out vec3 texCoord_v; \n"

		" void main(){ \n"
		"texCoord_v = position;\n"
		//remove translation info from view matrix to only keep rotation
		"mat4 V_no_rot = mat4(mat3(V)) ;\n"
		"vec4 pos = P * V_no_rot * vec4(position, 1.0); \n"
		// the positions xyz are divided by w after the vertex shader
		// the z component is equal to the depth value
		// we want a z always equal to 1.0 here, so we set z = w!
		// Remember: z=1.0 is the MAXIMUM depth value ;)
		"gl_Position = pos.xyww;\n"
		"\n"
		"}\n";

	const std::string sourceFCubeMap =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"precision mediump float; \n"
		"uniform samplerCube cubemapSampler; \n"
		"in vec3 texCoord_v; \n"
		"void main() { \n"
		"FragColor = texture(cubemapSampler,texCoord_v); \n"
		"} \n";


	Shader cubeMapShader = Shader(sourceVCubeMap, sourceFCubeMap);


	//1. Create Models

	char path1[] = "objet/cube.obj";
	char path2[] = "objet/mario_mushroom.obj";
	char path4[] = "objet/star.obj";
	char path3[] = "objet/pipe.obj";


	Object bloc1 = createObject(path1, glm::vec3(2.0, 0.0, 4.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object bloc2 = createObject(path1, glm::vec3(4.0, 0.0, 4.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object bloc3 = createObject(path1, glm::vec3(6.0, 0.0, 4.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object bloc4 = createObject(path1, glm::vec3(6.0, 2.0, 4.0), glm::vec3(0.9, 0.9, 0.9), shader);

	Object blocInterro1 = createObject(path1, glm::vec3(-4.0, 4.0, -3.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object blocInterro2 = createObject(path1, glm::vec3(4.0, 4.0, 6.0), glm::vec3(0.9, 0.9, 0.9), shader);

	Object blocStone1 = createObject(path1, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object blocStone2 = createObject(path1, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object blocStone3 = createObject(path1, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object blocStone4 = createObject(path1, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), shader);
	Object blocStone5 = createObject(path1, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), shader);

	Object pipe = createObject(path3, glm::vec3(2.3, 0.0, 0.0), glm::vec3(0.01, 0.01, 0.01), shader);

	Object mushroom = createObject(path2, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.3, 0.3, 0.3), shader);

	Object star = createObject(path4, glm::vec3(-2.3, 3.5, -2.0), glm::vec3(0.5, 0.5, 0.5), shader);
	Object ground = createObject(path1, glm::vec3(0.0, -1.0, 0.0), glm::vec3(10.0, 0.1, 10.0), shader);

	// CubeMap

	Object cubeMap(path1);
	cubeMap.makeObject(cubeMapShader);

	GLuint cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

	// texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//stbi_set_flip_vertically_on_load(true);

	std::string pathToCubeMap = "textures/";

	std::map<std::string, GLenum> facesToLoad = {
		{pathToCubeMap + "right.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "top.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "front.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "left.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "bottom.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "back.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	//load the six faces
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}


	//2. Choose a position for the light
	const glm::vec3 light_pos = glm::vec3(0.5, 0.5, -0.7);
	const glm::vec3 light_pos2 = glm::vec3(-2.3, 5.5, -2.0);
	const glm::vec3 light_pos3 = glm::vec3(-2.7, 5.7, -2.2);
	const glm::vec3 light_pos4 = glm::vec3(-2.3, 5.3, -1.8);

	double prev = 0;
	int deltaFrame = 0;
	//fps function
	auto fps = [&](double now) {
		double deltaTime = now - prev;
		deltaFrame++;
		if (deltaTime > 0.5) {
			prev = now;
			const double fpsCount = (double)deltaFrame / deltaTime;
			deltaFrame = 0;
			std::cout << "\r FPS: " << fpsCount;
		}
		};



	//////TEXTURE//////////
	GLuint tGround = loadTexture("textures/grass.jpg", GL_TEXTURE0);
	GLuint tStar = loadTexture("textures/star.jpg", GL_TEXTURE1);
	GLuint tMushroom = loadTexture("textures/mushroom.jpg", GL_TEXTURE2);
	GLuint tBloc = loadTexture("textures/bloc.jpg", GL_TEXTURE3);
	GLuint tInterro = loadTexture("textures/blocItem.jpg", GL_TEXTURE4);
	GLuint tStone = loadTexture("textures/blocStone.jpg", GL_TEXTURE5);

	GLuint texture = loadTexture("textures/grass.jpg", GL_TEXTURE0);
	GLuint texture2 = loadTexture("textures/star.jpg", GL_TEXTURE1);
	GLuint texture3 = loadTexture("textures/mushroom.jpg", GL_TEXTURE2);


	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	double timeValue = 0.0;
	bool startSong = false;

	vec3df soundPosition(0.0f, 0.0f, 0.0f);
	bool isMusicPlaying = false;


	//Rendering
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.99f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (timeValue > 3.0 && startSong==false)
		{
			SoundEngine->play2D("musics/Mario64TitleTheme.mp3", true);
			startSong = true;
			isMusicPlaying = true;
		}

		if (isMusicPlaying) {
			// Récupérer la position actuelle de la caméra
			vec3df listenerPosition(camera.Position.x, camera.Position.y, camera.Position.z);

			// Calculer la distance entre la caméra et le point du son
			float distance = listenerPosition.getDistanceFrom(soundPosition);

			// Ajuster la puissance sonore en fonction de la distance
			float minDistance = 5.0f;  // Distance à laquelle le son sera entendu à son volume maximal
			float maxDistance = 30.0f; // Distance à laquelle le son sera entendu au volume minimal
			float volume = 1.0f - (distance - minDistance) / (maxDistance - minDistance);

			// Limiter la plage du volume entre 0.0 et 1.0
			volume = std::max(0.0f, std::min(1.0f, volume));

			// Appliquer le volume au son en cours de lecture
			SoundEngine->setSoundVolume(volume);

		}

		//2. Use the shader Class to send the relevant uniform
		shader.use();
		glBindTexture(GL_TEXTURE_2D, tMushroom);
		shader.setInteger("textureType", 3);
		shader.setInteger("ourTexture", 0);
		shader.setMatrix4("M", mushroom.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere2.model));
		mushroom.draw();
 
		glBindTexture(GL_TEXTURE_2D, tBloc);
		shader.setInteger("textureType", 2);
		shader.setInteger("ourTexture", 0);

		shader.setMatrix4("M", bloc1.model);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_light_pos", light_pos);
		
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glm::mat4 itM = glm::inverseTranspose(bloc1.model);
		shader.setMatrix4("itM", itM);
		bloc1.draw();

		shader.setMatrix4("M", bloc2.model);
		bloc2.draw();

		shader.setMatrix4("M", bloc3.model);
		bloc3.draw();

		shader.setMatrix4("M", bloc4.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		bloc4.draw();


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tInterro);
		shader.setInteger("textureType",4); // 1 pour l'herbe
		shader.setInteger("ourTexture", 0);

		shader.setMatrix4("M", blocInterro1.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		blocInterro1.draw();
		shader.setMatrix4("M", blocInterro2.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		blocInterro2.draw();

		double deltaTime = now - prev;
		timeValue += deltaTime;  // Mettre à jour le temps
		prev = now;
		star.model = glm::translate(star.model, glm::vec3(0.0f, 0.05f * sin(3.0 * timeValue), 0.0f));
		star.model = glm::rotate(star.model, (float)0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

		glBindTexture(GL_TEXTURE_2D, tStar);
		shader.setInteger("textureType", 1);
		shader.setInteger("ourTexture", 0);
		shader.setMatrix4("M", star.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		shader.setVector3f("u_light_pos", light_pos2);
		shader.setVector3f("u_light_pos", light_pos3);
		shader.setVector3f("u_light_pos", light_pos4);
		star.draw();

		glBindTexture(GL_TEXTURE_2D, tStar);
		shader.setInteger("textureType", 1);
		shader.setInteger("ourTexture", 0);
		shader.setMatrix4("M", pipe.model);
		pipe.draw();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tGround);
		shader.setInteger("textureType", 0); // 1 pour l'herbe
		shader.setInteger("ourTexture", 0);

		shader.setMatrix4("M", ground.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		ground.draw();


		glDepthFunc(GL_LEQUAL);
		bloc1.draw();

		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		cubeMapShader.setInteger("cubemapTexture", 0);

		cubeMap.draw();
		glDepthFunc(GL_LESS);

		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void loadCubemapFace(const char* path, const GLenum& targetFace)
{
	int imWidth, imHeight, imNrChannels;
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{

		glTexImage2D(targetFace, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(targetFace);
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}
	stbi_image_free(data);
}


Object createObject(const char* path, const glm::vec3& translation, const glm::vec3& scale, Shader& shader)
{
	Object obj(path);
	obj.makeObject(shader);
	obj.model = glm::translate(obj.model, translation);
	obj.model = glm::scale(obj.model, scale);
	return obj;
}

GLuint loadTexture(const char* path, GLenum textureUnit)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load image, create texture and generate mipmaps
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
	}

	stbi_image_free(data);
	return texture;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	//3. Use the cameras class to change the parameters of the camera
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(LEFT, 0.1);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(RIGHT, 0.1);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(FORWARD, 0.1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(BACKWARD, 0.1);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(1, 0.0, 1);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(-1, 0.0, 1);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, -1.0, 1);


}
