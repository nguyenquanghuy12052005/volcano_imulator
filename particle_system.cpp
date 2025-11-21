
#include "particle_system.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <random>
#include <iostream>
#include <cmath>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// RNG
mt19937 g_rng(random_device{}());

float ParticleSystem::randFloat(float a, float b) {
    uniform_real_distribution<float> d(a, b);
    return d(g_rng);
}

void ParticleSystem::init() {
    lavaParticles.resize(MAX_PARTICLES);
    smokeParticles.resize(MAX_SMOKE);
    cout << "Particle system initialized" << endl;
}

void ParticleSystem::emitLava(Particle &p, float volcanoX, float volcanoY, float volcanoZ) {
    p.alive = true;
    
    // Phun từ miệng núi lửa - điều chỉnh tọa độ cho phù hợp với núi lửa 3D
    float angle = randFloat(0, 2 * M_PI);
    float radius = randFloat(0, 0.2f); // Miệng núi nhỏ
    p.pos.x = volcanoX + radius * cos(angle);
    p.pos.y = volcanoY;  // Đỉnh núi lửa (cao khoảng 2.5)
    p.pos.z = volcanoZ + radius * sin(angle);

    // Vận tốc: bay lên và tỏa ra xung quanh
    float speed = randFloat(3.0f, 8.0f) * eruptionPower;
    float verticalAngle = randFloat(M_PI * 0.1f, M_PI * 0.4f); // Góc bay lên
    
    p.vel.x = cos(angle) * sin(verticalAngle) * speed;
    p.vel.y = cos(verticalAngle) * speed; // Bay lên
    p.vel.z = sin(angle) * sin(verticalAngle) * speed;

    p.maxLife = randFloat(2.0f, 4.0f);
    p.life = p.maxLife;
    p.size = randFloat(0.1f, 0.3f) * globalSizeMul;  // Kích thước nhỏ hơn cho 3D

    // Màu dung nham
    p.r = 1.0f; p.g = 0.3f; p.b = 0.0f; p.a = 1.0f;
}

void ParticleSystem::emitSmoke(Particle &p, float volcanoX, float volcanoY, float volcanoZ) {
    p.alive = true;

    float angle = randFloat(0, 2 * M_PI);
    float radius = randFloat(0, 0.3f);
    p.pos.x = volcanoX + radius * cos(angle);
    p.pos.y = volcanoY + 0.1f;  // Trên miệng núi một chút
    p.pos.z = volcanoZ + radius * sin(angle);

    // Khói bay thẳng lên với độ ngẫu nhiên nhỏ
    p.vel.x = randFloat(-0.2f, 0.2f);
    p.vel.y = randFloat(1.0f, 3.0f) + 1.0f * eruptionPower;
    p.vel.z = randFloat(-0.2f, 0.2f);

    p.maxLife = randFloat(3.0f, 6.0f);
    p.life = p.maxLife;
    p.size = randFloat(0.2f, 0.5f) * (0.8f + 0.4f * eruptionPower / 2.0f);

    p.r = 0.3f; p.g = 0.3f; p.b = 0.3f; p.a = 0.6f;
}

void ParticleSystem::update(float dt, float volcanoX, float volcanoY, float volcanoZ) {
    // LAVA EMISSION
    static float emitAcc = 0.0f;
    int emitRate = int(baseEmitRate * eruptionPower);

    if (emitting) {
        emitAcc += emitRate * dt;
        int toEmit = (int)emitAcc;
        emitAcc -= toEmit;

        for (auto &p : lavaParticles) {
            if (toEmit <= 0) break;
            if (!p.alive) { 
                emitLava(p, volcanoX, volcanoY, volcanoZ); 
                --toEmit; 
            }
        }
    }

    // SMOKE EMISSION - luôn phun khói từ miệng núi
    static float smokeAcc = 0.0f;
    float smokeRate = 100.0f * eruptionPower;
    smokeAcc += smokeRate * dt;

    int toSmoke = (int)smokeAcc;
    smokeAcc -= toSmoke;

    for (int i = 0; i < toSmoke; i++) {
        for (auto &s : smokeParticles) {
            if (!s.alive) { 
                emitSmoke(s, volcanoX, volcanoY, volcanoZ); 
                break; 
            }
        }
    }

    // UPDATE LAVA
    float gravity = -8.0f;  // Giảm trọng lực cho 3D
    for (auto &p : lavaParticles) {
        if (!p.alive) continue;

        p.life -= dt;
        if (p.life <= 0.0f) { p.alive = false; continue; }

        p.vel.y += gravity * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;

        // Va chạm với mặt đất
        float ground = -0.5f;
        if (p.pos.y < ground) {
            p.pos.y = ground;
            p.vel.y *= -0.2f;  // Giảm độ nảy
            p.vel.x *= 0.3f;
            p.vel.z *= 0.3f;
            p.life -= 1.0f * dt;  // Chết nhanh hơn khi chạm đất
            
            // Tạo khói khi chạm đất
            for (auto &s : smokeParticles) {
                if (!s.alive) {
                    s.alive = true;
                    s.pos.x = p.pos.x;
                    s.pos.y = ground + 0.1f;
                    s.pos.z = p.pos.z;
                    s.vel.x = randFloat(-0.2f, 0.2f);
                    s.vel.y = randFloat(0.5f, 1.5f);
                    s.vel.z = randFloat(-0.2f, 0.2f);
                    s.maxLife = randFloat(1.0f, 2.0f);
                    s.life = s.maxLife;
                    s.size = randFloat(0.1f, 0.3f);
                    s.r = 0.2f; s.g = 0.2f; s.b = 0.2f; s.a = 0.4f;
                    break;
                }
            }
        }
    }

    // UPDATE SMOKE
    for (auto &s : smokeParticles) {
        if (!s.alive) continue;

        s.life -= dt;
        if (s.life <= 0.0f) { s.alive = false; continue; }

        s.vel.y += 0.5f * dt;  // Khói bay lên
        s.pos.x += s.vel.x * dt;
        s.pos.y += s.vel.y * dt;
        s.pos.z += s.vel.z * dt;

        s.vel.x *= (1.0f - 0.5f * dt);
        s.vel.z *= (1.0f - 0.5f * dt);

        float lifeRatio = s.life / s.maxLife;
        s.a = 0.4f * lifeRatio;
        s.size *= (1.0f + 0.1f * dt);  // Khói phình to
    }
}

