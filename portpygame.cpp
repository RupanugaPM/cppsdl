// dungeon_game.cpp - Complete C++ translation of the Python dungeon game
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <memory>
#include <random>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_map>
#include "renderer2d.cpp"

// Constants
static constexpr int SCREEN_WIDTH = 1200;
static constexpr int SCREEN_HEIGHT = 800;
static constexpr int FPS = 60;
static constexpr float GRAVITY = 0.8f;
static constexpr float JUMP_STRENGTH = -15.0f;
static constexpr float PLAYER_SPEED = 5.0f;
static constexpr float PI = 3.14159265f;
static constexpr float TWO_PI = 2.0f * PI;

// Colors
struct Color {
    Uint8 r, g, b, a;
    constexpr Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) : r(r), g(g), b(b), a(a) {}
};

static constexpr Color BLACK(0, 0, 0);
static constexpr Color DARK_GRAY(20, 20, 20);
static constexpr Color MEDIUM_GRAY(40, 40, 40);
static constexpr Color LIGHT_GRAY(80, 80, 80);
static constexpr Color LIGHTER_GRAY(120, 120, 120);
static constexpr Color FOG_GRAY(160, 160, 160);
static constexpr Color WHITE(255, 255, 255);
static constexpr Color SILHOUETTE = BLACK;
static constexpr Color BACKGROUND(180, 180, 180);
static constexpr Color FOG_COLOR(200, 200, 200);
static constexpr Color LIGHT_COLOR(255, 255, 255);

// Random number generator
static std::random_device rd;
static std::mt19937 rng(rd());
static std::uniform_real_distribution<float> rand_float(0.0f, 1.0f);
static std::uniform_int_distribution<int> rand_int(0, 100);

// Sound system
struct SoundSystem {
    SDL_AudioStream* jump_sound = nullptr;
    SDL_AudioStream* walk_sound = nullptr;
    SDL_AudioStream* fireball_sound = nullptr;
    SDL_AudioStream* music_stream = nullptr;

    Uint8* jump_data = nullptr;
    Uint32 jump_len = 0;
    Uint8* walk_data = nullptr;
    Uint32 walk_len = 0;
    Uint8* fireball_data = nullptr;
    Uint32 fireball_len = 0;

    bool walking_sound_playing = false;

    void init() {
        SDL_AudioSpec spec;

        // Load sound effects
        if (SDL_LoadWAV("sounds/jump.wav", &spec, &jump_data, &jump_len)) {
            jump_sound = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (jump_sound) SDL_ResumeAudioStreamDevice(jump_sound);
        }

        if (SDL_LoadWAV("sounds/walk.wav", &spec, &walk_data, &walk_len)) {
            walk_sound = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (walk_sound) SDL_ResumeAudioStreamDevice(walk_sound);
        }

        if (SDL_LoadWAV("sounds/fireball.wav", &spec, &fireball_data, &fireball_len)) {
            fireball_sound = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (fireball_sound) SDL_ResumeAudioStreamDevice(fireball_sound);
        }
    }

    void play_jump() {
        if (jump_sound && jump_data) {
            SDL_ClearAudioStream(jump_sound);
            SDL_PutAudioStreamData(jump_sound, jump_data, jump_len);
        }
    }

    void play_fireball() {
        if (fireball_sound && fireball_data) {
            SDL_ClearAudioStream(fireball_sound);
            SDL_PutAudioStreamData(fireball_sound, fireball_data, fireball_len);
        }
    }

    void start_walking() {
        if (!walking_sound_playing && walk_sound && walk_data) {
            walking_sound_playing = true;
            SDL_PutAudioStreamData(walk_sound, walk_data, walk_len);
        }
    }

    void stop_walking() {
        if (walking_sound_playing && walk_sound) {
            walking_sound_playing = false;
            SDL_ClearAudioStream(walk_sound);
        }
    }

    void update_walking() {
        if (walking_sound_playing && walk_sound && walk_data) {
            if (SDL_GetAudioStreamQueued(walk_sound) < (int)walk_len / 2) {
                SDL_PutAudioStreamData(walk_sound, walk_data, walk_len);
            }
        }
    }

    void cleanup() {
        if (jump_sound) SDL_DestroyAudioStream(jump_sound);
        if (walk_sound) SDL_DestroyAudioStream(walk_sound);
        if (fireball_sound) SDL_DestroyAudioStream(fireball_sound);
        if (music_stream) SDL_DestroyAudioStream(music_stream);
        SDL_free(jump_data);
        SDL_free(walk_data);
        SDL_free(fireball_data);
    }
};

static SoundSystem sound_system;

enum class GameState {
    MENU,
    PLAYING,
    LEVEL_COMPLETE,
    TRANSITIONING,
    ENDING
};

struct TransitionState {
    std::string phase = "swipe";
    float progress = 0.0f;
    SDL_Texture* old_level_texture = nullptr;
    SDL_Texture* new_level_texture = nullptr;
    float offset_x = 0;
    int target_level = 0;
    int start_level = 0;
    int direction = 1;
};

struct FogParticle {
    float x, y;
    int size;
    float speed;
    int opacity;
    float phase;

    FogParticle() {
        x = rand_int(rng) - 200 + rand_int(rng) % SCREEN_WIDTH;
        y = rand_int(rng) % SCREEN_HEIGHT;
        size = 50 + rand_int(rng) % 100;
        speed = 0.2f + rand_float(rng) * 0.3f;
        opacity = 20 + rand_int(rng) % 40;
        phase = rand_float(rng) * TWO_PI;
    }

    void update() {
        x += speed;
        phase += 0.01f;
        y += std::sin(phase) * 0.3f;

        if (x > SCREEN_WIDTH + size) {
            x = -size;
            y = rand_int(rng) % SCREEN_HEIGHT;
        }
    }

    void draw(Draw& draw) {
        // Simple fog circle with gradient
        for (int i = size; i > 0; i -= 5) {
            int alpha = opacity * i / size;
            draw.color(FOG_COLOR.r, FOG_COLOR.g, FOG_COLOR.b, alpha);
            draw.fill_circle(x, y, i);
        }
    }
};

struct DustParticle {
    float x, y;
    float vx, vy;
    float life;
    int size;

