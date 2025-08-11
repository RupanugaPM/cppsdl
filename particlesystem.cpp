// particle_system.hpp - Beautiful particle effects
#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "renderer2d.cpp"

struct Particle {
    float x, y;           // position
    float vx, vy;         // velocity
    float ax, ay;         // acceleration
    float life;           // lifetime (1.0 = born, 0.0 = dead)
    float decay;          // life decay rate
    float size;           // particle size
    float initial_size;   // starting size
    Uint8 r, g, b, a;     // color
    float fade_rate;      // alpha fade rate

    Particle(float x, float y, float vx, float vy, float life = 1.0f, float size = 3.0f)
        : x(x), y(y), vx(vx), vy(vy), ax(0), ay(0), life(life),
        decay(0.02f), size(size), initial_size(size),
        r(255), g(255), b(255), a(255), fade_rate(1.0f) {
    }

    void update(float dt = 1.0f) {
        vx += ax * dt;
        vy += ay * dt;
        x += vx * dt;
        y += vy * dt;
        life -= decay * dt;

        // Fade alpha based on life
        a = static_cast<Uint8>(255 * life * fade_rate);

        // Size can shrink with life for some effects
        size = initial_size * (0.3f + 0.7f * life);
    }

    bool is_dead() const { return life <= 0; }
};

class ParticleEmitter {
protected:
    std::vector<Particle> particles;
    std::mt19937 rng;
    std::uniform_real_distribution<float> rand_float{ 0.0f, 1.0f };
    float x, y;
    int spawn_rate;
    int spawn_counter;

public:
    bool active = true;

    ParticleEmitter(float x, float y, int spawn_rate = 5)
        : rng(std::random_device{}()), x(x), y(y), spawn_rate(spawn_rate), spawn_counter(0) {
        particles.reserve(1000);
    }

    void set_position(float nx, float ny) { x = nx; y = ny; }

    virtual void emit() = 0;
    virtual void update(float dt = 1.0f) {
        // Spawn new particles
        if (active && ++spawn_counter >= spawn_rate) {
            spawn_counter = 0;
            emit();
        }

        // Update existing particles
        for (auto& p : particles) {
            p.update(dt);
        }

        // Remove dead particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.is_dead(); }),
            particles.end()
        );
    }

    void render(Draw& draw) {
        // Sort by size for better rendering (bigger particles in back)
        std::sort(particles.begin(), particles.end(),
            [](const Particle& a, const Particle& b) { return a.size > b.size; });

        for (const auto& p : particles) {
            draw.color(p.r, p.g, p.b, p.a);
            if (p.size > 2) {
                draw.fill_circle(static_cast<int>(p.x), static_cast<int>(p.y),
                    static_cast<int>(p.size));
            }
            else {
                draw.point(p.x, p.y);
            }
        }
    }

    size_t count() const { return particles.size(); }
    void clear() { particles.clear(); }
};

// Fire effect - rises with heat, flickers
class FireEmitter : public ParticleEmitter {
public:
    FireEmitter(float x, float y) : ParticleEmitter(x, y, 2) {}

    void emit() override {
        for (int i = 0; i < 3; ++i) {
            float angle = rand_float(rng) * 6.28f;
            float speed = rand_float(rng) * 1.5f + 0.5f;
            float px = x + (rand_float(rng) - 0.5f) * 20;
            float py = y + (rand_float(rng) - 0.5f) * 10;

            Particle p(px, py,
                std::cos(angle) * speed * 0.3f,
                -2.0f - rand_float(rng) * 2.0f,  // rise up
                1.0f,
                rand_float(rng) * 4 + 2);

            p.ay = -0.15f;  // slight upward acceleration (hot air rises)
            p.decay = 0.015f + rand_float(rng) * 0.01f;

            // Fire colors: white -> yellow -> orange -> red -> dark red
            float color_phase = rand_float(rng);
            if (color_phase < 0.2f) {
                p.r = 255; p.g = 255; p.b = 200;  // white hot
            }
            else if (color_phase < 0.4f) {
                p.r = 255; p.g = 220; p.b = 100;  // yellow
            }
            else if (color_phase < 0.7f) {
                p.r = 255; p.g = 160; p.b = 40;   // orange
            }
            else {
                p.r = 220; p.g = 60; p.b = 20;    // red
            }

            particles.push_back(p);
        }
    }
};

// Water fountain - gravity affected droplets
class WaterEmitter : public ParticleEmitter {
public:
    WaterEmitter(float x, float y) : ParticleEmitter(x, y, 3) {}

    void emit() override {
        for (int i = 0; i < 5; ++i) {
            float angle = -1.57f + (rand_float(rng) - 0.5f) * 0.8f;  // mostly upward
            float speed = 4.0f + rand_float(rng) * 3.0f;

            Particle p(x + (rand_float(rng) - 0.5f) * 5, y,
                std::cos(angle) * speed,
                std::sin(angle) * speed,
                1.0f,
                rand_float(rng) * 2 + 1);

            p.ay = 0.3f;  // gravity
            p.decay = 0.008f;

            // Water colors: light blue to white
            p.r = 100 + rand_float(rng) * 100;
            p.g = 150 + rand_float(rng) * 105;
            p.b = 200 + rand_float(rng) * 55;
            p.fade_rate = 0.7f;

            particles.push_back(p);
        }
    }
};

// Lightning bolt - fast, branching electric effect
class LightningEmitter : public ParticleEmitter {
private:
    float target_x, target_y;
    int bolt_timer = 0;

public:
    LightningEmitter(float x, float y, float tx, float ty)
        : ParticleEmitter(x, y, 100), target_x(tx), target_y(ty) {
    }