// Shaders cho hệ thống hạt 3D
const char* particleVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aSize;
layout(location = 2) in vec4 aColor;

uniform mat4 uTransform;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uTransform * vec4(aPos, 1.0);
    gl_PointSize = aSize * 50.0; // Scale điểm cho phù hợp
}
)";

const char* particleFragmentShaderSrc = R"(
#version 330 core
in vec4 vColor;

out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = length(coord);
    if (dist > 1.0) discard;

    float alpha = vColor.a * smoothstep(1.0, 0.6, dist);
    FragColor = vec4(vColor.rgb, alpha);
}
)";

GLuint compileParticleShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, nullptr, log);
        cerr << "Particle Shader error: " << log << endl;
    }
    return s;
}

GLuint particleShaderProgram = 0;
GLuint particleVAO = 0, particleVBO = 0;

void ParticleSystem::render(const float* transformMatrix) {
    if (particleShaderProgram == 0) {
        // Khởi tạo shader và buffer lần đầu
        GLuint vs = compileParticleShader(GL_VERTEX_SHADER, particleVertexShaderSrc);
        GLuint fs = compileParticleShader(GL_FRAGMENT_SHADER, particleFragmentShaderSrc);

        particleShaderProgram = glCreateProgram();
        glAttachShader(particleShaderProgram, vs);
        glAttachShader(particleShaderProgram, fs);
        glLinkProgram(particleShaderProgram);
        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);

        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

        // Cấu trúc: pos3 + size1 + color4 = 8 floats
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));

        glBindVertexArray(0);
    }

    // Chuẩn bị dữ liệu hạt
    std::vector<float> particleData;
    
    // Thêm dung nham
    for (const auto &p : lavaParticles) {
        if (!p.alive) continue;
        particleData.insert(particleData.end(), {p.pos.x, p.pos.y, p.pos.z});
        particleData.push_back(p.size);
        particleData.insert(particleData.end(), {p.r, p.g, p.b, p.a});
    }
    
    // Thêm khói
    for (const auto &s : smokeParticles) {
        if (!s.alive) continue;
        particleData.insert(particleData.end(), {s.pos.x, s.pos.y, s.pos.z});
        particleData.push_back(s.size);
        particleData.insert(particleData.end(), {s.r, s.g, s.b, s.a});
    }

    if (particleData.empty()) return;

    // Render
    glUseProgram(particleShaderProgram);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, particleData.size() * sizeof(float), particleData.data(), GL_STREAM_DRAW);

    // Sử dụng ma trận transform được truyền vào
    GLint transformLoc = glGetUniformLocation(particleShaderProgram, "uTransform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_POINTS, 0, particleData.size() / 8);
    
    glBindVertexArray(0);
}

// Giữ nguyên phương thức render cũ để tương thích
void ParticleSystem::render() {
    // Tạo ma trận identity mặc định
    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    render(identity);
}

void ParticleSystem::handleInput(int key) {
    if (key == GLFW_KEY_SPACE) {
        emitting = !emitting;
        cout << "Particle Emitting: " << (emitting ? "ON" : "OFF") << endl;
    }
    else if (key == GLFW_KEY_C) {
        for (auto &p : lavaParticles) p.alive = false;
        for (auto &s : smokeParticles) s.alive = false;
        cout << "Particles Cleared\n";
    }
    else if (key == GLFW_KEY_EQUAL) {
        baseEmitRate = min(baseEmitRate + 50, 5000);
        cout << "EmitRate: " << baseEmitRate << endl;
    }
    else if (key == GLFW_KEY_MINUS) {
        baseEmitRate = max(baseEmitRate - 50, 0);
        cout << "EmitRate: " << baseEmitRate << endl;
    }
    else if (key == GLFW_KEY_LEFT_BRACKET) {
        eruptionPower = max(0.1f, eruptionPower - 0.1f);
        cout << "EruptionPower: " << eruptionPower << endl;
    }
    else if (key == GLFW_KEY_RIGHT_BRACKET) {
        eruptionPower = min(5.0f, eruptionPower + 0.1f);
        cout << "EruptionPower: " << eruptionPower << endl;
    }
}

ParticleSystem particleSystem;
