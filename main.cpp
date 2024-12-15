// Nagłówki
//#include "stdafx.h"
#include <GL/glew.h>
#include <SFML/Window.hpp>

#include <SFML/System/Time.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec2 aTexCoord;
in vec3 aNormal;
out vec3 Color;out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
void main(){
	Color = color;
	TexCoord = aTexCoord;
	gl_Position = proj * view * model * vec4(position, 1.0);
	Normal = mat3(transpose(inverse(model))) * aNormal;
	FragPos = vec3(model * vec4(position, 1.0));
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
out vec4 outColor;
uniform sampler2D texture1;

uniform bool uLightingEnabled;  // Zmienna sterująca oświetleniem
uniform bool useTexture; // Zmienna sterująca teksturą

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 ambientLightColor;
uniform float ambientStrength;
uniform float lightIntensity;
uniform vec3 difflightColor;

uniform vec3 objectColor;

void main()
{
	
	// Oświetlenie otoczenia
    vec3 ambientLightColor = vec3(1.0, 1.0, 1.0);
    vec4 ambient = ambientStrength * vec4(ambientLightColor, 1.0);

	// Oświetlenie rozproszone
    vec3 difflightColor = vec3(1.0, 1.0, 1.0);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * difflightColor * lightIntensity;

    if (uLightingEnabled)
    {
		if (useTexture)
			outColor = (ambient + vec4(diffuse, 1.0)) * texture(texture1, TexCoord);	
		else	
			outColor = (ambient + vec4(diffuse, 1.0)) * vec4(objectColor, 1.0);	
    }
    else
    {
		if (useTexture)
			outColor = texture(texture1, TexCoord);
		else
			outColor = vec4(objectColor, 1.0);
    }

}
)glsl";

// Funkcja do sprawdzania błędów kompilacji shaderów
void checkShaderCompilation(GLuint shader, const char* shaderName) {
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "Compilation " << shaderName << " ERROR\n" << infoLog << std::endl;
	}
	else {
		std::cout << "Compilation " << shaderName << " OK" << std::endl;
	}
}

// Funkcja do ładowania tekstury
unsigned int loadTexture(const std::string& texturePath) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Ustawienie parametrów tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Wczytanie tekstury za pomocą stb_image
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
	
	if (data) {
		// Elastyczne podejście zakłada wariant dla nrChannels != 3
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}	else
	{
		std::cout << "Failed to load texture" << std::endl;

		stbi_image_free(data);

		return 0;
	}
	stbi_image_free(data);

	return textureID;
}

// Funkcja do nakładania koloru
void setModelColor(GLuint shaderProgram, float r, float g, float b) {
	// Znajdź lokalizację uniformu w shaderze
	GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");
	if (colorLocation == -1) {
		std::cerr << "Failed to find uniform 'objectColor' in shader program." << std::endl;
		return;
	}
	// Ustaw kolor (RGB)
	glUniform3f(colorLocation, r, g, b);
}


// Struktura wierzchołków figury
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

// Funkcja do ładowania modelu z pliku obj
bool loadOBJ(const std::string& path, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open OBJ file: " << path << std::endl;
		return false;
	}

	std::vector<glm::vec3> temp_positions;
	std::vector<glm::vec3> temp_normals;
	std::vector<glm::vec2> temp_texCoords;

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "v") { // Wierzchołek
			glm::vec3 pos;
			ss >> pos.x >> pos.y >> pos.z;
			temp_positions.push_back(pos);
		}
		else if (prefix == "vn") { // Normalne
			glm::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (prefix == "vt") { // Tekstura
			glm::vec2 tex;
			ss >> tex.x >> tex.y;
			temp_texCoords.push_back(tex);
		}
		else if (prefix == "f") { // Ściany
			unsigned int posIdx[3], texIdx[3], normIdx[3];
			char slash; // do obsługi np. 1/2/3
			for (int i = 0; i < 3; i++) {
				ss >> posIdx[i] >> slash >> texIdx[i] >> slash >> normIdx[i];
				Vertex vertex;
				vertex.position = temp_positions[posIdx[i] - 1];
				vertex.texCoord = temp_texCoords[texIdx[i] - 1];
				vertex.normal = temp_normals[normIdx[i] - 1];
				vertices.push_back(vertex);
				indices.push_back(vertices.size() - 1);
			}
		}
	}
	file.close();
	return true;
}