    DustParticle(float x, float y) : x(x), y(y), life(1.0f) {
        vx = rand_float(rng) - 0.5f;
        vy = -1.0f - rand_float(rng) * 0.5f;
        size = 2 + rand_int(rng) % 3;
    }

    void update() {
        x += vx;
        y += vy;
        life -= 0.02f;
        vy += 0.02f;
    }

    void draw(Draw& draw) {
        if (life > 0) {
            int alpha = 100 * life;
            draw.color(LIGHT_GRAY.r, LIGHT_GRAY.g, LIGHT_GRAY.b, alpha);
            draw.fill_circle(x, y, size);
        }
    }

    bool is_dead() const { return life <= 0; }
};

struct Platform {
    SDL_FRect rect;
    bool solid;
};

struct BreakableBox {
    SDL_FRect rect;
    bool has_key;
    bool broken;
    std::vector<DustParticle> particles;
    bool key_collected;
    float key_y_offset;
    float key_float_phase;
    bool is_special_flag;

    BreakableBox(float x, float y, bool has_key = false, bool is_special = false)
        : has_key(has_key), broken(false), key_collected(false),
        key_y_offset(0), is_special_flag(is_special) {
        rect = { x, y, 70, 70 };
        key_float_phase = rand_float(rng) * TWO_PI;
    }

    void break_box() {
        if (!broken) {
            broken = true;
            for (int i = 0; i < 4; i++) {
                float angle = rand_float(rng) * TWO_PI;
                float speed = 2.0f + rand_float(rng) * 3.0f;
                DustParticle p(rect.x + rect.w / 2, rect.y + rect.h / 2);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed - 2;
                particles.push_back(p);
            }
        }
    }

    void update() {
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.is_dead(); }),
            particles.end()
        );

        for (auto& p : particles) {
            p.update();
        }

        if (broken && has_key && !key_collected) {
            key_float_phase += 0.1f;
            key_y_offset = std::sin(key_float_phase) * 5;
        }
    }

    bool collect_key() {
        if (broken && has_key && !key_collected) {
            key_collected = true;
            return true;
        }
        return false;
    }

    void draw(Draw& draw) {
        for (auto& p : particles) {
            p.draw(draw);
        }

        if (!broken) {
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            draw.fill_rect(rect.x, rect.y, rect.w, rect.h);
            draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
            draw.rect(rect.x, rect.y, rect.w, rect.h);
        }
        else if (has_key && !key_collected) {
            float key_x = rect.x + rect.w / 2;
            float key_y = rect.y + rect.h / 2 - 20 + key_y_offset;

            // Glowing key
            for (int i = 10; i > 0; i -= 2) {
                int alpha = 120 * i / 20;
                draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                draw.fill_circle(key_x, key_y, i);
            }

            // Key shape
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            draw.fill_circle(key_x, key_y, 6);
            draw.fill_rect(key_x - 2, key_y, 4, 12);
            draw.fill_rect(key_x - 2, key_y + 8, 6, 2);
            draw.fill_rect(key_x - 2, key_y + 11, 4, 2);
        }
    }
};

struct Fireball {
    SDL_FRect rect;
    float vel_x, vel_y;
    std::vector<DustParticle> particles;
    bool alive;
    int life;

    Fireball(float x, float y, float target_x, float target_y) : alive(true), life(60) {
        rect = { x, y, 16, 16 };
        float dx = target_x - x;
        float dy = target_y - y;
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > 0) {
            vel_x = (dx / distance) * 12;
            vel_y = (dy / distance) * 12;
        }
        else {
            vel_x = 12;
            vel_y = 0;
        }
        sound_system.play_fireball();
    }

    void update(std::vector<Platform>& platforms, std::vector<BreakableBox>& boxes) {
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.is_dead(); }),
            particles.end()
        );

        for (auto& p : particles) {
            p.update();
        }

        if (!alive) return;

        life--;
        if (life <= 0) {
            alive = false;
            return;
        }

        rect.x += vel_x;
        rect.y += vel_y;

        // Check platform collisions
        for (auto& platform : platforms) {
            if (platform.solid && SDL_HasRectIntersectionFloat(&rect, &platform.rect)) {
                explode();
                return;
            }
        }

        // Check box collisions
        for (auto& box : boxes) {
            if (!box.broken && SDL_HasRectIntersectionFloat(&rect, &box.rect)) {
                box.break_box();
                explode();
                return;
            }
        }

        // Add trail particles
        if (rand_float(rng) < 0.8f) {
            particles.push_back(DustParticle(
                rect.x + rect.w / 2 + (rand_int(rng) % 7 - 3),
                rect.y + rect.h / 2 + (rand_int(rng) % 7 - 3)
            ));
        }

        // Bounds check
        if (rect.x < -50 || rect.x > SCREEN_WIDTH + 50 ||
            rect.y < -50 || rect.y > SCREEN_HEIGHT + 50) {
            alive = false;
        }
    }

    void explode() {
        alive = false;
        for (int i = 0; i < 4; i++) {
            float angle = rand_float(rng) * TWO_PI;
            float speed = 2.0f + rand_float(rng) * 3.0f;
            DustParticle p(rect.x + rect.w / 2, rect.y + rect.h / 2);
            p.vx = std::cos(angle) * speed;
            p.vy = std::sin(angle) * speed;
            particles.push_back(p);
        }
    }

    void draw(Draw& draw) {
        for (auto& p : particles) {
            p.draw(draw);
        }

        if (alive) {
            // Glowing orb
            for (int i = 8; i > 0; i -= 2) {
                int alpha = 150 * i / 16;
                draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                draw.fill_circle(rect.x + rect.w / 2, rect.y + rect.h / 2, i);
            }
        }
    }
};

struct NPC {
    SDL_FRect rect;
    float x, y;
    std::map<std::string, std::vector<std::string>> dialogues;
    std::map<std::string, int> dialogue_indices;
    float bob_phase;
    bool show_prompt;
    std::string current_dialogue;
    int dialogue_timer;
    bool talking;
    float gesture_timer;
    bool facing_player;
    float arm_animation;
    int interaction_cooldown;

