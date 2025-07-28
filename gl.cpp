#include "gl.h"
#include <iostream>
#include <cmath>

#define GL_CHECK(x) do { x; } while(0)

const char* vShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;
out vec2 texCoord;
uniform mat4 transform;
void main() {
    gl_Position = transform * vec4(aPos, 0.0, 1.0);
    texCoord = aTex;
}
)";

const char* fShader = R"(
#version 330 core
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;

// BT.709的YUV到RGB转换
const mat3 yuv2rgb = mat3(
    1.0,   0.0,      1.5748,
    1.0,  -0.1873,  -0.4681,
    1.0,   1.8556,   0.0
);

void main() {
    // UV需要偏移
    float y = texture(texY, texCoord).r;
    float u = texture(texU, texCoord).r - 0.5;
    float v = texture(texV, texCoord).r - 0.5;
    
    // YUV到RGB
    vec3 rgb = yuv2rgb * vec3(y, u, v);
    
    rgb = clamp(rgb, 0.0, 1.0);
    
    fragColor = vec4(rgb, 1.0);
}
)";

GL::GL() : win(nullptr), w(0), h(0), pgm(0), vao(0), vbo(0), fbo(0), rbo(0), buf(nullptr) {
    for(int i = 0; i < 3; i++) tex[i] = 0;
}

GL::~GL() {
    c();
}

bool GL::i(int width, int height) {
    w = width;
    h = height;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    win = glfwCreateWindow(w, h, "GL", NULL, NULL);
    glfwMakeContextCurrent(win);
    glewInit();
    is(vShader, fShader);
    ct();
    buf = new unsigned char[w * h * 3];
    return true;
}

bool GL::p(AVFrame* in, AVFrame* out, float angle) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(pgm);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, in->data[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_RED, GL_UNSIGNED_BYTE, in->data[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_RED, GL_UNSIGNED_BYTE, in->data[2]);
    
    float rad = angle * 3.14159f / 180.0f;
    float c = cos(rad);
    float s = sin(rad);
    float tr[16] = {
        c,    -s,    0.0f, 0.0f,
        s,     c,    0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    };
    
    GLint loc = glGetUniformLocation(pgm, "transform");
    glUniformMatrix4fv(loc, 1, GL_FALSE, tr);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFinish();
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);
    
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            unsigned char r = buf[idx];
            unsigned char g = buf[idx+1];
            unsigned char b = buf[idx+2];
            out->data[0][y * out->linesize[0] + x] = (unsigned char)(0.2126f * r + 0.7152f * g + 0.0722f * b);
            if(y % 2 == 0 && x % 2 == 0) {
                int u_idx = (y/2) * out->linesize[1] + (x/2);
                int v_idx = (y/2) * out->linesize[2] + (x/2);
                out->data[1][u_idx] = (unsigned char)(-0.09991f * r - 0.33609f * g + 0.436f * b + 128);
                out->data[2][v_idx] = (unsigned char)(0.615f * r - 0.55861f * g - 0.05639f * b + 128);
            }
        }
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void GL::c() {
    if(buf) delete[] buf;
    if(rbo) glDeleteRenderbuffers(1, &rbo);
    if(fbo) glDeleteFramebuffers(1, &fbo);
    for(int i = 0; i < 3; i++) {
        if(tex[i]) glDeleteTextures(1, &tex[i]);
    }
    if(vbo) glDeleteBuffers(1, &vbo);
    if(vao) glDeleteVertexArrays(1, &vao);
    if(pgm) glDeleteProgram(pgm);
    if(win) {
        glfwDestroyWindow(win);
        glfwTerminate();
    }
}

bool GL::is(const char* v, const char* f) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &v, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &f, NULL);
    glCompileShader(fs);
    pgm = glCreateProgram();
    glAttachShader(pgm, vs);
    glAttachShader(pgm, fs);
    glLinkProgram(pgm);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

bool GL::ct() {
    float vertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    GL_CHECK(glGenVertexArrays(1, &vao));
    GL_CHECK(glGenBuffers(1, &vbo));
    GL_CHECK(glBindVertexArray(vao));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glGenTextures(3, tex));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex[0]));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex[1]));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w/2, h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex[2]));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w/2, h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL));
    GL_CHECK(glGenFramebuffers(1, &fbo));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    GL_CHECK(glGenRenderbuffers(1, &rbo));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, w, h));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo));
    GL_CHECK(glUseProgram(pgm));
    GL_CHECK(glUniform1i(glGetUniformLocation(pgm, "texY"), 0));
    GL_CHECK(glUniform1i(glGetUniformLocation(pgm, "texU"), 1));
    GL_CHECK(glUniform1i(glGetUniformLocation(pgm, "texV"), 2));
    return true;
} 