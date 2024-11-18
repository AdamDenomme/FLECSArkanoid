
// This reads .h2b files which are optimized binary .obj+.mtl files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "h2bParser.h"
#include "FileIntoString.h"
#include "components.h"
#include "gameplay.h"
#include <vector>
#include <list>
#include <string>
#include <chrono>

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// Used to print debug infomation from OpenGL, pulled straight from the official OpenGL wiki.
#ifndef NDEBUG
void APIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam) {


	std::string errMessage;
	errMessage = (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "");
	errMessage += " type = ";
	errMessage += type;
	errMessage += ", severity = ";
	errMessage += severity;
	errMessage += ", message = ";
	errMessage += message;
	errMessage += "\n";

	PrintLabeledDebugString("GL CALLBACK: ", errMessage.c_str());
}
#endif

struct UBO_DATA {
	GW::MATH::GVECTORF sunDirection;
	GW::MATH::GVECTORF sunColor;
	GW::MATH::GMATRIXF viewMatrix;
	GW::MATH::GMATRIXF projectionMatrix;
	GW::MATH::GMATRIXF worldMatrix;
	GW::MATH::GVECTORF fogColor;
	float fogDensity;
};

struct Light {
	std::string type;
	GW::MATH::GVECTORF color;
	float energy;
	GW::MATH::GMATRIXF transform;
};



// class Model contains everyhting needed to draw a single 3D model
class Model {
	// Name of the Model in the GameLevel (useful for debugging)
	std::string name;
	// Loads and stores CPU model data from .h2b file
	H2B::Parser cpuModel; // reads the .h2b format
	// Shader variables needed by this model. 
	GW::MATH::GMATRIXF world;
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;
	GLuint shaderExecutable = 0;
	GLuint vao = 0;
	GLuint vertexBufferObject = 0;
	GLuint indexBufferObject = 0;
	GLuint textureID = 0;
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;


public:
	
	void CompileVertexShader()
	{
		char errors[1024];
		GLint result;

		vertexShader = glCreateShader(GL_VERTEX_SHADER);

		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.glsl");
		const GLchar* strings[1] = { vertexShaderSource.c_str() };
		const GLint lengths[1] = { vertexShaderSource.length() };
		glShaderSource(vertexShader, 1, strings, lengths);

		glCompileShader(vertexShader);
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
		if (result == false)
		{
			glGetShaderInfoLog(vertexShader, 1024, NULL, errors);
			PrintLabeledDebugString("Vertex Shader Errors:\n", errors);
			abort();
			return;
		}
	}

	void CompileFragmentShader()
	{
		char errors[1024];
		GLint result;

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		std::string fragmentShaderSource = ReadFileIntoString("../Shaders/FragmentShader.glsl");
		const GLchar* strings[1] = { fragmentShaderSource.c_str() };
		const GLint lengths[1] = { fragmentShaderSource.length() };
		glShaderSource(fragmentShader, 1, strings, lengths);

		glCompileShader(fragmentShader);
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
		if (result == false)
		{
			glGetShaderInfoLog(fragmentShader, 1024, NULL, errors);
			PrintLabeledDebugString("Fragment Shader Errors:\n", errors);
			abort();
			return;
		}
	}

	void CreateExecutableShaderProgram()
	{
		char errors[1024];
		GLint result;

		shaderExecutable = glCreateProgram();
		glAttachShader(shaderExecutable, vertexShader);
		glAttachShader(shaderExecutable, fragmentShader);
		glLinkProgram(shaderExecutable);
		glGetProgramiv(shaderExecutable, GL_LINK_STATUS, &result);
		if (result == false)
		{
			glGetProgramInfoLog(shaderExecutable, 1024, NULL, errors);
			std::cout << errors << std::endl;
		}
	}

	const std::string& GetName() const {
		return name;
	}

