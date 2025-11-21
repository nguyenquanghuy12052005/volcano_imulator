// particle_system.cpp
// GLFW + GLAD + modern OpenGL particle system (volcano-like) + smoke + controls
// Build example:
// clang++ particle_system.cpp glad.c -std=c++17 -lglfw \
// -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o app

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <algorithm>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- CONFIG ---
const int MAX_PARTICLES = 3000;   // lava particles
const int MAX_SMOKE = 1500;       // smoke particles
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// --- Particle struct (CPU-side)
struct Vec2 { float x=0.0f, y=0.0f; };

struct Particle {
    Vec2 pos;
    Vec2 vel;
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 4.0f;
    float r=1, g=1, b=1, a=1;
    bool alive = false;
};

// CPU data
vector<Particle> lavaParticles;
vector<Particle> smokeParticles;

bool emitting = true;
int baseEmitRate = 300;     // base emit rate for lava
float eruptionPower = 1.0f; // 0.1 .. 5.0
float globalSizeMul = 1.0f; // multiplier for particle size
float gravity = -550.0f;

// RNG
mt19937 g_rng(random_device{}());
float randFloat(float a, float b) {
    uniform_real_distribution<float> d(a,b);
    return d(g_rng);
}

// ======================= EMIT LAVA =======================
void emitLava(Particle &p) {
    p.alive = true;
    p.pos.x = randFloat(-20.0f, 20.0f);
    p.pos.y = -WINDOW_HEIGHT/2.0f + 20.0f;

    float speed = randFloat(450.0f, 800.0f) * eruptionPower;
    float angle = randFloat(M_PI*0.4f, M_PI*0.6f);

    p.vel.x = cos(angle)*speed;
    p.vel.y = sin(angle)*speed;

    p.maxLife = randFloat(2.5f, 4.5f);
    p.life = p.maxLife;
    p.size = randFloat(4.0f, 9.0f) * globalSizeMul;

    p.r = 1.0f; p.g = 1.0f; p.b = 0.5f; p.a = 1.0f;
}

// ======================= EMIT SMOKE =======================
void emitSmoke(Particle &p, float x = 0.0f, float y = -WINDOW_HEIGHT/2.0f + 30.0f) {
    p.alive = true;

    p.pos.x = x + randFloat(-25.0f, 25.0f);
    p.pos.y = y + randFloat(0.0f, 10.0f);

    p.vel.x = randFloat(-10.0f, 10.0f);
    p.vel.y = randFloat(30.0f, 80.0f) + 20.0f * eruptionPower;

    p.maxLife = randFloat(3.0f, 6.0f);
    p.life = p.maxLife;

    p.size = randFloat(12.0f, 36.0f) * (0.8f + 0.4f * eruptionPower/2.0f);

    p.r = 0.3f; p.g = 0.3f; p.b = 0.3f; p.a = 0.6f;
}

// Init particle arrays
void initParticles() {
    lavaParticles.resize(MAX_PARTICLES);
    smokeParticles.resize(MAX_SMOKE);
}

