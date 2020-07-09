//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "stb_image.h"
//#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
//#include "tiny_obj_loader.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>

static const GLsizei WIDTH = 1080, HEIGHT = 720; //размеры окна
static int filling = 0;
static bool keys[1024]; //массив состояний кнопок - нажата/не нажата
static GLfloat lastX = 400, lastY = 300; //исходное положение мыши
static bool firstMouse = true;
static bool g_captureMouse         = true;  // Мышка захвачена нашим приложением или нет?
static bool g_capturedMouseJustNow = false;
static int g_shaderProgram = 0;


GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera(float3(0.0f, 0.0f, 5.0f));

//функция для обработки нажатий на кнопки клавиатуры
void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//std::cout << key << std::endl;
	switch (key)
	{
	case GLFW_KEY_ESCAPE: //на Esc выходим из программы
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	case GLFW_KEY_SPACE: //на пробел переключение в каркасный режим и обратно
		if (action == GLFW_PRESS)
		{
			if (filling == 0)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				filling = 1;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				filling = 0;
			}
		}
		break;
  case GLFW_KEY_1:
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  case GLFW_KEY_2:
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    break;
	default:
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

//функция для обработки клавиш мыши
void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    g_captureMouse = !g_captureMouse;


  if (g_captureMouse)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    g_capturedMouseJustNow = true;
  }
  else
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

}

//функция для обработки перемещения мыши
void OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
  if (firstMouse)
  {
    lastX = float(xpos);
    lastY = float(ypos);
    firstMouse = false;
  }

  GLfloat xoffset = float(xpos) - lastX;
  GLfloat yoffset = lastY - float(ypos);  

  lastX = float(xpos);
  lastY = float(ypos);

  if (g_captureMouse)
    camera.ProcessMouseMove(xoffset, yoffset);
}


void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
  camera.ProcessMouseScroll(GLfloat(yoffset));
}

void doCameraMovement(Camera &camera, GLfloat deltaTime)
{
  if (keys[GLFW_KEY_W])
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (keys[GLFW_KEY_A])
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (keys[GLFW_KEY_S])
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (keys[GLFW_KEY_D])
    camera.ProcessKeyboard(RIGHT, deltaTime);
}

