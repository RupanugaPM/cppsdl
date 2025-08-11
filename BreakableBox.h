//#pragma once
//#include <SDL3/SDL.h>
//#include <vector>
//#include <memory>
//#include "Particles.h"
//
//class BreakableBox {
//private:
//    SDL_Rect rect;
//    bool hasKey;
//    bool broken;
//    std::vector<std::unique_ptr<DustParticle>> particles;
//    bool keyCollected;
//    float keyYOffset;
//    float keyFloatPhase;
//    bool isSpecialFlag;
//
//public:
//    BreakableBox(int x, int y, bool hasKey = false, bool isSpecialFlag = false);
//    void breakBox();
//    void update();
//    void draw(SDL_Renderer* renderer);
//    bool collectKey();
//
//    SDL_Rect getRect() const { return rect; }
//    bool isBroken() const { return broken; }
//    bool hasKeyItem() const { return hasKey; }
//    bool isKeyCollected() const { return keyCollected; }
//    bool isSpecial() const { return isSpecialFlag; }
//};