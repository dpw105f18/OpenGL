#include "Window.h"
#include <iostream>
#define GLEW_STATIC
#include "include/GL/glew.h"
#include <string>
#include <fstream>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/random.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include <sstream>
#include "test_config.hpp"
#include "Camera.h"
#include "Scene.h"
#include <chrono>

#include "SkullVertices.h"
#include "SkullIndices.h"

void GLAPIENTRY message_callback(
	GLenum			source,
	GLenum			type,
	GLuint			id,
	GLenum			severity,
	GLsizei			length,
	const GLchar*	message,
	const void*		userParam)
{
	std::cerr << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "**ERROR**" : "") << "type = 0x" << std::hex << type << ", severity = 0x" << severity << ", message = " << message << std::endl;
}

bool load_shader(GLuint shader, std::string path)
{
	std::ifstream shader_stream(path, std::ios::ate);
	auto file_size = shader_stream.tellg();
	std::string vertex_shader_source(file_size, '\0');
	shader_stream.seekg(0);
	shader_stream.read(&vertex_shader_source[0], file_size);

	GLint result = GL_FALSE;
	int info_length;
	
	//std::cout << "Compiling shader: " << path << std::endl;
	auto shader_pointer = &vertex_shader_source[0];
	glShaderSource(shader, 1, &shader_pointer, nullptr);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
	if ( info_length > 0 ){
		std::string error_message(info_length+1, '\0');
		glGetShaderInfoLog(shader, info_length, nullptr, &error_message[0]);
		//std::cout << error_message << std::endl;
		return false;
	}
	return true;
}

GLuint load_shaders()
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	load_shader(vertex_shader, "shaders/shader.vert");
	load_shader(fragment_shader,
#ifdef TEST_USE_SKULL
		"shaders/skull.frag"
#else
		"shaders/shader.frag"
#endif
	);

	//std::cout << "Linking program\n";
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint result = GL_FALSE;
	int info_length;

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_length);
	if ( info_length > 0 ){
		std::string error_message(info_length+1, '\0');
		glGetProgramInfoLog(program, info_length, nullptr, &error_message[0]);
		//std::cout << &error_message[0] << std::endl;
	}
	
	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

size_t constexpr width = 800;
size_t constexpr height = 600;

//Returns pixel data in a std::vector<char>. Pixels are loaded in a RGBA format.
//the height and width of the image is provided through the parameters outWidth and outHeight.
static std::vector<char> readPixels(const std::string& localPath, int& outWidth, int& outHeight)
{
	int texChannels;
	auto pixels = stbi_load(localPath.c_str(), &outWidth, &outHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	auto result = std::vector<char>();
	result.resize(outWidth * outHeight * 4);
	memcpy(result.data(), pixels, result.size());
	stbi_image_free(pixels);

	return result;
}

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
};

std::vector<Vertex> cube_vertices{
	Vertex{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec2{ 0.0f, 0.0f } },
	Vertex{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec2{ 0.0f, 1.0f } },
	Vertex{ glm::vec3{ 0.5f,   0.5f,  0.5f }, glm::vec2{ 1.0f, 1.0f } },
	Vertex{ glm::vec3{ 0.5f,  -0.5f,  0.5f }, glm::vec2{ 1.0f, 0.0f } },
	Vertex{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec2{ 0.0f, 0.0f } },
	Vertex{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec2{ 0.0f, 1.0f } },
	Vertex{ glm::vec3{ 0.5f,   0.5f, -0.5f }, glm::vec2{ 1.0f, 1.0f } },
	Vertex{ glm::vec3{ 0.5f,  -0.5f, -0.5f }, glm::vec2{ 1.0f, 0.0f } }
};

std::vector<uint16_t> cube_indices{
	// Front
	0, 1, 2,
	0, 2, 3,
	// Top
	3, 7, 4,
	3, 4, 0,
	// Right
	3, 2, 6,
	3, 6, 7,
	// Back
	7, 6, 5,
	7, 5, 4,
	// Bottom
	1, 5, 6,
	1, 6, 2,
	// Left
	4, 5, 1,
	4, 1, 0
};