// Funkcja do podziału modelu na podstawie współrzędnej y
void splitModelByY(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
	float ySplit, std::vector<Vertex>& upperVertices, std::vector<unsigned int>& upperIndices,
	std::vector<Vertex>& lowerVertices, std::vector<unsigned int>& lowerIndices) {
	auto interpolateVertex = [](const Vertex& v1, const Vertex& v2, float t) {
		Vertex v;
		v.position = v1.position + t * (v2.position - v1.position);
		v.normal = glm::normalize(v1.normal + t * (v2.normal - v1.normal));
		v.texCoord = v1.texCoord + t * (v2.texCoord - v1.texCoord);
		return v;
		};

	for (size_t i = 0; i < indices.size(); i += 3) {
		unsigned int idx0 = indices[i];
		unsigned int idx1 = indices[i + 1];
		unsigned int idx2 = indices[i + 2];

		const Vertex& v0 = vertices[idx0];
		const Vertex& v1 = vertices[idx1];
		const Vertex& v2 = vertices[idx2];

		bool above0 = v0.position.y >= ySplit;
		bool above1 = v1.position.y >= ySplit;
		bool above2 = v2.position.y >= ySplit;

		std::vector<Vertex> upper, lower;

		if (above0) upper.push_back(v0); else lower.push_back(v0);
		if (above1) upper.push_back(v1); else lower.push_back(v1);
		if (above2) upper.push_back(v2); else lower.push_back(v2);

		if (upper.size() == 3) {
			// Wszystkie wierzchołki powyżej
			for (const Vertex& v : upper) {
				upperVertices.push_back(v);
				upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));
			}
		}
		else if (lower.size() == 3) {
			// Wszystkie wierzchołki poniżej
			for (const Vertex& v : lower) {
				lowerVertices.push_back(v);
				lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));
			}
		}
		else if (upper.size() == 2 && lower.size() == 1) {
			// Trójkąt przecinający płaszczyznę (2 wierzchołki powyżej)
			const Vertex* high1 = &upper[0];
			const Vertex* high2 = &upper[1];
			const Vertex* low = &lower[0];

			float t1 = (ySplit - low->position.y) / (high1->position.y - low->position.y);
			float t2 = (ySplit - low->position.y) / (high2->position.y - low->position.y);

			Vertex vSplit1 = interpolateVertex(*low, *high1, t1);
			Vertex vSplit2 = interpolateVertex(*low, *high2, t2);

			// Dodaj do górnej części
			upperVertices.push_back(*high1);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(*high2);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(vSplit1);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(vSplit1);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(vSplit2);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(*high2);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			// Dodaj do dolnej części
			lowerVertices.push_back(*low);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(vSplit1);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(vSplit2);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));
		}
		else if (upper.size() == 1 && lower.size() == 2) {
			// Trójkąt przecinający płaszczyznę (1 wierzchołek powyżej)
			const Vertex* high = &upper[0];
			const Vertex* low1 = &lower[0];
			const Vertex* low2 = &lower[1];

			float t1 = (ySplit - high->position.y) / (low1->position.y - high->position.y);
			float t2 = (ySplit - high->position.y) / (low2->position.y - high->position.y);

			Vertex vSplit1 = interpolateVertex(*high, *low1, t1);
			Vertex vSplit2 = interpolateVertex(*high, *low2, t2);

			// Dodaj do górnej części
			upperVertices.push_back(*high);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(vSplit1);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			upperVertices.push_back(vSplit2);
			upperIndices.push_back(static_cast<unsigned int>(upperVertices.size() - 1));

			// Dodaj do dolnej części
			lowerVertices.push_back(vSplit1);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(*low1);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(*low2);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(vSplit1);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(*low2);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));

			lowerVertices.push_back(vSplit2);
			lowerIndices.push_back(static_cast<unsigned int>(lowerVertices.size() - 1));
		}
	}
}