    NPC(float x, float y, std::map<std::string, std::vector<std::string>> dialogues)
        : x(x), y(y), dialogues(dialogues), show_prompt(false),
        dialogue_timer(0), talking(false), gesture_timer(0),
        facing_player(false), arm_animation(0), interaction_cooldown(0) {
        rect = { x, y - 45, 28, 45 };
        bob_phase = rand_float(rng) * TWO_PI;
    }

    void update(SDL_FRect& player_rect, int from_level) {
        bob_phase += 0.05f;
        rect.y = y - 45 + std::sin(bob_phase) * 2;

        float dx = player_rect.x + player_rect.w / 2 - (rect.x + rect.w / 2);
        float dy = player_rect.y + player_rect.h / 2 - (rect.y + rect.h / 2);
        float distance = std::sqrt(dx * dx + dy * dy);
        show_prompt = distance < 60;

        if (show_prompt) {
            facing_player = player_rect.x > rect.x;
        }

        if (interaction_cooldown > 0) {
            interaction_cooldown--;
        }

        if (dialogue_timer > 0) {
            dialogue_timer--;
            talking = true;
            gesture_timer += 0.15f;
            arm_animation = std::sin(gesture_timer) * 20;
        }
        else {
            talking = false;
            arm_animation *= 0.9f;
        }
    }

    void interact(int from_level, int current_level) {
        if (interaction_cooldown > 0) return;

        std::string key = (from_level != current_level) ?
            "from_" + std::to_string(from_level) : "default";

        if (dialogues.find(key) == dialogues.end()) {
            key = "default";
        }

        auto& dialogue_list = dialogues[key];
        if (dialogue_list.empty()) {
            dialogue_list.push_back("...");
        }

        if (dialogue_indices.find(key) == dialogue_indices.end()) {
            dialogue_indices[key] = 0;
        }

        current_dialogue = dialogue_list[dialogue_indices[key]];
        dialogue_indices[key] = (dialogue_indices[key] + 1) % dialogue_list.size();

        dialogue_timer = 180;
        gesture_timer = 0;
        interaction_cooldown = 20;
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        float cx = rect.x + rect.w / 2;
        float cy = rect.y + rect.h / 2;

        // Draw NPC body (simplified)
        draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);

        // Head
        draw.fill_circle(cx, rect.y + 10, 8);

        // Body
        std::vector<SDL_FPoint> body_points = {
            {cx - 10, rect.y + 15},
            {cx + 10, rect.y + 15},
            {cx + 12, rect.y + rect.h},
            {cx - 12, rect.y + rect.h}
        };
        draw.fill_polygon(body_points, SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);

        // Show prompt
        if (show_prompt && dialogue_timer <= 0) {
            float prompt_y = rect.y - 35;

            // Glow
            for (int i = 15; i > 0; i -= 3) {
                int alpha = 80 * i / 15;
                draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                draw.fill_circle(cx, prompt_y, i);
            }

            // E prompt
            draw.color(WHITE.r, WHITE.g, WHITE.b);
            draw.fill_rect(cx - 12, prompt_y - 12, 24, 24);
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            SDL_RenderDebugText(renderer, cx - 4, prompt_y - 6, "E");
        }

        // Show dialogue
        if (dialogue_timer > 0 && !current_dialogue.empty()) {
            int alpha = std::min(255, dialogue_timer * 8);
            if (dialogue_timer > 30) alpha = 255;

            // Speech bubble background
            int text_width = current_dialogue.length() * 8;
            int bubble_width = text_width + 20;
            int bubble_height = 30;

            draw.color(WHITE.r, WHITE.g, WHITE.b, alpha * 0.9);
            draw.fill_rect(cx - bubble_width / 2, rect.y - bubble_height - 20,
                bubble_width, bubble_height);

            // Dialogue text
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            SDL_RenderDebugText(renderer, cx - text_width / 2,
                rect.y - bubble_height - 10, current_dialogue.c_str());
        }
    }
};

struct Door {
    SDL_FRect rect;
    int target_level;
    std::string label;
    float glow_timer;
    std::vector<DustParticle> particles;
    bool locked;

    Door(float x, float y, int target, const std::string& label = "")
        : target_level(target), label(label), glow_timer(0), locked(false) {
        rect = { x, y, 50, 70 };
    }

    void update() {
        glow_timer += 0.05f;

        if (!locked && rand_float(rng) < 0.02f) {
            DustParticle p(rect.x + rect.w / 2 + (rand_int(rng) % 31 - 15),
                rect.y + rand_int(rng) % (int)rect.h);
            p.vy -= 0.5f;
            particles.push_back(p);
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.is_dead(); }),
            particles.end()
        );

        for (auto& p : particles) {
            p.update();
            p.vy -= 0.1f;
        }
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        for (auto& p : particles) {
            p.draw(draw);
        }

        if (!locked) {
            float glow_intensity = (std::sin(glow_timer) + 1) * 0.3f;
            for (int i = 6; i > 0; i -= 2) {
                int alpha = 100 * glow_intensity * i / 20;
                draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                draw.rect(rect.x - i * 2, rect.y - i * 2,
                    rect.w + i * 4, rect.h + i * 4);
            }
        }

        draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
        draw.fill_rect(rect.x, rect.y, rect.w, rect.h);
        draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
        draw.rect(rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10);

        if (locked) {
            draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
            draw.fill_rect(rect.x + rect.w / 2 - 8, rect.y + rect.h / 2 - 8, 16, 16);
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            draw.fill_circle(rect.x + rect.w / 2, rect.y + rect.h / 2, 3);
        }

        if (!label.empty()) {
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            SDL_RenderDebugText(renderer, rect.x + rect.w / 2 - 4,
                rect.y - 20, label.c_str());
        }
    }
};

struct Player {
    SDL_FRect rect;
    float vel_x, vel_y;
    bool on_ground;
    bool on_drop_platform;
    bool dropping;
    int drop_timer;
    bool drop_key_pressed;
    std::vector<DustParticle> particles;

    // Animation
    std::string animation_state;
    int animation_timer;
    float walk_cycle;
    int land_timer;
    float idle_timer;
    bool facing_right;
    float head_offset;
    float arm_swing;

