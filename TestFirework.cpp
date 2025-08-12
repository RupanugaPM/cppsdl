#include <SDL3/SDL.h>
#include <vector>
#include <random>
#include <memory>
#include <algorithm>
#include <cmath>

// The renderer helper code you provided.
// It's included directly here to make a single, compilable file.
#include "renderer2d.cpp"

// --- Constants ---
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const SDL_FPoint GRAVITY = { 0.0f, 0.2f };

// --- Utility for random numbers ---
std::random_device rd;
std::mt19937 gen(rd());

float random_float(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

int random_int(int min, int max) {
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}


// --- Particle Class ---
// Represents a single point of light in the simulation
class Particle {
public:
    Particle(float x, float y, bool isBooster, Uint8 r, Uint8 g, Uint8 b)
        : m_pos({ x, y }), m_isBooster(isBooster), m_r(r), m_g(g), m_b(b), m_lifespan(255.0f) {

        if (m_isBooster) {
            // The main firework rocket that flies up
            m_vel = { 0.0f, random_float(-18.0f, -10.0f) };
        }
        else {
            // An explosion particle with a random outward velocity
            float angle = random_float(0, 2 * 3.14159265f);
            float speed = random_float(1.0f, 10.0f);
            m_vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        }
    }

    void applyForce(const SDL_FPoint& force) {
        m_vel.x += force.x;
        m_vel.y += force.y;
    }

    void update() {
        if (!m_isBooster) {
            // Explosion particles slow down and fade
            m_vel.x *= 0.95f;
            m_vel.y *= 0.95f;
            m_lifespan -= 4.0f;
        }
        m_pos.x += m_vel.x;
        m_pos.y += m_vel.y;
    }

    void draw(Draw& drawer) const {
        // Use lifespan for alpha to fade out
        Uint8 alpha = static_cast<Uint8>(std::max(0.0f, m_lifespan));
        drawer.color(m_r, m_g, m_b, alpha);
        drawer.fill_circle(static_cast<int>(m_pos.x), static_cast<int>(m_pos.y), 2);
    }

    bool isDone() const {
        return m_lifespan <= 0;
    }

    SDL_FPoint getPos() const { return m_pos; }
    float getVelY() const { return m_vel.y; }

private:
    SDL_FPoint m_pos;
    SDL_FPoint m_vel;
    bool m_isBooster;
    Uint8 m_r, m_g, m_b;
    float m_lifespan;
};


// --- Firework Class ---
// Manages the booster and the subsequent particle explosion
class Firework {
public:
    Firework() {
        // Create a firework with a random color starting at the bottom of the screen
        m_r = random_int(50, 255);
        m_g = random_int(50, 255);
        m_b = random_int(50, 255);
        m_booster = std::make_unique<Particle>(random_float(0, SCREEN_WIDTH), SCREEN_HEIGHT, true, m_r, m_g, m_b);
    }

    void update() {
        if (!m_exploded) {
            m_booster->applyForce(GRAVITY);
            m_booster->update();
            // Explode when the booster reaches its apex (starts falling down)
            if (m_booster->getVelY() >= 0) {
                explode();
            }
        }
        else {
            for (auto& p : m_particles) {
                p.applyForce(GRAVITY);
                p.update();
            }
            // Remove particles that have faded out
            m_particles.erase(
                std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) {
                    return p.isDone();
                    }),
                m_particles.end()
            );
        }
    }

    void draw(Draw& drawer) const {
        if (!m_exploded) {
            m_booster->draw(drawer);
        }
        else {
            for (const auto& p : m_particles) {
                p.draw(drawer);
            }
        }
    }

    bool isDone() const {
        return m_exploded && m_particles.empty();
    }

private:
    void explode() {
        m_exploded = true;
        SDL_FPoint boomPos = m_booster->getPos();
        int num_particles = random_int(50, 150);
        m_particles.reserve(num_particles);
        for (int i = 0; i < num_particles; ++i) {
            m_particles.emplace_back(boomPos.x, boomPos.y, false, m_r, m_g, m_b);
        }
    }

    std::unique_ptr<Particle> m_booster;
    std::vector<Particle> m_particles;
    bool m_exploded = false;
    Uint8 m_r, m_g, m_b;
};


// --- Main Application ---
//int main(int argc, char* argv[]) {
//    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
//        return 1;
//    }
//
//    SDL_Window* window = SDL_CreateWindow("SDL3 Fireworks", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
//    if (!window) {
//        SDL_Log("Unable to create window: %s", SDL_GetError());
//        SDL_Quit();
//        return 1;
//    }
//
//    // --- **FIXED RENDERER CREATION** ---
//    // The old flags are removed in SDL3. Accelerated is default.
//    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
//    if (!renderer) {
//        SDL_Log("Unable to create renderer: %s", SDL_GetError());
//        SDL_DestroyWindow(window);
//        SDL_Quit();
//        return 1;
//    }
//
//    // VSync is now set with a separate function. (1 = enabled)
//    SDL_SetRenderVSync(renderer, 1);
//    // --- End of Fix ---
//
//    Draw draw(renderer); // Our drawing helper
//    std::vector<Firework> fireworks;
//
//    bool running = true;
//    SDL_Event event;
//
//    while (running) {
//        // --- Event Handling ---
//        while (SDL_PollEvent(&event)) {
//            if (event.type == SDL_EVENT_QUIT) {
//                running = false;
//            }
//        }
//
//        // --- Update Logic ---
//
//        // Randomly launch a new firework.
//        if (random_int(0, 100) < 4) { // Using integers can sometimes avoid compiler warnings
//            fireworks.emplace_back();
//        }
//
//        // Update all fireworks
//        for (auto& fw : fireworks) {
//            fw.update();
//        }
//
//        // Remove fireworks that are finished
//        fireworks.erase(
//            std::remove_if(fireworks.begin(), fireworks.end(), [](const Firework& fw) {
//                return fw.isDone();
//                }),
//            fireworks.end()
//        );
//
//        // --- Rendering ---
//
//        // Draw a semi-transparent background to create a trail effect
//        draw.color(0, 0, 0, 35);
//        draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
//
//        // Draw all fireworks
//        for (const auto& fw : fireworks) {
//            fw.draw(draw);
//        }
//
//        draw.present();
//        SDL_Delay(16);
//    }
//
//    // --- Cleanup ---
//    SDL_DestroyRenderer(renderer);
//    SDL_DestroyWindow(window);
//    SDL_Quit();
//
//    return 0;
//}