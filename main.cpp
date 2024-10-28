// Nagłówki
//#include "stdafx.h"
#include <GL/glew.h>
#include <SFML/Window.hpp>

#include <iostream>
#include <cmath>

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
out vec3 Color;
void main(){
	Color = color;
	gl_Position = vec4(position, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
out vec4 outColor;
void main()
{
	outColor = vec4(Color, 1.0);
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

// Funkcja do generowania wierzchołków wielokąta
void generatePolygonVerticles(GLfloat* vertices, int numVertices, float radius) {
	for (int i = 0; i < numVertices; i++) {
		float angle = 2.0f * 3.14159265358979323846 * i / numVertices;
		vertices[i * 6] = radius * cos(angle);  // X
		vertices[i * 6 + 1] = radius * sin(angle);  // Y
		vertices[i * 6 + 2] = 0.0f;  // Z
		// Kolory RGB
		vertices[i * 6 + 3] = (float)rand() / RAND_MAX;  // R
		vertices[i * 6 + 4] = (float)rand() / RAND_MAX;  // G
		vertices[i * 6 + 5] = (float)rand() / RAND_MAX;  // B
	}
}

// Funkcja do aktualizcji punktów
GLfloat* update(GLfloat* vertices, int numVertices, GLuint vbo) {
	delete[] vertices;
	vertices = new GLfloat[numVertices * 6];

	generatePolygonVerticles(vertices, numVertices, 1.0f);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 6, vertices, GL_DYNAMIC_DRAW);

	return vertices;
}

int main()
{
	srand(time(NULL));

	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;

	// Okno renderingu
	sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	// Inicjalizacja GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzchołkowych
	GLuint vbo;
	glGenBuffers(1, &vbo);

	// Parametry wielokąta
	int punkty_ = 3;
	GLfloat* vertices = new GLfloat[punkty_ * 6];

	// Generowanie współrzędnych w cylindrycznych
	generatePolygonVerticles(vertices, punkty_, 1.0f);

	// Domyślny typ prymitywu
	GLenum primitiveType = GL_TRIANGLES
		;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * punkty_ * 6, vertices, GL_STATIC_DRAW);

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

	// Specifikacja formatu danych wierzchołkowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	// Rozpoczęcie pętli zdarzeń
	bool running = true;
	while (running) {
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			case sf::Event::KeyPressed:
				switch (windowEvent.key.code) {
				case sf::Keyboard::Num1:
					primitiveType = GL_POINTS;
					std::cout << "Zmieniono tryb na: " << "GL_POINTS" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num2:
					primitiveType = GL_LINES;
					std::cout << "Zmieniono tryb na: " << "GL_LINES" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num3:
					primitiveType = GL_LINE_LOOP;
					std::cout << "Zmieniono tryb na: " << "GL_LINE_LOOP" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num4:
					primitiveType = GL_LINE_STRIP;
					std::cout << "Zmieniono tryb na: " << "GL_LINE_STRIP" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num5:
					primitiveType = GL_TRIANGLES;
					std::cout << "Zmieniono tryb na: " << "GL_TRIANGLES" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num6:
					primitiveType = GL_TRIANGLE_STRIP;
					std::cout << "Zmieniono tryb na: " << "GL_TRIANGLE_STRIP" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num7:
					primitiveType = GL_TRIANGLE_FAN;
					std::cout << "Zmieniono tryb na: " << "GL_TRIANGLE_FAN" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num8:
					primitiveType = GL_QUADS;
					std::cout << "Zmieniono tryb na: " << "GL_QUADS" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num9:
					primitiveType = GL_QUAD_STRIP;
					std::cout << "Zmieniono tryb na: " << "GL_QUAD_STRIP" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				case sf::Keyboard::Num0:
					primitiveType = GL_POLYGON;
					std::cout << "Zmieniono tryb na: " << "GL_POLYGON" << std::endl;
					vertices = update(vertices, punkty_, vbo);
					break;
				default:
					break;
				}
			case sf::Event::MouseMoved:
				int nowe_punkty_ = std::max(3, 3 + windowEvent.mouseMove.y / 20);
				if (nowe_punkty_ == punkty_)
					break;
				punkty_ = nowe_punkty_;
				vertices = update(vertices, punkty_, vbo);
				std::cout << "Nowa liczba wierzcholkow: " << punkty_ << std::endl;
				break;
			}		
		}
		// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Narysowanie trójkąta na podstawie 3 wierzchołków
		glDrawArrays(primitiveType, 0, punkty_);
		// Wymiana buforów tylni/przedni
		window.display();
	}
	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	delete[] vertices;
	// Zamknięcie okna renderingu
	window.close();
	return 0;
}