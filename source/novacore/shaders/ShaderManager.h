#pragma once

#if defined(NOVACORE_ANDROID)
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

#include <string>
#include <unordered_map>

struct Shader {
    GLuint program = 0;
};

class ShaderManager {
public:
    static void Init();
    static void Shutdown();

    static bool LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    static void UnloadShader(const std::string& name);
    static void ClearAll();

    static void Use(const std::string& name);
    static void Unuse();

    static GLuint GetProgram(const std::string& name);
    static bool HasShader(const std::string& name);

    static void SetUniformInt(const std::string& name, const std::string& uniform, int value);
    static void SetUniformFloat(const std::string& name, const std::string& uniform, float value);
    static void SetUniformVec2(const std::string& name, const std::string& uniform, float x, float y);
    static void SetUniformVec3(const std::string& name, const std::string& uniform, float x, float y, float z);
    static void SetUniformVec4(const std::string& name, const std::string& uniform, float x, float y, float z, float w);
    static void SetUniformMat4(const std::string& name, const std::string& uniform, const float* matrix);

private:
    static GLuint CompileStage(GLenum type, const std::string& source);
    static std::string ReadFile(const std::string& path);
    static GLint GetUniformLocation(GLuint program, const std::string& uniform);

    static std::unordered_map<std::string, Shader> shaderCache;
    static std::string activeShader;
};