    // Abilities
    std::map<std::string, bool> abilities;
    bool jump_available;
    bool double_jump_available;
    bool can_double_jump;
    bool jump_pressed;
    bool can_fireball;
    std::vector<Fireball> fireballs;
    int fireball_cooldown;

    int keys;

    Player(float x, float y) : vel_x(0), vel_y(0), on_ground(false),
        on_drop_platform(false), dropping(false), drop_timer(0),
        drop_key_pressed(false), animation_state("idle"), animation_timer(0),
        walk_cycle(0), land_timer(0), idle_timer(0), facing_right(true),
        head_offset(0), arm_swing(0), jump_available(false),
        double_jump_available(false), can_double_jump(false),
        jump_pressed(false), can_fireball(false), fireball_cooldown(0), keys(0) {
        rect = { x, y, 24, 36 };
    }

    void set_position(float x, float y) {
        rect.x = x;
        rect.y = y;
        rect.w = 25;
        rect.h = 40;
    }

    void set_abilities(std::map<std::string, bool> new_abilities) {
        abilities = new_abilities;
        jump_available = abilities["jump"];
        double_jump_available = abilities["double_jump"];
        can_fireball = abilities["fireball"];
    }

    void update(std::vector<Platform>& platforms, int mouse_x, int mouse_y,
        std::vector<BreakableBox>& boxes) {
        const bool* keys_state = SDL_GetKeyboardState(nullptr);
        vel_x = 0;

        // Movement
        if (keys_state[SDL_SCANCODE_LEFT] || keys_state[SDL_SCANCODE_A]) {
            vel_x = -PLAYER_SPEED;
            facing_right = false;
        }
        if (keys_state[SDL_SCANCODE_RIGHT] || keys_state[SDL_SCANCODE_D]) {
            vel_x = PLAYER_SPEED;
            facing_right = true;
        }

        // Walking sound
        if (on_ground && std::abs(vel_x) > 0) {
            sound_system.start_walking();
        }
        else {
            sound_system.stop_walking();
        }
        sound_system.update_walking();

        // Update animation state
        if (land_timer > 0) {
            animation_state = "landing";
            land_timer--;
        }
        else if (!on_ground) {
            animation_state = (vel_y < -2) ? "jumping" : "falling";
        }
        else if (std::abs(vel_x) > 0) {
            animation_state = "walking";
        }
        else {
            animation_state = "idle";
        }

        animation_timer++;

        // Walking animation
        if (animation_state == "walking") {
            walk_cycle += std::abs(vel_x) * 0.15f;
            arm_swing = std::sin(walk_cycle) * 12;
            head_offset = std::abs(std::sin(walk_cycle * 2)) * 0.5f;
        }
        else {
            walk_cycle = 0;
            arm_swing *= 0.85f;
        }

        // Idle animation
        if (animation_state == "idle") {
            idle_timer += 0.04f;
            head_offset = std::sin(idle_timer) * 0.3f;
        }

        // Jump animation
        if (animation_state == "jumping") {
            arm_swing = -8;
        }
        else if (animation_state == "falling") {
            arm_swing = 12;
        }

        // Drop through platforms
        bool drop_key = keys_state[SDL_SCANCODE_S] || keys_state[SDL_SCANCODE_DOWN];

        if (drop_key && !drop_key_pressed && on_drop_platform) {
            dropping = true;
            drop_timer = 10;
            vel_y = 2;
        }

        drop_key_pressed = drop_key;

        if (drop_timer > 0) {
            drop_timer--;
        }
        else {
            dropping = false;
        }

        // Jumping
        bool jump_key = keys_state[SDL_SCANCODE_SPACE] ||
            keys_state[SDL_SCANCODE_UP] ||
            keys_state[SDL_SCANCODE_W];

        if (jump_available && jump_key && !jump_pressed) {
            if (on_ground) {
                sound_system.play_jump();
                vel_y = JUMP_STRENGTH;
                can_double_jump = double_jump_available;
                for (int i = 0; i < 3; i++) {
                    particles.push_back(DustParticle(
                        rect.x + rect.w / 2 + (rand_int(rng) % 17 - 8),
                        rect.y + rect.h
                    ));
                }
            }
            else if (can_double_jump) {
                sound_system.play_jump();
                vel_y = JUMP_STRENGTH * 0.85f;
                can_double_jump = false;
                for (int i = 0; i < 4; i++) {
                    float angle = rand_float(rng) * TWO_PI;
                    float speed = 2.0f + rand_float(rng) * 2.0f;
                    DustParticle p(rect.x + rect.w / 2, rect.y + rect.h / 2);
                    p.vx = std::cos(angle) * speed;
                    p.vy = std::sin(angle) * speed;
                    particles.push_back(p);
                }
            }
        }

        jump_pressed = jump_key;

        // Fireball
        if (can_fireball && fireball_cooldown <= 0) {
            if (keys_state[SDL_SCANCODE_F] || keys_state[SDL_SCANCODE_LSHIFT]) {
                fireballs.push_back(Fireball(
                    rect.x + rect.w / 2, rect.y + rect.h / 2,
                    mouse_x, mouse_y
                ));
                fireball_cooldown = 20;
            }
        }

        if (fireball_cooldown > 0) {
            fireball_cooldown--;
        }

        // Gravity
        vel_y += GRAVITY;
        if (vel_y > 20) vel_y = 20;

        bool was_falling = !on_ground && vel_y > 5;

        // Move horizontally
        rect.x += vel_x;
        rect.x = std::max(0.0f, std::min(rect.x, (float)(SCREEN_WIDTH - rect.w)));
        check_collisions(platforms, true);

        // Move vertically
        rect.y += vel_y;
        on_ground = false;
        on_drop_platform = false;
        check_collisions(platforms, false);

        // Landing effect
        if (on_ground && was_falling) {
            land_timer = 8;
            for (int i = 0; i < 6; i++) {
                particles.push_back(DustParticle(
                    rect.x + rect.w / 2 + (rand_int(rng) % 25 - 12),
                    rect.y + rect.h
                ));
            }
        }

        // Update particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.is_dead(); }),
            particles.end()
        );

        for (auto& p : particles) {
            p.update();
        }

