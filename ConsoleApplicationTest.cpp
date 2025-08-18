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

// particle_demo.cpp - Standalone Particle System Demo
//#include <SDL3/SDL.h>
//#include <SDL3/SDL_main.h>
//#include <iostream>
//#include <vector>
//#include <string>
//#include <sstream>
//#include <iomanip>
//#include <chrono>
//#include "particle_system.hpp"
//#include "renderer2d.cpp"
//
//// Demo constants
//constexpr int SCREEN_WIDTH = 1600;
//constexpr int SCREEN_HEIGHT = 900;
//constexpr int FPS = 60;
//
//// Demo state
//class ParticleDemo {
//private:
//    SDL_Window* window;
//    SDL_Renderer* renderer;
//    Draw draw;
//    bool running;
//
//    // Particle system
//    ParticleSystem::ParticleSystemManager* particleManager;
//
//    // Current demo
//    int currentDemo;
//    std::vector<std::string> demoNames;
//    std::vector<ParticleSystem::ParticleEmitter*> activeEmitters;
//
//    // Interactive state
//    bool mousePressed;
//    float mouseX, mouseY;
//    float lastMouseX, lastMouseY;
//    bool showStats;
//    bool showHelp;
//    bool paused;
//
//    // Performance tracking
//    float frameTime;
//    float updateTime;
//    float drawTime;
//    int particleCount;
//    int frameCount;
//    float fpsTimer;
//    float currentFPS;
//
//    // Special demo states
//    float demoTimer;
//    int fireworksCount;
//    float galaxyRotation;
//    std::vector<ParticleSystem::Vec2> electricityPoints;
//    ParticleSystem::ParticleEmitter* mouseEmitter;
//    ParticleSystem::ParticleEmitter* persistentFire;
//    ParticleSystem::ParticleEmitter* rain;
//    ParticleSystem::ParticleEmitter* snow;
//
//public:
//    ParticleDemo() :
//        window(nullptr), renderer(nullptr), running(true),
//        particleManager(nullptr), currentDemo(0),
//        mousePressed(false), mouseX(0), mouseY(0),
//        lastMouseX(0), lastMouseY(0),
//        showStats(true), showHelp(false), paused(false),
//        frameTime(0), updateTime(0), drawTime(0),
//        particleCount(0), frameCount(0), fpsTimer(0),
//        currentFPS(0), demoTimer(0), fireworksCount(0),
//        galaxyRotation(0), mouseEmitter(nullptr),
//        persistentFire(nullptr), rain(nullptr), snow(nullptr) {
//
//        initializeDemoNames();
//    }
//
//    ~ParticleDemo() {
//        cleanup();
//    }
//
//    void initializeDemoNames() {
//        demoNames = {
//            "1. Fire Effect",
//            "2. Magic Particles",
//            "3. Explosion",
//            "4. Smoke",
//            "5. Sparkles",
//            "6. Rain",
//            "7. Snow",
//            "8. Lightning",
//            "9. Portal",
//            "10. Galaxy",
//            "11. Waterfall",
//            "12. Aurora Borealis",
//            "13. Poison Cloud",
//            "14. Healing Aura",
//            "15. Blood Splatter",
//            "16. Shockwave",
//            "17. Confetti",
//            "18. Fireworks",
//            "19. Cosmic Dust",
//            "20. Plasma Field",
//            "21. Mouse Trail",
//            "22. Fountain",
//            "23. Tornado",
//            "24. Matrix Rain",
//            "25. Energy Shield",
//            "26. Disintegration",
//            "27. Black Hole",
//            "28. Rainbow Wave",
//            "29. Butterfly Swarm",
//            "30. All Effects Combined"
//        };
//    }
//
//    bool init() {
//        if (!SDL_Init(SDL_INIT_VIDEO)) {
//            SDL_Log("SDL_Init failed: %s", SDL_GetError());
//            return false;
//        }
//
//        window = SDL_CreateWindow(
//            "Advanced Particle System Demo - SDL3",
//            SCREEN_WIDTH, SCREEN_HEIGHT,
//            SDL_WINDOW_RESIZABLE
//        );
//
//        if (!window) {
//            SDL_Log("Window creation failed: %s", SDL_GetError());
//            return false;
//        }
//
//        renderer = SDL_CreateRenderer(window, nullptr);
//        if (!renderer) {
//            SDL_Log("Renderer creation failed: %s", SDL_GetError());
//            return false;
//        }
//
//        SDL_SetRenderVSync(renderer, 1);
//        draw.set_renderer(renderer);
//
//        // Initialize particle system
//        particleManager = new ParticleSystem::ParticleSystemManager(renderer);
//
//        // Start with first demo
//        loadDemo(0);
//
//        return true;
//    }
//
//    void cleanup() {
//        if (particleManager) {
//            delete particleManager;
//            particleManager = nullptr;
//        }
//
//        if (renderer) {
//            SDL_DestroyRenderer(renderer);
//            renderer = nullptr;
//        }
//
//        if (window) {
//            SDL_DestroyWindow(window);
//            window = nullptr;
//        }
//
//        SDL_Quit();
//    }
//
//    void loadDemo(int index) {
//        using namespace ParticleSystem;
//
//        // Clear existing emitters
//        particleManager->clear();
//        activeEmitters.clear();
//        mouseEmitter = nullptr;
//        persistentFire = nullptr;
//        rain = nullptr;
//        snow = nullptr;
//        electricityPoints.clear();
//
//        currentDemo = index;
//        demoTimer = 0;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        switch (index) {
//        case 0: // Fire
//            createFireDemo();
//            break;
//        case 1: // Magic
//            createMagicDemo();
//            break;
//        case 2: // Explosion
//            createExplosionDemo();
//            break;
//        case 3: // Smoke
//            createSmokeDemo();
//            break;
//        case 4: // Sparkles
//            createSparklesDemo();
//            break;
//        case 5: // Rain
//            createRainDemo();
//            break;
//        case 6: // Snow
//            createSnowDemo();
//            break;
//        case 7: // Lightning
//            createLightningDemo();
//            break;
//        case 8: // Portal
//            createPortalDemo();
//            break;
//        case 9: // Galaxy
//            createGalaxyDemo();
//            break;
//        case 10: // Waterfall
//            createWaterfallDemo();
//            break;
//        case 11: // Aurora
//            createAuroraDemo();
//            break;
//        case 12: // Poison
//            createPoisonDemo();
//            break;
//        case 13: // Healing
//            createHealingDemo();
//            break;
//        case 14: // Blood
//            createBloodDemo();
//            break;
//        case 15: // Shockwave
//            createShockwaveDemo();
//            break;
//        case 16: // Confetti
//            createConfettiDemo();
//            break;
//        case 17: // Fireworks
//            createFireworksDemo();
//            break;
//        case 18: // Cosmic Dust
//            createCosmicDustDemo();
//            break;
//        case 19: // Plasma
//            createPlasmaDemo();
//            break;
//        case 20: // Mouse Trail
//            createMouseTrailDemo();
//            break;
//        case 21: // Fountain
//            createFountainDemo();
//            break;
//        case 22: // Tornado
//            createTornadoDemo();
//            break;
//        case 23: // Matrix
//            createMatrixRainDemo();
//            break;
//        case 24: // Energy Shield
//            createEnergyShieldDemo();
//            break;
//        case 25: // Disintegration
//            createDisintegrationDemo();
//            break;
//        case 26: // Black Hole
//            createBlackHoleDemo();
//            break;
//        case 27: // Rainbow
//            createRainbowWaveDemo();
//            break;
//        case 28: // Butterflies
//            createButterflySwarmDemo();
//            break;
//        case 29: // All Combined
//            createCombinedDemo();
//            break;
//        default:
//            createFireDemo();
//            break;
//        }
//    }
//
//    // Demo creation functions
//    void createFireDemo() {
//        using namespace ParticleSystem;
//
//        // Multiple fire sources
//        for (int i = 0; i < 5; ++i) {
//            float x = SCREEN_WIDTH * (0.2f + i * 0.15f);
//            float y = SCREEN_HEIGHT * 0.8f;
//
//            auto fire = particleManager->createFireEffect({ x, y });
//            fire->emissionRate = 100;
//            fire->sizeRange = { 10.0f + i * 2, 20.0f + i * 2 };
//            activeEmitters.push_back(fire);
//
//            // Add smoke on top
//            auto smoke = particleManager->createEmitter();
//            smoke->position = { x, y - 50 };
//            smoke->emissionRate = 20;
//            smoke->lifetimeRange = { 2.0f, 3.0f };
//            smoke->sizeRange = { 15.0f, 25.0f };
//            smoke->speedRange = { 20.0f, 40.0f };
//            smoke->angleRange = { -HALF_PI - 0.2f, -HALF_PI + 0.2f };
//            smoke->colorRamp = {
//                {0.0f, Color(100, 100, 100, 100)},
//                {1.0f, Color(50, 50, 50, 0)}
//            };
//            smoke->shape = ParticleShape::SMOKE_PUFF;
//            smoke->blendMode = BlendMode::NORMAL;
//            activeEmitters.push_back(smoke);
//        }
//
//        persistentFire = activeEmitters[0];
//    }
//
//    void createMagicDemo() {
//        using namespace ParticleSystem;
//
//        auto magic = particleManager->createMagicEffect({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
//        magic->emissionRate = 100;
//
//        // Add orbiting particles
//        auto orbiter = particleManager->createEmitter();
//        orbiter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        orbiter->emissionRate = 30;
//        orbiter->pattern = EmissionPattern::RING;
//        orbiter->patternRadius = 100;
//        orbiter->lifetimeRange = { 3.0f, 4.0f };
//        orbiter->sizeRange = { 5.0f, 10.0f };
//        orbiter->speedRange = { 0, 0 };
//        orbiter->colorRamp = {
//            {0.0f, Color::hsv(0, 1, 1)},
//            {0.25f, Color::hsv(90, 1, 1)},
//            {0.5f, Color::hsv(180, 1, 1)},
//            {0.75f, Color::hsv(270, 1, 1)},
//            {1.0f, Color::hsv(360, 1, 1, 0)}
//        };
//        orbiter->shape = ParticleShape::STAR;
//        orbiter->blendMode = BlendMode::ADD;
//        orbiter->enableGlow = true;
//        orbiter->behaviors.push_back(ParticleBehavior::ORBIT);
//        orbiter->targetPosition = orbiter->position;
//
//        activeEmitters.push_back(magic);
//        activeEmitters.push_back(orbiter);
//    }
//
//    void createExplosionDemo() {
//        using namespace ParticleSystem;
//        // Will be triggered on mouse click
//    }
//
//    void createSmokeDemo() {
//        using namespace ParticleSystem;
//
//        auto smoke = particleManager->createEmitter();
//        smoke->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.7f };
//        smoke->emissionRate = 50;
//        smoke->pattern = EmissionPattern::CONE;
//        smoke->patternAngle = HALF_PI / 2;
//        smoke->rotation = -HALF_PI;
//        smoke->lifetimeRange = { 3.0f, 5.0f };
//        smoke->sizeRange = { 20.0f, 40.0f };
//        smoke->speedRange = { 30.0f, 60.0f };
//        smoke->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };
//        smoke->colorRamp = {
//            {0.0f, Color(150, 150, 150, 200)},
//            {0.5f, Color(100, 100, 100, 150)},
//            {1.0f, Color(50, 50, 50, 0)}
//        };
//        smoke->shape = ParticleShape::SMOKE_PUFF;
//        smoke->blendMode = BlendMode::NORMAL;
//        smoke->gravity = { 0, -20 };
//        smoke->turbulence = 30;
//        smoke->behaviors.push_back(ParticleBehavior::TURBULENCE);
//
//        activeEmitters.push_back(smoke);
//    }
//
//    void createSparklesDemo() {
//        using namespace ParticleSystem;
//
//        auto sparkles = particleManager->createEmitter();
//        sparkles->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        sparkles->emissionRate = 100;
//        sparkles->pattern = EmissionPattern::SPHERE;
//        sparkles->patternRadius = 200;
//        sparkles->lifetimeRange = { 1.0f, 2.0f };
//        sparkles->sizeRange = { 2.0f, 6.0f };
//        sparkles->speedRange = { 10.0f, 30.0f };
//        sparkles->colorRamp = {
//            {0.0f, Color(255, 255, 255)},
//            {0.5f, Color(255, 220, 100)},
//            {1.0f, Color(255, 255, 255, 0)}
//        };
//        sparkles->shape = ParticleShape::SPARKLE;
//        sparkles->blendMode = BlendMode::ADD;
//        sparkles->enableGlow = true;
//        sparkles->enablePulse = true;
//        sparkles->pulseRate = 5.0f;
//        sparkles->enableShimmer = true;
//        sparkles->shimmerRate = 10.0f;
//
//        activeEmitters.push_back(sparkles);
//    }
//
//    void createRainDemo() {
//        using namespace ParticleSystem;
//
//        rain = particleManager->createEmitter();
//        rain->position = { SCREEN_WIDTH / 2.0f, -50 };
//        rain->emissionRate = 500;
//        rain->pattern = EmissionPattern::LINE;
//        rain->patternRadius = SCREEN_WIDTH / 2;
//        rain->lifetimeRange = { 3.0f, 4.0f };
//        rain->sizeRange = { 1.0f, 2.0f };
//        rain->speedRange = { 400.0f, 500.0f };
//        rain->angleRange = { HALF_PI - 0.1f, HALF_PI + 0.1f };
//        rain->colorRamp = {
//            {0.0f, Color(150, 150, 255, 100)},
//            {1.0f, Color(150, 150, 255, 50)}
//        };
//        rain->shape = ParticleShape::CIRCLE;
//        rain->blendMode = BlendMode::NORMAL;
//        rain->gravity = { 0, 200 };
//        rain->enableTrails = true;
//        rain->trailLength = 10;
//
//        // Splash particles
//        auto splash = particleManager->createEmitter();
//        splash->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 50 };
//        splash->emissionRate = 200;
//        splash->pattern = EmissionPattern::LINE;
//        splash->patternRadius = SCREEN_WIDTH / 2;
//        splash->lifetimeRange = { 0.2f, 0.4f };
//        splash->sizeRange = { 2.0f, 4.0f };
//        splash->speedRange = { 50.0f, 100.0f };
//        splash->angleRange = { -HALF_PI - 1, -HALF_PI + 1 };
//        splash->colorRamp = {
//            {0.0f, Color(200, 200, 255, 150)},
//            {1.0f, Color(200, 200, 255, 0)}
//        };
//        splash->shape = ParticleShape::CIRCLE;
//        splash->blendMode = BlendMode::ADD;
//
//        activeEmitters.push_back(rain);
//        activeEmitters.push_back(splash);
//    }
//
//    void createSnowDemo() {
//        using namespace ParticleSystem;
//
//        snow = particleManager->createEmitter();
//        snow->position = { SCREEN_WIDTH / 2.0f, -50 };
//        snow->emissionRate = 100;
//        snow->pattern = EmissionPattern::LINE;
//        snow->patternRadius = SCREEN_WIDTH / 2;
//        snow->lifetimeRange = { 5.0f, 8.0f };
//        snow->sizeRange = { 2.0f, 6.0f };
//        snow->speedRange = { 30.0f, 60.0f };
//        snow->angleRange = { HALF_PI - 0.3f, HALF_PI + 0.3f };
//        snow->colorRamp = {
//            {0.0f, Color(255, 255, 255, 200)},
//            {1.0f, Color(255, 255, 255, 100)}
//        };
//        snow->shape = ParticleShape::CIRCLE;
//        snow->blendMode = BlendMode::NORMAL;
//        snow->gravity = { 0, 30 };
//        snow->wind = { 20, 0 };
//        snow->behaviors.push_back(ParticleBehavior::WANDER);
//
//        activeEmitters.push_back(snow);
//    }
//
//    void createLightningDemo() {
//        using namespace ParticleSystem;
//
//        // Create chain lightning effect
//        electricityPoints.clear();
//        electricityPoints.push_back({ 100, SCREEN_HEIGHT / 2.0f });
//        electricityPoints.push_back({ SCREEN_WIDTH - 100, SCREEN_HEIGHT / 2.0f });
//    }
//
//    void createPortalDemo() {
//        using namespace ParticleSystem;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        // Portal ring
//        auto portal = particleManager->createEmitter();
//        portal->position = center;
//        portal->emissionRate = 200;
//        portal->pattern = EmissionPattern::RING;
//        portal->patternRadius = 100;
//        portal->lifetimeRange = { 2.0f, 3.0f };
//        portal->sizeRange = { 3.0f, 8.0f };
//        portal->speedRange = { 0, 20 };
//        portal->colorRamp = {
//            {0.0f, Color(100, 50, 255)},
//            {0.5f, Color(200, 100, 255)},
//            {1.0f, Color(50, 0, 150, 0)}
//        };
//        portal->shape = ParticleShape::CIRCLE;
//        portal->blendMode = BlendMode::ADD;
//        portal->enableGlow = true;
//
//        // Swirling particles
//        ForceField vortex;
//        vortex.position = center;
//        vortex.radius = 200;
//        vortex.strength = 100;
//        vortex.type = ForceField::VORTEX;
//        portal->addForceField(vortex);
//
//        // Center glow
//        auto glow = particleManager->createEmitter();
//        glow->position = center;
//        glow->emissionRate = 50;
//        glow->pattern = EmissionPattern::POINT;
//        glow->lifetimeRange = { 0.5f, 1.0f };
//        glow->sizeRange = { 20.0f, 40.0f };
//        glow->speedRange = { 0, 10 };
//        glow->colorRamp = {
//            {0.0f, Color(255, 200, 255, 100)},
//            {1.0f, Color(100, 50, 255, 0)}
//        };
//        glow->shape = ParticleShape::CIRCLE;
//        glow->blendMode = BlendMode::ADD;
//        glow->enableGlow = true;
//        glow->glowIntensity = 2.0f;
//
//        activeEmitters.push_back(portal);
//        activeEmitters.push_back(glow);
//    }
//
//    void createGalaxyDemo() {
//        using namespace ParticleSystem;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        // Star field
//        auto stars = particleManager->createEmitter();
//        stars->position = center;
//        stars->emissionRate = 200;
//        stars->pattern = EmissionPattern::SPIRAL;
//        stars->patternRadius = 300;
//        stars->lifetimeRange = { 5.0f, 10.0f };
//        stars->sizeRange = { 1.0f, 4.0f };
//        stars->speedRange = { 0, 10 };
//        stars->colorRamp = {
//            {0.0f, Color(255, 255, 255)},
//            {0.3f, Color(200, 200, 255)},
//            {0.6f, Color(255, 200, 200)},
//            {1.0f, Color(255, 255, 255, 0)}
//        };
//        stars->shape = ParticleShape::STAR;
//        stars->blendMode = BlendMode::ADD;
//        stars->enableGlow = true;
//        stars->enableShimmer = true;
//
//        // Nebula clouds
//        auto nebula = particleManager->createEmitter();
//        nebula->position = center;
//        nebula->emissionRate = 30;
//        nebula->pattern = EmissionPattern::CIRCLE;
//        nebula->patternRadius = 200;
//        nebula->lifetimeRange = { 8.0f, 12.0f };
//        nebula->sizeRange = { 50.0f, 100.0f };
//        nebula->speedRange = { 0, 5 };
//        nebula->colorRamp = {
//            {0.0f, Color(100, 50, 150, 50)},
//            {0.5f, Color(200, 100, 100, 30)},
//            {1.0f, Color(50, 50, 200, 0)}
//        };
//        nebula->shape = ParticleShape::SMOKE_PUFF;
//        nebula->blendMode = BlendMode::ADD;
//
//        activeEmitters.push_back(stars);
//        activeEmitters.push_back(nebula);
//    }
//
//    void createWaterfallDemo() {
//        using namespace ParticleSystem;
//
//        // Water stream
//        auto water = particleManager->createEmitter();
//        water->position = { SCREEN_WIDTH / 2.0f, 100 };
//        water->emissionRate = 500;
//        water->pattern = EmissionPattern::LINE;
//        water->patternRadius = 50;
//        water->lifetimeRange = { 2.0f, 3.0f };
//        water->sizeRange = { 2.0f, 5.0f };
//        water->speedRange = { 50.0f, 100.0f };
//        water->angleRange = { HALF_PI - 0.1f, HALF_PI + 0.1f };
//        water->colorRamp = {
//            {0.0f, Color(150, 200, 255, 150)},
//            {1.0f, Color(200, 220, 255, 50)}
//        };
//        water->shape = ParticleShape::CIRCLE;
//        water->blendMode = BlendMode::NORMAL;
//        water->gravity = { 0, 300 };
//        water->enableTrails = true;
//        water->trailLength = 5;
//
//        // Mist at bottom
//        auto mist = particleManager->createEmitter();
//        mist->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100 };
//        mist->emissionRate = 100;
//        mist->pattern = EmissionPattern::CIRCLE;
//        mist->patternRadius = 100;
//        mist->lifetimeRange = { 2.0f, 4.0f };
//        mist->sizeRange = { 20.0f, 40.0f };
//        mist->speedRange = { 20.0f, 50.0f };
//        mist->angleRange = { -PI, 0 };
//        mist->colorRamp = {
//            {0.0f, Color(220, 240, 255, 100)},
//            {1.0f, Color(200, 220, 255, 0)}
//        };
//        mist->shape = ParticleShape::SMOKE_PUFF;
//        mist->blendMode = BlendMode::NORMAL;
//        mist->gravity = { 0, -50 };
//
//        activeEmitters.push_back(water);
//        activeEmitters.push_back(mist);
//    }
//
//    void createAuroraDemo() {
//        using namespace ParticleSystem;
//
//        // Aurora waves
//        for (int i = 0; i < 3; ++i) {
//            auto aurora = particleManager->createEmitter();
//            aurora->position = { SCREEN_WIDTH / 2.0f, 200.0f + i * 50 };
//            aurora->emissionRate = 100;
//            aurora->pattern = EmissionPattern::LINE;
//            aurora->patternRadius = SCREEN_WIDTH / 2;
//            aurora->lifetimeRange = { 3.0f, 5.0f };
//            aurora->sizeRange = { 30.0f, 60.0f };
//            aurora->speedRange = { 10.0f, 30.0f };
//            aurora->angleRange = { 0, TWO_PI };
//
//            float hue = 120 + i * 60; // Green to blue
//            aurora->colorRamp = {
//                {0.0f, Color::hsv(hue, 0.8f, 0.8f, 0.0f)},
//                {0.2f, Color::hsv(hue, 0.8f, 0.8f, 0.3f)},
//                {0.5f, Color::hsv(hue + 30, 0.7f, 1.0f, 0.2f)},
//                {1.0f, Color::hsv(hue + 60, 0.6f, 0.6f, 0.0f)}
//            };
//
//            aurora->shape = ParticleShape::SMOKE_PUFF;
//            aurora->blendMode = BlendMode::ADD;
//            aurora->behaviors.push_back(ParticleBehavior::FLOW_FIELD);
//            aurora->wind = { 50, 0 };
//
//            activeEmitters.push_back(aurora);
//        }
//    }
//
//    void createPoisonDemo() {
//        using namespace ParticleSystem;
//
//        auto poison = particleManager->createEmitter();
//        poison->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        poison->emissionRate = 80;
//        poison->pattern = EmissionPattern::CIRCLE;
//        poison->patternRadius = 50;
//        poison->lifetimeRange = { 2.0f, 4.0f };
//        poison->sizeRange = { 20.0f, 40.0f };
//        poison->speedRange = { 20.0f, 50.0f };
//        poison->angleRange = { 0, TWO_PI };
//        poison->colorRamp = {
//            {0.0f, Color(50, 200, 50, 150)},
//            {0.5f, Color(100, 255, 50, 100)},
//            {1.0f, Color(50, 150, 50, 0)}
//        };
//        poison->shape = ParticleShape::SMOKE_PUFF;
//        poison->blendMode = BlendMode::NORMAL;
//        poison->gravity = { 0, -20 };
//        poison->turbulence = 40;
//        poison->enableDistortion = true;
//        poison->distortionAmount = 0.2f;
//
//        // Bubbles
//        auto bubbles = particleManager->createEmitter();
//        bubbles->position = poison->position;
//        bubbles->emissionRate = 20;
//        bubbles->pattern = EmissionPattern::CIRCLE;
//        bubbles->patternRadius = 30;
//        bubbles->lifetimeRange = { 1.0f, 2.0f };
//        bubbles->sizeRange = { 5.0f, 15.0f };
//        bubbles->speedRange = { 30.0f, 60.0f };
//        bubbles->angleRange = { -PI, 0 };
//        bubbles->colorRamp = {
//            {0.0f, Color(150, 255, 150, 100)},
//            {1.0f, Color(100, 200, 100, 0)}
//        };
//        bubbles->shape = ParticleShape::BUBBLE;
//        bubbles->blendMode = BlendMode::ADD;
//        bubbles->gravity = { 0, -50 };
//
//        activeEmitters.push_back(poison);
//        activeEmitters.push_back(bubbles);
//    }
//
//    void createHealingDemo() {
//        using namespace ParticleSystem;
//
//        auto heal = particleManager->createEmitter();
//        heal->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        heal->emissionRate = 60;
//        heal->pattern = EmissionPattern::CIRCLE;
//        heal->patternRadius = 80;
//        heal->lifetimeRange = { 2.0f, 3.0f };
//        heal->sizeRange = { 5.0f, 15.0f };
//        heal->speedRange = { 20.0f, 40.0f };
//        heal->angleRange = { -PI, 0 };
//        heal->colorRamp = {
//            {0.0f, Color(100, 255, 100, 0)},
//            {0.3f, Color(200, 255, 200, 200)},
//            {0.7f, Color(150, 255, 150, 150)},
//            {1.0f, Color(255, 255, 255, 0)}
//        };
//        heal->shape = ParticleShape::CROSS;
//        heal->blendMode = BlendMode::ADD;
//        heal->enableGlow = true;
//        heal->glowIntensity = 1.5f;
//        heal->gravity = { 0, -30 };
//        heal->enablePulse = true;
//        heal->pulseRate = 1.0f;
//        heal->pulseAmount = 0.3f;
//
//        // Holy light rays
//        auto rays = particleManager->createEmitter();
//        rays->position = heal->position;
//        rays->emissionRate = 10;
//        rays->pattern = EmissionPattern::POINT;
//        rays->lifetimeRange = { 1.0f, 2.0f };
//        rays->sizeRange = { 100.0f, 200.0f };
//        rays->speedRange = { 0, 0 };
//        rays->colorRamp = {
//            {0.0f, Color(255, 255, 200, 50)},
//            {1.0f, Color(255, 255, 255, 0)}
//        };
//        rays->shape = ParticleShape::STAR;
//        rays->blendMode = BlendMode::ADD;
//        rays->angularVelRange = { 10, 30 };
//
//        activeEmitters.push_back(heal);
//        activeEmitters.push_back(rays);
//    }
//
//    void createBloodDemo() {
//        using namespace ParticleSystem;
//        // Will be triggered on mouse click
//    }
//
//    void createShockwaveDemo() {
//        using namespace ParticleSystem;
//        // Will be triggered on mouse click
//    }
//
//    void createConfettiDemo() {
//        using namespace ParticleSystem;
//
//        auto confetti = particleManager->createEmitter();
//        confetti->position = { SCREEN_WIDTH / 2.0f, 100 };
//        confetti->emissionRate = 100;
//        confetti->pattern = EmissionPattern::CONE;
//        confetti->patternAngle = HALF_PI;
//        confetti->rotation = HALF_PI;
//        confetti->lifetimeRange = { 3.0f, 5.0f };
//        confetti->sizeRange = { 5.0f, 10.0f };
//        confetti->speedRange = { 100.0f, 300.0f };
//        confetti->angleRange = { HALF_PI - 0.5f, HALF_PI + 0.5f };
//        confetti->angularVelRange = { -360, 360 };
//
//        // Random colors for each particle
//        confetti->onParticleSpawn = [](Particle& p) {
//            float hue = ParticleSystem::random_float(0, 360);
//            p.colorRamp = {
//                {0.0f, Color::hsv(hue, 1.0f, 1.0f)},
//                {1.0f, Color::hsv(hue, 1.0f, 1.0f, 0)}
//            };
//            p.shape = static_cast<ParticleShape>(
//                ParticleSystem::random_int(0, 5)
//                );
//            };
//
//        confetti->shape = ParticleShape::SQUARE;
//        confetti->blendMode = BlendMode::NORMAL;
//        confetti->gravity = { 0, 200 };
//        confetti->drag = 0.98f;
//
//        activeEmitters.push_back(confetti);
//    }
//
//    void createFireworksDemo() {
//        using namespace ParticleSystem;
//        fireworksCount = 0;
//        // Will spawn periodically in update
//    }
//
//    void createCosmicDustDemo() {
//        using namespace ParticleSystem;
//
//        auto dust = particleManager->createEmitter();
//        dust->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        dust->emissionRate = 50;
//        dust->pattern = EmissionPattern::SPHERE;
//        dust->patternRadius = 300;
//        dust->lifetimeRange = { 5.0f, 10.0f };
//        dust->sizeRange = { 1.0f, 3.0f };
//        dust->speedRange = { 5.0f, 20.0f };
//        dust->angleRange = { 0, TWO_PI };
//        dust->colorRamp = {
//            {0.0f, Color(200, 200, 255, 0)},
//            {0.2f, Color(200, 200, 255, 200)},
//            {0.8f, Color(255, 200, 200, 200)},
//            {1.0f, Color(255, 255, 200, 0)}
//        };
//        dust->shape = ParticleShape::CIRCLE;
//        dust->blendMode = BlendMode::ADD;
//        dust->enableGlow = true;
//        dust->behaviors.push_back(ParticleBehavior::WANDER);
//        dust->behaviors.push_back(ParticleBehavior::FLOW_FIELD);
//
//        activeEmitters.push_back(dust);
//    }
//
//    void createPlasmaDemo() {
//        using namespace ParticleSystem;
//
//        auto plasma = particleManager->createEmitter();
//        plasma->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        plasma->emissionRate = 200;
//        plasma->pattern = EmissionPattern::CIRCLE;
//        plasma->patternRadius = 100;
//        plasma->lifetimeRange = { 1.0f, 2.0f };
//        plasma->sizeRange = { 10.0f, 30.0f };
//        plasma->speedRange = { 0, 50 };
//        plasma->angleRange = { 0, TWO_PI };
//
//        // Dynamic plasma colors
//        plasma->onParticleUpdate = [](Particle& p) {
//            float t = p.age / p.lifetime;
//            p.color = ParticleUtils::plasmaColor(t + p.position.x * 0.01f);
//            };
//
//        plasma->shape = ParticleShape::CIRCLE;
//        plasma->blendMode = BlendMode::ADD;
//        plasma->enableGlow = true;
//        plasma->glowIntensity = 2.0f;
//        plasma->turbulence = 50;
//        plasma->enableDistortion = true;
//
//        // Electric field
//        ForceField field;
//        field.position = plasma->position;
//        field.radius = 200;
//        field.strength = 100;
//        field.type = ForceField::TURBULENCE;
//        plasma->addForceField(field);
//
//        activeEmitters.push_back(plasma);
//    }
//
//    void createMouseTrailDemo() {
//        using namespace ParticleSystem;
//
//        mouseEmitter = particleManager->createEmitter();
//        mouseEmitter->emissionRate = 100;
//        mouseEmitter->pattern = EmissionPattern::POINT;
//        mouseEmitter->lifetimeRange = { 0.5f, 1.0f };
//        mouseEmitter->sizeRange = { 5.0f, 15.0f };
//        mouseEmitter->speedRange = { 0, 20 };
//        mouseEmitter->angleRange = { 0, TWO_PI };
//        mouseEmitter->colorRamp = {
//            {0.0f, ParticleUtils::rainbow(0)},
//            {0.2f, ParticleUtils::rainbow(0.2f)},
//            {0.4f, ParticleUtils::rainbow(0.4f)},
//            {0.6f, ParticleUtils::rainbow(0.6f)},
//            {0.8f, ParticleUtils::rainbow(0.8f)},
//            {1.0f, ParticleUtils::rainbow(1.0f)}
//        };
//        mouseEmitter->shape = ParticleShape::STAR;
//        mouseEmitter->blendMode = BlendMode::ADD;
//        mouseEmitter->enableGlow = true;
//        mouseEmitter->enableTrails = true;
//        mouseEmitter->trailLength = 10;
//
//        activeEmitters.push_back(mouseEmitter);
//    }
//
//    void createFountainDemo() {
//        using namespace ParticleSystem;
//
//        auto fountain = particleManager->createEmitter();
//        fountain->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100 };
//        fountain->emissionRate = 200;
//        fountain->pattern = EmissionPattern::CONE;
//        fountain->patternAngle = HALF_PI / 3;
//        fountain->rotation = -HALF_PI;
//        fountain->lifetimeRange = { 2.0f, 3.0f };
//        fountain->sizeRange = { 3.0f, 8.0f };
//        fountain->speedRange = { 200.0f, 400.0f };
//        fountain->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };
//        fountain->colorRamp = {
//            {0.0f, Color(100, 150, 255, 200)},
//            {0.5f, Color(150, 200, 255, 150)},
//            {1.0f, Color(200, 220, 255, 0)}
//        };
//        fountain->shape = ParticleShape::CIRCLE;
//        fountain->blendMode = BlendMode::NORMAL;
//        fountain->gravity = { 0, 400 };
//        fountain->enableTrails = true;
//        fountain->trailLength = 8;
//        fountain->drag = 0.99f;
//
//        activeEmitters.push_back(fountain);
//    }
//
//    void createTornadoDemo() {
//        using namespace ParticleSystem;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        auto tornado = particleManager->createEmitter();
//        tornado->position = center;
//        tornado->emissionRate = 300;
//        tornado->pattern = EmissionPattern::CIRCLE;
//        tornado->patternRadius = 150;
//        tornado->lifetimeRange = { 2.0f, 4.0f };
//        tornado->sizeRange = { 5.0f, 20.0f };
//        tornado->speedRange = { 50.0f, 100.0f };
//        tornado->angleRange = { 0, TWO_PI };
//        tornado->colorRamp = {
//            {0.0f, Color(100, 100, 100, 150)},
//            {1.0f, Color(50, 50, 50, 0)}
//        };
//        tornado->shape = ParticleShape::SMOKE_PUFF;
//        tornado->blendMode = BlendMode::NORMAL;
//
//        // Strong vortex force
//        ForceField vortex;
//        vortex.position = center;
//        vortex.radius = 300;
//        vortex.strength = 200;
//        vortex.type = ForceField::VORTEX;
//        tornado->addForceField(vortex);
//
//        // Upward force
//        tornado->gravity = { 0, -100 };
//        tornado->turbulence = 50;
//
//        activeEmitters.push_back(tornado);
//    }
//
//    void createMatrixRainDemo() {
//        using namespace ParticleSystem;
//
//        auto matrix = particleManager->createEmitter();
//        matrix->position = { SCREEN_WIDTH / 2.0f, -50 };
//        matrix->emissionRate = 100;
//        matrix->pattern = EmissionPattern::LINE;
//        matrix->patternRadius = SCREEN_WIDTH / 2;
//        matrix->lifetimeRange = { 3.0f, 6.0f };
//        matrix->sizeRange = { 8.0f, 12.0f };
//        matrix->speedRange = { 50.0f, 150.0f };
//        matrix->angleRange = { HALF_PI, HALF_PI };
//        matrix->colorRamp = {
//            {0.0f, Color(0, 255, 0, 0)},
//            {0.1f, Color(0, 255, 0, 255)},
//            {0.9f, Color(0, 255, 0, 255)},
//            {1.0f, Color(0, 100, 0, 0)}
//        };
//        matrix->shape = ParticleShape::SQUARE;
//        matrix->blendMode = BlendMode::ADD;
//        matrix->enableTrails = true;
//        matrix->trailLength = 20;
//        /*matrix->trailFadeRate = 0.8f;*/
//
//        activeEmitters.push_back(matrix);
//    }
//
//    void createEnergyShieldDemo() {
//        using namespace ParticleSystem;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        auto shield = particleManager->createEmitter();
//        shield->position = center;
//        shield->emissionRate = 200;
//        shield->pattern = EmissionPattern::RING;
//        shield->patternRadius = 150;
//        shield->lifetimeRange = { 1.0f, 2.0f };
//        shield->sizeRange = { 5.0f, 10.0f };
//        shield->speedRange = { 0, 20 };
//        shield->angleRange = { 0, TWO_PI };
//        shield->colorRamp = {
//            {0.0f, Color(100, 200, 255, 100)},
//            {0.5f, Color(150, 220, 255, 150)},
//            {1.0f, Color(200, 240, 255, 0)}
//        };
//        shield->shape = ParticleShape::HEXAGON;
//        shield->blendMode = BlendMode::ADD;
//        shield->enableGlow = true;
//        shield->enablePulse = true;
//        shield->pulseRate = 2.0f;
//        shield->pulseAmount = 0.2f;
//
//        // Hexagonal grid effect
//        shield->onParticleUpdate = [center](Particle& p) {
//            // Keep particles on shield surface
//            Vec2 diff = p.position - center;
//            float dist = diff.length();
//            if (dist > 0) {
//                p.position = center + diff.normalized() * 150;
//            }
//            };
//
//        activeEmitters.push_back(shield);
//    }
//
//    void createDisintegrationDemo() {
//        using namespace ParticleSystem;
//        // Will be triggered on mouse click
//    }
//
//    void createBlackHoleDemo() {
//        using namespace ParticleSystem;
//
//        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
//
//        // Accretion disk
//        auto disk = particleManager->createEmitter();
//        disk->position = center;
//        disk->emissionRate = 200;
//        disk->pattern = EmissionPattern::RING;
//        disk->patternRadius = 200;
//        disk->lifetimeRange = { 3.0f, 5.0f };
//        disk->sizeRange = { 2.0f, 5.0f };
//        disk->speedRange = { 50.0f, 100.0f };
//        disk->angleRange = { 0, TWO_PI };
//        disk->colorRamp = {
//            {0.0f, Color(255, 200, 100, 200)},
//            {0.5f, Color(255, 100, 50, 150)},
//            {1.0f, Color(100, 0, 0, 0)}
//        };
//        disk->shape = ParticleShape::CIRCLE;
//        disk->blendMode = BlendMode::ADD;
//        disk->enableGlow = true;
//        disk->enableTrails = true;
//        disk->trailLength = 15;
//
//        // Strong gravitational pull
//        ForceField blackHole;
//        blackHole.position = center;
//        blackHole.radius = 400;
//        blackHole.strength = 300;
//        blackHole.type = ForceField::ATTRACT;
//        blackHole.falloff = 2.5f;
//        disk->addForceField(blackHole);
//
//        // Event horizon
//        auto horizon = particleManager->createEmitter();
//        horizon->position = center;
//        horizon->emissionRate = 1;
//        horizon->pattern = EmissionPattern::POINT;
//        horizon->lifetimeRange = { 10.0f, 10.0f };
//        horizon->sizeRange = { 80.0f, 80.0f };
//        horizon->speedRange = { 0, 0 };
//        horizon->colorRamp = {
//            {0.0f, Color(0, 0, 0, 255)},
//            {1.0f, Color(0, 0, 0, 255)}
//        };
//        horizon->shape = ParticleShape::CIRCLE;
//        horizon->blendMode = BlendMode::NORMAL;
//
//        activeEmitters.push_back(disk);
//        activeEmitters.push_back(horizon);
//    }
//
//    void createRainbowWaveDemo() {
//        using namespace ParticleSystem;
//
//        auto wave = particleManager->createEmitter();
//        wave->position = { 100, SCREEN_HEIGHT / 2.0f };
//        wave->emissionRate = 200;
//        wave->pattern = EmissionPattern::POINT;
//        wave->lifetimeRange = { 2.0f, 3.0f };
//        wave->sizeRange = { 10.0f, 20.0f };
//        wave->speedRange = { 200.0f, 300.0f };
//        wave->angleRange = { 0, 0 };
//
//        // Rainbow colors based on time
//        wave->onParticleSpawn = [this](Particle& p) {
//            float hue = std::fmod(demoTimer * 100, 360);
//            p.colorRamp = {
//                {0.0f, Color::hsv(hue, 1.0f, 1.0f)},
//                {0.5f, Color::hsv(hue + 60, 1.0f, 1.0f)},
//                {1.0f, Color::hsv(hue + 120, 1.0f, 1.0f, 0)}
//            };
//            };
//
//        wave->shape = ParticleShape::CIRCLE;
//        wave->blendMode = BlendMode::ADD;
//        wave->enableGlow = true;
//        wave->behaviors.push_back(ParticleBehavior::TURBULENCE);
//
//        activeEmitters.push_back(wave);
//    }
//
//    void createButterflySwarmDemo() {
//        using namespace ParticleSystem;
//
//        auto butterflies = particleManager->createEmitter();
//        butterflies->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        butterflies->emissionRate = 20;
//        butterflies->pattern = EmissionPattern::CIRCLE;
//        butterflies->patternRadius = 200;
//        butterflies->lifetimeRange = { 10.0f, 15.0f };
//        butterflies->sizeRange = { 15.0f, 25.0f };
//        butterflies->speedRange = { 30.0f, 60.0f };
//        butterflies->angleRange = { 0, TWO_PI };
//
//        // Random butterfly colors
//        butterflies->onParticleSpawn = [](Particle& p) {
//            float hue = ParticleSystem::random_float(0, 360);
//            p.colorRamp = {
//                {0.0f, Color::hsv(hue, 0.8f, 1.0f)},
//                {1.0f, Color::hsv(hue, 0.8f, 1.0f)}
//            };
//            p.behaviors.push_back(ParticleBehavior::WANDER);
//            p.behaviors.push_back(ParticleBehavior::FLOCK);
//            p.shape = ParticleShape::HEART; // Butterfly-like
//            p.angularVelocity = ParticleSystem::random_float(-90, 90);
//            };
//
//        butterflies->blendMode = BlendMode::NORMAL;
//        butterflies->behaviors.push_back(ParticleBehavior::WANDER);
//
//        activeEmitters.push_back(butterflies);
//    }
//
//    void createCombinedDemo() {
//        using namespace ParticleSystem;
//
//        // Create multiple effects at once
//        createFireDemo();
//        createMagicDemo();
//        createSparklesDemo();
//
//        // Add some ambient particles
//        auto ambient = particleManager->createEmitter();
//        ambient->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
//        ambient->emissionRate = 50;
//        ambient->pattern = EmissionPattern::BOX;
//        ambient->patternRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2.0f;
//        ambient->lifetimeRange = { 5.0f, 10.0f };
//        ambient->sizeRange = { 1.0f, 3.0f };
//        ambient->speedRange = { 5.0f, 15.0f };
//        ambient->angleRange = { 0, TWO_PI };
//        ambient->colorRamp = {
//            {0.0f, Color(200, 200, 255, 0)},
//            {0.5f, Color(200, 200, 255, 100)},
//            {1.0f, Color(200, 200, 255, 0)}
//        };
//        ambient->shape = ParticleShape::CIRCLE;
//        ambient->blendMode = BlendMode::ADD;
//        ambient->behaviors.push_back(ParticleBehavior::WANDER);
//
//        activeEmitters.push_back(ambient);
//    }
//
//    void handleEvents() {
//        SDL_Event event;
//        while (SDL_PollEvent(&event)) {
//            if (event.type == SDL_EVENT_QUIT) {
//                running = false;
//            }
//            else if (event.type == SDL_EVENT_KEY_DOWN) {
//                handleKeyPress(event.key.key);
//            }
//            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
//                mousePressed = true;
//                handleMouseClick(event.button.x, event.button.y);
//            }
//            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
//                mousePressed = false;
//            }
//            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
//                lastMouseX = mouseX;
//                lastMouseY = mouseY;
//                mouseX = event.motion.x;
//                mouseY = event.motion.y;
//            }
//        }
//    }
//
//    void handleKeyPress(SDL_Keycode key) {
//        using namespace ParticleSystem;
//
//        switch (key) {
//        case SDLK_ESCAPE:
//            running = false;
//            break;
//
//        case SDLK_SPACE:
//            paused = !paused;
//            break;
//
//        case SDLK_H:
//            showHelp = !showHelp;
//            break;
//
//        case SDLK_S:
//            showStats = !showStats;
//            break;
//
//        case SDLK_C:
//            particleManager->clear();
//            activeEmitters.clear();
//            break;
//
//        case SDLK_R:
//            loadDemo(currentDemo);
//            break;
//
//        case SDLK_LEFT:
//            loadDemo((currentDemo - 1 + demoNames.size()) % demoNames.size());
//            break;
//
//        case SDLK_RIGHT:
//            loadDemo((currentDemo + 1) % demoNames.size());
//            break;
//
//        case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
//        case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
//            loadDemo(key - SDLK_1);
//            break;
//
//        case SDLK_0:
//            loadDemo(9);
//            break;
//
//        case SDLK_E: {
//            // Trigger explosion at mouse position
//            particleManager->createExplosionEffect({ mouseX, mouseY });
//            break;
//        }
//
//        case SDLK_F: {
//            // Create fire at mouse position
//            auto fire = particleManager->createFireEffect({ mouseX, mouseY });
//            activeEmitters.push_back(fire);
//            break;
//        }
//
//        case SDLK_M: {
//            // Create magic at mouse position
//            auto magic = particleManager->createMagicEffect({ mouseX, mouseY });
//            activeEmitters.push_back(magic);
//            break;
//        }
//        }
//    }
//
//    void handleMouseClick(float x, float y) {
//        using namespace ParticleSystem;
//
//        // Demo-specific mouse interactions
//        switch (currentDemo) {
//        case 2: // Explosion demo
//            particleManager->createExplosionEffect({ x, y });
//            break;
//
//        case 7: { // Lightning demo
//            if (electricityPoints.size() >= 2) {
//                electricityPoints.clear();
//            }
//            electricityPoints.push_back({ x, y });
//            if (electricityPoints.size() == 2) {
//                auto lightning = particleManager->createElectricityEffect(
//                    electricityPoints[0], electricityPoints[1]
//                );
//                activeEmitters.push_back(lightning);
//            }
//            break;
//        }
//
//        case 14: { // Blood demo
//            auto blood = particleManager->createEmitter();
//            blood->position = { x, y };
//            blood->burstMode = true;
//            blood->burstCount = 50;
//            blood->active = false;
//            blood->pattern = EmissionPattern::CONE;
//            blood->patternAngle = HALF_PI;
//            blood->rotation = HALF_PI;
//            blood->lifetimeRange = { 1.0f, 2.0f };
//            blood->sizeRange = { 3.0f, 8.0f };
//            blood->speedRange = { 100.0f, 300.0f };
//            blood->angleRange = { HALF_PI - 0.5f, HALF_PI + 0.5f };
//            blood->colorRamp = {
//                {0.0f, Color(200, 0, 0)},
//                {0.5f, Color(150, 0, 0)},
//                {1.0f, Color(100, 0, 0, 0)}
//            };
//            blood->shape = ParticleShape::CIRCLE;
//            blood->blendMode = BlendMode::NORMAL;
//            blood->gravity = { 0, 300 };
//            blood->enableTrails = true;
//            blood->trailLength = 5;
//            blood->burst();
//            activeEmitters.push_back(blood);
//            break;
//        }
//
//        case 15: { // Shockwave demo
//            auto shockwave = particleManager->createEmitter();
//            shockwave->position = { x, y };
//            shockwave->burstMode = true;
//            shockwave->burstCount = 1;
//            shockwave->active = false;
//            shockwave->lifetimeRange = { 1.0f, 1.0f };
//            shockwave->sizeRange = { 10.0f, 10.0f };
//            shockwave->speedRange = { 0, 0 };
//            shockwave->colorRamp = {
//                {0.0f, Color(255, 255, 255, 200)},
//                {1.0f, Color(255, 255, 255, 0)}
//            };
//            shockwave->shape = ParticleShape::RING;
//            shockwave->blendMode = BlendMode::ADD;
//            shockwave->onParticleUpdate = [](Particle& p) {
//                p.size = p.startSize + p.age * 300;
//                };
//            shockwave->burst();
//            activeEmitters.push_back(shockwave);
//            break;
//        }
//
//        case 25: { // Disintegration demo
//            auto disintegrate = particleManager->createEmitter();
//            disintegrate->position = { x, y };
//            disintegrate->burstMode = true;
//            disintegrate->burstCount = 100;
//            disintegrate->active = false;
//            disintegrate->pattern = EmissionPattern::CIRCLE;
//            disintegrate->patternRadius = 20;
//            disintegrate->lifetimeRange = { 1.0f, 2.0f };
//            disintegrate->sizeRange = { 2.0f, 5.0f };
//            disintegrate->speedRange = { 50.0f, 150.0f };
//            disintegrate->angleRange = { 0, TWO_PI };
//            disintegrate->colorRamp = {
//                {0.0f, Color(100, 200, 255)},
//                {0.5f, Color(50, 100, 200)},
//                {1.0f, Color(0, 50, 100, 0)}
//            };
//            disintegrate->shape = ParticleShape::SQUARE;
//            disintegrate->blendMode = BlendMode::ADD;
//            disintegrate->enableGlow = true;
//            disintegrate->behaviors.push_back(ParticleBehavior::TURBULENCE);
//            disintegrate->burst();
//            activeEmitters.push_back(disintegrate);
//            break;
//        }
//        }
//    }
//
//    void update(float dt) {
//        if (paused) return;
//
//        demoTimer += dt;
//
//        // Update particle system
//        auto start = std::chrono::high_resolution_clock::now();
//        particleManager->update(dt);
//        auto end = std::chrono::high_resolution_clock::now();
//        updateTime = std::chrono::duration<float, std::milli>(end - start).count();
//
//        // Get particle count
//        particleManager->getPerformanceStats(updateTime, drawTime, particleCount);
//
//        // Demo-specific updates
//        switch (currentDemo) {
//        case 17: { // Fireworks
//            if (demoTimer > 1.0f) {
//                demoTimer = 0;
//                float x = ParticleSystem::random_float(200, SCREEN_WIDTH - 200);
//                float y = ParticleSystem::random_float(100, SCREEN_HEIGHT / 2);
//
//                auto firework = particleManager->createExplosionEffect({ x, y });
//                firework->colorRamp = {
//                    {0.0f, ParticleSystem::Color::hsv(
//                        ParticleSystem::random_float(0, 360), 1, 1)},
//                    {0.5f, ParticleSystem::Color::hsv(
//                        ParticleSystem::random_float(0, 360), 0.8f, 0.8f)},
//                    {1.0f, ParticleSystem::Color(100, 100, 100, 0)}
//                };
//                activeEmitters.push_back(firework);
//                fireworksCount++;
//
//                // Clean up old fireworks
//                if (fireworksCount > 10) {
//                    if (!activeEmitters.empty()) {
//                        activeEmitters.erase(activeEmitters.begin());
//                    }
//                    fireworksCount--;
//                }
//            }
//            break;
//        }
//
//        case 20: { // Mouse trail
//            if (mouseEmitter) {
//                mouseEmitter->position = { mouseX, mouseY };
//            }
//            break;
//        }
//
//        case 9: { // Galaxy rotation
//            galaxyRotation += dt * 0.5f;
//            for (auto emitter : activeEmitters) {
//                emitter->rotation = galaxyRotation;
//            }
//            break;
//        }
//        }
//
//        // FPS calculation
//        frameCount++;
//        fpsTimer += dt;
//        if (fpsTimer >= 1.0f) {
//            currentFPS = frameCount / fpsTimer;
//            frameCount = 0;
//            fpsTimer = 0;
//        }
//    }
//
//    void render() {
//        // Clear screen with gradient background
//        for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
//            int intensity = 20 + (y * 30 / SCREEN_HEIGHT);
//            draw.color(intensity, intensity, intensity + 10);
//            draw.fill_rect(0, y, SCREEN_WIDTH, 2);
//        }
//
//        // Draw particles
//        auto start = std::chrono::high_resolution_clock::now();
//        particleManager->draw();
//        auto end = std::chrono::high_resolution_clock::now();
//        drawTime = std::chrono::duration<float, std::milli>(end - start).count();
//
//        // Draw UI
//        drawUI();
//
//        draw.present();
//    }
//
//    void drawUI() {
//        // Draw stats
//        if (showStats) {
//            drawStats();
//        }
//
//        // Draw help
//        if (showHelp) {
//            drawHelp();
//        }
//
//        // Draw demo name
//        drawDemoName();
//
//        // Draw instructions
//        drawInstructions();
//    }
//
//    void drawStats() {
//        int y = 10;
//        int lineHeight = 20;
//
//        // Background
//        draw.color(0, 0, 0, 200);
//        draw.fill_rect(10, 10, 250, 150);
//        draw.color(255, 255, 255);
//        draw.rect(10, 10, 250, 150);
//
//        // Stats text
//        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//
//        std::stringstream ss;
//        ss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//
//        ss.str("");
//        ss << "Particles: " << particleCount;
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//
//        ss.str("");
//        ss << "Emitters: " << activeEmitters.size();
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//
//        ss.str("");
//        ss << "Update: " << std::fixed << std::setprecision(2) << updateTime << " ms";
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//
//        ss.str("");
//        ss << "Draw: " << std::fixed << std::setprecision(2) << drawTime << " ms";
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//
//        ss.str("");
//        ss << "Total: " << std::fixed << std::setprecision(2)
//            << (updateTime + drawTime) << " ms";
//        SDL_RenderDebugText(renderer, 20, y += lineHeight, ss.str().c_str());
//    }
//
//    void drawHelp() {
//        // Background
//        draw.color(0, 0, 0, 220);
//        draw.fill_rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 200, 400, 400);
//        draw.color(255, 255, 255);
//        draw.rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 200, 400, 400);
//
//        // Help text
//        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//        int y = SCREEN_HEIGHT / 2 - 180;
//        int lineHeight = 25;
//
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 50, y, "CONTROLS");
//        y += lineHeight * 2;
//
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "Left/Right Arrow - Switch demos");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "0-9 Keys - Jump to specific demo");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "Space - Pause/Resume");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "R - Restart current demo");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "C - Clear all particles");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "S - Toggle stats");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "H - Toggle this help");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "E - Explosion at mouse");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "F - Fire at mouse");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "M - Magic at mouse");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += lineHeight,
//            "ESC - Exit");
//    }
//
//    void drawDemoName() {
//        // Demo name background
//        draw.color(0, 0, 0, 180);
//        draw.fill_rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 60, 400, 50);
//        draw.color(255, 255, 255);
//        draw.rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 60, 400, 50);
//
//        // Demo name text
//        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//        SDL_SetRenderScale(renderer, 1.5f, 1.5f);
//
//        std::string name = demoNames[currentDemo];
//        int textWidth = name.length() * 8;
//        SDL_RenderDebugText(renderer,
//            (SCREEN_WIDTH / 2 - textWidth / 2) / 1.5f,
//            (SCREEN_HEIGHT - 45) / 1.5f,
//            name.c_str());
//
//        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
//
//        // Navigation hints
//        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 195, SCREEN_HEIGHT - 25,
//            "< Previous");
//        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 + 140, SCREEN_HEIGHT - 25,
//            "Next >");
//    }
//
//    void drawInstructions() {
//        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
//        SDL_RenderDebugText(renderer, 10, SCREEN_HEIGHT - 30,
//            "Press H for help | S for stats | Space to pause");
//
//        if (paused) {
//            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
//            SDL_SetRenderScale(renderer, 2.0f, 2.0f);
//            SDL_RenderDebugText(renderer,
//                (SCREEN_WIDTH / 2 - 40) / 2,
//                (SCREEN_HEIGHT / 2) / 2,
//                "PAUSED");
//            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
//        }
//    }
//
//    void run() {
//        Uint64 frameStart, frameTime;
//        const Uint64 frameDelay = 1000 / FPS;
//
//        while (running) {
//            frameStart = SDL_GetTicks();
//
//            handleEvents();
//            update(1.0f / FPS);
//            render();
//
//            frameTime = SDL_GetTicks() - frameStart;
//            if (frameDelay > frameTime) {
//                SDL_Delay(frameDelay - frameTime);
//            }
//        }
//    }
//};
//
////// Main entry point
////int main(int argc, char* argv[]) {
////    (void)argc; (void)argv;
////
////    SDL_SetAppMetadata("Particle System Demo", "1.0", "com.example.particles");
////
////    ParticleDemo demo;
////    if (!demo.init()) {
////        SDL_Log("Failed to initialize demo");
////        return -1;
////    }
////
////    demo.run();
////    return 0;
////}