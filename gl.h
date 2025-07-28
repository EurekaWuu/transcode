#ifndef GL_H
#define GL_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
extern "C" {
#include <libavutil/frame.h>
}

class GL {
public:
    GL();
    ~GL();
    bool i(int w, int h);
    bool p(AVFrame* in, AVFrame* out, float angle);
    void c();

private:
    bool is(const char* v, const char* f);
    bool ct();
    GLFWwindow* win;
    int w;
    int h;
    GLuint pgm;
    GLuint vao;
    GLuint vbo;
    GLuint tex[3];
    GLuint fbo;
    GLuint rbo;
    unsigned char* buf;
};

#endif 