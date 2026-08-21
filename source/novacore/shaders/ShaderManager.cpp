#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

std::unordered_map<std::string, Shader> ShaderManager::shaderCache;
std::string ShaderManager::activeShader;

void ShaderManager::Init() {
    shaderCache.clear();
    activeShader.clear();
}

void ShaderManager::Shutdown() {
    ClearAll();
}

std::string ShaderManager::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint ShaderManager::CompileStage(GLenum type, const std::string& source) {
    GLuint stage = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(stage, 1, &sourcePtr, nullptr);
    glCompileShader(stage);

    GLint success = 0;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(stage, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(logLength > 0 ? logLength : 1);
        glGetShaderInfoLog(stage, logLength, nullptr, log.data());

        std::cerr << "Shader compile error: " << log.data() << std::endl;

        glDeleteShader(stage);
        return 0;
    }

    return stage;
}

bool ShaderManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    auto it = shaderCache.find(name);
    if (it != shaderCache.end()) {
        return true;
    }

    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        return false;
    }

    GLuint vertexStage = CompileStage(GL_VERTEX_SHADER, vertexSource);
    if (vertexStage == 0) {
        return false;
    }

    GLuint fragmentStage = CompileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentStage == 0) {
        glDeleteShader(vertexStage);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexStage);
    glAttachShader(program, fragmentStage);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    glDeleteShader(vertexStage);
    glDeleteShader(fragmentStage);

    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(logLength > 0 ? logLength : 1);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());

        std::cerr << "Shader link error: " << log.data() << std::endl;

        glDeleteProgram(program);
        return false;
    }

    Shader shader;
    shader.program = program;
    shaderCache[name] = shader;

    return true;
}

void ShaderManager::UnloadShader(const std::string& name) {
    auto it = shaderCache.find(name);
    if (it != shaderCache.end()) {
        glDeleteProgram(it->second.program);
        shaderCache.erase(it);

        if (activeShader == name) {
            activeShader.clear();
        }
    }
}

void ShaderManager::ClearAll() {
    for (auto& pair : shaderCache) {
        glDeleteProgram(pair.second.program);
    }
    shaderCache.clear();
    activeShader.clear();
}

void ShaderManager::Use(const std::string& name) {
    auto it = shaderCache.find(name);
    if (it == shaderCache.end()) {
        std::cerr << "Shader not found: " << name << std::endl;
        return;
    }

    glUseProgram(it->second.program);
    activeShader = name;
}

void ShaderManager::Unuse() {
    glUseProgram(0);
    activeShader.clear();
}

GLuint ShaderManager::GetProgram(const std::string& name) {
    auto it = shaderCache.find(name);
    if (it != shaderCache.end()) {
        return it->second.program;
    }
    return 0;
}

bool ShaderManager::HasShader(const std::string& name) {
    return shaderCache.find(name) != shaderCache.end();
}

GLint ShaderManager::GetUniformLocation(GLuint program, const std::string& uniform) {
    return glGetUniformLocation(program, uniform.c_str());
}

void ShaderManager::SetUniformInt(const std::string& name, const std::string& uniform, int value) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void ShaderManager::SetUniformFloat(const std::string& name, const std::string& uniform, float value) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void ShaderManager::SetUniformVec2(const std::string& name, const std::string& uniform, float x, float y) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void ShaderManager::SetUniformVec3(const std::string& name, const std::string& uniform, float x, float y, float z) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void ShaderManager::SetUniformVec4(const std::string& name, const std::string& uniform, float x, float y, float z, float w) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void ShaderManager::SetUniformMat4(const std::string& name, const std::string& uniform, const float* matrix) {
    GLuint program = GetProgram(name);
    if (program == 0) return;

    GLint location = GetUniformLocation(program, uniform);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    }
}
