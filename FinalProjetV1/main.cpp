#include<iostream>
#include <chrono>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_inverse.hpp>

#include <irrKlang.h>
using namespace irrklang;
#pragma comment(lib, "irrKlang.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"
#include "shader.h"
#include "object.h"


const int width = 1400;
const int height = 1000;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void loadCubemapFace(const char* file, const GLenum& targetCube);
Object createObject(const char* path, const glm::vec3& translation, const glm::vec3& scale, Shader& shader);
GLuint loadTexture(const char* path, GLenum textureUnit);

Camera camera(glm::vec3(1.0, 0.0, -6.0), glm::vec3(0.0, 1.0, 0.0), 90.0);

float rectangleVertices[] =
{
	// Coords    // texCoords
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f,

	 1.0f,  1.0f,  1.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f
};

 
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
		"uniform int textureType;\n"
		"in vec3 normal; \n"
		"in vec3 tangent;\n"  // Nouvel attribut pour les tangentes
		"in vec3 bitangent;\n"  // Nouvel attribut pour les bitangentes

		"out vec2 v_tex; \n"
		"out vec3 v_frag_coord; \n"
		"out vec4 fragPosLight;\n"
		
		"out vec3 v_normal; \n"
		"out mat3 TBN;\n"  // Nouvelle sortie pour la matrice TBN

		"uniform mat4 M; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"uniform vec3 u_light_pos; \n"
		"uniform mat4 lightProjection;\n"

		" void main(){ \n"

		"   v_tex = tex_coord;\n"
		"   if (textureType == 0) {\n"  // Vérifiez le type de texture (1 pour l'herbe)
		"       v_tex = tex_coord * 10.0;\n" // Divisez les coordonnées de texture par 5.0 pour l'herbe
		"   }\n"

		"vec4 frag_coord = M * vec4(position, 1.0);"
		"gl_Position = P*V*frag_coord;\n"
		//"v_normal = vec3(itM * vec4(normal, 1.0)); \n"
		"v_frag_coord = frag_coord.xyz; \n"
		"   vec3 T = normalize(vec3(M[0][0], M[1][0], M[2][0]));\n"
		"   vec3 B = normalize(vec3(M[0][1], M[1][1], M[2][1]));\n"
		"   vec3 N = normalize(vec3(M[0][2], M[1][2], M[2][2]));\n"

		"   TBN = mat3(T, B, N);\n"
		"	fragPosLight = lightProjection * vec4(v_frag_coord,1.0f);\n"
		//"	TangentLightPos = TBN * u_light_pos;\n"
		"}\n";
	const std::string sourceF = "#version 330 core\n"
		"out vec4 FragColor;"
		"precision mediump float; \n"

		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"
		"in vec2 v_tex; \n"
		"in mat3 TBN;\n" // new
		"in vec4 fragPosLight;\n"

		"uniform vec3 u_view_pos; \n"
		"uniform sampler2D ourTexture; \n"
		"uniform sampler2D normalMap; \n"

		"struct Light{\n"
		"vec3 light_pos; \n"
		"float ambient_strength; \n"
		"float diffuse_strength; \n"
		"float specular_strength; \n"
		//attenuation factor
		"float constant;\n"
		"float linear;\n"
		"float quadratic;\n"
		"};\n"

		"uniform Light light;"
		"uniform float shininess; \n"

		"float specularCalculation(vec3 N, vec3 L, vec3 V ){ \n"
		"vec3 R = reflect (-L,N);  \n " //reflect (-L,N) is  equivalent to //max (2 * dot(N,L) * N - L , 0.0) ;
		"float cosTheta = dot(R , V); \n"
		"float spec = pow(max(cosTheta,0.0), 32.0); \n"
		"return light.specular_strength * spec;\n"
		"}\n"

		"void main() { \n"
