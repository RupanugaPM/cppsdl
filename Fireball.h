//#pragma once
//#include <SDL3/SDL.h>
//#include <SDL_mixer.h>
//#include <vector>
//#include <memory>
//#include "Particles.h"
//#include "Constants.h"
//
//class BreakableBox;
//
//class Fireball {
//private:
//    SDL_Rect rect;
//    float velX, velY;
//    std::vector<std::unique_ptr<DustParticle>> particles;
//    bool alive;
//    int life;
//
//    void explode();
//
//public:
//    Fireball(float x, float y, float targetX, float targetY, Mix_Chunk* fireballSound);
//    void update(const std::vector<Platform>& platforms, std::vector<BreakableBox*>& breakableBoxes);
//    void draw(SDL_Renderer* renderer);
//
//    bool isAlive() const { return alive; }
//    bool hasParticles() const { return !particles.empty(); }
//};