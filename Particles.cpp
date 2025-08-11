//#include "Particles.h"
//#include "Constants.h"
//#include <cmath>
//
//static std::random_device rd;
//static std::mt19937 gen(rd());
//
//FogParticle::FogParticle() {
//    std::uniform_int_distribution<> sizeDist(50, 150);
//    std::uniform_real_distribution<> speedDist(0.2f, 0.5f);
//    std::uniform_int_distribution<> opacityDist(20, 60);
//    std::uniform_real_distribution<> phaseDist(0, 3.14159265f * 2);
//    std::uniform_int_distribution<> xDist(-200, SCREEN_WIDTH);
//    std::uniform_int_distribution<> yDist(0, SCREEN_HEIGHT);
//
//    x = xDist(gen);
//    y = yDist(gen);
//    size = sizeDist(gen);
//    speed = speedDist(gen);
//    opacity = opacityDist(gen);
//    phase = phaseDist(gen);
//}
//
//void FogParticle::update() {
//    x += speed;
//    phase += 0.01f;
//    y += sin(phase) * 0.3f;
//
//    if (x > SCREEN_WIDTH + size) {
//        x = -size;
//        std::uniform_int_distribution<> yDist(0, SCREEN_HEIGHT);
//        y = yDist(gen);
//    }
//}
//
//void FogParticle::draw(SDL_Renderer* renderer) {
//    for (int i = size; i > 0; i -= 5) {
//        int alpha = opacity * (i / (float)size);
//        SDL_SetRenderDrawColor(renderer, FOG_COLOR.r, FOG_COLOR.g, FOG_COLOR.b, alpha);
//
//        // Draw filled circle
//        for (int w = 0; w < i * 2; w++) {
//            for (int h = 0; h < i * 2; h++) {
//                int dx = i - w;
//                int dy = i - h;
//                if ((dx * dx + dy * dy) <= (i * i)) {
//                    SDL_RenderPoint(renderer, x + w - i, y + h - i);
//                }
//            }
//        }
//    }
//}
//
//DustParticle::DustParticle(float x, float y) : x(x), y(y), life(1.0f) {
//    std::uniform_real_distribution<> vxDist(-0.5f, 0.5f);
//    std::uniform_real_distribution<> vyDist(-1.0f, -0.5f);
//    std::uniform_int_distribution<> sizeDist(2, 4);
//
//    vx = vxDist(gen);
//    vy = vyDist(gen);
//    size = sizeDist(gen);
//}
//
//void DustParticle::update() {
//    x += vx;
//    y += vy;
//    life -= 0.02f;
//    vy += 0.02f;
//}
//
//void DustParticle::draw(SDL_Renderer* renderer) {
//    if (life > 0) {
//        int alpha = 100 * life;
//        SDL_SetRenderDrawColor(renderer, LIGHT_GRAY.r, LIGHT_GRAY.g, LIGHT_GRAY.b, alpha);
//
//        // Draw filled circle
//        for (int w = 0; w < size * 2; w++) {
//            for (int h = 0; h < size * 2; h++) {
//                int dx = size - w;
//                int dy = size - h;
//                if ((dx * dx + dy * dy) <= (size * size)) {
//                    SDL_RenderPoint(renderer, x + w - size, y + h - size);
//                }
//            }
//        }
//    }
//}