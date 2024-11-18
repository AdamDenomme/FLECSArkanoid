// This is a sample of how to load a level in a object oriented fashion.
// Feel free to use this code as a base and tweak it for your needs.

// This reads .h2b files which are optimized binary .obj+.mtl files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Systems/h2bParser.h"
#include "Systems/FileIntoString.h"
#include "OpenGLExtensions.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../../flecs-3.1.4/flecs.h"
#include <vector>
#include <list>
#include <string>
#include <chrono>
#include <iomanip> 

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

struct CameraParams {
	float lens;
	float fov;
	float clipStart;
	float clipEnd;
	GW::MATH::GVECTORF position;
	GW::MATH::GVECTORF rotation;
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
			textureID = 0;  // Indicate no texture
			return false;
		}
	}

	std::string GetName() const {
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
		glUniform3f(glGetUniformLocation(shaderExecutable, "ambientColor"), 0.2f, 0.2f, 0.2f); // Example ambient color

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

// * NOTE: *
// Unlike the DOP version, this class was not designed to reuse data in anyway or process it efficiently.
// You can find ways to make it more efficient by sharing pointers to resources and sorting the models.
// However, this is tricky to implement and can be prone to errors. (OOP data isolation becomes an issue)
// A huge positive is that everything you want to draw is totally self contained and easy to see/find.
// This means updating matricies, adding new objects & removing old ones from the world is a breeze. 
// You can even easily load brand new models from disk at run-time without much trouble.
// The last major downside is trying to do things like dynamic lights, shadows and sorted transparency. 
// Effects like these expect your model set to be processed/traversed in unique ways which can be awkward.   

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
	GW::INPUT::GInput input;
	GW::INPUT::GController controller;

	// Global variables
	
	UBO_DATA uboData;
	GLuint ubo = 0;
	GLuint shaderExecutable = 0;
	std::vector<Light> lights;
	GW::MATH::GVECTORF sunDirection;
	GW::MATH::GVECTORF sunColor;
	GLuint textureID = 0;
	float FOV;
	unsigned int width, height;
	float aspectRatio;

	bool isDayTime = true;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimeSwitch;
public:

	std::shared_ptr<flecs::world> ecs;
	std::vector<flecs::entity> entVec;

	Level_Objects() : ecs(std::make_shared<flecs::world>()) {}

	GLuint GetShaderProgram() const {
		return shaderExecutable;
	}

	Level_Objects(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GOpenGLSurface _ogl) : win(_win), ogl(_ogl), ecs(std::make_shared<flecs::world>()) {
		InitializeMatricesAndLighting();
		InitializeUBO();
		CompileShaders();
		glUseProgram(shaderExecutable);
		input.Create(win);
		controller.Create();
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

	void PrintEntities(GW::SYSTEM::GLog log) {
		log.LogCategorized("MESSAGE", "Printing all entities:");

		for (const auto& entity : entVec) {
			if (entity.is_valid()) {
				std::cout << "Entity: " << entity.name() << std::endl;

				// Check and print Position component
				if (entity.has<ESG::Position>()) {
					const ESG::Position* pos = entity.get<ESG::Position>();
					std::cout << "Position: (" << pos->value.x << ", " << pos->value.y << ", " << pos->value.z << ")" << std::endl;
				}

				// Check and print Orientation component
				if (entity.has<ESG::Orientation>()) {
					const ESG::Orientation* orient = entity.get<ESG::Orientation>();
					// Print the matrix elements here as needed
					for (int i = 0; i < 4; ++i) {
						for (int j = 0; j < 4; ++j) {
							std::cout << std::setw(8) << std::setprecision(4) << orient->value.data[i * 4 + j] << " ";
						}
						std::cout << std::endl;
					}
				}
			}
			else {
				std::cout << "Invalid entity" << std::endl;
			}
		}

		log.LogCategorized("MESSAGE", "Finished printing all entities.");
	}

	// Global Flecs world
	
	bool LoadLevel(const char* gameLevelPath, const char* h2bFolderPath, GW::SYSTEM::GLog log) {
		log.LogCategorized("EVENT", "LOADING GAME LEVEL [OBJECT ORIENTED]");
		log.LogCategorized("MESSAGE", "Begin Reading Game Level Text File.");
		flecs::entity playerEntity;
		Model playerModel;
		UnloadLevel(); // clear previous level data if there is any
		GW::SYSTEM::GFile file;
		file.Create();
		if (-file.OpenTextRead(gameLevelPath)) {
			log.LogCategorized("ERROR", (std::string("Game level not found: ") + gameLevelPath).c_str());
			return false;
		}
		char linebuffer[1024];
			int nameIndex = 0;
		while (+file.ReadLine(linebuffer, 1024, '\n')) {
				 nameIndex += 1;
			if (linebuffer[0] == '\0')
				break;
			if (std::strcmp(linebuffer, "MESH") == 0) {
				Model newModel;
				file.ReadLine(linebuffer, 1024, '\n');
				log.LogCategorized("INFO", (std::string("Model Detected: ") + linebuffer).c_str());
				newModel.SetName(linebuffer);
				std::string modelFile = linebuffer;
				modelFile = modelFile.substr(0, modelFile.find_last_of("\n"));
				std::string anvil = "anvil_low";
				//if (modelFile != anvil) { modelFile += nameIndex; }
				char* modelName = modelFile.data();
				


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
				log.LogCategorized("INFO", loc.c_str());

				auto entity = ecs->entity()
					.set_name(modelName)
					.set<ESG::Position>({ transform.row4 })
					.set<ESG::Orientation>({ transform });

				std::cout << "Entity: " << entity.name() << std::endl;
				entVec.push_back(entity);

				log.LogCategorized("MESSAGE", "Begin Importing .H2B File Data.");
				
				modelFile = modelFile.substr(0, modelFile.find_first_of("."));

				modelFile += ".h2b";

				modelFile = std::string(h2bFolderPath) + "/" + modelFile;
				newModel.SetWorldMatrix(transform);
				if (newModel.LoadModelDataFromDisk(modelFile.c_str())) {
					file.ReadLine(linebuffer, 1024, '\n');
					if (std::strcmp(linebuffer, "TEXTURE") == 0) {
						file.ReadLine(linebuffer, 1024, '\n');
						std::string textureFile = linebuffer;
						if (newModel.LoadTextureFromFile(textureFile.c_str())) {
							allObjectsInLevel.push_back(std::move(newModel));
							log.LogCategorized("INFO", (std::string("Texture Loaded: ") + textureFile).c_str());
						}
						else {
							log.LogCategorized("ERROR", (std::string("Texture Not Found: ") + textureFile).c_str());
						}
					}
					else {
						log.LogCategorized("ERROR", "Texture information missing in game level file.");
					}
					log.LogCategorized("INFO", (std::string("H2B Imported: ") + modelFile).c_str());
				}
				else {
					log.LogCategorized("ERROR", (std::string("H2B Not Found: ") + modelFile).c_str());
					log.LogCategorized("WARNING", "Loading will continue but model(s) are missing.");
				}
				log.LogCategorized("MESSAGE", "Importing of .H2B File Data Complete.");
			}
			else if (std::strcmp(linebuffer, "LIGHT") == 0) {
				file.ReadLine(linebuffer, 1024, '\n'); // Read the type
				std::string lightType = linebuffer;

				if (lightType == "SUN") {
					Light sunLight;
					sunLight.type = "SUN";
					file.ReadLine(linebuffer, 1024, '\n'); // Read the color
					std::sscanf(linebuffer, "Color: %f %f %f", &sunLight.color.x, &sunLight.color.y, &sunLight.color.z);
					sunLight.color.w = 1.0f;

					file.ReadLine(linebuffer, 1024, '\n'); // Read the direction
					std::sscanf(linebuffer, "Direction: %f %f %f", &sunLight.transform.row3.x, &sunLight.transform.row3.y, &sunLight.transform.row3.z);

					file.ReadLine(linebuffer, 1024, '\n'); // Read the energy
					std::sscanf(linebuffer, "Energy: %f", &sunLight.energy);

					sunDirection = sunLight.transform.row3;
					sunColor = sunLight.color;
					uboData.sunColor = sunColor;
					uboData.sunDirection = sunDirection;

					lights.push_back(sunLight);
					log.LogCategorized("INFO", "Sun data parsed successfully.");
				}
				else if (lightType == "POINT") {
					Light pointLight;
					pointLight.type = "POINT";
					file.ReadLine(linebuffer, 1024, '\n'); // Read the color
					std::sscanf(linebuffer, "Color: %f %f %f", &pointLight.color.x, &pointLight.color.y, &pointLight.color.z);
					pointLight.color.w = 1.0f;

					file.ReadLine(linebuffer, 1024, '\n'); // Read the position
					std::sscanf(linebuffer, "Position: %f %f %f", &pointLight.transform.row4.x, &pointLight.transform.row4.y, &pointLight.transform.row4.z);

					file.ReadLine(linebuffer, 1024, '\n'); // Read the energy
					std::sscanf(linebuffer, "Energy: %f", &pointLight.energy);

					lights.push_back(pointLight);
					log.LogCategorized("INFO", "Point light data parsed successfully.");
				}
			}
		}
		log.LogCategorized("MESSAGE", "Game Level File Reading Complete.");
		log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU [OBJECT ORIENTED]");
		PrintEntities(log);
		return true;
	}
	// Upload the CPU level to GPU
	void UploadLevelToGPU(/*pass handle to API device if needed*/) {
		// iterate over each model and tell it to draw itself
		for (auto& e : allObjectsInLevel) {
			e.UploadModelData2GPU(/*forward handle to API device if needed*/);
		}
	}

	void EnableWireframeMode() {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	void DisableWireframeMode() {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void UpdateECS(float deltaTime) {
		// Progress the ECS world
		ecs->progress(deltaTime);
	}

	void Level_Objects::RenderLevel(std::vector<Model>& updatedModels) {

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
			// Assuming we have a light source at index 0 that is the sun
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

		for (auto& model : updatedModels) {
			model.DrawModel(shaderExecutable, uboData, ubo, mapCenter);
		}
	}


	void UpdateAndRender(float deltaTime) {
		ecs->progress(deltaTime);

		// Temporary list to store updated models
		std::vector<Model> updatedModels;

		// Update each model based on entity data
		for (auto& model : allObjectsInLevel) {
			bool modelUpdated = false;
			// Find the corresponding entity
			for (const auto& entity : entVec) {
				if (entity.is_valid() && std::string(entity.name()) == model.GetName()) {
					std::cout << "Entity found: " << entity.name() << " for Model: " << model.GetName() << std::endl;

					// Get the latest position and orientation from the entity
					const ESG::Position* pos = entity.get<ESG::Position>();
					const ESG::Orientation* orient = entity.get<ESG::Orientation>();

					// Update model's world matrix
					if (pos && orient) {
						GW::MATH::GMATRIXF worldMatrix = orient->value;
						worldMatrix.row4 = pos->value; // Set position in the world matrix
						model.SetWorldMatrix(worldMatrix);

						for (int i = 0; i < 4; ++i) {
							for (int j = 0; j < 4; ++j) {
								std::cout << std::setw(8) << std::setprecision(4) << orient->value.data[i * 4 + j] << " ";
							}
							std::cout << std::endl;
						}
						modelUpdated = true;
					}

					break;
				}
			}
			if (modelUpdated) {
				updatedModels.push_back(model);
			}
			else {
				std::cerr << "No matching entity found for Model: " << model.GetName() << std::endl;
			}
		}

		// Render all updated models
		RenderLevel(updatedModels);
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
		fprintf(stderr, "Failed here lmao\n");
		glGenBuffers(1, &ubo);
		if (ubo == 0) {
			fprintf(stderr, "Failed to generate buffer.\n");
			return;
		}
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		if (glGetError() != GL_NO_ERROR) {
			fprintf(stderr, "Error binding buffer.\n");
			return;
		}
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO_DATA), nullptr, GL_STATIC_DRAW);
		if (glGetError() != GL_NO_ERROR) {
			fprintf(stderr, "Error allocating buffer data.\n");
			return;
		}
		glUseProgram(shaderExecutable);
		if (glGetError() != GL_NO_ERROR) {
			fprintf(stderr, "Error using shader program.\n");
			return;
		}
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
		GW::MATH::GVECTORF cameraPosition = { -1.0f, 2.0f, 2.5f };
		GW::MATH::GVECTORF targetPosition = { -1.0f, 2.0f, 0.0f };
		GW::MATH::GVECTORF upDirection = { 0.0f, 1.0f, 0.0f };
		 
		matrixProxy.LookAtRHF(cameraPosition, targetPosition, upDirection, view);

		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		float fov = 90.0f * (G_PI_F / 180.0f);
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

};

