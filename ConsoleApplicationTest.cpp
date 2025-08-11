// main.cpp - Demo using the optimized Draw struct

#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <memory>
#include "particlesystem.cpp"
#include "renderer2d.cpp"

static constexpr int WINDOW_WIDTH = 1000;
static constexpr int WINDOW_HEIGHT = 700;
static constexpr float PI = 3.14159265f;
static constexpr float TWO_PI = 2.0f * PI;

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    SDL_SetAppMetadata("Geometry & Particles Demo", "1.0", "com.example.particles");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Beautiful Particle Effects",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);
    if (!sdl_renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Draw draw(sdl_renderer);

    // Create particle emitters
    auto fire = std::make_unique<FireEmitter>(150, 600);
    auto water = std::make_unique<WaterEmitter>(850, 600);
    auto lightning = std::make_unique<LightningEmitter>(500, 100, 500, 400);
    auto sparkle = std::make_unique<SparkleEmitter>(500, 300);
    auto smoke = std::make_unique<SmokeEmitter>(150, 550);
    auto explosion = std::make_unique<ExplosionEmitter>(750, 300);

    // Pre-allocate buffers
    std::vector<SDL_FPoint> wave_points;
    wave_points.reserve(60);
    std::array<SDL_FRect, 16> bars;

    bool running = true;
    int frame_count = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                else if (event.key.key == SDLK_SPACE) {
                    // Trigger explosion on spacebar
                    explosion->reset(750 + (rand() % 200 - 100), 300 + (rand() % 100 - 50));
                    explosion->explode();
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Move sparkle emitter with mouse
                sparkle->set_position(event.motion.x, event.motion.y);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Set lightning target to mouse position
                lightning->set_target(event.button.x, event.button.y);
            }
        }

        frame_count++;

        // Animation
        const Uint64 now = SDL_GetTicks();
        const float t = static_cast<float>((now % 5000) / 5000.0);
        const float pulse = 0.5f + 0.5f * std::sin(TWO_PI * t);
        const float angle = TWO_PI * t;

        // Clear with darker background for better particle visibility
        draw.color(10, 12, 20);
        draw.clear();

        // === Original geometry demo (dimmed for particles) ===

        // White line and point
        draw.color(100, 100, 100);
        draw.line(40, 40, 260, 120);
        draw.point(280, 60);

        // Orange rect outline
        draw.color(120, 80, 30);
        draw.rect(40, 160, 140 + 60 * pulse, 90 + 40 * pulse);

        // Purple filled rect
        draw.color(90, 40, 70);
        draw.fill_rect(220, 160, 140 + 40 * pulse, 90);

        // Bar graph
        for (int i = 0; i < 16; ++i) {
            float w = WINDOW_WIDTH / 16.0f;
            float h = (i + 1) * 6 * (0.5f + 0.5f * pulse);
            bars[i] = { i * w, WINDOW_HEIGHT - h - 10, w - 2, h };
        }
        draw.color(60, 60, 60);
        draw.fill_rects(bars.data(), 16);

        // Pentagon
        std::vector<SDL_FPoint> pentagon = {
            {600, 120}, {680, 160}, {660, 240}, {540, 240}, {520, 160}
        };
        draw.fill_polygon(pentagon, 40, 80, 110, 180);
        draw.color(100, 100, 100);
        draw.polygon(pentagon);

        // Circles
        draw.color(110, 90, 30);
        draw.circle(150, 420, 60);

        // === Update and render particles ===

        // Update all emitters
        fire->update();
        water->update();
        lightning->update();
        sparkle->update();
        smoke->update();
        explosion->update();

        // Move fire in a circle
        fire->set_position(150 + 30 * std::cos(angle), 600 + 20 * std::sin(angle));

        // Periodic explosion
        if (frame_count % 180 == 0) {
            explosion->reset(750 + (rand() % 200 - 100), 300 + (rand() % 100 - 50));
            explosion->explode();
        }

        // Render particles (order matters for visual effect)
        smoke->render(draw);      // Smoke in back
        explosion->render(draw);   // Explosion
        fire->render(draw);        // Fire
        water->render(draw);       // Water fountain
        sparkle->render(draw);     // Sparkles
        lightning->render(draw);   // Lightning on top

        // === Animated wave with glow effect ===
        wave_points.clear();
        for (int x = 0; x < 300; x += 5) {
            float y = 450 + 30 * std::sin(x * 0.02f + angle);
            wave_points.push_back({ static_cast<float>(x + 350), y });
        }

        // Glow effect for wave
        draw.color(50, 150, 50, 100);
        for (int i = 0; i < 3; ++i) {
            for (size_t j = 0; j < wave_points.size(); ++j) {
                draw.fill_circle(wave_points[j].x, wave_points[j].y + i * 2, 3 - i);
            }
        }
        draw.color(150, 255, 150);
        draw.lines(wave_points);

        // === UI text background ===
        draw.color(0, 0, 0, 180);
        draw.fill_rect(10, 10, 250, 80);

        // Info text would go here (need text rendering for actual text)
        draw.color(255, 255, 255);
        draw.rect(10, 10, 250, 80);

        // Particle count indicators (dots representing particle density)
        draw.color(255, 100, 50);
        for (size_t i = 0; i < std::min(fire->count() / 10, size_t(20)); ++i) {
            draw.point(15 + i * 3, 25);
        }

        draw.color(100, 150, 255);
        for (size_t i = 0; i < std::min(water->count() / 10, size_t(20)); ++i) {
            draw.point(15 + i * 3, 35);
        }

        draw.color(200, 200, 255);
        for (size_t i = 0; i < std::min(lightning->count() / 5, size_t(20)); ++i) {
            draw.point(15 + i * 3, 45);
        }

        // Present
        draw.present();
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}