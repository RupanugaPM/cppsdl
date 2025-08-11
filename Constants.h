//#pragma once
//#include <SDL3/SDL.h>
//#include <vector>
//#include <map>
//#include <string>
//
//// Screen dimensions
//constexpr int SCREEN_WIDTH = 1200;
//constexpr int SCREEN_HEIGHT = 800;
//constexpr int FPS = 60;
//
//// Physics
//constexpr float GRAVITY = 0.8f;
//constexpr float JUMP_STRENGTH = -15.0f;
//constexpr float PLAYER_SPEED = 5.0f;
//
//// Colors
//struct Color {
//    Uint8 r, g, b, a;
//    Color(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0, Uint8 a = 255)
//        : r(r), g(g), b(b), a(a) {
//    }
//};
//
//const Color BLACK(0, 0, 0);
//const Color DARK_GRAY(20, 20, 20);
//const Color MEDIUM_GRAY(40, 40, 40);
//const Color LIGHT_GRAY(80, 80, 80);
//const Color LIGHTER_GRAY(120, 120, 120);
//const Color FOG_GRAY(160, 160, 160);
//const Color WHITE(255, 255, 255);
//const Color SILHOUETTE = BLACK;
//const Color BACKGROUND(180, 180, 180);
//const Color FOG_COLOR(200, 200, 200);
//const Color LIGHT_COLOR(255, 255, 255);
//
//// Game States
//enum class GameState {
//    MENU,
//    PLAYING,
//    LEVEL_COMPLETE,
//    TRANSITIONING,
//    ENDING
//};
//
//// Platform structure
//struct Platform {
//    SDL_Rect rect;
//    bool solid;
//
//    Platform(int x, int y, int w, int h, bool s = true)
//        : rect{ x, y, w, h }, solid(s) {
//    }
//};
//
//// Door data
//struct DoorData {
//    int x, y;
//    int targetLevel;
//    std::string label;
//    bool locked;
//
//    DoorData(int x, int y, int target, const std::string& lbl = "", bool lock = false)
//        : x(x), y(y), targetLevel(target), label(lbl), locked(lock) {
//    }
//};
//
//// Box data
//struct BoxData {
//    int x, y;
//    bool hasKey;
//    bool isSpecialFlag;
//
//    BoxData(int x, int y, bool key = false, bool special = false)
//        : x(x), y(y), hasKey(key), isSpecialFlag(special) {
//    }
//};
//
//// NPC data
//struct NPCData {
//    int x, y;
//    std::map<std::string, std::vector<std::string>> dialogues;
//
//    NPCData(int x, int y, const std::map<std::string, std::vector<std::string>>& dlg)
//        : x(x), y(y), dialogues(dlg) {
//    }
//};
//
//// Light data
//struct LightData {
//    int x, y;
//
//    LightData(int x, int y) : x(x), y(y) {}
//};
//
//// Level data structure
//struct LevelData {
//    std::vector<Platform> platforms;
//    std::pair<int, int> playerStart;
//    std::vector<DoorData> doors;
//    std::vector<BoxData> breakableBoxes;
//    std::vector<NPCData> npcs;
//    std::vector<LightData> lights;
//    std::map<std::string, bool> abilities;
//};