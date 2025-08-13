//// main.cpp - Complete with all original geometry and particles
//#include <iostream>
//#include <vector>
//#include <array>
//#include <cmath>
//#include <memory>
////#include <bits/stdc++.h>
//#include <SDL3/SDL.h>
//#include <SDL3/SDL_main.h>
//#include "particlesystem.cpp"
//#include "renderer2d.cpp"
//
//static constexpr int WINDOW_WIDTH = 1000;
//static constexpr int WINDOW_HEIGHT = 700;
//static constexpr float PI = 3.14159265f;
//static constexpr float TWO_PI = 2.0f * PI;
//
//int main(int argc, char* argv[]) {
//    (void)argc; (void)argv;
//
//    SDL_SetAppMetadata("Geometry & Particles Demo", "1.0", "com.example.particles");
//
//    if (!SDL_Init(SDL_INIT_VIDEO)) {
//        SDL_Log("SDL_Init failed: %s", SDL_GetError());
//        return -1;
//    }
//
//    SDL_Window* window = SDL_CreateWindow(
//        "Beautiful Geometry with Particle Effects",
//        WINDOW_WIDTH, WINDOW_HEIGHT,
//        SDL_WINDOW_RESIZABLE
//    );
//
//    if (!window) {
//        SDL_Log("Window creation failed: %s", SDL_GetError());
//        SDL_Quit();
//        return -1;
//    }
//
//    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);
//    if (!sdl_renderer) {
//        SDL_Log("Renderer creation failed: %s", SDL_GetError());
//        SDL_DestroyWindow(window);
//        SDL_Quit();
//        return -1;
//    }
//
//    // Create draw object
//    Draw draw(sdl_renderer);
//
//    // Create particle emitters (no pointers, just stack objects)
//    FireEmitter fire(150, 600);
//    WaterEmitter water(850, 600);
//    LightningEmitter lightning(500, 100, 500, 400);
//    SparkleEmitter sparkle(500, 300);
//    SmokeEmitter smoke(150, 550);
//    ExplosionEmitter explosion(750, 300);
//
//    // Pre-allocate buffers
//    std::vector<SDL_FPoint> wave_points;
//    wave_points.reserve(60);
//    std::vector<SDL_FPoint> star_points;
//    star_points.reserve(10);
//    std::array<SDL_FRect, 16> bars;
//
//    bool running = true;
//    int frame_count = 0;
//
//    while (running) {
//        SDL_Event event;
//        while (SDL_PollEvent(&event)) {
//            if (event.type == SDL_EVENT_QUIT) {
//                running = false;
//            }
//            else if (event.type == SDL_EVENT_KEY_DOWN) {
//                if (event.key.key == SDLK_ESCAPE) {
//                    running = false;
//                }
//                else if (event.key.key == SDLK_SPACE) {
//                    explosion.reset(750 + (rand() % 200 - 100), 300 + (rand() % 100 - 50));
//                    explosion.explode();
//                }
//            }
//            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
//                sparkle.set_position(event.motion.x, event.motion.y);
//            }
//            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
//                lightning.set_target(event.button.x, event.button.y);
//            }
//        }
//
//        frame_count++;
//
//        // Animation timing
//        const Uint64 now = SDL_GetTicks();
//        const float t = static_cast<float>((now % 5000) / 5000.0);
//        const float pulse = 0.5f + 0.5f * std::sin(TWO_PI * t);
//        const float angle = TWO_PI * t;
//
//        // Clear with dark background
//        draw.color(24, 28, 38);
//        draw.clear();
//
//        // === ALL ORIGINAL GEOMETRY ===
//
//        // White line and point
//        draw.color(255, 255, 255);
//        draw.line(40, 40, 260, 120);
//        draw.point(280, 60);
//
//        // Orange rect outline (pulsing)
//        draw.color(220, 160, 60);
//        draw.rect(40, 160, 140 + 60 * pulse, 90 + 40 * pulse);
//
//        // Purple filled rect (pulsing)
//        draw.color(180, 80, 140);
//        draw.fill_rect(220, 160, 140 + 40 * pulse, 90);
//
//        // Bar graph
//        for (int i = 0; i < 16; ++i) {
//            float w = WINDOW_WIDTH / 16.0f;
//            float h = (i + 1) * 6 * (0.5f + 0.5f * pulse);
//            bars[i] = { i * w, WINDOW_HEIGHT - h - 10, w - 2, h };
//        }
//        draw.color(240, 240, 240);
//        draw.fill_rects(bars.data(), 16);
//
//        // Pentagon (filled + outline)
//        std::vector<SDL_FPoint> pentagon = {
//            {600, 120}, {680, 160}, {660, 240}, {540, 240}, {520, 160}
//        };
//        draw.fill_polygon(pentagon, 80, 160, 220, 220);
//        draw.color(255, 255, 255);
//        draw.polygon(pentagon);
//
//        // Triangle (filled + outline)
//        std::vector<SDL_FPoint> triangle = {
//            {420, 360}, {520, 300}, {620, 380}
//        };
//        draw.fill_polygon(triangle, 200, 200, 80);
//        draw.color(255, 255, 255);
//        draw.polygon(triangle);
//
//        // Circle outline
//        draw.color(220, 180, 60);
//        draw.circle(150, 420, 60);
//
//        // Filled circle (pulsing)
//        draw.color(100, 200, 120);
//        int fill_radius = 40 + static_cast<int>(30 * pulse);
//        draw.fill_circle(320, 500, fill_radius);
//
//        // Ellipse
//        draw.color(255, 160, 200);
//        draw.ellipse(720, 520, 120, 60, 64);
//
//        // Rotating rectangle
//        float cx = 150, cy = 220;
//        float hw = (90 + 30 * pulse) * 0.5f;
//        float hh = (60 + 20 * pulse) * 0.5f;
//        float s = std::sin(angle), c = std::cos(angle);
//
//        std::vector<SDL_FPoint> rot_rect = {
//            {cx + (-hw * c - -hh * s), cy + (-hw * s + -hh * c)},
//            {cx + (hw * c - -hh * s), cy + (hw * s + -hh * c)},
//            {cx + (hw * c - hh * s), cy + (hw * s + hh * c)},
//            {cx + (-hw * c - hh * s), cy + (-hw * s + hh * c)}
//        };
//        draw.color(180, 220, 255);
//        draw.polygon(rot_rect);
//
//        // Animated wave
//        wave_points.clear();
//        for (int x = 0; x < 300; x += 5) {
//            float y = 600 + 30 * std::sin(x * 0.02f + angle);
//            wave_points.push_back({ static_cast<float>(x + 50), y });
//        }
//        draw.color(150, 255, 150);
//        draw.lines(wave_points);
//
//        // Star polygon
//        star_points.clear();
//        for (int i = 0; i < 10; ++i) {
//            float star_angle = (i / 10.0f) * TWO_PI - PI / 2;
//            float radius = (i % 2 == 0) ? 60 : 25;
//            star_points.push_back({
//                850 + radius * std::cos(star_angle),
//                350 + radius * std::sin(star_angle)
//                });
//        }
//        draw.fill_polygon(star_points, 255, 215, 0, 200);
//        draw.color(255, 255, 255);
//        draw.polygon(star_points);
//
//        // Nested rectangles
//        draw.color(100, 150, 255, 180);
//        for (int i = 0; i < 5; ++i) {
//            float offset = i * 10.0f;
//            draw.rect(700 + offset, 350 + offset, 120 - offset * 2, 80 - offset * 2);
//        }
//
//        // === PARTICLE EFFECTS ===
//
//        // Update all emitters
//        fire.update();
//        water.update();
//        lightning.update();
//        sparkle.update();
//        smoke.update();
//        explosion.update();
//
//        // Move fire in a circle
//        fire.set_position(150 + 30 * std::cos(angle * 2), 600 + 20 * std::sin(angle * 2));
//
//        // Periodic explosion
//        if (frame_count % 180 == 0) {
//            explosion.reset(750 + (rand() % 200 - 100), 300 + (rand() % 100 - 50));
//            explosion.explode();
//        }
//
//        // Render particles in order
//        smoke.render(draw);
//        explosion.render(draw);
//        fire.render(draw);
//        water.render(draw);
//        sparkle.render(draw);
//        lightning.render(draw);
//
//        // === ADDITIONAL EFFECTS ===
//
//        // Animated points in a circle
//        draw.color(255, 255, 100, 200);
//        for (int i = 0; i < 12; ++i) {
//            float point_angle = (i / 12.0f) * TWO_PI + angle;
//            float px = 850 + 80 * std::cos(point_angle);
//            float py = 150 + 80 * std::sin(point_angle);
//            draw.point(px, py);
//        }
//
//        // Particle count indicators
//        draw.color(0, 0, 0, 180);
//        draw.fill_rect(10, 10, 250, 60);
//        draw.color(255, 255, 255);
//        draw.rect(10, 10, 250, 60);
//
//        // Fire particles indicator
//        draw.color(255, 100, 50);
//        for (size_t i = 0; i < std::min(fire.count() / 10, size_t(20)); ++i) {
//            draw.point(15 + i * 3, 25);
//        }
//
//        // Water particles indicator
//        draw.color(100, 150, 255);
//        for (size_t i = 0; i < std::min(water.count() / 10, size_t(20)); ++i) {
//            draw.point(15 + i * 3, 40);
//        }
//
//        // Lightning particles indicator
//        draw.color(200, 200, 255);
//        for (size_t i = 0; i < std::min(lightning.count() / 5, size_t(20)); ++i) {
//            draw.point(15 + i * 3, 55);
//        }
//
//        // Present frame
//        draw.present();
//        SDL_Delay(16);
//    }
//
//    // Cleanup
//    SDL_DestroyRenderer(sdl_renderer);
//    SDL_DestroyWindow(window);
//    SDL_Quit();
//
//    return 0;
//}