// ======================= UPDATE PHYSICS =======================
void updateParticles(float dt) {
    // LAVA EMISSION
    static float emitAcc = 0.0f;
    int emitRate = int(baseEmitRate * eruptionPower);

    if (emitting) {
        emitAcc += emitRate * dt;
        int toEmit = (int)emitAcc;
        emitAcc -= toEmit;

        for (auto &p : lavaParticles) {
            if (toEmit <= 0) break;
            if (!p.alive) { emitLava(p); --toEmit; }
        }
    }

    // SMOKE EMISSION
    static float smokeAcc = 0.0f;
    float smokeRate = 50.0f * eruptionPower;
    smokeAcc += smokeRate * dt;

    int toSmoke = (int)smokeAcc;
    smokeAcc -= toSmoke;

    for (int i=0; i<toSmoke; i++) {
        for (auto &s : smokeParticles) {
            if (!s.alive) { emitSmoke(s); break; }
        }
    }

    // UPDATE LAVA
    for (auto &p : lavaParticles) {
        if (!p.alive) continue;

        p.life -= dt;
        if (p.life <= 0.0f) { p.alive = false; continue; }

        p.vel.y += gravity * dt;

        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;

        float ground = -WINDOW_HEIGHT/2.0f;
        if (p.pos.y < ground) {
            p.pos.y = ground;
            p.vel.y *= -0.5f;
            p.vel.x *= 0.7f;
            p.life -= 0.5f * dt;

            int spawned = 0;
            for (auto &s : smokeParticles) {
                if (!s.alive && spawned < 3) {
                    emitSmoke(s, p.pos.x, ground + 2.0f);
                    ++spawned;
                }
            }
        }
    }

    // UPDATE SMOKE
    for (auto &s : smokeParticles) {
        if (!s.alive) continue;

        s.life -= dt;
        if (s.life <= 0.0f) { s.alive = false; continue; }

        s.vel.y += 5.0f * dt;
        s.pos.x += s.vel.x * dt;
        s.pos.y += s.vel.y * dt;

        s.vel.x *= (1.0f - 0.3f*dt);

        float lifeRatio = s.life / s.maxLife;
        s.a = 0.25f * lifeRatio;
        s.size *= (1.0f + 0.02f * dt);
    }
}

// ======================= SHADERS =======================
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in float aSize;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aLifeRatio;

uniform mat4 uProj;

out vec4 vColor;
out float vLife;

void main() {
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    gl_PointSize = aSize;
    vColor = aColor;
    vLife = aLifeRatio;
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec4 vColor;
in float vLife;

out vec4 FragColor;

void main() {
    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float dist = length(c);
    if (dist > 1.0) discard;

    float alpha = vColor.a * clamp(vLife * 1.25, 0.0, 1.0);
    alpha *= smoothstep(1.0, 0.6, dist);

    FragColor = vec4(vColor.rgb, alpha);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, nullptr, log);
        cerr << "Shader error: " << log << endl;
    }
    return s;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, 2048, nullptr, log);
        cerr << "Link error: " << log << endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return p;
}

// Ortho matrix
void orthoMat(float l, float r, float b, float t, float n, float f, float out[16]) {
    for (int i=0; i<16; i++) out[i] = 0.0f;

    out[0] = 2.0f / (r - l);
    out[5] = 2.0f / (t - b);
    out[10] = -2.0f/(f - n);
    out[12] = -(r + l)/(r - l);
    out[13] = -(t + b)/(t - b);
    out[14] = -(f + n)/(f - n);
    out[15] = 1.0f;
}