void SaveToFile(const std::string& file, const std::string& data)
{
	std::ofstream fs;
	fs.open(file, std::ofstream::app);
	fs << data;
	fs.close();
}

int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	HGLRC context{};

	TestConfiguration::SetTestConfiguration(lpCmdLine);
	auto testConfig = TestConfiguration::GetInstance();

	auto cubeCountPerDim = testConfig.cubeDimension;
	auto paddingFactor = testConfig.cubePadding;

	Camera camera = Camera::Default();
	auto heightFOV = camera.FieldOfView() / (float(width) / float(height));
	auto base = (cubeCountPerDim + (cubeCountPerDim - 1) * paddingFactor) / 2.0f;
	auto camDistance = base / std::tan(heightFOV / 2);
	float z = camDistance + base + camera.Near();

	camera.SetPosition({ 0.0f, 0.0f, z, 1.0f });
	auto magicFactor = 2;
	camera.SetFar(magicFactor * (z + base + camera.Near()));
	auto scene = Scene(camera, cubeCountPerDim, paddingFactor);

	try{
		Window window(hInstance, "MyWindow", "MyWindow", width, height);

		HDC hdc = GetDC(window.GetHandle());
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
			PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
			32,                   // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                   // Number of bits for the depthbuffer
			8,                    // Number of bits for the stencilbuffer
			0,                    // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		auto pixel_format = ChoosePixelFormat(hdc, &pfd);

		SetPixelFormat(hdc, pixel_format, &pfd);

		context = wglCreateContext(hdc);
		wglMakeCurrent(hdc, context);

		auto result = glewInit();
		if(result != GLEW_OK) throw std::runtime_error(std::string("Failed to initialize GLEW. Error: ") + (char*)glewGetErrorString(result));

		//glEnable(GL_DEBUG_OUTPUT);
		//glDebugMessageCallback(message_callback, nullptr);

		//std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

#ifdef TEST_USE_SKULL
		#define vertices skullVertices
		#define indices skullIndices
#else
		#define vertices cube_vertices
		#define indices cube_indices
#endif
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vertexbuffer, indexbuffer;

		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &indexbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

#ifndef TEST_USE_SKULL
		int texW, texH;
		auto pixels = readPixels("textures/texture.png", texW, texH);

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif

		const auto shader_program = load_shaders();

		glm::mat4 projection = glm::perspective(
			camera.FieldOfView(), 
			static_cast<float>(width)/static_cast<float>(height),
			camera.Near(), camera.Far());
		glm::mat4 view = lookAt(
			glm::vec3(camera.Position()), 
			glm::vec3(camera.Target()), 
			glm::vec3(camera.Up())
		);
		auto vp = projection * view;
		auto vp_location = glGetUniformLocation(shader_program, "vp");
		auto model_location = glGetUniformLocation(shader_program, "model");

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LESS);
		glFrontFace(GL_CW);
		glCullFace(GL_BACK);

		bool running = true;

		using Clock = std::chrono::high_resolution_clock;
		auto startTime = Clock::now();
		auto lastUpdate = Clock::now();
		auto lastFrame = Clock::now();
		long fps = 0;
		long oldFps = 0;

		std::stringstream frametimeCsv;
		frametimeCsv << "frametime (ms)\n";
		
		std::stringstream fpsCsv;
		fpsCsv << "FPS\n";

		auto runTime = Clock::now() - startTime;
		auto currentDataCount = 0;

		glClearColor(0.1f, 0.2f, 0.8f, 1.0f);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
		   0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		   sizeof(Vertex::position)/sizeof(float),                  // size
		   GL_FLOAT,           // type
		   GL_FALSE,           // normalized?
		   sizeof(Vertex),	   // stride
		   reinterpret_cast<void*>(offsetof(Vertex, position)) // array buffer offset
		);
		glVertexAttribPointer(
		   1,                  // attribute 1
		   sizeof(Vertex::uv)/sizeof(float), // size
		   GL_FLOAT,           // type
		   GL_TRUE,           // normalized?
		   sizeof(Vertex),     // stride
		   reinterpret_cast<void*>(offsetof(Vertex, uv))             // array buffer offset
		);

		do
		{
			MSG message;
			if(PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) 
			{
				if(message.message == WM_QUIT) running = false;
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else
			{
				auto deltaTime = (Clock::now() - lastUpdate);
				auto frameTime = (Clock::now() - lastFrame);
				lastFrame = Clock::now();

				auto deltaTimeMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(deltaTime).count();

				using namespace std::chrono_literals;
				if (deltaTime > 1s) {
					lastUpdate = Clock::now();
					auto frameTimeCount = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(frameTime).count();

					std::stringstream ss;
					ss << "FPS: " << fps
						<< " --- Avg. Frame Time: " << deltaTimeMs / fps << "ms"
						<< " --- Last Frame Time: " << frameTimeCount << "ms";
					SetWindowText(window.GetHandle(), ss.str().c_str());
					
					if (TestConfiguration::GetInstance().recordFPS) {
						fpsCsv << oldFps << "\n";
					}

					oldFps = fps;
					fps = 0;

					if (TestConfiguration::GetInstance().recordFrameTime) {
						frametimeCsv << frameTimeCount << "\n";
						++currentDataCount;
					}
				}

				glUseProgram(shader_program);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

				glUniformMatrix4fv(vp_location, 1, GL_FALSE, &vp[0][0]);

				for(auto& ro : scene.renderObjects())
				{
					auto model_matrix = translate(glm::mat4(1.0f), glm::vec3(ro.x(),ro.y(), ro.z()));
					glUniformMatrix4fv(model_location, 1, GL_FALSE, &model_matrix[0][0]);
					glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);
				}
				SwapBuffers(hdc);
				++fps;
			}
			runTime = Clock::now() - startTime;
		} while(running && ((testConfig.seconds == 0 || std::chrono::duration<double, std::milli>(runTime).count() < testConfig.seconds * 1000) && testConfig.dataCount == 0 || currentDataCount < testConfig.dataCount));

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		auto now = time(NULL);
		tm* localNow = new tm();
		localtime_s(localNow, &now);

		auto yearStr = std::to_string((1900 + localNow->tm_year));
		auto monthStr = localNow->tm_mon < 9 ? "0" + std::to_string(localNow->tm_mon + 1) : std::to_string(localNow->tm_mon + 1);
		auto dayStr = localNow->tm_mday < 10 ? "0" + std::to_string(localNow->tm_mday) : std::to_string(localNow->tm_mday);
		auto hourStr = localNow->tm_hour < 10 ? "0" + std::to_string(localNow->tm_hour) : std::to_string(localNow->tm_hour);
		auto minStr = localNow->tm_min < 10 ? "0" + std::to_string(localNow->tm_min) : std::to_string(localNow->tm_min);
		auto secStr = localNow->tm_sec < 10 ? "0" + std::to_string(localNow->tm_sec) : std::to_string(localNow->tm_sec);

		auto fname = yearStr + monthStr + dayStr + hourStr + minStr + secStr;

		if (testConfig.exportCsv) {
			auto csvStr = testConfig.MakeString(";");
			SaveToFile("conf_" + fname + ".csv", csvStr);
		}

		if (testConfig.recordFPS) {
			SaveToFile("fps_" + fname + ".csv", fpsCsv.str());
		}

		if (testConfig.recordFrameTime) {
			SaveToFile("frameTime_" + fname + ".csv", frametimeCsv.str());
		}

		delete localNow;
	}
	catch(std::runtime_error& e)
	{
		MessageBox(nullptr, e.what(), "Error!", MB_ICONERROR);
	}
	wglDeleteContext(context);
	return 0;
}
