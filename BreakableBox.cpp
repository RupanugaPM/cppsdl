//#include "BreakableBox.h"
//#include "Constants.h"
//#include <cmath>
//#include <random>
//
//static std::random_device rd;
//static std::mt19937 gen(rd());
//
//BreakableBox::BreakableBox(int x, int y, bool hasKey, bool isSpecialFlag)
//    : rect{ x, y, 70, 70 }, hasKey(hasKey), broken(false),
//    keyCollected(false), keyYOffset(0), isSpecialFlag(isSpecialFlag) {
//    std::uniform_real_distribution<> phaseDist(0, M_PI * 2);
//    keyFloatPhase = phaseDist(gen);
//}
//
//void BreakableBox::breakBox() {
//    if (!broken) {
//        broken = true;
//
//        std::uniform_real_distribution<> angleDist(0, M_PI * 2);
//        std::uniform_real_distribution<> speedDist(2.0f, 5.0f);
//
//        for (int i = 0; i < 4; i++) {
//            float angle = angleDist(gen);
//            float speed = speedDist(gen);
//
//            auto particle = std::make_unique<DustParticle>(
//                rect.x + rect.w / 2,
//                rect.y + rect.h / 2
//            );
//            particle->setVelocity(
//                cos(angle) * speed,
//                sin(angle) * speed - 2
//            );
//            particles.push_back(std::move(particle));
//        }
//    }
//}
//
//void BreakableBox::update() {
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
//    // Update floating key animation
//    if (broken && hasKey && !keyCollected) {
//        keyFloatPhase += 0.1f;
//        keyYOffset = sin(keyFloatPhase) * 5;
//    }
//}
//
//bool BreakableBox::collectKey() {
//    if (broken && hasKey && !keyCollected) {
//        keyCollected = true;
//        return true;
//    }
//    return false;
//}
//
//void BreakableBox::draw(SDL_Renderer* renderer) {
//    // Draw particles
//    for (const auto& particle : particles) {
//        particle->draw(renderer);
//    }
//
//    if (!broken) {
//        // Draw silhouette box
//        SDL_SetRenderDrawColor(renderer, SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b, 255);
//        SDL_RenderFillRect(renderer, &rect);
//
//        // Draw subtle highlight
//        SDL_SetRenderDrawColor(renderer, DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b, 255);
//        SDL_RenderRect(renderer, &rect);
//    }
//    else if (hasKey && !keyCollected) {
//        int keyX = rect.x + rect.w / 2;
//        int keyY = rect.y + rect.h / 2 - 20 + keyYOffset;
//
//        // Draw glowing effect
//        for (int i = 10; i > 0; i -= 2) {
//            int alpha = 120 * (i / 20.0f);
//            SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, alpha);
//
//            // Draw filled circle for glow
//            for (int w = 0; w < i * 2; w++) {
//                for (int h = 0; h < i * 2; h++) {
//                    int dx = i - w;
//                    int dy = i - h;
//                    if ((dx * dx + dy * dy) <= (i * i)) {
//                        SDL_RenderPoint(renderer, keyX + w - i, keyY + h - i);
//                    }
//                }
//            }
//        }
//
//        // Draw key silhouette
//        SDL_SetRenderDrawColor(renderer, SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b, 255);
//
//        // Key head (circle)
//        for (int w = 0; w < 12; w++) {
//            for (int h = 0; h < 12; h++) {
//                int dx = 6 - w;
//                int dy = 6 - h;
//                if ((dx * dx + dy * dy) <= 36) {
//                    SDL_RenderPoint(renderer, keyX + w - 6, keyY + h - 6);
//                }
//            }
//        }
//
//        // Key body
//        SDL_Rect keyBody = { keyX - 2, keyY, 4, 12 };
//        SDL_RenderFillRect(renderer, &keyBody);
//
//        // Key teeth
//        SDL_Rect tooth1 = { keyX - 2, keyY + 8, 6, 2 };
//        SDL_Rect tooth2 = { keyX - 2, keyY + 11, 4, 2 };
//        SDL_RenderFillRect(renderer, &tooth1);
//        SDL_RenderFillRect(renderer, &tooth2);
//    }
//}