GLuint loadFile(const char* p, GLuint& vao) {
    struct normStuct {
        uint32_t index;
        float kd[3];
    };
    std::vector<float> vertices;
    std::vector<float> norm;
    std::vector<float> norms;
    std::vector<float> tex;
    std::vector<float> texs;
    std::vector<uint32_t> indicies;
    std::vector<normStuct> normIndicies;
    std::vector<uint32_t> uvIndices;

    std::string path = (std::string)p;
    path.append("obj");

    int fileType = 0; // Вручную пишем вначале файла его тип (11: поверхность состоит из 4 вершин, указаны индексы нормалей и текстурных коор)
    // 21: Поверхность состоит из 4 вершин, однако без указания текст.
    // 22: Тоже самое но из 3
    // 31: Поверхность состоит их 4 вершин, однако без указания нормалей
    // 32: Тоже самое но из 3

    FILE* file = fopen(path.c_str(), "rb");
    float kd[3] = { 1, 1, 1 };
    while (true) {
        char lineHeader[128];

        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;

        if (fileType == 0) {
            fileType = atoi(lineHeader);
            std::cout << fileType;
        }

        if (strcmp(lineHeader, "v") == 0) {
            float x, y, z;
            fscanf(file, "%f %f %f\n", &x, &y, &z);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(1.0f);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            float x, y;
            fscanf(file, "%f %f\n", &x, &y);
            tex.push_back(x);
            tex.push_back(y);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            float x, y, z;
            fscanf(file, "%f %f %f\n", &x, &y, &z);
            norm.push_back(x);
            norm.push_back(y);
            norm.push_back(z);
            norm.push_back(1.0f);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
            char c[256];
            int matches;
            switch (fileType) {
            case 11:
                matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2], &vertexIndex[3], &uvIndex[3], &normalIndex[3]);

                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                uvIndices.push_back(uvIndex[0] - 1);
                uvIndices.push_back(uvIndex[1] - 1);
                uvIndices.push_back(uvIndex[2] - 1);
                normIndicies.push_back({ normalIndex[0] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[1] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[2] - 1, {kd[0], kd[1],kd[2]} });

                indicies.push_back(vertexIndex[3] - 1);
                uvIndices.push_back(uvIndex[3] - 1);
                normIndicies.push_back({ normalIndex[3] - 1, {kd[0], kd[1],kd[2]} });
                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                uvIndices.push_back(uvIndex[0] - 1);
                uvIndices.push_back(uvIndex[2] - 1);
                normIndicies.push_back({ normalIndex[0] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[2] - 1, {kd[0], kd[1],kd[2]} });
            case 12: {
                matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                uvIndices.push_back(uvIndex[0] - 1);
                uvIndices.push_back(uvIndex[1] - 1);
                normIndicies.push_back({ normalIndex[0] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[1] - 1, {kd[0], kd[1],kd[2]} });
                uvIndices.push_back(uvIndex[2] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                normIndicies.push_back({ normalIndex[2] - 1, {kd[0], kd[1],kd[2]} });

                break;
            }
            case 21:
                matches = fscanf(file, "%d//%d %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2], &vertexIndex[3], &normalIndex[3]);
                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                indicies.push_back(vertexIndex[3] - 1);
                normIndicies.push_back({ normalIndex[0] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[1] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[2] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[3] - 1, {kd[0], kd[1],kd[2]} });
                break;
            case 22:
                matches = fscanf(file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                normIndicies.push_back({ normalIndex[0] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[1] - 1, {kd[0], kd[1],kd[2]} });
                normIndicies.push_back({ normalIndex[2] - 1, {kd[0], kd[1],kd[2]} });
                break;
            case 31:
                matches = fscanf(file, "%d/%d %d/%d %d/%d %d/%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2], &vertexIndex[3], &uvIndex[3]);
                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                indicies.push_back(vertexIndex[3] - 1);
                uvIndices.push_back(uvIndex[0] - 1);
                uvIndices.push_back(uvIndex[1] - 1);
                uvIndices.push_back(uvIndex[2] - 1);
                uvIndices.push_back(uvIndex[3] - 1);
                break;
            case 32:
                matches = fscanf(file, "%d/%d %d/%d %d/%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2]);
                indicies.push_back(vertexIndex[0] - 1);
                indicies.push_back(vertexIndex[1] - 1);
                indicies.push_back(vertexIndex[2] - 1);
                uvIndices.push_back(uvIndex[0] - 1);
                uvIndices.push_back(uvIndex[1] - 1);
                uvIndices.push_back(uvIndex[2] - 1);
                break;
            }
        }
        else if (strcmp(lineHeader, "usemtl") == 0) {
            char mtlLink[128];
            fscanf(file, "%s\n", &mtlLink);
            FILE* file1 = fopen((std::string(p).append("mtl")).c_str(), "rb");
            while (true) {
                char lineHeader[128];
                char mtlName[64];

                int res = fscanf(file1, "%s %s\n", mtlName, lineHeader);
                if (res == EOF)
                    break;

                if (strcmp(lineHeader, mtlLink) == 0) {
                    char k[16];
                    float t1, t2, t3;
                    //fscanf(file1, "\n");
                    fscanf(file1, "%s\n", k);
                    fscanf(file1, "%s %d %d %d\n", k, &kd[0], &kd[1], &kd[2]);
                    fscanf(file1, "%s %d %d %d\n", k, &t1, &t2, &t3);
                }
            }
        }

    };


    if (fileType != 31 && fileType != 32) {
        for (int i = 0; i < vertices.size() / 4; i++) {
            for (int j = 0; j < normIndicies.size(); j++) {
                if (indicies.at(j) == i) {
                    norms.push_back(norm.at(normIndicies.at(j).index * 4) * normIndicies.at(j).kd[0]);
                    norms.push_back(norm.at(normIndicies.at(j).index * 4 + 1) * normIndicies.at(j).kd[1]);
                    norms.push_back(norm.at(normIndicies.at(j).index * 4 + 2) * normIndicies.at(j).kd[2]);
                    norms.push_back(1.0f);
                    break;
                }
            }
        }
    }
    else {
        norms = norm;
    }

    if (fileType != 21 && fileType != 22) {
        for (int i = 0; i < vertices.size() / 4; i++) {
            for (int j = 0; j < uvIndices.size(); j++) {
                if (indicies.at(j) == i) {
                    texs.push_back(tex.at(uvIndices.at(j) * 2));
                    texs.push_back(tex.at(uvIndices.at(j) * 2 + 1));
                    break;
                }
            }
        }
    }
    else {
        texs = tex;
    }

    std::vector<float> tangent, bitangent;

    for (int i = 0; i < vertices.size() / 4; i += 1) {
        float3 edge1, edge2;
        float2 deltaUV1, deltaUV2;
        if (i < vertices.size() - 3) {
            edge1 = float3(vertices.at(i + 4), vertices.at(i + 5), vertices.at(i + 6)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            edge2 = float3(vertices.at(i + 8), vertices.at(i + 9), vertices.at(i + 10)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            deltaUV1 = float2(texs.at(i + 3), texs.at(i + 4)) - float2(texs.at(i), texs.at(i + 1));
            deltaUV2 = float2(texs.at(i + 5), texs.at(i + 6)) - float2(texs.at(i), texs.at(i + 1));
        }
        else if (i == vertices.size() - 2) {
            edge1 = float3(vertices.at(i + 4), vertices.at(i + 5), vertices.at(i + 6)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            edge2 = float3(vertices.at(0), vertices.at(1), vertices.at(2)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            deltaUV1 = float2(texs.at(i + 3), texs.at(i + 4)) - float2(texs.at(i), texs.at(i + 1));
            deltaUV2 = float2(texs.at(0), texs.at(1)) - float2(texs.at(i), texs.at(i + 1));
        }
        else {
            edge1 = float3(vertices.at(0), vertices.at(1), vertices.at(2)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            edge2 = float3(vertices.at(3), vertices.at(4), vertices.at(5)) - float3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
            deltaUV1 = float2(texs.at(0), texs.at(1)) - float2(texs.at(i), texs.at(i + 1));
            deltaUV2 = float2(texs.at(2), texs.at(3)) - float2(texs.at(i), texs.at(i + 1));
        }
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        float3 tangent1, bitangent1;

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = normalize(bitangent1);

        tangent.push_back(tangent1.x);
        tangent.push_back(tangent1.y);
        tangent.push_back(tangent1.z);

        bitangent.push_back(bitangent1.x);
        bitangent.push_back(bitangent1.y);
        bitangent.push_back(bitangent1.z);
    }

    GLuint vboVertices, vboIndices, vboNormals, vboTexCoords, vboTangent, vboBitangent;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(GLfloat), norms.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboTexCoords);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, texs.size() * sizeof(GLfloat), &texs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &vboTangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboTangent);
    glBufferData(GL_ARRAY_BUFFER, tangent.size() * sizeof(GLfloat), &tangent[0], GL_STATIC_DRAW);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(4);

    glGenBuffers(1, &vboBitangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboBitangent);
    glBufferData(GL_ARRAY_BUFFER, bitangent.size() * sizeof(GLfloat), &bitangent[0], GL_STATIC_DRAW);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(5);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(int), indicies.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indicies.size();
}


GLuint stone(GLuint& vao) {
    return loadFile("../models/RockSet.", vao);
}

GLuint wall(GLuint& vao) {
	return loadFile("../models/wall2.", vao);
}




GLsizei CreateCylinder(GLuint& vao, float radius, int numberSlices, float height)
{
    int numberTriangles = numberSlices * 2 + numberSlices * 2;
    int numberVertices = numberSlices * 2;
    int numberIndices = numberTriangles * 3;

    float angleStep = (360.0f / numberSlices) * 3.14159265358979323846f / 180;
    float4 center = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 top = float4(center.x, center.y + height, center.z, center.w);

    std::vector<float> positions = { center.x, center.y, center.z, center.w};
    std::vector<float> normals = { 0.0f, -1.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f, 1.0f };
    std::vector<uint32_t> indices;

    float4 angleVector = float4(0.0f, 0.0f, radius, 1.0f);
    float4 edge;
    float newX;
    float newZ;

    for (int i = 0; i < numberSlices; i++)
    {
        edge = center + angleVector;
        edge.w = 1.0f;
        positions.push_back(edge.x);
        positions.push_back(edge.y);
        positions.push_back(edge.z);
        positions.push_back(edge.w);
        positions.push_back(edge.x);
        positions.push_back(edge.y + height);
        positions.push_back(edge.z);
        positions.push_back(edge.w);

        newZ = angleVector.z * cosf(angleStep) + (-1 * sinf(angleStep) * angleVector.x);
        newX = angleVector.z * sinf(angleStep) + cosf(angleStep) * angleVector.x;
        angleVector.x = newX;
        angleVector.z = newZ;
    }
    positions.push_back(top.x);
    positions.push_back(top.y);
    positions.push_back(top.z);
    positions.push_back(top.w);


    for (uint32_t i = 0; i < numberVertices - 3; i = i + 2)
    {
        indices.push_back(0u);
        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
        indices.push_back(i + 4);
        indices.push_back(i + 2);
        indices.push_back(i + 4);
        indices.push_back(numberVertices + 1);
    }
    indices.push_back(0u);
    indices.push_back(numberVertices - 1);
    indices.push_back(1u);

    indices.push_back(numberVertices - 1);
    indices.push_back(1u);
    indices.push_back(numberVertices);

    indices.push_back(1u);
    indices.push_back(numberVertices);
    indices.push_back(2u);

    indices.push_back(numberVertices);
    indices.push_back(2u);
    indices.push_back(numberVertices + 1);

    float4 side0;
    float4 side1 = top - center;
    float4 normal;
    for (int i = 4; i <= positions.size() - 20; i = i + 8)
    {
        side0 = float4(positions.at(i + 8), positions.at(i + 9), positions.at(i + 10), positions.at(i + 11)) - float4(positions.at(i), positions.at(i + 1), positions.at(i + 2), positions.at(i + 3));
        normal = scal(side0, side1);
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
        normals.push_back(normal.w);
    }
    side0 = float4(positions.at(4), positions.at(5), positions.at(6), positions.at(7)) - float4(positions.at(numberSlices * 4), positions.at(numberSlices * 4 + 1), positions.at(numberSlices * 4 + 2), positions.at(numberSlices * 4 + 3));
    normal = scal(side0, side1);
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
    normals.push_back(normal.w);

    

    GLuint vboVertices, vboIndices, vboNormals;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

GLsizei CreateCone(GLuint& vao, float radius, int numberSlices, float height)
{
    int numberTriangles = numberSlices * 2;
    int numberVertices = numberSlices + 1;
    int numberIndices = numberTriangles * 3;

    float angleStep = (360.0f / numberSlices) * 3.14159265358979323846f / 180;
    float4 center = float4(0.0f, 0.0f, 0.0f, 1.0f);

    std::vector<float> positions = { center.x, center.y, center.z, center.w };
    std::vector<float> normals = { 0.0f, -1.0f, 0.0f, 1.0f };
    std::vector<float> texcoords(numberSlices * 2 + 1, 0.0f);
    std::vector<uint32_t> indices;

    float4 angleVector = float4(0.0f, 0.0f, radius, 1.0f);
    float4 edge;
    float newX;
    float newZ;
    for (int i = 0; i < numberSlices; i++)
    {
        edge = center + angleVector;
        edge.w = 1.0f;
        positions.push_back(edge.x);
        positions.push_back(edge.y);
        positions.push_back(edge.z);
        positions.push_back(edge.w);

        newZ = angleVector.z * cosf(angleStep) + (-1 * sinf(angleStep) * angleVector.x);
        newX = angleVector.z * sinf(angleStep) + cosf(angleStep) * angleVector.x;
        angleVector.x = newX;
        angleVector.z = newZ;
    }
    float4 top = float4(center.x, center.y + height, center.z, center.w);
    positions.push_back(top.x);
    positions.push_back(top.y);
    positions.push_back(top.z);
    positions.push_back(top.w);


    for (uint32_t i = 0; i < numberSlices - 1; i++)
    {
        indices.push_back(0u);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }
    indices.push_back(0u);
    indices.push_back((uint32_t)numberSlices);
    indices.push_back(1u);
    for (uint32_t i = 0; i < numberSlices - 1; i++)
    {
        indices.push_back(i + 1);
        indices.push_back(i + 2);
        indices.push_back((uint32_t)numberVertices);
    }
    indices.push_back((uint32_t)numberSlices);
    indices.push_back(1u);
    indices.push_back((uint32_t)numberVertices);

    float4 side0;
    float4 side1;
    float4 normal;
    uint32_t i = 4;
    while (i <= positions.size() - 12)
    {
        side0 = float4(positions.at(i), positions.at(i + 1u), positions.at(i + 2u), positions.at(i + 3u)) - top;
        side1 = float4(positions.at(i + 4u), positions.at(i + 5u), positions.at(i + 6u), positions.at(i + 7u)) - top;
        normal = scal(side0, side1);
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
        normals.push_back(normal.w);
        i += 4;
    }
    side0 = float4(positions.at(4u), positions.at(5u), positions.at(6u), positions.at(7u)) - top;

    normal = scal(side1, side0);
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
    normals.push_back(normal.w);

    GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboTexCoords);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), &texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

GLsizei CreateSphere(float radius, int numberSlices, GLuint &vao)
{
  int i, j;

  int numberParallels = numberSlices;
  int numberVertices = (numberParallels + 1) * (numberSlices + 1);
  int numberIndices = numberParallels * numberSlices * 3;

  float angleStep = (2.0f * 3.14159265358979323846f) / ((float) numberSlices);
  //float helpVector[3] = {0.0f, 1.0f, 0.0f};

  std::vector<float> pos(numberVertices * 4, 0.0f);
  std::vector<float> norm(numberVertices * 4, 0.0f);
  std::vector<float> texcoords(numberVertices * 2, 0.0f);

  std::vector<int> indices(numberIndices, -1);

  for (i = 0; i < numberParallels + 1; i++)
  {
    for (j = 0; j < numberSlices + 1; j++)
    {
      int vertexIndex = (i * (numberSlices + 1) + j) * 4;
      int normalIndex = (i * (numberSlices + 1) + j) * 4;
      int texCoordsIndex = (i * (numberSlices + 1) + j) * 2;

      pos.at(vertexIndex + 0) = radius * sinf(angleStep * (float) i) * sinf(angleStep * (float) j);
      pos.at(vertexIndex + 1) = radius * cosf(angleStep * (float) i);
      pos.at(vertexIndex + 2) = radius * sinf(angleStep * (float) i) * cosf(angleStep * (float) j);
      pos.at(vertexIndex + 3) = 1.0f;

      norm.at(normalIndex + 0) = pos.at(vertexIndex + 0) / radius;
      norm.at(normalIndex + 1) = pos.at(vertexIndex + 1) / radius;
      norm.at(normalIndex + 2) = pos.at(vertexIndex + 2) / radius;
      norm.at(normalIndex + 3) = 1.0f;

      texcoords.at(texCoordsIndex + 0) = (float) j / (float) numberSlices;
      texcoords.at(texCoordsIndex + 1) = (1.0f - (float) i) / (float) (numberParallels - 1);
    }
  }

  int *indexBuf = &indices[0];

  for (i = 0; i < numberParallels; i++)
  {
    for (j = 0; j < numberSlices; j++)
    {
      *indexBuf++ = i * (numberSlices + 1) + j;
      *indexBuf++ = (i + 1) * (numberSlices + 1) + j;
      *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);

      *indexBuf++ = i * (numberSlices + 1) + j;
      *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);
      *indexBuf++ = i * (numberSlices + 1) + (j + 1);

      int diff = int(indexBuf - &indices[0]);
      if (diff >= numberIndices)
        break;
    }
    int diff = int(indexBuf - &indices[0]);
    if (diff >= numberIndices)
      break;
  }

  GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vboIndices);

  glBindVertexArray(vao);

  glGenBuffers(1, &vboVertices);
  glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
  glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(GLfloat), &pos[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(0);

  glGenBuffers(1, &vboNormals);
  glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
  glBufferData(GL_ARRAY_BUFFER, norm.size() * sizeof(GLfloat), &norm[0], GL_STATIC_DRAW);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(1);

  glGenBuffers(1, &vboTexCoords);
  glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
  glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), &texcoords[0], GL_STATIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

  glBindVertexArray(0);

  return indices.size();
}

GLsizei CreatePlane(GLuint& vao, float4 a, float size)
{
    std::vector<float> positions = {
        a.x, a.y, a.z, a.w,
        a.x + size, a.y, a.z, a.w,
        a.x, a.y, a.z + size, a.w,
        a.x + size, a.y, a.z + size, a.w
    };

    std::vector<float> normals = { 0.0f, 1.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f, 1.0f };

    std::vector<uint32_t> indices = { 0u, 1u, 2u,
                                      1u, 2u, 3u};

    GLuint vboVertices, vboIndices, vboNormals;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

GLsizei CreateCuboid(GLuint& vao, float4 a, float ax, float ay, float az)
{
    std::vector<float> positions = {
        a.x, a.y, a.z, a.w,
        a.x + ax, a.y, a.z, a.w,
        a.x, a.y, a.z + az, a.w,
        a.x + ax, a.y, a.z + az, a.w,
        a.x, a.y + ay, a.z, a.w,
        a.x + ax, a.y + ay, a.z, a.w,
        a.x, a.y + ay, a.z + az, a.w,
        a.z + ax, a.y + ay, a.z + az, a.w
    };
    std::vector<uint32_t> indices = {
        0u, 1u, 3u,
        0u, 3u, 2u,
        0u, 4u, 5u,
        0u, 5u, 1u,
        1u, 5u, 3u,
        5u, 7u, 3u,
        0u, 4u, 6u,
        0u, 6u, 2u,
        2u, 6u, 3u,
        6u, 7u, 3u,
        4u, 5u, 6u,
        5u, 7u, 6u
    };

    std::vector<float> normals = {
        0.0f, -1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, -1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };

    GLuint vboVertices, vboIndices, vboNormals;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

void CreateSkybox(GLuint& vao)
{
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

GLsizei CreateTriangle(GLuint& vao)
{
  std::vector<float> positions = { -1.0f, 0.0f, 0.0f, 1.0f,
                                    1.0f, 0.0f, 0.0f, 1.0f,
                                    0.0f, 2.0f, 0.0f, 1.0f };

  std::vector<float> normals = {  0.0f, 0.0f, 1.0f, 1.0f,
                                  0.0f, 0.0f, 1.0f, 1.0f,
                                  0.0f, 0.0f, 1.0f, 1.0f };

  std::vector<float> texCoords = { 0.0f, 0.0f,
                                   0.5f, 1.0f,
                                   1.0f, 0.0f };

  std::vector<uint32_t> indices = { 0u, 1u, 2u };

  GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vboIndices);

  glBindVertexArray(vao);

  glGenBuffers(1, &vboVertices);
  glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
  glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(0);

  glGenBuffers(1, &vboNormals);
  glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(1);

  glGenBuffers(1, &vboTexCoords);
  glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
  glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(GLfloat), texCoords.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);

  return indices.size();
}

unsigned int getTexture(const char *path)
{
    unsigned int texture; 
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return (texture);
}

unsigned int getCubeTexture(std::vector<std::string> planes)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < planes.size(); i++)
    {
        unsigned char* data = stbi_load(planes[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << planes[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

int initGL()
{
	int res = 0;

	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	//выводим в консоль некоторую информацию о драйвере и контексте opengl
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  std::cout << "Controls: "<< std::endl;
  std::cout << "press right mouse button to capture/release mouse cursor  "<< std::endl;
  std::cout << "press spacebar to alternate between shaded wireframe and fill display modes" << std::endl;
  std::cout << "press ESC to exit" << std::endl;

	return 0;
}

int main(int argc, char** argv)
{
	if(!glfwInit())
        return -1;

	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); 


  GLFWwindow*  window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window); 

	//регистрируем коллбеки для обработки сообщений от пользователя - клавиатура, мышь..
	glfwSetKeyCallback        (window, OnKeyboardPressed);  
	glfwSetCursorPosCallback  (window, OnMouseMove); 
  glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
	glfwSetScrollCallback     (window, OnMouseScroll);
	glfwSetInputMode          (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

	if(initGL() != 0) 
		return -1;
	
  //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR)
		gl_error = glGetError();

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders;
	shaders[GL_VERTEX_SHADER]   = "../shaders/vertex.glsl";
	shaders[GL_FRAGMENT_SHADER] = "../shaders/lambert.frag";
	ShaderProgram lambert(shaders); GL_CHECK_ERRORS;

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  GLuint vaoTriangle;
  GLsizei triangleIndices = CreateTriangle(vaoTriangle);

  GLuint vaoSphere;
  float radius = 1.0f;
  GLsizei sphereIndices = CreateSphere(radius, 8, vaoSphere);

  //ADDED
  GLuint vaoCuboid;
  GLsizei cuboidIndices = CreateCuboid(vaoCuboid, float4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 2.0f, 1.0f);

  GLuint vaoPlane;
  GLsizei planeIndices = CreatePlane(vaoPlane, float4(-30.0f, -10.0f, -30.0f, 1.0f), 60.0f);

  GLuint vaoCone;
  GLsizei coneIndices = CreateCone(vaoCone, 1.0f, 6, 7.0f);

  GLuint vaoCylinder;
  GLsizei cylinderIndices = CreateCylinder(vaoCylinder, 0.6f, 6, 30.0f);

  GLuint vaoSkybox;
  CreateSkybox(vaoSkybox);

  //objects

  GLuint vaoStones;
  GLsizei stonesIndices = stone(vaoStones);

  GLuint vaoWall;
  GLsizei wallIndices = wall(vaoWall);


  //--------TEXTURE----------//

  unsigned int texture = getTexture("../textures/Sun.jpg");
  unsigned int stoneTexture = getTexture("../models/RockTexture1.jpg");
  unsigned int wallTexture = getTexture("../models/wall1.jpg");



  std::vector<std::string> boxPlanes = {
      "../textures/skybox/right.jpg",
      "../textures/skybox/left.jpg",
	  "../textures/skybox/top.jpg",
	  "../textures/skybox/bottom.jpg",
      "../textures/skybox/front.jpg",
      "../textures/skybox/back.jpg"
  };
  unsigned int boxTextures = getCubeTexture(boxPlanes);
  

  //-------END TEXTURE-------//
  
  std::vector<float> xTrans;
  std::vector<float> zTrans;
  std::vector<float> xzScale;
  std::vector<float> yScale;

  for (int i = 0; i < 30; i++)
  {
      xTrans.push_back(2.0f + float(rand()) / (RAND_MAX / (10.0f - 2.0f)));
      zTrans.push_back(2.0f + float(rand()) / (RAND_MAX / (10.0f - 2.0f)));
      xzScale.push_back(0.5f + float(rand()) / (RAND_MAX / (1.5f - 1.0f)));
      yScale.push_back(0.5f + float(rand()) / (RAND_MAX / (2.5f - 0.5f)));

  }



  //END ADDED

  glViewport(0, 0, WIDTH, HEIGHT);  GL_CHECK_ERRORS;
  glEnable(GL_DEPTH_TEST);  GL_CHECK_ERRORS;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<float3> translations(100);
    float2 translation;
    for (unsigned int i = 0; i < 100; i++)
    {
        translation.x = 9.0f + float(rand()) / (RAND_MAX / (25.0f - 6.0f));
        translation.y = 12.0f + float(rand()) / (RAND_MAX / (25.0f - 6.0f));
        translations[i].x = translation.x;
        translations[i].z = translation.y;
    }

	//цикл обработки сообщений и отрисовки сцены каждый кадр
	while (!glfwWindowShouldClose(window))
	{
		//считаем сколько времени прошло за кадр
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
    doCameraMovement(camera, deltaTime);

		//очищаем экран каждый кадр
		glClearColor(0.9f, 0.95f, 0.97f, 1.0f); GL_CHECK_ERRORS;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_CHECK_ERRORS;

    lambert.StartUseShader(); GL_CHECK_ERRORS;

    float4x4 view       = camera.GetViewMatrix();
    
    
    /////////////////////////////BOX//////////////////////////
    glDepthFunc(GL_LEQUAL);

    float4x4 view1;
    view1.row[0] = camera.GetViewMatrix().row[0];
    view1.row[1] = camera.GetViewMatrix().row[1];
    view1.row[2] = camera.GetViewMatrix().row[2];
    float4x4 projection = projectionMatrixTransposed(camera.zoom, float(WIDTH) / float(HEIGHT), 0.1f, 1000.0f);
	  float4x4 model; 

    lambert.SetUniform("view", view1);       GL_CHECK_ERRORS;
    lambert.SetUniform("projection", projection); GL_CHECK_ERRORS;
    lambert.SetUniform("type", 1);
    lambert.SetUniform("skybox", 2);
    lambert.SetUniform("verType", 0);

   
    glBindVertexArray(vaoSkybox);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, boxTextures);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    //////////////////END SKYBOX/////////////////////
    
    lambert.SetUniform("view", view);       GL_CHECK_ERRORS;
    lambert.SetUniform("type", 2);
    lambert.SetUniform("verType", 2);

    lambert.SetUniform("material.diffuse", 6);
    lambert.SetUniform("material.specular", 1);
    lambert.SetUniform("material.color", float3(0.46f, 0.76f, 0.97f));


    glBindVertexArray(vaoSphere);
    {
        lambert.SetUniform("lightColor", float3(0.56f, 0.76f, 0.99f));
        glBindTexture(GL_TEXTURE_2D, texture);
        lambert.SetUniform("type", 5);

       
        lambert.SetUniform("pointLights[0].position", float3(0.0f, -5.0f, 0.0f));
        lambert.SetUniform("pointLights[1].position", float3(0.0f, 0.0f, 0.0f));

        lambert.SetUniform("pointLights[0].ambient", float3(1.0f, 1.5f, 0.5f));
        lambert.SetUniform("pointLights[0].diffuse", float3(0.4f, 0.4f, 0.4f));
        lambert.SetUniform("pointLights[0].specular", float3(0.5f, 0.4f, 0.5f));

		lambert.SetUniform("pointLights[1].ambient", float3(0.3f, 0.7f, 0.5f));
		lambert.SetUniform("pointLights[1].diffuse", float3(0.5f, 0.5f, 0.5f));
		lambert.SetUniform("pointLights[1].specular", float3(0.3f, 0.7f, 0.5f));

        lambert.SetUniform("pointLights[0].constant", 1.0f);
        lambert.SetUniform("pointLights[1].constant", 1.0f);

        lambert.SetUniform("pointLights[0].linear", 0.09f);
        lambert.SetUniform("pointLights[1].linear", 0.09f);

        lambert.SetUniform("directLight.direction", float3(0.0f, -1.0f, 0.0f));
        lambert.SetUniform("directLight.ambient", float3(1.0f, 1.0f, 1.0f));
        lambert.SetUniform("directLight.specular", float3(0.f, 0.2f, 0.2f));
        lambert.SetUniform("directLight.diffuse", float3(0.3f, 0.2f, 0.3f));

        lambert.SetUniform("normalMap", 5);
        lambert.SetUniform("useNormalMap", false);

        lambert.SetUniform("material.diffuse", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture);

      model = transpose(mul(translate4x4(float3(0.0f, 0.0f, 0.0f)), scale4x4(float3(0.4f, 0.4f, 0.4f))));
      lambert.SetUniform("model", model); GL_CHECK_ERRORS;
      glDrawElements(GL_TRIANGLE_STRIP, sphereIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    }
    glBindVertexArray(0); GL_CHECK_ERRORS;

    lambert.SetUniform("type", 0);

    /*glBindVertexArray(vaoTriangle); GL_CHECK_ERRORS;
    {
      model = transpose(mul(translate4x4(float3(-3.0f, 4.0f, 0.0f)), rotate_Y_4x4(45.0f)));
      lambert.SetUniform("model", model); GL_CHECK_ERRORS;
      glDrawElements(GL_TRIANGLE_STRIP, triangleIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    }
    glBindVertexArray(0); GL_CHECK_ERRORS; */

    //ADDED
    glBindVertexArray(vaoCuboid); GL_CHECK_ERRORS;
    {
        model = transpose(mul(translate4x4(float3(-0.0f, -5.0f, 0.0f)), scale4x4(float3(0.2f, 0.2f, 0.2f))));
        lambert.SetUniform("lightColor", float3(1.0f, 0.5f, 0.0f));
        lambert.SetUniform("type", 3);

        lambert.SetUniform("model", model); GL_CHECK_ERRORS;
        glDrawElements(GL_TRIANGLE_STRIP, cuboidIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    }
    glBindVertexArray(0); GL_CHECK_ERRORS; 

    glBindVertexArray(vaoPlane); GL_CHECK_ERRORS;
    {
        model = transpose(translate4x4(float3(-4.0f, 5.0f, -4.0f)));
        lambert.SetUniform("type", 3);
		lambert.SetUniform("lightColor", float3(0.2f, 0.5f, 0.3f));
        lambert.SetUniform("model", model); GL_CHECK_ERRORS;
        glDrawElements(GL_TRIANGLE_STRIP, planeIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    }
    glBindVertexArray(0); GL_CHECK_ERRORS;

    lambert.SetUniform("type", 4);
    lambert.SetUniform("material.color", float3(0.46f, 0.76f, 0.97f));
    lambert.SetUniform("material.shininess", 0.3f);


    
    /////////////////////////MODELS OBJECTS
    lambert.SetUniform("type", 5);
    lambert.SetUniform("verType", 2);
    lambert.SetUniform("material.diffuse", 1);
    glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_2D, stoneTexture);
    glBindVertexArray(vaoStones); GL_CHECK_ERRORS;
    {
        float4x4 model1;
        //model1 = mul(model, translate4x4(float3(0.0f, 0.0f, 0.0f)));
        //model1 = transpose(mul(model1, scale4x4(float3(0.5f, 0.5f, 0.5f))));
        model = transpose(mul(translate4x4(float3(5.0f, -5.25f, 4.0f)), scale4x4(float3(0.3f, 0.3f, 0.3f))));
        lambert.SetUniform("model", model); GL_CHECK_ERRORS;
        glDrawElements(GL_TRIANGLES, stonesIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    }
    glBindVertexArray(0); GL_CHECK_ERRORS;

	glBindTexture(GL_TEXTURE_2D, wallTexture);
	glBindVertexArray(vaoWall); GL_CHECK_ERRORS;
	{
		float4x4 model1;
		//model1 = mul(model, translate4x4(float3(0.0f, 0.0f, 0.0f)));
		//model1 = transpose(mul(model1, scale4x4(float3(0.5f, 0.5f, 0.5f))));
		model = transpose(mul(mul(translate4x4(float3(1.0f, -5.0f, -4.0f)), rotate_Z_4x4(-150 * LiteMath::DEG_TO_RAD)), scale4x4(float3(0.1f, 0.3f, 0.3f ))));
		lambert.SetUniform("model", model); GL_CHECK_ERRORS;
		glDrawElements(GL_TRIANGLES, wallIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;

		model = transpose(mul(mul(translate4x4(float3(5.0f, -3.5f, -4.0f)), rotate_Z_4x4(-90 * LiteMath::DEG_TO_RAD)), scale4x4(float3(0.1f, 0.3f, 0.3f))));
		lambert.SetUniform("model", model); GL_CHECK_ERRORS;
		glDrawElements(GL_TRIANGLES, wallIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;

		model = transpose(mul(mul(translate4x4(float3(-4.0f, -3.5f, -4.0f)), rotate_Z_4x4(-90 * LiteMath::DEG_TO_RAD)), scale4x4(float3(0.1f, 0.3f, 0.3f))));
		lambert.SetUniform("model", model); GL_CHECK_ERRORS;
		glDrawElements(GL_TRIANGLES, wallIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
	}


    ////////////////////END MODELS OBJECTS
    
    
    lambert.SetUniform("type", 4);
    lambert.SetUniform("verType", 1);
	lambert.SetUniform("material.color", float3(0.0f, 1.0f, 0.0f));
    
    for (unsigned int i = 0; i < 100; i++)
    {
        std::string index = std::to_string(i);
        lambert.SetUniform(("offsets[" + index + "]").c_str(), translations[i]);
    }
	 glBindVertexArray(vaoCone); GL_CHECK_ERRORS;
    {
        //model = transpose(mul(translate4x4(float3(xTrans.at(i), -4.0f, zTrans.at(i))), scale4x4(float3(xzScale.at(i), yScale.at(i), xzScale.at(i)))));
        model = transpose(translate4x4(float3(-30.0f, -5.0f, -20.0f))); GL_CHECK_ERRORS;
        lambert.SetUniform("model", model); GL_CHECK_ERRORS;
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, coneIndices, GL_UNSIGNED_INT, nullptr, 100); GL_CHECK_ERRORS;   
    }
	 
    glBindVertexArray(0); GL_CHECK_ERRORS;
    
  
    //END ADDED

    lambert.StopUseShader(); GL_CHECK_ERRORS;

		glfwSwapBuffers(window); 
	}

  glDeleteVertexArrays(1, &vaoSphere);
  glDeleteVertexArrays(1, &vaoTriangle);
  //ADDED
  glDeleteVertexArrays(1, &vaoCuboid);
  glDeleteVertexArrays(1, &vaoPlane);
  glDeleteVertexArrays(1, &vaoCone);
  glDeleteVertexArrays(1, &vaoSkybox);

  //END ADDED

	glfwTerminate();
	return 0;
}