// Funkcja do tworzenia i konfigurowania VAO, VBO i EBO
void setupBuffers(GLuint& vao, GLuint& vbo, GLuint& ebo, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLuint shaderProgram) {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	GLint norAttrib = glGetAttribLocation(shaderProgram, "aNormal");
	glEnableVertexAttribArray(norAttrib);
	glVertexAttribPointer(norAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	GLint texAttrib = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

	glBindVertexArray(0);
}

int main()
{
	srand(time(NULL));

	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	
	// Okno renderingu
	sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL | FPS: NULL", sf::Style::Titlebar | sf::Style::Close, settings);

	// Przechwycenie kursora myszy i ukrycie go
	window.setMouseCursorGrabbed(true); //przechwycenie kursora myszy w oknie 
	window.setMouseCursorVisible(false); //ukrycie kursora myszy
	
	// Włączenie z-bufora
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Inicjalizacja GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Generowanie figur
	std::vector<Vertex> chairVertices;
	std::vector<unsigned int> chairIndices;
	if (!loadOBJ("models/simple_chair2.obj", chairVertices, chairIndices)) {
		std::cerr << "Failed to load chair model" << std::endl;
		return -1;
	}

	std::vector<Vertex> tableVertices;
	std::vector<unsigned int> tableIndices;
	if (!loadOBJ("models/simple_table.obj", tableVertices, tableIndices)) {
		std::cerr << "Failed to load table model" << std::endl;
		return -1;
	}

	// Dwie nowe figury na podstawie krzesła
	std::vector<Vertex> upperVertices, lowerVertices;
	std::vector<unsigned int> upperIndices, lowerIndices;

	splitModelByY(chairVertices, chairIndices, 3.8f, upperVertices, upperIndices, lowerVertices, lowerIndices);

	// Utworzenie i skompilowanie shadera wierzchołków
	GLuint vertexShader =
		glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// Utworzenie i skompilowanie shadera fragmentów
	GLuint fragmentShader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	// Sprawdzenie poprawności skompilowania shaderów
	checkShaderCompilation(vertexShader, "vertexShader");
	checkShaderCompilation(fragmentShader, "fragmentShader");

	// Zlinkowanie obu shaderów w jeden wspólny program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Stworzenie macierzy modelu
	glm::mat4 chairModel = glm::mat4(1.0f);
	chairModel = glm::translate(chairModel, glm::vec3(-1.5f, 0.0f, 3.0f));
	chairModel = glm::rotate(chairModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	chairModel = glm::scale(chairModel, glm::vec3(0.5f, 0.5f, 0.5f));

	glm::mat4 tableModel = glm::mat4(1.0f);
	tableModel = glm::translate(tableModel, glm::vec3(-1.5f, 0.0f, 0.0f));
	tableModel = glm::scale(tableModel, glm::vec3(0.5f, 0.5f, 0.5f));

	glm::mat4 upperChairModel = glm::mat4(1.0f);
	upperChairModel = glm::translate(upperChairModel, glm::vec3(0.5f, 0.0f, 0.0f));
	upperChairModel = glm::rotate(upperChairModel, glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	upperChairModel = glm::scale(upperChairModel, glm::vec3(0.5f, 0.5f, 0.5f));

	glm::mat4 lowerChairModel = glm::mat4(1.0f);
	lowerChairModel = glm::translate(lowerChairModel, glm::vec3(0.5f, 0.0f, 0.0f));
	lowerChairModel = glm::rotate(lowerChairModel, glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lowerChairModel = glm::scale(lowerChairModel, glm::vec3(0.5f, 0.5f, 0.5f));

	// Stworzenie macierzy widoku
	glm::mat4 view;
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	// Stworzenie macierzy projekcji
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.06f, 100.0f);

	// Wysłanie do shadera macierzy modelu
	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
	//glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(chairModel));
	//glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(tableModel));

	// Wysłanie do shadera macierzy widoku
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	// Wysłanie do shadera macierzy projekcji
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
	
	// Utworzenie VAO, VBO i EBO dla krzesła 
	GLuint vaoChair, vboChair, eboChair;
	setupBuffers(vaoChair, vboChair, eboChair, chairVertices, chairIndices, shaderProgram);
	
	
	// Utworzenie VAO, VBO i EBO dla stołu
	GLuint vaoTable, vboTable, eboTable;
	setupBuffers(vaoTable, vboTable, eboTable, tableVertices, tableIndices, shaderProgram);
	

	// Utworzenie VAO, VBO i EBO dla górnej części krzesła
	GLuint vaoUpperChair, vboUpperChair, eboUpperChair;
	setupBuffers(vaoUpperChair, vboUpperChair, eboUpperChair, upperVertices, upperIndices, shaderProgram);

	// Utworzenie VAO, VBO i EBO dla dolnej części krzesła
	GLuint vaoLowerChair, vboLowerChair, eboLowerChair;
	setupBuffers(vaoLowerChair, vboLowerChair, eboLowerChair, lowerVertices, lowerIndices, shaderProgram);

	unsigned int chairTexture = loadTexture("textures/wood1.jpg");
	unsigned int tableTexture = loadTexture("textures/star_wars.jpg");
	unsigned int upperChairTexture = loadTexture("textures/wood3.jpg");

	// Wysłanie do shadera pozycji źródła światła
	glm::vec3 lightPos(-1.6f, 10.5f, -0.5f);
	GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
	glUniform3fv(uniLightPos, 1, &lightPos[0]);
	

	bool lightingEnabled = true;  // Stan włączenia oświetlenia
	float ambientStrength = 0.1f;
	float lightIntensity = 1.0f;
	bool changingAmbient = false;


	bool useTexture = false;

	// Zmienne kontrolujące rysowanie pociętego krzesła
	bool drawLowerChair = true;
	bool drawUpperChair = true;

	// Wysłanie do shadera zmiennych odpowiedzialnych za oświetlenie
	GLint lightingEnabledLocation = glGetUniformLocation(shaderProgram, "uLightingEnabled");
	GLint ambientStrengthLocation = glGetUniformLocation(shaderProgram, "ambientStrength");
	GLint lightIntensityLocation = glGetUniformLocation(shaderProgram, "lightIntensity");
	GLint useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

	glUniform1i(lightingEnabledLocation, lightingEnabled);
	glUniform1f(ambientStrengthLocation, ambientStrength);
	glUniform1f(lightIntensityLocation, lightIntensity);
	glUniform1f(useTextureLocation, useTexture);
	

	// Rozpoczęcie pętli zdarzeń
	bool running = true;

	glm::vec3 cameraPos = glm::vec3(4.0f, 5.0f, 8.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Kąty podrzebne do kontroli nachylenia
	double yaw = -110.0, pitch = -30.0;

	// Czułość myszy
	float sensitivity = 0.05f;

	// Utworzenie obiektów Clock i Time
	sf::Clock clock, clock2;
	sf::Time time;

	// Ustalenie stałej liczby klatek
	window.setFramerateLimit(120);

	// Zmienna do zliczania klatek
	int frameCount = 0;

	while (running) {

		// Czas wykonania pętli
		time = clock.getElapsedTime();
		clock.restart();

		// Wyznaczenie szybkości poruszania kamery
		float cameraSpeed = 0.000002f * time.asMicroseconds();

		// Licznik aktualnej liczby FPS
		frameCount++;

		// Aktualizacja wyświetlenia liczby FPS co sekundę
		if (clock2.getElapsedTime().asSeconds() >= 1.0f)
		{
			window.setTitle("OpenGL | FPS: " + std::to_string(frameCount));

			frameCount = 0;
			clock2.restart();
		}
		
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			case sf::Event::KeyPressed:
				if (windowEvent.key.code == sf::Keyboard::Escape) {
					running = false;
				}
				/*
				if (windowEvent.key.code >= sf::Keyboard::Num0 && windowEvent.key.code <= sf::Keyboard::Num9)
					primitiveType = setPrimitive(windowEvent);
				break;
				*/
				// Obsługa klawiszy 1-6 do zmiany oświetlenia
				if (windowEvent.key.code == sf::Keyboard::Num1) {
					std::cout << "Oświetlenie: brak\n";
					lightingEnabled = false;  // Wyłączamy pełne oświetlenie, włączamy tylko ambient
				}
				if (windowEvent.key.code == sf::Keyboard::Num2) {
					std::cout << "Oświetlenie: punktowe\n";
					lightingEnabled = true;   // Włączamy pełne oświetlenie
				}
				if (windowEvent.key.code == sf::Keyboard::Num3) {
					if (changingAmbient) {
						ambientStrength += 0.1f;
						std::cout << "Zwiekszenie swiatla otoczenia: " << ambientStrength << "\n";
						glUniform1f(ambientStrengthLocation, ambientStrength);
					}
					else {
						lightIntensity += 0.1f;
						std::cout << "Zwiekszenie swiatla punktowego: " << lightIntensity << std::endl;
						glUniform1f(lightIntensityLocation, lightIntensity);
					}

				}
				if (windowEvent.key.code == sf::Keyboard::Num4) {
					if (changingAmbient) {
						ambientStrength -= 0.1f;
						if (ambientStrength < 0.0f) ambientStrength = 0.0f;  // Minimalna wartość
						std::cout << "Zmniejszenie swiatla otoczenia " << ambientStrength << "\n";
						glUniform1f(ambientStrengthLocation, ambientStrength);
					}
					else {
						lightIntensity -= 0.1f;
						if (lightIntensity < 0.0f) lightIntensity = 0.0f;  // Minimalna wartość
						std::cout << "Zmniejszenie swiatla punktowego: " << lightIntensity << std::endl;
						glUniform1f(lightIntensityLocation, lightIntensity);
					}
				}
				if (windowEvent.key.code == sf::Keyboard::Num5) {
					lightPos = cameraPos;  // Ustawienie źródła światła na pozycję kamery
					uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
					glUniform3fv(uniLightPos, 1, &lightPos[0]);  // Wyślij do shaderów
					std::cout << "Ustawiono nowa pozycje zrodla swiatla na: " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << "\n";
				}
				if (windowEvent.key.code == sf::Keyboard::Num6) {
					changingAmbient = !changingAmbient;
					if (changingAmbient)
						std::cout << "Ustawiono tryb: zmiana swiatla otoczenia\n";
					else
						std::cout << "Ustawiono tryb: zmiana swiatla punktowego\n";
				}
				if (windowEvent.key.code == sf::Keyboard::Num7) {
					drawLowerChair = !drawLowerChair;
				}
				if (windowEvent.key.code == sf::Keyboard::Num8) {
					drawUpperChair = !drawUpperChair;
				}
				break;
			
			case sf::Event::MouseMoved:
				sf::Vector2i centerPosition(window.getSize().x / 2, window.getSize().y / 2);
				sf::Vector2i localPosition = sf::Mouse::getPosition(window);
				double xoffset = localPosition.x - centerPosition.x;
				double yoffset = localPosition.y - centerPosition.y;

				xoffset *= sensitivity;
				yoffset *= sensitivity;

				yaw += xoffset;
				pitch -= yoffset;

				if (pitch > 89.0f)
					pitch = 89.0f;
				if (pitch < -89.0f)
					pitch = -89.0f;
				sf::Mouse::setPosition(centerPosition, window);

				break;
			}			
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			cameraPos += cameraSpeed * cameraFront;
			//std::cout << "Ruch kamery: PRZOD\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
			cameraPos -= cameraSpeed * cameraFront;
			//std::cout << "Ruch kamery: TYL\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
			//std::cout << "Ruch kamery: LEWO\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
			//std::cout << "Ruch kamery: PRAWO\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
			cameraPos.y += cameraSpeed;
			//std::cout << "Ruch kamery: GORA\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
			cameraPos.y -= cameraSpeed;
			//std::cout << "Ruch kamery: DOL\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			yaw -= cameraSpeed * 30;
			//std::cout << "Obrot kamery: LEWO\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			yaw += cameraSpeed * 30;
			//std::cout << "Obrot kamery: PRAWO\n";
		}
		
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);
		
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
			
		GLint uniView = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		// Rysowanie krzesła	
		setModelColor(shaderProgram, 1.0f, 1.0f, 0.0f); // Żółty kolor
		
		// Ładowanie tekstury
		if (chairTexture != 0) {
			useTexture = true;
			glActiveTexture(chairTexture);
			glBindTexture(GL_TEXTURE_2D, chairTexture);
		}

		glUniform1f(useTextureLocation, useTexture);
		glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(chairModel));
		glBindVertexArray(vaoChair);
		glDrawElements(GL_TRIANGLES, chairIndices.size(), GL_UNSIGNED_INT, 0);
		useTexture = false;

		// Rysowanie stołu
		setModelColor(shaderProgram, 0.0f, 1.0f, 0.0f); // Zielony kolor
		
		// Ładowanie tekstury
		if (tableTexture != 0) {
			useTexture = true;
			glActiveTexture(tableTexture);
			glBindTexture(GL_TEXTURE_2D, tableTexture);
		}

		glUniform1f(useTextureLocation, useTexture);
		glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(tableModel));
		glBindVertexArray(vaoTable);
		glDrawElements(GL_TRIANGLES, tableIndices.size(), GL_UNSIGNED_INT, 0);
		useTexture = false;
		
		// Rysowanie dolnej części krzesła
		if (drawLowerChair) {
			setModelColor(shaderProgram, 0.0f, 0.0f, 1.0f); // Niebieski kolor

			// Ładowanie tekstury
			if (upperChairTexture != 0) {
				useTexture = true;
				glActiveTexture(upperChairTexture);
				glBindTexture(GL_TEXTURE_2D, upperChairTexture);
			}

			glUniform1f(useTextureLocation, useTexture);
			glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(lowerChairModel));
			glBindVertexArray(vaoLowerChair);
			glDrawElements(GL_TRIANGLES, lowerIndices.size(), GL_UNSIGNED_INT, 0);
			useTexture = false;
		}

		// Rysowanie górnej części krzesła
		if (drawUpperChair) {
			setModelColor(shaderProgram, 1.0f, 0.0f, 0.0f); // Czerwony kolor

			// Ładowanie tekstury
			if (upperChairTexture != 0) {
				useTexture = true;
				glActiveTexture(upperChairTexture);
				glBindTexture(GL_TEXTURE_2D, upperChairTexture);
			}

			glUniform1f(useTextureLocation, useTexture);
			glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(upperChairModel));
			glBindVertexArray(vaoUpperChair);
			glDrawElements(GL_TRIANGLES, upperIndices.size(), GL_UNSIGNED_INT, 0);
			useTexture = false;
		}


		glUniform1i(lightingEnabledLocation, lightingEnabled);

		// Wymiana buforów tylni/przedni
		window.display();
	}
	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(1, &vboChair);
	glDeleteVertexArrays(1, &vaoChair);

	glDeleteBuffers(1, &vboUpperChair);
	glDeleteVertexArrays(1, &vaoUpperChair);

	glDeleteBuffers(1, &vboLowerChair);
	glDeleteVertexArrays(1, &vaoLowerChair);

	glDeleteBuffers(1, &vboTable);
	glDeleteVertexArrays(1, &vaoTable);

	// Usunięcie dynamicznej tablicy
	//delete[] vertices;

	// Zamknięcie okna renderingu
	window.close();
	return 0;
}