        // Update fireballs
        for (auto& f : fireballs) {
            f.update(platforms, boxes);
        }

        fireballs.erase(
            std::remove_if(fireballs.begin(), fireballs.end(),
                [](const Fireball& f) { return !f.alive && f.particles.empty(); }),
            fireballs.end()
        );
    }

    void check_collisions(std::vector<Platform>& platforms, bool horizontal) {
        for (auto& platform : platforms) {
            if (SDL_HasRectIntersectionFloat(&rect, &platform.rect)) {
                if (horizontal) {
                    if (platform.solid) {
                        if (vel_x > 0) {
                            rect.x = platform.rect.x - rect.w;
                        }
                        else {
                            rect.x = platform.rect.x + platform.rect.w;
                        }
                    }
                }
                else {
                    if (!platform.solid) {
                        if (vel_y > 0 && !dropping) {
                            if (rect.y + rect.h - vel_y <= platform.rect.y + 5) {
                                rect.y = platform.rect.y - rect.h;
                                vel_y = 0;
                                on_ground = true;
                                on_drop_platform = true;
                            }
                        }
                    }
                    else {
                        if (vel_y > 0) {
                            rect.y = platform.rect.y - rect.h;
                            vel_y = 0;
                            on_ground = true;
                        }
                        else {
                            rect.y = platform.rect.y + platform.rect.h;
                            vel_y = 0;
                        }
                    }
                }
            }
        }
    }

    void draw(Draw& draw) {
        // Draw particles
        for (auto& p : particles) {
            p.draw(draw);
        }

        // Draw fireballs
        for (auto& f : fireballs) {
            f.draw(draw);
        }

        // Draw player (simplified stick figure)
        float cx = rect.x + rect.w / 2;
        float cy = rect.y + rect.h / 2;
        float head_y = rect.y + 5 + head_offset;

        if (animation_state == "landing") {
            head_y += 2;
        }

        // White outline
        draw.color(WHITE.r, WHITE.g, WHITE.b);
        draw.fill_circle(cx, head_y, 6);

        // Head
        draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
        draw.fill_circle(cx, head_y, 5);

        // Body
        draw.line(cx, head_y + 5, cx, rect.y + rect.h - 8);

        // Arms
        float arm_y = rect.y + 18;
        draw.line(cx, arm_y, cx - 5 - arm_swing * 0.3f, arm_y + 8);
        draw.line(cx, arm_y, cx + 5 + arm_swing * 0.3f, arm_y + 8);

        // Legs
        float hip_y = rect.y + rect.h - 8;
        if (animation_state == "walking") {
            float left_phase = std::sin(walk_cycle);
            float right_phase = std::sin(walk_cycle + PI);

            draw.line(cx, hip_y, cx - 3 + left_phase * 4, rect.y + rect.h);
            draw.line(cx, hip_y, cx + 3 + right_phase * 4, rect.y + rect.h);
        }
        else {
            draw.line(cx, hip_y, cx - 4, rect.y + rect.h);
            draw.line(cx, hip_y, cx + 4, rect.y + rect.h);
        }

        // Double jump indicator
        if (double_jump_available && can_double_jump && !on_ground) {
            for (int i = 6; i > 0; i -= 2) {
                int alpha = 100 * i / 15;
                draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                draw.fill_circle(cx, rect.y - 20, i);
            }
        }
    }
};

struct Light {
    float x, y;
    float radius;
    float flicker_timer;

    Light(float x, float y) : x(x), y(y), radius(200), flicker_timer(rand_float(rng)* TWO_PI) {}

    void update() {
        flicker_timer += 0.03f;
    }

    void draw(Draw& draw) {
        // Simple light effect
        float flicker = std::sin(flicker_timer) * 20;
        float current_radius = radius + flicker;
        // Light rendering handled differently in SDL
    }
};

struct Level {
    int level_number;
    std::vector<Platform> platforms;
    std::pair<float, float> player_start;
    std::vector<Door> doors;
    std::vector<Light> lights;
    std::vector<BreakableBox> breakable_boxes;
    std::map<std::string, bool> player_abilities;
    int keys_required;
    std::vector<FogParticle> fog_particles;
    std::vector<NPC> npcs;
    bool lift_blur;

    Level(int level_num) : level_number(level_num), keys_required(0), lift_blur(false) {
        player_start = { 100, 400 };

        // Add fog particles
        for (int i = 0; i < 4; i++) {
            fog_particles.push_back(FogParticle());
        }
    }

    void update(Player& player, int from_level) {
        for (auto& fog : fog_particles) {
            fog.update();
        }

        for (auto& door : doors) {
            door.update();
        }

        for (auto& light : lights) {
            light.update();
        }

        for (auto& box : breakable_boxes) {
            box.update();
            if (box.is_special_flag && box.broken) {
                lift_blur = true;
            }

            // Check key collection
            if (box.broken && box.has_key && !box.key_collected) {
                float dx = player.rect.x + player.rect.w / 2 - (box.rect.x + box.rect.w / 2);
                float dy = player.rect.y + player.rect.h / 2 - (box.rect.y + box.rect.h / 2);
                if (std::abs(dx) < 30 && std::abs(dy) < 30) {
                    if (box.collect_key()) {
                        player.keys++;
                    }
                }
            }
        }

        for (auto& npc : npcs) {
            npc.update(player.rect, from_level);
        }

        // Unlock doors with keys
        for (auto& door : doors) {
            if (door.locked && player.keys > 0) {
                door.locked = false;
                player.keys--;
            }
        }
    }

    void draw_background(Draw& draw) {
        // Gradient background
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            float ratio = (float)y / SCREEN_HEIGHT;
            int gray = BACKGROUND.r * (1 - ratio * 0.3f);
            draw.color(gray, gray, gray);
            draw.line(0, y, SCREEN_WIDTH, y);
        }

