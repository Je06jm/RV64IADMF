#include "Framebuffer.hpp"

#include <array>
#include <string>
#include <iostream>
#include <cstdlib>

MemoryFramebuffer::MemoryFramebuffer(Address base, Address size, Word width, Word height) : MemoryRegion(TYPE_FRAMEBUFFER, 0, base, size, true, true), width{width}, height{height} {
    word_buffer.resize(size, 0);
    for (auto& word : word_buffer)
        word = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    static constexpr std::array quad = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,

        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), quad.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    static const std::string vert_code =
    R"(#version 430 core
    
    in vec2 aPos;
    in vec2 aUV;
    
    out vec2 vUV; 
    
    void main() {
        gl_Position = vec4(aPos, 0.5, 1.0);
        vUV = aUV;
    })";

    static const std::string frag_code =
    R"(#version 430 core
    out vec3 fColor;
    
    in vec2 vUV;
    
    uniform sampler2D framebuffer;
    
    void main() {
        fColor = texture(framebuffer, vUV).rgb;
    })";

    auto CompileShader = [](auto type, const char* code) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            std::cerr << "Shader compile error: " << log << "\n" << code << std::endl;
            std::exit(EXIT_FAILURE);
        }
        return shader;
    };

    const char* vert_const_code = vert_code.c_str();
    const char* frag_const_code = frag_code.c_str();

    auto vert = CompileShader(GL_VERTEX_SHADER, vert_const_code);
    auto frag = CompileShader(GL_FRAGMENT_SHADER, frag_const_code);

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

MemoryFramebuffer::~MemoryFramebuffer() {
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vao);
    glDeleteTextures(1, &texture);
}

void MemoryFramebuffer::DrawBuffer() const {
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    static auto location = glGetUniformLocation(program, "framebuffer");

    glUniform1ui(location, 0);
    
    Lock();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, word_buffer.data());    

    Unlock();

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

std::shared_ptr<MemoryFramebuffer> MemoryFramebuffer::Create(Address base, Word width, Word height) {
    Address size = width * height * 4;

    size = (size + sizeof(Word) - 1) & ~(sizeof(Word) - 1);

    return std::shared_ptr<MemoryFramebuffer>(new MemoryFramebuffer(base & ~3, size, width, height));
}