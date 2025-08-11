//#include "Fireball.h"
//#include "BreakableBox.h"
//#include <cmath>
//#include <random>
//
//static std::random_device rd;
//static std::mt19937 gen(rd());
//
//Fireball::Fireball(float x, float y, float targetX, float targetY, Mix_Chunk* fireballSound)
//    : rect{ (int)x, (int)y, 16, 16 }, alive(true), life(60) {
//
//    float dx = targetX - x;
//    float dy = targetY - y;
//    float distance = sqrt(dx * dx + dy * dy);
//
//    if (distance > 0) {
//        velX = (dx / distance) * 12;
//        velY = (dy / distance) * 12;
//    }
//    else {
//        velX = 12;
//        velY = 0;
//    }
//
//    if (fireballSound) {
//        Mix_PlayChannel(-1, fireballSound, 0);
//    }
//}
//
//void Fireball::update(const std::vector<Platform>& platforms, std::vector<BreakableBox*>& breakableBoxes) {
//    // Update particles
//    particles.erase(
//        std::remove_if(particles.begin(), particles.end(),
//            [](const std::unique_ptr<DustParticle>& p) { return !p->isAlive(); }),
//        particles.end()
//    );
//
//    for (auto& particle : particles) {
//        particle->update();
//    }
//
//    if (!alive) return;
//
//    life--;
//    if (life <= 0) {
//        alive = false;
//        return;
//    }
//
//    rect.x += velX;
//    rect.y += velY;
//
//    // Check platform collisions
//    for (const auto& platform : platforms) {
//        if (platform.solid && SDL_HasRectIntersection(&rect, &platform.rect)) {
//            explode();
//            return;
//        }
//    }
//
//    // Check box collisions
//    for (auto* box : breakableBoxes) {
//        SDL_Rect boxRect = box->getRect();
//        if (!box->isBroken() && SDL_HasRectIntersection(&rect, &boxRect)) {
//            box->breakBox();
//            explode();
//            return;
//        }
//    }
//
//    // Add trailing particles
//    std::uniform_real_distribution<> chance(0.0f, 1.0f);
//    if (chance(gen) < 0.8f) {
//        std::uniform_int_distribution<> offset(-3, 3);
//        particles.push_back(std::make_unique<DustParticle>(
//            rect.x + rect.w / 2 + offset(gen),
//            rect.y + rect.h / 2 + offset(gen)
//        ));
//    }
//
//    // Check if out of bounds
//    if (rect.x < -50 || rect.x > SCREEN_WIDTH + 50 ||
//        rect.y < -50 || rect.y > SCREEN_HEIGHT + 50) {
//        alive = false;
//    }
//}
//
//void Fireball::explode() {
//    alive = false;
//
//    std::uniform_real_distribution<> angleDist(0, M_PI * 2);
//    std::uniform_real_distribution<> speedDist(2.0f, 5.0f);
//
//    for (int i = 0; i < 4; i++) {
//        float angle = angleDist(gen);
//        float speed = speedDist(gen);
//
//        auto particle = std::make_unique<DustParticle>(rect.x + rect.w / 2, rect.y + rect.h / 2);
//        particle->setVelocity(cos(angle) * speed, sin(angle) * speed);
//        particles.push_back(std::move(particle));
//    }
//}
//
//void Fireball::draw(SDL_Renderer* renderer) {
//    for (const auto& particle : particles) {
//        particle->draw(renderer);
//    }
//
//    if (alive) {
//        // Draw glowing orb
//        for (int i = 3; i > 0; i -= 2) {
//            int alpha = 150 * (i / 16.0f);
//            SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, alpha);
//
//            // Draw filled circle
//            int centerX = rect.x + rect.w / 2;
//            int centerY = rect.y + rect.h / 2;
//
//            for (int w = 0; w < i * 2; w++) {
//                for (int h = 0; h < i * 2; h++) {
//                    int dx = i - w;
//                    int dy = i - h;
//                    if ((dx * dx + dy * dy) <= (i * i)) {
//                        SDL_RenderPoint(renderer, centerX + w - i, centerY + h - i);
//                    }
//                }
//            }
//        }
//    }
//}