	inline void SetName(std::string modelName) {
		name = modelName;
	}
	inline void SetWorldMatrix(GW::MATH::GMATRIXF worldMatrix) {
		world = worldMatrix;
	}

	bool LoadModelDataFromDisk(const char* h2bPath) {
		// if this succeeds "cpuModel" should now contain all the model's info
		return cpuModel.Parse(h2bPath);
	}

	bool LoadTextureFromFile(const char* texturePath) {
		int width, height, nrChannels;
		unsigned char* data = stbi_load(texturePath, &width, &height, &nrChannels, 0);
		if (data) {
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			stbi_image_free(data);
			return true;
		}
		else {
			stbi_image_free(data);
			return false;
		}
	}

	bool UploadModelData2GPU(/*specific API device for loading*/) {
		vertices = cpuModel.vertices;
		indices = cpuModel.indices;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vertexBufferObject);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(H2B::VERTEX), vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &indexBufferObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

		// Set up vertex attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(H2B::VERTEX), (void*)offsetof(H2B::VERTEX, pos));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(H2B::VERTEX), (void*)offsetof(H2B::VERTEX, uvw));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(H2B::VERTEX), (void*)offsetof(H2B::VERTEX, nrm));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);

		return true;
		
	}
	bool DrawModel(GLuint shaderExecutable, UBO_DATA& uboData, GLuint ubo, GW::MATH::GVECTORF mapCenter) {
		glBindVertexArray(vao);

		// Update world matrix
		uboData.worldMatrix = world;
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBO_DATA), &uboData);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// Bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(shaderExecutable, "texture_diffuse"), 0);

		// Set ambient color
		glUniform3f(glGetUniformLocation(shaderExecutable, "ambientColor"), 0.2f, 0.2f, 0.2f); 

		// Set fog uniforms
		glUniform3fv(glGetUniformLocation(shaderExecutable, "fogColor"), 1, &uboData.fogColor.x);
		glUniform1f(glGetUniformLocation(shaderExecutable, "fogDensity"), uboData.fogDensity);

		// Set map center
		glUniform3fv(glGetUniformLocation(shaderExecutable, "mapCenter"), 1, &mapCenter.x);

		// Ensure the correct shader program is being used
		glUseProgram(shaderExecutable);

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		return true;
	}
	bool FreeResources(/*specific API device for unloading*/) {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vertexBufferObject);
		glDeleteBuffers(1, &indexBufferObject);
		glDeleteTextures(1, &textureID);
		return true;
	}
}; 

// class Level_Objects is simply a list of all the Models currently used by the level
class Level_Objects {

	// store all our models
	std::list<Model> allObjectsInLevel;
	GW::MATH::GMATRIXF view;
	GW::MATH::GMATRIXF projection;
	GW::MATH::GMatrix matrixProxy;
	GW::MATH::GVector vectorProxy;
	GW::MATH::GVECTORF mapCenter;
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GOpenGLSurface ogl;
	std::shared_ptr<Level_Data> levelData;
	std::shared_ptr<flecs::world> world;

	// Global variables
	
	UBO_DATA uboData;
	GLuint ubo = 0;
	GLuint shaderExecutable = 0;
	std::vector<Light> lights;
	GW::MATH::GVECTORF sunDirection;
	GW::MATH::GVECTORF sunColor;
	float FOV;
	unsigned int width, height;
	float aspectRatio;

	bool isDayTime = true;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimeSwitch;
public:
	Level_Objects() = default;

	GLuint GetShaderProgram() const {
		return shaderExecutable;
	}

	Level_Objects(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GOpenGLSurface _ogl, std::shared_ptr<Level_Data> _levelData, std::shared_ptr<flecs::world> _world)
		: win(_win), ogl(_ogl), levelData(_levelData), world(_world) {
		InitializeMatricesAndLighting();
		InitializeUBO();
		CompileShaders();
		glUseProgram(shaderExecutable);
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		FOV = 65.0f * (G_PI_F / 180.0f);
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		mapCenter = { 0.0f, 0.0f, 0.0f };
		lastTimeSwitch = std::chrono::high_resolution_clock::now();
	}

	
	
