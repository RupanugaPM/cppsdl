//#pragma once
//#include <SDL3/SDL.h>
//#include <random>
//
//class FogParticle {
//private:
//    float x, y;
//    int size;
//    float speed;
//    int opacity;
//    float phase;
//
//public:
//    FogParticle();
//    void update();
//    void draw(SDL_Renderer* renderer);
//};
//
//class DustParticle {
//private:
//    float x, y;
//    float vx, vy;
//    float life;
//    int size;
//
//public:
//    DustParticle(float x, float y);
//    void update();
//    void draw(SDL_Renderer* renderer);
//    bool isAlive() const { return life > 0; }
//
//    void setVelocity(float vx, float vy) { this->vx = vx; this->vy = vy; }
//};