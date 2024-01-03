#include<iostream>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_inverse.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"
#include "shader.h"
#include "object.h"


const int width = 1200;
const int height = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);


Camera camera(glm::vec3(1.0, 0.0, -6.0), glm::vec3(0.0, 1.0, 0.0), 90.0);


int main(int argc, char* argv[])
{
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
		"in vec3 normal; \n"

		"out vec3 v_diffuse; \n"

		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"uniform vec3 u_light_pos; \n"

		" void main(){ \n"
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

		"void main() { \n"
		"FragColor = vec4(v_diffuse, 1.0); \n"
		"} \n";


	Shader shader(sourceV, sourceF);

	//1. Load the model for 3 types of spheres

	char path1[] = "objet/cube.obj";
	char path2[] = "objet/mario_mushroom.obj";
	char path4[] = "objet/mario_stars.obj";

	Object sphere1(path1);
	sphere1.makeObject(shader);
	sphere1.model = glm::translate(sphere1.model, glm::vec3(2.3, 0.0, 0.0));
	sphere1.model = glm::scale(sphere1.model, glm::vec3(0.9, 0.9, 0.9));

	Object sphere2(path2);
	sphere2.makeObject(shader);
	sphere2.model = glm::translate(sphere2.model, glm::vec3(0.0, 0.0, 0.0));
	sphere2.model = glm::scale(sphere2.model, glm::vec3(0.3, 0.3, 0.3));

	Object sphere3(path1);
	sphere3.makeObject(shader);
	sphere3.model = glm::translate(sphere3.model, glm::vec3(-2.3, 0.0, -0.0));
	sphere3.model = glm::scale(sphere3.model, glm::vec3(0.9, 0.9, 0.9));

	Object sphere4(path4);
	sphere4.makeObject(shader);
	sphere4.model = glm::translate(sphere4.model, glm::vec3(-2.3, 2.0, -2.0));
	sphere4.model = glm::scale(sphere4.model, glm::vec3(0.9, 0.9, 0.9));


	//2. Choose a position for the light
	const glm::vec3 light_pos = glm::vec3(0.5, 0.5, -0.7);


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






	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();


	//Rendering
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.99f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		//2. Use the shader Class to send the relevant uniform
		shader.use();

		shader.setMatrix4("M", sphere1.model);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_light_pos", light_pos);

		glm::mat4 itM = glm::inverseTranspose(sphere1.model);
		shader.setMatrix4("itM", itM);
		sphere1.draw();

		shader.setMatrix4("M", sphere2.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere2.model));
		sphere2.draw();

		shader.setMatrix4("M", sphere3.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		sphere3.draw();

		sphere4.model = glm::translate(sphere4.model, glm::vec3(0.0f, -0.0f, 0.0f));
		sphere4.model = glm::rotate(sphere4.model, (float)0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

		shader.setMatrix4("M", sphere4.model);
		//shader.setMatrix4("itM", glm::inverseTranspose(sphere3.model));
		sphere4.draw();

		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
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