        // Fog particles
        for (auto& fog : fog_particles) {
            fog.draw(draw);
        }
    }

    void draw_platforms(Draw& draw) {
        for (auto& platform : platforms) {
            if (!platform.solid) {
                // Drop-through platform
                draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b, 180);
                draw.fill_rect(platform.rect.x, platform.rect.y, platform.rect.w, 8);
                draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
                draw.line(platform.rect.x, platform.rect.y,
                    platform.rect.x + platform.rect.w, platform.rect.y);
            }
            else {
                // Solid platform
                draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
                draw.fill_rect(platform.rect.x, platform.rect.y,
                    platform.rect.w, platform.rect.h);
                draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
                draw.line(platform.rect.x, platform.rect.y,
                    platform.rect.x + platform.rect.w, platform.rect.y);
            }
        }
    }
};

struct EndingScreen {
    struct Star {
        float x, y, z;
        float speedz;
    };

    std::vector<Star> stars;
    int timer;
    std::vector<std::string> story_texts;
    int current_text_index;
    int text_display_timer;
    bool fade_to_menu;
    int fade_timer;

    EndingScreen() : timer(0), current_text_index(0), text_display_timer(0),
        fade_to_menu(false), fade_timer(0) {
        // Initialize stars
        for (int i = 0; i < 150; i++) {
            Star s;
            s.x = (rand_int(rng) % SCREEN_WIDTH) - SCREEN_WIDTH / 2;
            s.y = (rand_int(rng) % SCREEN_HEIGHT) - SCREEN_HEIGHT / 2;
            s.z = SCREEN_WIDTH / 2 + rand_int(rng) % (SCREEN_WIDTH / 2);
            s.speedz = 3;
            stars.push_back(s);
        }

        story_texts = {
            "You step through the doorway into the endless void...",
            "The dungeon fades behind as the mage transports you back to your world.",
            "The mage's laughter fades as you drift through space...",
            "Perhaps this was always your destiny.",
            "To be summoned, to escape, to return to the stars.",
            "...",
            "Thank you for playing.",
            "Created by big homie, RPM",
            "and Small homie, SD"
        };
    }

    bool update() {
        timer++;

        // Update stars
        for (auto& star : stars) {
            star.z -= star.speedz;

            if (star.z <= 20) {
                star.x = (rand_int(rng) % SCREEN_WIDTH) - SCREEN_WIDTH / 2;
                star.y = (rand_int(rng) % SCREEN_HEIGHT) - SCREEN_HEIGHT / 2;
                star.z = SCREEN_WIDTH / 2 + rand_int(rng) % (SCREEN_WIDTH / 2);
            }
        }

        text_display_timer++;

        if (text_display_timer > 180) {
            text_display_timer = 0;
            current_text_index++;

            if (current_text_index >= story_texts.size()) {
                fade_to_menu = true;
            }
        }

        if (fade_to_menu) {
            fade_timer += 2;
            if (fade_timer > 255) {
                return true;  // Return to menu
            }
        }

        return false;
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Black background
        draw.color(BLACK.r, BLACK.g, BLACK.b);
        draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Draw stars
        for (auto& star : stars) {
            float sx = (star.x / star.z) * (SCREEN_WIDTH / 2);
            float sy = (star.y / star.z) * (SCREEN_HEIGHT / 2);
            float radius = ((SCREEN_WIDTH - star.z) / (float)SCREEN_WIDTH) * 4;

            if (radius > 0) {
                // Glow effect
                for (int i = radius * 2; i > 0; i--) {
                    int alpha = 255 * (i / (radius * 2)) * 0.5f;
                    draw.color(WHITE.r, WHITE.g, WHITE.b, alpha);
                    draw.fill_circle(SCREEN_WIDTH / 2 + sx, SCREEN_HEIGHT / 2 + sy, i);
                }

                draw.color(WHITE.r, WHITE.g, WHITE.b);
                draw.fill_circle(SCREEN_WIDTH / 2 + sx, SCREEN_HEIGHT / 2 + sy, radius);
            }
        }

        // Draw text
        if (current_text_index < story_texts.size()) {
            std::string& text = story_texts[current_text_index];

            int opacity = 255;
            if (text_display_timer < 30) {
                opacity = (text_display_timer / 30.0f) * 255;
            }
            else if (text_display_timer > 150) {
                opacity = ((180 - text_display_timer) / 30.0f) * 255;
            }

            draw.color(WHITE.r, WHITE.g, WHITE.b, opacity);

            // Center text
            int text_width = text.length() * 8;
            SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - text_width / 2,
                SCREEN_HEIGHT / 2, text.c_str());
        }

        // Fade to black
        if (fade_to_menu) {
            draw.color(BLACK.r, BLACK.g, BLACK.b, std::min(255, fade_timer));
            draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
};

struct Menu {
    struct Button {
        SDL_FRect rect;
        std::string label;
    };

    std::map<std::string, Button> buttons;
    std::string hover;
    std::vector<DustParticle> particles;
    float bg_phase;
    std::vector<FogParticle> fog_particles;

    Menu() : bg_phase(0) {
        buttons["start"] = { {SCREEN_WIDTH / 2 - 120, 400, 240, 50}, "START" };
        buttons["quit"] = { {SCREEN_WIDTH / 2 - 120, 480, 240, 50}, "QUIT" };

        for (int i = 0; i < 4; i++) {
            fog_particles.push_back(FogParticle());
        }
    }

    void update(int mouse_x, int mouse_y) {
        hover = "";

        for (auto& [name, button] : buttons) {
            SDL_FPoint mouse = { (float)mouse_x, (float)mouse_y };
            if (SDL_PointInRectFloat(&mouse, &button.rect)) {
                hover = name;
                if (rand_float(rng) < 0.1f) {
                    particles.push_back(DustParticle(
                        button.rect.x + button.rect.w / 2 + (rand_int(rng) % 81 - 40),
                        button.rect.y + button.rect.h / 2
                    ));
                }
            }
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.is_dead(); }),
            particles.end()
        );

        for (auto& p : particles) {
            p.update();
        }

        for (auto& fog : fog_particles) {
            fog.update();
        }

