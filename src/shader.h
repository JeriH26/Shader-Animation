#pragma once

#include <string>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

std::string readFileToString(const std::string &path);
GLuint createProgramFromFiles(const char *vertexSource, const std::string &fragPath);