    void set_target(float tx, float ty) { target_x = tx; target_y = ty; }

    void emit() override {
        if (bolt_timer > 0) return;
        bolt_timer = 20 + rand_float(rng) * 40;

        // Main bolt
        float dx = target_x - x;
        float dy = target_y - y;
        float dist = std::sqrt(dx * dx + dy * dy);
        int segments = static_cast<int>(dist / 10);

        float px = x, py = y;
        for (int i = 0; i < segments; ++i) {
            float t = (i + 1) / static_cast<float>(segments);
            float nx = x + dx * t + (rand_float(rng) - 0.5f) * 30;
            float ny = y + dy * t + (rand_float(rng) - 0.5f) * 30;

            // Create particles along the line
            for (int j = 0; j < 3; ++j) {
                float pt = j / 3.0f;
                Particle p(px + (nx - px) * pt, py + (ny - py) * pt,
                    (rand_float(rng) - 0.5f) * 0.5f,
                    (rand_float(rng) - 0.5f) * 0.5f,
                    0.5f, 2);

                p.decay = 0.1f + rand_float(rng) * 0.1f;
                p.r = 200 + rand_float(rng) * 55;
                p.g = 200 + rand_float(rng) * 55;
                p.b = 255;

                particles.push_back(p);

                // Branches
                if (rand_float(rng) < 0.3f) {
                    float branch_angle = std::atan2(ny - py, nx - px) + (rand_float(rng) - 0.5f);
                    float branch_len = 20 + rand_float(rng) * 30;

                    for (int k = 0; k < 5; ++k) {
                        float bt = k / 5.0f;
                        Particle bp(p.x + std::cos(branch_angle) * branch_len * bt,
                            p.y + std::sin(branch_angle) * branch_len * bt,
                            0, 0, 0.3f, 1);
                        bp.decay = 0.15f;
                        bp.r = 150; bp.g = 150; bp.b = 255;
                        particles.push_back(bp);
                    }
                }
            }
            px = nx; py = ny;
        }
    }

    void update(float dt = 1.0f) override {
        if (bolt_timer > 0) bolt_timer--;
        ParticleEmitter::update(dt);
    }
};

// Sparkle/magic effect
class SparkleEmitter : public ParticleEmitter {
public:
    SparkleEmitter(float x, float y) : ParticleEmitter(x, y, 5) {}

    void emit() override {
        for (int i = 0; i < 2; ++i) {
            float angle = rand_float(rng) * 6.28f;
            float speed = rand_float(rng) * 2.0f;

            Particle p(x + (rand_float(rng) - 0.5f) * 30,
                y + (rand_float(rng) - 0.5f) * 30,
                std::cos(angle) * speed,
                std::sin(angle) * speed - 0.5f,
                1.0f,
                rand_float(rng) * 3 + 1);

            p.decay = 0.02f;
            p.ay = -0.05f;

            // Rainbow colors
            float hue = rand_float(rng) * 360;
            float c = 1.0f;
            float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2) - 1));

            if (hue < 60) { p.r = 255; p.g = x * 255; p.b = 0; }
            else if (hue < 120) { p.r = x * 255; p.g = 255; p.b = 0; }
            else if (hue < 180) { p.r = 0; p.g = 255; p.b = x * 255; }
            else if (hue < 240) { p.r = 0; p.g = x * 255; p.b = 255; }
            else if (hue < 300) { p.r = x * 255; p.g = 0; p.b = 255; }
            else { p.r = 255; p.g = 0; p.b = x * 255; }

            particles.push_back(p);
        }
    }
};

// Smoke effect
class SmokeEmitter : public ParticleEmitter {
public:
    SmokeEmitter(float x, float y) : ParticleEmitter(x, y, 4) {}

    void emit() override {
        Particle p(x + (rand_float(rng) - 0.5f) * 15,
            y,
            (rand_float(rng) - 0.5f) * 0.5f,
            -0.5f - rand_float(rng) * 0.5f,
            1.0f,
            rand_float(rng) * 8 + 4);

        p.decay = 0.005f;
        p.ay = -0.02f;  // smoke rises

        Uint8 gray = 50 + rand_float(rng) * 50;
        p.r = p.g = p.b = gray;
        p.fade_rate = 0.5f;

        particles.push_back(p);
    }
};

// Explosion effect
class ExplosionEmitter : public ParticleEmitter {
private:
    bool exploded = false;

public:
    ExplosionEmitter(float x, float y) : ParticleEmitter(x, y, 1000) {}

    void explode() {
        if (exploded) return;
        exploded = true;

        // Create explosion particles
        for (int i = 0; i < 100; ++i) {
            float angle = rand_float(rng) * 6.28f;
            float speed = rand_float(rng) * 8.0f + 2.0f;

            Particle p(x, y,
                std::cos(angle) * speed,
                std::sin(angle) * speed,
                1.0f,
                rand_float(rng) * 5 + 2);

            p.decay = 0.01f + rand_float(rng) * 0.02f;

            // Explosion colors: white -> yellow -> orange -> red
            float color = rand_float(rng);
            if (color < 0.3f) {
                p.r = 255; p.g = 255; p.b = 200;
            }
            else if (color < 0.6f) {
                p.r = 255; p.g = 200; p.b = 50;
            }
            else {
                p.r = 255; p.g = 100; p.b = 0;
            }

            particles.push_back(p);
        }
    }

    void emit() override {} // Explosions don't continuously emit

    void reset(float nx, float ny) {
        x = nx; y = ny;
        exploded = false;
        particles.clear();
    }
};