        bg_phase += 0.01f;
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Background gradient
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            int gray = 160 - (y / (float)SCREEN_HEIGHT) * 60;
            draw.color(gray, gray, gray);
            draw.line(0, y, SCREEN_WIDTH, y);
        }

        // Fog
        for (auto& fog : fog_particles) {
            fog.draw(draw);
        }

        // Particles
        for (auto& p : particles) {
            p.draw(draw);
        }

        // Title
        draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
        SDL_SetRenderScale(renderer, 2.0f, 2.0f);
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 4 - 100, 60, "TTIGSBAMTGOOTD");
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);

        // Buttons
        for (auto& [name, button] : buttons) {
            if (hover == name) {
                // Glow effect
                draw.color(WHITE.r, WHITE.g, WHITE.b, 50);
                draw.fill_rect(button.rect.x - 10, button.rect.y - 10,
                    button.rect.w + 20, button.rect.h + 20);
            }

            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            draw.fill_rect(button.rect.x, button.rect.y, button.rect.w, button.rect.h);
            draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
            draw.rect(button.rect.x, button.rect.y, button.rect.w, button.rect.h);

            draw.color(hover == name ? WHITE.r : LIGHT_GRAY.r,
                hover == name ? WHITE.g : LIGHT_GRAY.g,
                hover == name ? WHITE.b : LIGHT_GRAY.b);

            int text_width = button.label.length() * 8;
            SDL_RenderDebugText(renderer,
                button.rect.x + button.rect.w / 2 - text_width / 2,
                button.rect.y + button.rect.h / 2 - 4,
                button.label.c_str());
        }
    }

    std::string handle_click(int x, int y) {
        SDL_FPoint mouse = { (float)x, (float)y };

        for (auto& [name, button] : buttons) {
            if (SDL_PointInRectFloat(&mouse, &button.rect)) {
                return name;
            }
        }

        return "";
    }
};

// Main Game class
class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Draw draw;
    GameState state;
    Menu menu;
    int current_level;
    int from_level;
    std::vector<Level> levels;
    Level* level;
    Player player;
    TransitionState transition;
    EndingScreen ending_screen;
    bool running;
    Uint64 last_time;