	inline void SetViewMatrix(GW::MATH::GMATRIXF viewMatrix) {
		view = viewMatrix;
	}

	inline void SetProjectionMatrix(GW::MATH::GMATRIXF projectionMatrix) {
		projection = projectionMatrix;
	}


	// Imports the default level txt format and creates a Model from each .h2b
	bool LoadLevel(const char* gameLevelPath, const char* h2bFolderPath, GW::SYSTEM::GLog log) {
		log.LogCategorized("EVENT", "LOADING GAME LEVEL [OBJECT ORIENTED]");
		log.LogCategorized("MESSAGE", "Begin Reading Game Level Text File.");

		UnloadLevel(); // clear previous level data if there is any
		GW::SYSTEM::GFile file;
		file.Create();
		if (-file.OpenTextRead(gameLevelPath)) {
			log.LogCategorized("ERROR", (std::string("Game level not found: ") + gameLevelPath).c_str());
			return false;
		}
		char linebuffer[1024];
		while (+file.ReadLine(linebuffer, 1024, '\n')) {
			if (linebuffer[0] == '\0')
				break;
			if (std::strcmp(linebuffer, "MESH") == 0) {
				Model newModel;
				file.ReadLine(linebuffer, 1024, '\n');
				newModel.SetName(linebuffer);
				std::string modelFile = linebuffer;
				modelFile = modelFile.substr(0, modelFile.find_last_of("."));
				modelFile += ".h2b";

				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(linebuffer, 1024, '\n');
					std::sscanf(linebuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);

				modelFile = std::string(h2bFolderPath) + "/" + modelFile;
				newModel.SetWorldMatrix(transform);
				if (newModel.LoadModelDataFromDisk(modelFile.c_str())) {
					file.ReadLine(linebuffer, 1024, '\n');
					if (std::strcmp(linebuffer, "TEXTURE") == 0) {
						file.ReadLine(linebuffer, 1024, '\n');
						std::string textureFile = linebuffer;
						if (newModel.LoadTextureFromFile(textureFile.c_str())) {
							allObjectsInLevel.push_back(std::move(newModel));
						}
						else {
							log.LogCategorized("ERROR", (std::string("Texture Not Found: ") + textureFile).c_str());
						}
					}
					else {
						log.LogCategorized("ERROR", "Texture information missing in game level file.");
					}
				}
				else {
					log.LogCategorized("ERROR", (std::string("H2B Not Found: ") + modelFile).c_str());
					log.LogCategorized("WARNING", "Loading will continue but model(s) are missing.");
				}
			}
			else if (std::strcmp(linebuffer, "LIGHT") == 0) {
				file.ReadLine(linebuffer, 1024, '\n'); // Read the type
				std::string lightType = linebuffer;

				if (lightType == "SUN") {
					file.ReadLine(linebuffer, 1024, '\n'); // Read the color
					float r, g, b;
					std::sscanf(linebuffer, "Color: %f %f %f", &r, &g, &b);
					sunColor = { r, g, b, 1.0f };

					file.ReadLine(linebuffer, 1024, '\n'); // Read the direction
					float x, y, z;
					std::sscanf(linebuffer, "Direction: %f %f %f", &x, &y, &z);
					sunDirection = { x, y, z, 0.0f };

					log.LogCategorized("INFO", "Sun data parsed successfully.");
				}
			}
		}
		log.LogCategorized("MESSAGE", "Game Level File Reading Complete.");
		log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU [OBJECT ORIENTED]");
		return true;
	}
	// Upload the CPU level to GPU
	void UploadLevelToGPU(/*pass handle to API device if needed*/) {
		// iterate over each model and tell it to draw itself
		for (auto& e : allObjectsInLevel) {
			e.UploadModelData2GPU(/*forward handle to API device if needed*/);
		}
	}
	// Draws all objects in the level
	void RenderLevel() {
		glUseProgram(shaderExecutable);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
		UpdateUBO();

		// Set light properties uniforms
		GLint lightDirLocation = glGetUniformLocation(shaderExecutable, "lightDir");
		GLint lightColorLocation = glGetUniformLocation(shaderExecutable, "lightColor");

		if (lightDirLocation == -1 || lightColorLocation == -1) {
			std::cerr << "Failed to get uniform location for lightDir or lightColor." << std::endl;
		}
		else {
			if (!lights.empty()) {
				glUniform3fv(lightDirLocation, 1, &sunDirection.x);
				glUniform3fv(lightColorLocation, 1, &sunColor.x);
			}
			else {
				// Fallback to hardcoded values if no lights are available
				GW::MATH::GVECTORF hardcodedLightDir = { 0.5f, 1.0f, 0.3f };
				GW::MATH::GVECTORF hardcodedLightColor = { 1.0f, 1.0f, 1.0f };
				glUniform3fv(lightDirLocation, 1, &hardcodedLightDir.x);
				glUniform3fv(lightColorLocation, 1, &hardcodedLightColor.x);
			}
		}

		for (auto& e : allObjectsInLevel) {
			// Update the world matrix from the corresponding Flecs entity
			auto flecsEntity = world->lookup(e.GetName().c_str());
			if (flecsEntity.is_valid()) {
				const ModelTransform* transform = flecsEntity.get<ModelTransform>();
				if (transform) {
					e.SetWorldMatrix(transform->matrix);
				}
			}
			e.DrawModel(shaderExecutable, uboData, ubo, mapCenter);
		}
	}
	// used to wipe CPU & GPU level data between levels
	void UnloadLevel() {
		for (auto& e : allObjectsInLevel) {
			e.FreeResources();
		}
		allObjectsInLevel.clear();
		lights.clear();
	}