// ======================= MAIN =======================
int main() {
    if (!glfwInit()) {
        cerr << "glfwInit failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        "Volcano + Smoke (OpenGL Particle System)", nullptr, nullptr
    );

    if (!win) {
        cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(win);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Failed to load GL via GLAD\n";
        return -1;
    }

    cout << "GL Version: " << glGetString(GL_VERSION) << endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    GLuint prog = createProgram();
    glUseProgram(prog);

    // === VAO / VBO ===
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    constexpr int ELEM = 8; // (pos2 + size1 + color4 + life1)
    int count = MAX_PARTICLES + MAX_SMOKE;

    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*ELEM*count, nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, ELEM*sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, ELEM*sizeof(float), (void*)(2*sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, ELEM*sizeof(float), (void*)(3*sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, ELEM*sizeof(float), (void*)(7*sizeof(float)));

    glBindVertexArray(0);

    // Projection
    float proj[16];
    orthoMat(-WINDOW_WIDTH/2, WINDOW_WIDTH/2, -WINDOW_HEIGHT/2, WINDOW_HEIGHT/2, -1, 1, proj);

    GLint locProj = glGetUniformLocation(prog, "uProj");
    glUseProgram(prog);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, proj);

    initParticles();

    // KEY EVENTS
    glfwSetKeyCallback(win, [](GLFWwindow* w, int key, int sc, int action, int mods){
        if (action != GLFW_PRESS) return;

        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);

        if (key == GLFW_KEY_SPACE) {
            emitting = !emitting;
            cout << "Emitting: " << (emitting ? "ON" : "OFF") << endl;
        }

        if (key == GLFW_KEY_C) {
            for (auto &p : lavaParticles) p.alive = false;
            for (auto &s : smokeParticles) s.alive = false;
            cout << "Cleared\n";
        }

        if (key == GLFW_KEY_EQUAL) {
            baseEmitRate = min(baseEmitRate+50, 5000);
            cout << "EmitRate: " << baseEmitRate << endl;
        }

        if (key == GLFW_KEY_MINUS) {
            baseEmitRate = max(baseEmitRate-50, 0);
            cout << "EmitRate: " << baseEmitRate << endl;
        }

        if (key == GLFW_KEY_LEFT_BRACKET) {
            eruptionPower = max(0.1f, eruptionPower - 0.1f);
            cout << "EruptionPower: " << eruptionPower << endl;
        }

        if (key == GLFW_KEY_RIGHT_BRACKET) {
            eruptionPower = min(5.0f, eruptionPower + 0.1f);
            cout << "EruptionPower: " << eruptionPower << endl;
        }

        if (key == GLFW_KEY_K) {
            globalSizeMul = max(0.1f, globalSizeMul - 0.1f);
            cout << "SizeMul: " << globalSizeMul << endl;
        }

        if (key == GLFW_KEY_L) {
            globalSizeMul = min(5.0f, globalSizeMul + 0.1f);
            cout << "SizeMul: " << globalSizeMul << endl;
        }

        if (key == GLFW_KEY_S) {
            static bool smokeOn = true;
            smokeOn = !smokeOn;

            if (!smokeOn) {
                for (auto &s : smokeParticles) s.alive = false;
                cout << "Smoke OFF\n";
            } else cout << "Smoke ON\n";
        }
    });

    float last = glfwGetTime();

    vector<float> gpuBuf(ELEM * count);

    while (!glfwWindowShouldClose(win)) {
        float now = glfwGetTime();
        float dt = now - last;
        last = now;

        if (dt > 0.05f) dt = 0.05f;

        updateParticles(dt);

        int idx = 0;

        // LAVA
        for (auto &p : lavaParticles) {
            if (!p.alive) {
                for (int i=0;i<ELEM;i++) gpuBuf[idx++] = 0.0f;
                continue;
            }

            float lifeRatio = p.life / p.maxLife;

            float r, g, b, a;
            if (lifeRatio > 0.8f) { r=1; g=1; b=0.2; a=1; }
            else if (lifeRatio > 0.5f) { r=1; g=0.5; b=0; a=1; }
            else if (lifeRatio > 0.2f) { r=0.6; g=0.1; b=0.1; a=1; }
            else { r=0.3; g=0.3; b=0.3; a=max(0.0f, lifeRatio*3.0f); }

            gpuBuf[idx++] = p.pos.x;
            gpuBuf[idx++] = p.pos.y;
            gpuBuf[idx++] = p.size * globalSizeMul;
            gpuBuf[idx++] = r;
            gpuBuf[idx++] = g;
            gpuBuf[idx++] = b;
            gpuBuf[idx++] = a;
            gpuBuf[idx++] = lifeRatio;
        }

        // SMOKE
        for (auto &s : smokeParticles) {
            if (!s.alive) {
                for (int i=0;i<ELEM;i++) gpuBuf[idx++] = 0.0f;
                continue;
            }

            float lifeRatio = s.life / s.maxLife;

            gpuBuf[idx++] = s.pos.x;
            gpuBuf[idx++] = s.pos.y;
            gpuBuf[idx++] = s.size;
            gpuBuf[idx++] = s.r;
            gpuBuf[idx++] = s.g;
            gpuBuf[idx++] = s.b;
            gpuBuf[idx++] = s.a;
            gpuBuf[idx++] = lifeRatio;
        }

        // UPLOAD GPU
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*gpuBuf.size(), nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*gpuBuf.size(), gpuBuf.data());

        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, count);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteProgram(prog);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