public:
    Game() : window(nullptr), renderer(nullptr), draw(nullptr),
        state(GameState::MENU), current_level(0), from_level(0),
        level(nullptr), player(0, 0), running(true) {

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
            running = false;
            return;
        }

        window = SDL_CreateWindow(
            "That time I got summon by a mage to break free from the dungeon",
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            SDL_Log("Failed to create window: %s", SDL_GetError());
            running = false;
            return;
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            SDL_Log("Failed to create renderer: %s", SDL_GetError());
            running = false;
            return;
        }

        draw.set_renderer(renderer);
        sound_system.init();

        load_levels();
        last_time = SDL_GetTicks();
    }

    void load_levels() {
        // Create 8 levels (simplified for space)
        for (int i = 0; i < 8; i++) {
            Level lvl(i);

            // Basic platforms for all levels
            lvl.platforms.push_back({ {150, 700, 900, 100}, true });
            lvl.platforms.push_back({ {150, 150, 50, 600}, true });
            lvl.platforms.push_back({ {1000, 150, 50, 600}, true });
            lvl.platforms.push_back({ {200, 150, 850, 50}, true });

            // Add level-specific elements
            if (i == 0) {
                // Level 1 - Starting level
                lvl.platforms.push_back({ {500, 500, 200, 20}, true });
                lvl.platforms.push_back({ {350, 450, 150, 20}, false });

                lvl.player_start = { 250, 660 };

                lvl.doors.push_back(Door(950, 630, 1));
                lvl.doors.push_back(Door(100, 230, -1, "Exit"));
                lvl.doors[1].locked = true;

                lvl.breakable_boxes.push_back(BreakableBox(130, 230, true, true));

                // Create NPC with dialogues
                std::map<std::string, std::vector<std::string>> dialogues;
                dialogues["default"] = {
                    "Hey there, sorry for summoning you",
                    "I am stuck in this dungeon",
                    "Help me escape!",
                    "Try moving to the next door",
                    "Still here? Go on...",
                    "Hint: choose door 1"
                };
                lvl.npcs.push_back(NPC(350, 700, dialogues));

                lvl.lights.push_back(Light(600, 200));
                lvl.lights.push_back(Light(200, 300));
            }
            else if (i == 1) {
                // Level 2 - Jump ability
                lvl.platforms.push_back({ {500, 500, 200, 20}, true });
                lvl.platforms.push_back({ {350, 450, 150, 20}, false });

                lvl.player_start = { 250, 660 };
                lvl.player_abilities["jump"] = true;

                lvl.doors.push_back(Door(850, 630, 0, "2"));
                lvl.doors.push_back(Door(950, 630, 2, "1"));

                std::map<std::string, std::vector<std::string>> dialogues;
                dialogues["default"] = {
                    "You can jump now!",
                    "Press SPACE or W",
                    "Next floor choose door 2"
                };
                lvl.npcs.push_back(NPC(350, 700, dialogues));

                lvl.lights.push_back(Light(600, 200));
            }
            else if (i == 3) {
                // Level 4 - Double jump
                lvl.platforms.push_back({ {500, 500, 200, 20}, true });
                lvl.platforms.push_back({ {650, 500, 50, 200}, true });
                lvl.platforms.push_back({ {500, 250, 50, 250}, true });

                lvl.player_start = { 250, 660 };
                lvl.player_abilities["jump"] = true;
                lvl.player_abilities["double_jump"] = true;

                lvl.doors.push_back(Door(850, 630, 2, "2"));
                lvl.doors.push_back(Door(950, 630, 4, "1"));

                std::map<std::string, std::vector<std::string>> dialogues;
                dialogues["default"] = {
                    "Double jump unlocked!",
                    "Jump twice to reach new heights"
                };
                lvl.npcs.push_back(NPC(350, 700, dialogues));
            }
            else if (i == 5) {
                // Level 6 - Fireball
                lvl.platforms.push_back({ {500, 500, 200, 20}, true });
                lvl.platforms.push_back({ {650, 500, 50, 200}, true });

                lvl.player_start = { 250, 660 };
                lvl.player_abilities["jump"] = true;
                lvl.player_abilities["double_jump"] = true;
                lvl.player_abilities["fireball"] = true;

                lvl.doors.push_back(Door(850, 630, 4, "2"));
                lvl.doors.push_back(Door(950, 630, 6, "1"));
                lvl.doors[1].locked = true;

                lvl.breakable_boxes.push_back(BreakableBox(580, 630));
                lvl.breakable_boxes.push_back(BreakableBox(200, 530));
                lvl.breakable_boxes.push_back(BreakableBox(550, 430, true));

                std::map<std::string, std::vector<std::string>> dialogues;
                dialogues["default"] = {
                    "Press F to cast fireballs!",
                    "Break boxes to find the key"
                };
                lvl.npcs.push_back(NPC(350, 700, dialogues));
            }

            // Add default platforms/doors for other levels
            if (i > 1 && i != 3 && i != 5) {
                lvl.platforms.push_back({ {500, 500, 200, 20}, true });
                lvl.doors.push_back(Door(850, 630, (i - 1) % 8, "2"));
                lvl.doors.push_back(Door(950, 630, (i + 1) % 8, "1"));

                std::map<std::string, std::vector<std::string>> dialogues;
                dialogues["default"] = { "Keep going..." };
                lvl.npcs.push_back(NPC(350, 700, dialogues));
            }

            levels.push_back(lvl);
        }
    }

    void start_level(int level_index) {
        if (level_index >= 0 && level_index < levels.size()) {
            level = &levels[level_index];
            player.set_position(level->player_start.first, level->player_start.second);
            player.set_abilities(level->player_abilities);
            current_level = level_index;
            state = GameState::PLAYING;
        }
    }

    void start_transition(int target_level) {
        sound_system.stop_walking();

        transition.start_level = current_level;
        transition.target_level = target_level;
        from_level = current_level;

        // Create transition surfaces (simplified)
        transition.phase = "swipe";
        transition.progress = 0.0f;
        transition.offset_x = 0;

        start_level(target_level);
        state = GameState::TRANSITIONING;
    }

    void update_transition() {
        float speed = 0.02f;
        if (transition.phase == "swipe") {
            transition.progress += speed;
            transition.offset_x = transition.progress * SCREEN_WIDTH;
            if (transition.progress >= 1.0f) {
                state = GameState::PLAYING;
            }
        }
    }

    void handle_events() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (state == GameState::MENU) {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    std::string action = menu.handle_click(x, y);

                    if (action == "start") {
                        start_level(0);
                    }
                    else if (action == "quit") {
                        running = false;
                    }
                }
            }
        }
    }

    void update() {
        // Frame timing
        Uint64 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        if (state == GameState::MENU) {
            menu.update(mouse_x, mouse_y);
        }
        else if (state == GameState::PLAYING) {
            player.update(level->platforms, mouse_x, mouse_y, level->breakable_boxes);
            level->update(player, from_level);

            // Check E key for NPC interaction
            const bool* keys = SDL_GetKeyboardState(nullptr);
            if (keys[SDL_SCANCODE_E]) {
                for (auto& npc : level->npcs) {
                    if (npc.show_prompt) {
                        npc.interact(from_level, current_level);
                    }
                }
            }

            // Check door collisions
            for (auto& door : level->doors) {
                if (!door.locked && SDL_HasRectIntersectionFloat(&player.rect, &door.rect)) {
                    if (door.target_level == -1) {
                        // Exit door - go to ending
                        sound_system.stop_walking();
                        state = GameState::ENDING;
                        ending_screen = EndingScreen();
                    }
                    else {
                        start_transition(door.target_level);
                    }
                    break;
                }
            }
        }
        else if (state == GameState::TRANSITIONING) {
            update_transition();
        }
        else if (state == GameState::ENDING) {
            if (ending_screen.update()) {
                state = GameState::MENU;
                menu = Menu();
            }
        }
    }

    void draw() {
        draw.color(0, 0, 0);
        draw.clear();

        if (state == GameState::MENU) {
            menu.draw(draw, renderer);
        }
        else if (state == GameState::PLAYING) {
            level->draw_background(draw);
            level->draw_platforms(draw);

            for (auto& box : level->breakable_boxes) {
                box.draw(draw);
            }

            for (auto& door : level->doors) {
                door.draw(draw, renderer);
            }

            for (auto& npc : level->npcs) {
                npc.draw(draw, renderer);
            }

            player.draw(draw);

            // Draw UI
            if (player.can_fireball) {
                // Crosshair
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                draw.color(WHITE.r, WHITE.g, WHITE.b, 100);
                draw.circle(mouse_x, mouse_y, 8);
                draw.line(mouse_x - 10, mouse_y, mouse_x + 10, mouse_y);
                draw.line(mouse_x, mouse_y - 10, mouse_x, mouse_y + 10);
            }

            // Abilities text
            int ui_y = 20;
            draw.color(LIGHT_GRAY.r, LIGHT_GRAY.g, LIGHT_GRAY.b);

            if (player.double_jump_available) {
                SDL_RenderDebugText(renderer, 20, ui_y, "Double Jump");
                ui_y += 25;
            }

            if (player.can_fireball) {
                SDL_RenderDebugText(renderer, 20, ui_y, "Light: F");
                ui_y += 25;
            }

            if (player.keys > 0) {
                draw.color(WHITE.r, WHITE.g, WHITE.b);
                SDL_RenderDebugTextFormat(renderer, 20, ui_y, "Keys: %d", player.keys);
            }

            // Hint
            draw.color(LIGHT_GRAY.r, LIGHT_GRAY.g, LIGHT_GRAY.b, 100);
            SDL_RenderDebugText(renderer, 20, SCREEN_HEIGHT - 30, "S: Drop");

        }
        else if (state == GameState::TRANSITIONING) {
            // Simple transition effect
            draw.color(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b);
            draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            // Draw sliding levels (simplified)
            float old_x = -transition.offset_x;
            float new_x = SCREEN_WIDTH - transition.offset_x;

            // Just show a sliding effect
            draw.color(SILHOUETTE.r, SILHOUETTE.g, SILHOUETTE.b);
            draw.fill_rect(old_x, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            draw.fill_rect(new_x, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        }
        else if (state == GameState::ENDING) {
            ending_screen.draw(draw, renderer);
        }

        draw.present();
    }

    void run() {
        while (running) {
            handle_events();
            update();
            draw();

            // Frame limiting
            SDL_Delay(16);  // ~60 FPS
        }
    }

    ~Game() {
        sound_system.cleanup();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    Game game;
    game.run();
    return 0;
}