	void InitializeUBO() {
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO_DATA), nullptr, GL_STATIC_DRAW);
		glUseProgram(shaderExecutable);
		GLuint uboIndex = glGetUniformBlockIndex(shaderExecutable, "UboData");
		if (uboIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(shaderExecutable, uboIndex, 0);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void UpdateUBO() {
		uboData.viewMatrix = view;
		uboData.projectionMatrix = projection;
		uboData.sunDirection = sunDirection; // Use the parsed sun direction
		uboData.sunColor = sunColor; // Use the parsed sun color
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBO_DATA), &uboData);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// Set the light direction and color uniforms
		GLint lightDirLocation = glGetUniformLocation(shaderExecutable, "lightDir");
		GLint lightColorLocation = glGetUniformLocation(shaderExecutable, "lightColor");
		glUseProgram(shaderExecutable);
		glUniform3fv(lightDirLocation, 1, &uboData.sunDirection.x);
		glUniform3fv(lightColorLocation, 1, &uboData.sunColor.x);

		GLint fogColorLocation = glGetUniformLocation(shaderExecutable, "fogColor");
		GLint fogDensityLocation = glGetUniformLocation(shaderExecutable, "fogDensity");
		glUniform3fv(fogColorLocation, 1, &uboData.fogColor.x);
		glUniform1f(fogDensityLocation, uboData.fogDensity);

		GLint mapCenterLocation = glGetUniformLocation(shaderExecutable, "mapCenter");
		glUniform3fv(mapCenterLocation, 1, &mapCenter.x);
	}

	void InitializeMatricesAndLighting() {
		matrixProxy.IdentityF(uboData.worldMatrix);

		//Camera//
		GW::MATH::GVECTORF cameraPosition = { -0.2f, 2.0f, 3.3f };
		GW::MATH::GVECTORF targetPosition = { -0.2f, 2.0f, 0.0f };
		GW::MATH::GVECTORF upDirection = { 0.0f, 1.0f, 0.0f };

		matrixProxy.LookAtRHF(cameraPosition, targetPosition, upDirection, view);

		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		float fov = 110.0f * (G_PI_F / 180.0f);
		float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		matrixProxy.ProjectionOpenGLRHF(fov, aspectRatio, 0.1f, 100.0f, projection);

		vectorProxy.NormalizeF(sunDirection, uboData.sunDirection);
		uboData.sunColor = sunColor;


	}


	void CompileShaders() {
		char errors[1024];
		GLint result;

		// Vertex Shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.glsl");
		const GLchar* vertexStrings[1] = { vertexShaderSource.c_str() };
		const GLint vertexLengths[1] = { vertexShaderSource.length() };
		glShaderSource(vertexShader, 1, vertexStrings, vertexLengths);
		glCompileShader(vertexShader);
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
		if (result == false) {
			glGetShaderInfoLog(vertexShader, 1024, NULL, errors);
			PrintLabeledDebugString("Vertex Shader Errors:\n", errors);
			abort();
		}

		// Fragment Shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		std::string fragmentShaderSource = ReadFileIntoString("../Shaders/FragmentShader.glsl");
		const GLchar* fragmentStrings[1] = { fragmentShaderSource.c_str() };
		const GLint fragmentLengths[1] = { fragmentShaderSource.length() };
		glShaderSource(fragmentShader, 1, fragmentStrings, fragmentLengths);
		glCompileShader(fragmentShader);
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
		if (result == false) {
			glGetShaderInfoLog(fragmentShader, 1024, NULL, errors);
			PrintLabeledDebugString("Fragment Shader Errors:\n", errors);
			abort();
		}

		// Link Shader Program
		shaderExecutable = glCreateProgram();
		glAttachShader(shaderExecutable, vertexShader);
		glAttachShader(shaderExecutable, fragmentShader);
		glLinkProgram(shaderExecutable);
		glGetProgramiv(shaderExecutable, GL_LINK_STATUS, &result);
		if (result == false) {
			glGetProgramInfoLog(shaderExecutable, 1024, NULL, errors);
			PrintLabeledDebugString("Shader Program Linking Errors:\n", errors);
			abort();
		}

		// Cleanup shaders (no longer needed once linked)
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		// Use the shader program
		glUseProgram(shaderExecutable);
	}

	void ChangeLevel(const char* levelPath, const char* modelPath, std::shared_ptr<Level_Data>& gameLevel, Level_Objects& objectOrientedLoader, std::unique_ptr<Gameplay>& engine, GW::SYSTEM::GLog& log)
	{
		log.LogCategorized("EVENT", "Changing Level");

		objectOrientedLoader.UnloadLevel();

		auto newGameLevel = std::make_shared<Level_Data>();
		if (newGameLevel->LoadLevel(levelPath, modelPath, log))
		{
			gameLevel = newGameLevel;

			objectOrientedLoader.LoadLevel(levelPath, modelPath, log);
			objectOrientedLoader.UploadLevelToGPU();


			engine = std::make_unique<Gameplay>(*gameLevel, log);
		}
		else
		{
			log.LogCategorized("ERROR", "Failed to load new level");
		}
	}

	void CheckForLevelChange(std::shared_ptr<Level_Data>& gameLevel, Level_Objects& objectOrientedLoader, std::unique_ptr<Gameplay>& engine, GW::INPUT::GInput& input, GW::SYSTEM::GLog& log)
	{
		float f1State = 0, f2State = 0;
		input.GetState(G_KEY_F1, f1State);
		input.GetState(G_KEY_F2, f2State);

		if (f1State > 0)
		{
			ChangeLevel("../GameLevel_1.txt", "../Models", gameLevel, objectOrientedLoader, engine, log);
		}
		else if (f2State > 0)
		{
			ChangeLevel("../GameLevel.txt", "../Models", gameLevel, objectOrientedLoader, engine, log);
		}
	}

};

