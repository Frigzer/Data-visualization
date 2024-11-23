﻿// Nagłówki
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

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec2 aTexCoord;
out vec3 Color;out vec2 TexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
void main(){
	Color = color;
	TexCoord = aTexCoord;
	gl_Position = proj * view * model * vec4(position, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
out vec4 outColor;
uniform sampler2D texture1;
void main()
{
	outColor=texture(texture1, TexCoord);
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

		vertices[i * 6 + 3] = (float)rand() / RAND_MAX;  // R
		vertices[i * 6 + 4] = (float)rand() / RAND_MAX;  // G
		vertices[i * 6 + 5] = (float)rand() / RAND_MAX;  // B
	}
}

// Funkcja do aktualizcji punktów
GLfloat* update(GLfloat* vertices, int numVertices, GLuint vbo) {
	delete[] vertices;
	vertices = new GLfloat[numVertices * 6];

	generatePolygonVerticles(vertices, numVertices, 0.5f);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 8, vertices, GL_DYNAMIC_DRAW);

	return vertices;
}

// Funkcja do ustawienia prymitywu
GLenum setPrimitive(sf::Event mouseEvent) {
	switch (mouseEvent.key.code)
	{
	case sf::Keyboard::Num1:
		std::cout << "Zmieniono tryb na: " << "GL_POINTS" << std::endl;
		return GL_POINTS;
	case sf::Keyboard::Num2:
		std::cout << "Zmieniono tryb na: " << "GL_LINES" << std::endl;
		return GL_LINES;
	case sf::Keyboard::Num3:
		std::cout << "Zmieniono tryb na: " << "GL_LINE_LOOP" << std::endl;
		return GL_LINE_LOOP;
	case sf::Keyboard::Num4:
		std::cout << "Zmieniono tryb na: " << "GL_LINE_STRIP" << std::endl;
		return GL_LINE_STRIP;
	case sf::Keyboard::Num5:
		std::cout << "Zmieniono tryb na: " << "GL_TRIANGLES" << std::endl;
		return GL_TRIANGLES;
	case sf::Keyboard::Num6:
		std::cout << "Zmieniono tryb na: " << "GL_TRIANGLE_STRIP" << std::endl;
		return GL_TRIANGLE_STRIP;
	case sf::Keyboard::Num7:
		std::cout << "Zmieniono tryb na: " << "GL_TRIANGLE_FAN" << std::endl;
		return GL_TRIANGLE_FAN;
	case sf::Keyboard::Num8:
		std::cout << "Zmieniono tryb na: " << "GL_QUADS" << std::endl;
		return GL_QUADS;
	case sf::Keyboard::Num9:
		std::cout << "Zmieniono tryb na: " << "GL_QUAD_STRIP" << std::endl;
		return GL_QUAD_STRIP;
	case sf::Keyboard::Num0:
		std::cout << "Zmieniono tryb na: " << "GL_POLYGON" << std::endl;
		return GL_POLYGON;
	default:
		break;
	}
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

	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzchołkowych
	GLuint vbo;
	glGenBuffers(1, &vbo);

	// Parametry wielokąta
	int punkty_ = 4;
	// GLfloat* vertices = new GLfloat[punkty_ * 6];

	// Generowanie współrzędnych cylindrycznych
	// generatePolygonVerticles(vertices, punkty_, 0.5f);

	// Generowanie sześcianu
	punkty_ = 36;
	GLfloat vertices[] = {
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};

	// Załadowanie tekstury
	unsigned int texture1;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("textures//haisenberg.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	// Domyślny typ prymitywu
	GLenum primitiveType = GL_TRIANGLES;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * punkty_ * 8, vertices, GL_STATIC_DRAW);

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
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// Stworzenie macierzy widoku
	glm::mat4 view;
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	// Stworzenie macierzy projekcji
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.06f, 100.0f);

	// Wysłanie do shadera macierzy modelu
	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

	// Wysłanie do shadera macierzy widoku
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	// Wysłanie do shadera macierzy projekcji
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	// Specifikacja formatu danych wierzchołkowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	// Wysłanie do shadera tekstury
	GLint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(TexCoord);	glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 *sizeof(GLfloat)));
	// Rozpoczęcie pętli zdarzeń
	bool running = true;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Kąty podrzebne do kontroli nachylenia
	double yaw = -90.0, pitch = 0.0;

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

		// Narysowanie wybranego prymitywu na podstawie wybranej liczby wierzchołków
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

	// Usunięcie dynamicznej tablicy
	//delete[] vertices;

	// Zamknięcie okna renderingu
	window.close();
	return 0;
}