/*		"vec3 N = normalize(v_normal);\n"
		"vec3 L = normalize(light.light_pos - v_frag_coord) ; \n"
		"vec3 V = normalize(u_view_pos - v_frag_coord); \n"
		"float specular = specularCalculation( N, L, V); \n"
		"float diffuse = light.diffuse_strength * max(dot(N,L),0.0);\n"
		"float distance = length(light.light_pos - v_frag_coord);"
		"float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);"
		"float lightIntensity = light.ambient_strength + attenuation * (diffuse + specular); \n"
		"   vec4 texColor = texture(ourTexture, v_tex); \n"
		"   vec3 finalColor = texColor.rgb * lightIntensity; \n"
		"   FragColor = vec4(finalColor, texColor.a); \n"*/
		// Sets lightCoords to cull space
		
		"   vec3 normal = texture(normalMap, v_tex).rgb * 2.0 - 1.0;\n"
		"   normal = normalize(TBN * normal);\n"
		"   vec3 N = normalize(normal);\n"
		"   vec3 L = normalize(light.light_pos - v_frag_coord);\n"
		"   vec3 V = normalize(u_view_pos - v_frag_coord);\n"
		
		"   float specular = specularCalculation(N, L, V);\n"
		"   float diffuse = light.diffuse_strength * max(dot(N, L), 0.0);\n"
		"   float distance = length(light.light_pos - v_frag_coord);\n"
		"   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);\n"
		"   float lightIntensity = light.ambient_strength + attenuation * (diffuse + specular);\n"
		"   vec4 texColor = texture(ourTexture, v_tex);\n"
		"   vec3 finalColor = texColor.rgb * lightIntensity;\n"
		"   FragColor = vec4(finalColor, texColor.a);\n"
		"} \n";


	Shader shader(sourceV, sourceF);

	const std::string sourceVRef = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coords; \n"
		"in vec3 normal; \n"

		"out vec3 v_frag_coord; \n"
		"out vec3 v_normal; \n"

		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"


		" void main(){ \n"
		"vec4 frag_coord = M*vec4(position, 1.0); \n"
		"gl_Position = P*V*frag_coord; \n"
		"v_normal = vec3(itM * vec4(normal, 1.0)); \n"
		"v_frag_coord = frag_coord.xyz; \n"
		"\n"
		"}\n";

	const std::string sourceFRef = "#version 400 core\n"
		"out vec4 FragColor;\n"
		"precision mediump float; \n"

		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"

		"uniform vec3 u_view_pos; \n"
		"uniform samplerCube cubemapSampler; \n"

		"void main() { \n"
		"vec3 N = normalize(v_normal);\n"
		"vec3 V = normalize(u_view_pos - v_frag_coord); \n"
		"vec3 R = reflect(-V,N); \n"
		"FragColor = texture(cubemapSampler,R); \n"
		"} \n";

	Shader shaderRef(sourceVRef, sourceFRef);

	const std::string sourceVCubeMap = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coords; \n"
		"in vec3 normal; \n"
		
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"

		"out vec3 texCoord_v; \n"

		" void main(){ \n"
		"texCoord_v = position;\n"
		"mat4 V_no_rot = mat4(mat3(V)) ;\n"
		"vec4 pos = P * V_no_rot * vec4(position, 1.0); \n"
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

	const std::string sourceVRect = "#version 330 core\n"
		"layout(location = 0) in vec2 inPos;\n"
		"layout(location = 1) in vec2 inTexCoords;\n"
		"uniform int pressT;\n"
		"out vec2 texCoords; \n"

		" void main(){ \n"
		"gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);\n"
		"texCoords = inTexCoords;\n"
		"\n"
		"}\n";

	const std::string sourceFRect =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"uniform int pressT;\n"
		"in vec2 texCoords; \n"
		"uniform sampler2D screenTexture; \n"
		"void main() { \n"
		"   if (pressT == 0) {\n"  
		"		FragColor = texture(screenTexture,texCoords); \n"
		"   }\n"
		"   else {\n"  
		"		FragColor = vec4(vec3(1.0 - texture(screenTexture, texCoords)), 1.0);\n"
		"   }\n"
		"} \n";


	Shader shaderFrameBuffer(sourceVRect, sourceFRect);


	unsigned int rectVAO, rectVBO;
	glGenVertexArrays(1, &rectVAO);
	glGenBuffers(1, &rectVBO);
	glBindVertexArray(rectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));



	//1. Create Models

	char path1[] = "objet/cube.obj";
	char path2[] = "objet/mario_mushroom.obj";
	char path4[] = "objet/star.obj";
	char path3[] = "objet/pipe.obj";


	Object bloc1 = createObject(path1, glm::vec3(8.0, 4.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object bloc2 = createObject(path1, glm::vec3(4.0, 4.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object bloc3 = createObject(path1, glm::vec3(9.0, 0.0, 2.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object bloc4 = createObject(path1, glm::vec3(9.0, 0.0, -6.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object bloc5 = createObject(path1, glm::vec3(-4.0, 0.0, 2.0), glm::vec3(1.0, 1.0, 1.0), shader);

	Object blocVoyante = createObject(path1, glm::vec3(-9.0, 0.0, 0.0), glm::vec3(0.3, 1.3, 0.3), shader);

	Object blocInterro1 = createObject(path1, glm::vec3(6.0, 4.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object blocInterro2 = createObject(path1, glm::vec3(9.0, 0.0, -8.0), glm::vec3(1.0, 1.0, 1.0), shader);

	Object blocStone1 = createObject(path1, glm::vec3(-9.0, 0.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object blocStone2 = createObject(path1, glm::vec3(-7.0, 0.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object blocStone3 = createObject(path1, glm::vec3(-9.0, 0.0, 7.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object blocStone4 = createObject(path1, glm::vec3(-9.0, 2.0, 9.0), glm::vec3(1.0, 1.0, 1.0), shader);
	Object blocStone5 = createObject(path1, glm::vec3(11.0, 1.0, -3.0), glm::vec3(1.0, 3.0, 6.0), shader);

	Object pipe = createObject(path3, glm::vec3(-8.0, 0.0, -8.0), glm::vec3(0.02, 0.02, 0.02), shader);

	Object mushroom = createObject(path2, glm::vec3(9.0, -0.3, -2.0), glm::vec3(0.3, 0.3, 0.3), shader);

	Object star = createObject(path4, glm::vec3(-9.0, 6.0, 9.0), glm::vec3(0.5, 0.5, 0.5), shader);
	Object ground = createObject(path1, glm::vec3(0.0, -2.0, 0.0), glm::vec3(12.0, 1.0, 10.0), shader);

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

	std::string pathToCubeMap = "textures/";

	std::map<std::string, GLenum> facesToLoad = {
		{pathToCubeMap + "right.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "top.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "front.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "left.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "bottom.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "back.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}

	// Reflection
	char path7[] = "objet/sphere_smooth.obj";

	Object sphere1(path7);
	sphere1.makeObject(shaderRef);

	glm::vec3 light_posRef = glm::vec3(1.0, 2.0, 1.5);
	glm::mat4 modelRef = glm::mat4(1.0);
	modelRef = glm::translate(modelRef, glm::vec3(-9.0, 2.0 , 0.0));
	modelRef = glm::scale(modelRef, glm::vec3(0.5, 0.5, 0.5));

	glm::mat4 inverseModelRef = glm::transpose(glm::inverse(modelRef));

	glm::mat4 viewRef = camera.GetViewMatrix();
	glm::mat4 perspectiveRef = camera.GetProjectionMatrix();

	float ambientRef = 0.1;
	float diffuseRef = 0.5;
	float specularRef = 0.8;

	glm::vec3 materialColourRef = glm::vec3(0.5f, 0.6, 0.8);

	//Rendering
	shaderRef.use();
	shaderRef.setFloat("shininess", 32.0f);
	shaderRef.setVector3f("materialColour", materialColourRef);
	shaderRef.setFloat("light.ambient_strength", ambientRef);
	shaderRef.setFloat("light.diffuse_strength", diffuseRef);
	shaderRef.setFloat("light.specular_strength", specularRef);
	shaderRef.setFloat("light.constant", 1.0);
	shaderRef.setFloat("light.linear", 0.14);
	shaderRef.setFloat("light.quadratic", 0.07);



	//Position for the light
	const glm::vec3 light_pos = glm::vec3(-9.0, 6.0, 9.0);

	float ambient = 0.3;
	float diffuse = 0.5;
	float specular = 0.8;

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

	//Texture
	GLuint tGround = loadTexture("textures/grass.jpg", GL_TEXTURE0);
	GLuint tStar = loadTexture("textures/star.jpg", GL_TEXTURE1);
	GLuint tMushroom = loadTexture("textures/mushroom.jpg", GL_TEXTURE2);
	GLuint tBloc = loadTexture("textures/bloc.jpg", GL_TEXTURE3);
	GLuint tInterro = loadTexture("textures/blocItem.jpg", GL_TEXTURE4);
	GLuint tStone = loadTexture("textures/blocStone.jpg", GL_TEXTURE5);
	GLuint tPipe = loadTexture("textures/pipe.jpg", GL_TEXTURE6);

	//framebuffer
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//texture framebuffer
	GLuint fbtexture;
	glGenTextures(1, &fbtexture);
	glBindTexture(GL_TEXTURE_2D, fbtexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbtexture, 0);

	// renderbuffer
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shaderFrameBuffer.use();
	glUniform1i(glGetUniformLocation(shaderFrameBuffer.ID, "ourTexture"), 0);


	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	double timeValue = 0.0;
	bool startSong = false;

	vec3df soundPosition(0.0f, 0.0f, 0.0f);
	bool isMusicPlaying = false;

	float direction = 1.0f;

	int pressT = 0;
	int keyTState = GLFW_RELEASE;
	double timeButtonT = 0.0;


	//Rendering
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.99f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		if (timeValue > 3.0 && startSong==false)
		{
			SoundEngine->play2D("musics/MarioGalaxyCoffreJouet.mp3", true);
			startSong = true;
			isMusicPlaying = true;
		}

		if (isMusicPlaying) {
			vec3df listenerPosition(camera.Position.x, camera.Position.y, camera.Position.z);

			float distance = listenerPosition.getDistanceFrom(soundPosition);
			float minDistance = 5.0f;  
			float maxDistance = 45.0f; 
			float volume = 1.0f - (distance - minDistance) / (maxDistance - minDistance);
			volume = std::max(0.0f, std::min(1.0f, volume));

			SoundEngine->setSoundVolume(volume);

		}

		shader.use();
		shader.setVector3f("light.light_pos", light_pos);
		shader.setFloat("light.ambient_strength", ambient);
		shader.setFloat("light.diffuse_strength", diffuse);
		shader.setFloat("light.specular_strength", specular);
		shader.setFloat("light.constant", 1.0);
		shader.setFloat("light.linear", 0.2);
		shader.setFloat("light.quadratic", 0.1);

		shader.setVector3f("light.light_pos", light_pos);


		glBindTexture(GL_TEXTURE_2D, tMushroom);
		shader.setInteger("textureType", 1);
		shader.setInteger("ourTexture", 2);

		// Deplacement Champignon
		glm::vec3 target1(9.0f, 0.0f, -4.4f);
		glm::vec3 target2(9.0f, 0.0f, 0.4f);
		if (mushroom.model[3][2] >= target2.z || mushroom.model[3][2] <= target1.z) {
			SoundEngine->play2D("musics/Bump.mp3", false);
			direction *= -1.0f;
			mushroom.model = glm::translate(mushroom.model, glm::vec3(0.0f, 0.0f, 0.15f * direction));
		}
		else {
			mushroom.model = glm::translate(mushroom.model, glm::vec3(0.0f, 0.0f, 0.10f * direction));
		}
		shader.setMatrix4("M", mushroom.model);
		mushroom.draw();
 
		glBindTexture(GL_TEXTURE_2D, tBloc);
		shader.setInteger("ourTexture", 3);

		shader.setMatrix4("M", bloc1.model);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_light_pos", light_pos);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		bloc1.draw();

		shader.setMatrix4("M", bloc2.model);
		bloc2.draw();

		shader.setMatrix4("M", bloc3.model);
		bloc3.draw();

		shader.setMatrix4("M", bloc4.model);
		bloc4.draw();

		shader.setMatrix4("M", bloc5.model);
		bloc5.draw();


		glBindTexture(GL_TEXTURE_2D, tStone);
		shader.setInteger("ourTexture", 5);
		shader.setMatrix4("M", blocStone1.model);

		shader.setMatrix4("M", blocStone1.model);
		blocStone1.draw();

		shader.setMatrix4("M", blocStone2.model);
		blocStone2.draw();

		shader.setMatrix4("M", blocStone3.model);
		blocStone3.draw();

		shader.setMatrix4("M", blocStone4.model);
		blocStone4.draw();

		shader.setMatrix4("M", blocStone5.model);
		blocStone5.draw();

		shader.setMatrix4("M", blocVoyante.model);
		blocVoyante.draw();

		glBindTexture(GL_TEXTURE_2D, tInterro);
		shader.setInteger("ourTexture",4); 

		shader.setMatrix4("M", blocInterro1.model);
		blocInterro1.draw();

		shader.setMatrix4("M", blocInterro2.model);
		blocInterro2.draw();

		// Mettre à jour le temps
		double deltaTime = now - prev;
		timeValue += deltaTime;  
		prev = now;
		star.model = glm::translate(star.model, glm::vec3(0.0f, 0.05f * sin(3.0 * timeValue), 0.0f));
		star.model = glm::rotate(star.model, (float)0.05f, glm::vec3(0.0f, 1.0f, 0.0f));

		glBindTexture(GL_TEXTURE_2D, tStar);
		shader.setInteger("ourTexture", 1);
		shader.setMatrix4("M", star.model);
		star.draw();

		glBindTexture(GL_TEXTURE_2D, tPipe);
		shader.setInteger("ourTexture", 6);
		shader.setMatrix4("M", pipe.model);
		pipe.draw();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tGround);
		shader.setInteger("textureType", 0); 
		shader.setInteger("ourTexture", 0);
		shader.setMatrix4("M", ground.model);
		ground.draw();

		glDepthFunc(GL_LEQUAL);

		
		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		cubeMapShader.setInteger("cubemapTexture", 0);
		cubeMap.draw();
		
		viewRef = camera.GetViewMatrix();

		shaderRef.use();

		shaderRef.setMatrix4("M", modelRef);
		shaderRef.setMatrix4("itM", inverseModelRef);
		shaderRef.setMatrix4("V", viewRef);
		shaderRef.setMatrix4("P", perspectiveRef);
		shaderRef.setVector3f("u_view_pos", camera.Position);
		auto deltaRef = light_posRef + glm::vec3(0.0, 0.0, 2 * std::sin(now));

		shaderRef.setVector3f("light.light_pos", deltaRef);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		cubeMapShader.setInteger("cubemapTexture", 0);

		glDepthFunc(GL_LEQUAL);
		sphere1.draw();
		
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); 
		shaderFrameBuffer.use();
		glBindVertexArray(rectVAO);
		glDisable(GL_DEPTH_TEST); 
		glBindTexture(GL_TEXTURE_2D, fbtexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		shaderFrameBuffer.setInteger("pressT", pressT);

		fps(now);
		glfwSwapBuffers(window);
		
		double timeForTButton = now - timeButtonT;
		int newKeyTState = glfwGetKey(window, GLFW_KEY_T);
		if (newKeyTState == GLFW_PRESS && keyTState == GLFW_RELEASE) {
			if (timeForTButton > 3.0) {
				if (pressT == 0)
					pressT = 1;
				else
					pressT = 0;
				timeButtonT = now;
			}
		}

		keyTState = newKeyTState;

	}
	//clean up ressource
	glDeleteFramebuffers(1, &fbo);
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
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}
	stbi_image_free(data);
}

Object createObject(const char* path, const glm::vec3& translation, const glm::vec3& scale, Shader& shader){
	Object obj(path);
	obj.makeObject(shader);
	obj.model = glm::translate(obj.model, translation);
	obj.model = glm::scale(obj.model, scale);
	return obj;
}

GLuint loadTexture(const char* path, GLenum textureUnit){
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

	if (data){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else{
		std::cout << "Failed to load texture: " << path << std::endl;
	}

	stbi_image_free(data);
	return texture;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
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
