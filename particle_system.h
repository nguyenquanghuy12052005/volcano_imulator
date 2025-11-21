
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>

// Sửa lại để tránh xung đột với main.cpp
struct ParticleVec3 { 
    float x = 0.0f; 
    float y = 0.0f; 
    float z = 0.0f; 
    
    ParticleVec3() = default;
    ParticleVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Particle {
    ParticleVec3 pos;
    ParticleVec3 vel;
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 4.0f;
    float r = 1, g = 1, b = 1, a = 1;
    bool alive = false;
};

class ParticleSystem {
public:
    void init();
    void update(float dt, float volcanoX, float volcanoY, float volcanoZ);
    void render();
    void render(const float* transformMatrix); // Thêm phương thức mới
    void handleInput(int key);

    bool emitting = true;
    int baseEmitRate = 300;
    float eruptionPower = 1.0f;
    float globalSizeMul = 1.0f;

private:
    std::vector<Particle> lavaParticles;
    std::vector<Particle> smokeParticles;
    
    const int MAX_PARTICLES = 3000;
    const int MAX_SMOKE = 1500;
    
    void emitLava(Particle &p, float volcanoX, float volcanoY, float volcanoZ);
    void emitSmoke(Particle &p, float volcanoX, float volcanoY, float volcanoZ);
    float randFloat(float a, float b);
};

extern ParticleSystem particleSystem;

#endif
