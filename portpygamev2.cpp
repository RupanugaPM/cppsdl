// main.cpp - COMPLETE SDL3 Limbo-style game implementation
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <array>
#include <sstream>
#include "renderer2d.cpp"

// Constants
constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 800;
constexpr int FPS = 60;
constexpr float GRAVITY = 0.8f;
constexpr float JUMP_STRENGTH = -15.0f;
constexpr float PLAYER_SPEED = 5.0f;
constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;

// Forward declarations
class Player;
class Level;
class NPC;
class Door;
class BreakableBox;
class Menu;
class EndingScreen;
class Fireball;

// Enums
enum class GameState {
    MENU,
    PLAYING,
    LEVEL_COMPLETE,
    TRANSITIONING,
    ENDING
};

// Random number generators
std::random_device rd;
std::mt19937 gen(rd());

float random_float(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

int random_int(int min, int max) {
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

// ===== AUDIO MANAGER =====
class AudioManager {
private:
    SDL_AudioDeviceID audio_device;
    std::unordered_map<std::string, SDL_AudioStream*> sound_streams;
    std::unordered_map<std::string, std::vector<Uint8>> sound_data;
    SDL_AudioStream* music_stream;
    SDL_AudioStream* walking_stream;
    bool walking_playing;

public:
    AudioManager() : audio_device(0), music_stream(nullptr), walking_stream(nullptr), walking_playing(false) {}

    ~AudioManager() {
        cleanup();
    }

    bool init() {
        audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!audio_device) {
            SDL_Log("Failed to open audio device: %s", SDL_GetError());
            return false;
        }
        return true;
    }

    void cleanup() {
        stopWalking();

        for (auto& [name, stream] : sound_streams) {
            if (stream) {
                SDL_DestroyAudioStream(stream);
            }
        }
        sound_streams.clear();
        sound_data.clear();

        if (music_stream) {
            SDL_DestroyAudioStream(music_stream);
            music_stream = nullptr;
        }

        if (walking_stream) {
            SDL_DestroyAudioStream(walking_stream);
            walking_stream = nullptr;
        }

        if (audio_device) {
            SDL_CloseAudioDevice(audio_device);
            audio_device = 0;
        }
    }

    bool loadSound(const std::string& name, const std::string& filepath) {
        // Create dummy sound for now - in production, load actual WAV files
        SDL_AudioSpec spec;
        spec.freq = 44100;
        spec.format = SDL_AUDIO_S16;
        spec.channels = 2;

        SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &spec);
        if (!stream) return false;

        SDL_BindAudioStream(audio_device, stream);
        sound_streams[name] = stream;

        // Generate simple sound data (sine wave for testing)
        std::vector<Uint8> data(44100); // 1 second of audio
        for (size_t i = 0; i < data.size(); i += 2) {
            Sint16 sample = static_cast<Sint16>(std::sin(i * 0.01f) * 1000);
            data[i] = sample & 0xFF;
            data[i + 1] = (sample >> 8) & 0xFF;
        }
        sound_data[name] = data;

        return true;
    }

    void playSound(const std::string& name, float volume = 1.0f) {
        auto it = sound_streams.find(name);
        if (it != sound_streams.end() && it->second) {
            SDL_SetAudioStreamGain(it->second, volume);
            auto& data = sound_data[name];
            SDL_PutAudioStreamData(it->second, data.data(), static_cast<int>(data.size()));
        }
    }

    void startWalking(float volume = 0.4f) {
        if (!walking_playing) {
            playSound("walk", volume);
            walking_playing = true;
        }
    }

    void stopWalking() {
        if (walking_playing) {
            auto it = sound_streams.find("walk");
            if (it != sound_streams.end() && it->second) {
                SDL_ClearAudioStream(it->second);
            }
            walking_playing = false;
        }
    }

    void playMusic(const std::string& name, float volume = 0.4f) {
        stopMusic();
        playSound(name, volume);
    }

    void stopMusic() {
        if (music_stream) {
            SDL_ClearAudioStream(music_stream);
        }
    }

    void fadeOutMusic(int ms) {
        // Simplified - in production would implement actual fade
        stopMusic();
    }
};

// ===== PARTICLE SYSTEM =====
class DustParticle {
public:
    float x, y;
    float vx, vy;
    float life;
    float size;

    DustParticle(float px, float py)
        : x(px), y(py), life(1.0f) {
        vx = random_float(-0.5f, 0.5f);
        vy = random_float(-1.0f, -0.5f);
        size = random_float(2, 4);
    }

    void update() {
        x += vx;
        y += vy;
        life -= 0.02f;
        vy += 0.02f; // gravity
    }

    void draw(Draw& draw) {
        if (life > 0) {
            int alpha = static_cast<int>(100 * life);
            draw.color(120, 120, 120, alpha);
            draw.fill_circle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(size));
        }
    }
};

class FogParticle {
public:
    float x, y;
    float size;
    float speed;
    float opacity;
    float phase;

    FogParticle(float px, float py)
        : x(px), y(py) {
        size = static_cast<float>(random_int(50, 150));
        speed = random_float(0.2f, 0.5f);
        opacity = static_cast<float>(random_int(20, 60));
        phase = random_float(0, PI * 2);
    }

    void update() {
        x += speed;
        phase += 0.01f;
        y += std::sin(phase) * 0.3f;

        if (x > SCREEN_WIDTH + size) {
            x = -size;
            y = static_cast<float>(random_int(0, SCREEN_HEIGHT));
        }
    }

    void draw(Draw& draw) {
        for (int i = static_cast<int>(size); i > 0; i -= 5) {
            int alpha = static_cast<int>(opacity * (i / size));
            draw.color(200, 200, 200, alpha);
            draw.fill_circle(static_cast<int>(x), static_cast<int>(y), i);
        }
    }
};

// ===== FIREBALL CLASS =====
class Fireball {
public:
    SDL_FRect rect;
    float vel_x, vel_y;
    std::vector<DustParticle> particles;
    bool alive;
    int life;

    Fireball(float x, float y, float target_x, float target_y)
        : rect{ x, y, 16, 16 }, alive(true), life(60) {
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
    }

    void update(const std::vector<SDL_FRect>& platforms, std::vector<BreakableBox*>& boxes);
    void explode();
    void draw(Draw& draw);
};

// ===== BREAKABLE BOX CLASS =====
class BreakableBox {
public:
    SDL_FRect rect;
    bool has_key;
    bool broken;
    bool key_collected;
    float key_y_offset;
    float key_float_phase;
    bool is_special_flag;
    std::vector<DustParticle> particles;

    BreakableBox(float x, float y, bool key = false, bool special = false)
        : rect{ x, y, 70, 70 }, has_key(key), broken(false),
        key_collected(false), key_y_offset(0),
        is_special_flag(special) {
        key_float_phase = random_float(0, PI * 2);
    }

    void break_box() {
        if (!broken) {
            broken = true;
            for (int i = 0; i < 4; ++i) {
                float angle = random_float(0, PI * 2);
                float speed = random_float(2, 5);
                DustParticle particle(rect.x + rect.w / 2, rect.y + rect.h / 2);
                particle.vx = std::cos(angle) * speed;
                particle.vy = std::sin(angle) * speed - 2;
                particles.push_back(particle);
            }
        }
    }

    void update() {
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.life <= 0; }),
            particles.end()
        );

        for (auto& particle : particles) {
            particle.update();
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
        for (auto& particle : particles) {
            particle.draw(draw);
        }

        if (!broken) {
            draw.color(0, 0, 0); // Silhouette
            draw.fill_rect(rect.x, rect.y, rect.w, rect.h);
            draw.color(20, 20, 20); // Subtle highlight
            draw.rect(rect.x, rect.y, rect.w, rect.h);
        }
        else if (has_key && !key_collected) {
            float key_x = rect.x + rect.w / 2;
            float key_y = rect.y + rect.h / 2 - 20 + key_y_offset;

            // Glowing key
            for (int i = 10; i > 0; i -= 2) {
                int alpha = 120 * (i / 20);
                draw.color(255, 255, 255, alpha);
                draw.fill_circle(static_cast<int>(key_x), static_cast<int>(key_y), i);
            }

            // Key silhouette
            draw.color(0, 0, 0);
            draw.fill_circle(static_cast<int>(key_x), static_cast<int>(key_y), 6);
            draw.fill_rect(key_x - 2, key_y, 4, 12);
            draw.fill_rect(key_x - 2, key_y + 8, 6, 2);
            draw.fill_rect(key_x - 2, key_y + 11, 4, 2);
        }
    }
};

// ===== NPC CLASS =====
class NPC {
public:
    SDL_FRect rect;
    float x, y;
    std::unordered_map<std::string, std::vector<std::string>> dialogues;
    float bob_phase;
    bool show_prompt;
    std::string current_dialogue;
    int dialogue_timer;
    bool talking;
    float gesture_timer;
    bool facing_player;
    float arm_animation;
    std::unordered_map<std::string, size_t> dialogue_indices;
    int interaction_cooldown;

    NPC(float nx, float ny, std::unordered_map<std::string, std::vector<std::string>> dlg)
        : x(nx), y(ny), dialogues(dlg), bob_phase(random_float(0, PI * 2)),
        show_prompt(false), dialogue_timer(0), talking(false),
        gesture_timer(0), facing_player(false), arm_animation(0),
        interaction_cooldown(0) {
        rect = { x, y - 45, 28, 45 };
    }

    void update(const SDL_FRect& player_rect, int from_level) {
        bob_phase += 0.05f;
        rect.y = y - 45 + std::sin(bob_phase) * 2;

        float distance = std::sqrt(
            std::pow(player_rect.x + player_rect.w / 2 - rect.x - rect.w / 2, 2) +
            std::pow(player_rect.y + player_rect.h / 2 - rect.y - rect.h / 2, 2)
        );
        show_prompt = distance < 60;

        if (show_prompt) {
            facing_player = player_rect.x + player_rect.w / 2 > rect.x + rect.w / 2;
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

        // Draw NPC body as silhouette
        draw.color(0, 0, 0);

        // Head (hood-like shape)
        std::vector<SDL_FPoint> head_points = {
            {cx - 8, rect.y + 8},
            {cx - 6, rect.y + 2},
            {cx, rect.y},
            {cx + 6, rect.y + 2},
            {cx + 8, rect.y + 8},
            {cx + 7, rect.y + 14},
            {cx - 7, rect.y + 14}
        };
        draw.polygon(head_points);

        // Body (cloak/robe)
        std::vector<SDL_FPoint> body_points = {
            {cx - 7, rect.y + 14},
            {cx + 7, rect.y + 14},
            {cx + 10, rect.y + 25},
            {cx + 12, rect.y + rect.h - 2},
            {cx - 12, rect.y + rect.h - 2},
            {cx - 10, rect.y + 25}
        };
        draw.polygon(body_points);

        // Arms based on talking state
        if (talking) {
            if (facing_player) {
                // Right arm gesturing
                float gesture_angle = arm_animation;
                draw.lines({ {cx + 7, rect.y + 20},
                           {cx + 12 + gesture_angle * 0.3f, rect.y + 24},
                           {cx + 14 + gesture_angle * 0.5f, rect.y + 22 - std::abs(gesture_angle) * 0.2f} });
            }
            else {
                // Left arm gesturing
                float gesture_angle = arm_animation;
                draw.lines({ {cx - 7, rect.y + 20},
                           {cx - 12 - gesture_angle * 0.3f, rect.y + 24},
                           {cx - 14 - gesture_angle * 0.5f, rect.y + 22 - std::abs(gesture_angle) * 0.2f} });
            }
        }

        // Show interaction prompt
        if (show_prompt && dialogue_timer <= 0) {
            // Glowing E prompt
            for (int i = 15; i > 0; i -= 3) {
                int alpha = 80 * (i / 15);
                draw.color(255, 255, 255, alpha);
                draw.fill_circle(static_cast<int>(cx), static_cast<int>(rect.y - 35), i);
            }

            // E text box
            draw.color(0, 0, 0);
            draw.fill_rect(cx - 12, rect.y - 47, 24, 24);
            draw.color(255, 255, 255);
            draw.rect(cx - 10, rect.y - 45, 20, 20);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDebugText(renderer, cx - 5, rect.y - 40, "E");
        }

        // Show dialogue bubble
        if (dialogue_timer > 0 && !current_dialogue.empty()) {
            int alpha = std::min(255, dialogue_timer * 8);
            if (dialogue_timer > 150) alpha = 255;

            // Calculate bubble size based on text
            int text_width = static_cast<int>(current_dialogue.length() * 8);
            text_width = std::min(400, std::max(100, text_width));
            int bubble_height = 60;

            float bubble_x = cx - text_width / 2;
            float bubble_y = rect.y - bubble_height - 20;

            // Speech bubble background
            draw.color(255, 255, 255, static_cast<int>(alpha * 0.9f));
            draw.fill_rect(bubble_x, bubble_y, static_cast<float>(text_width), static_cast<float>(bubble_height));
            draw.color(0, 0, 0, alpha);
            draw.rect(bubble_x, bubble_y, static_cast<float>(text_width), static_cast<float>(bubble_height));

            // Bubble tail
            std::vector<SDL_FPoint> tail_points = {
                {cx - 10, bubble_y + bubble_height},
                {cx + 10, bubble_y + bubble_height},
                {cx, bubble_y + bubble_height + 10}
            };
            draw.color(255, 255, 255, static_cast<int>(alpha * 0.9f));
            draw.polygon(tail_points);

            // Render dialogue text with word wrap
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
            int chars_per_line = text_width / 8;
            int lines = 0;
            size_t pos = 0;

            while (pos < current_dialogue.length() && lines < 3) {
                size_t end = std::min(pos + chars_per_line, current_dialogue.length());
                // Find last space for word wrap
                if (end < current_dialogue.length()) {
                    size_t last_space = current_dialogue.rfind(' ', end);
                    if (last_space != std::string::npos && last_space > pos) {
                        end = last_space;
                    }
                }

                std::string line = current_dialogue.substr(pos, end - pos);
                SDL_RenderDebugText(renderer, bubble_x + 10, bubble_y + 10 + lines * 15, line.c_str());
                pos = end;
                if (pos < current_dialogue.length() && current_dialogue[pos] == ' ') pos++;
                lines++;
            }
        }
    }
};

// ===== DOOR CLASS =====
class Door {
public:
    SDL_FRect rect;
    int target_level;
    std::string label;
    float glow_timer;
    bool locked;
    std::vector<DustParticle> particles;

    Door(float x, float y, int target, const std::string& lbl = "", bool lock = false)
        : rect{ x, y, 50, 70 }, target_level(target), label(lbl),
        glow_timer(0), locked(lock) {
    }

    void update() {
        glow_timer += 0.05f;

        if (!locked && random_float(0, 1) < 0.02f) {
            DustParticle particle(
                rect.x + rect.w / 2 + random_float(-15, 15),
                rect.y + random_float(0, rect.h)
            );
            particle.vy -= 0.5f;
            particles.push_back(particle);
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.life <= 0; }),
            particles.end()
        );

        for (auto& particle : particles) {
            particle.update();
            particle.vy -= 0.1f;
        }
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        for (auto& particle : particles) {
            particle.draw(draw);
        }

        if (!locked) {
            // Glow effect
            float glow_intensity = (std::sin(glow_timer) + 1) * 0.3f;
            for (int i = 20; i > 0; i -= 4) {
                int alpha = static_cast<int>(100 * glow_intensity * (i / 20.0f));
                draw.color(255, 255, 255, alpha);
                draw.rect(rect.x - i / 2, rect.y - i / 2, rect.w + i, rect.h + i);
            }
        }

        // Door body
        draw.color(0, 0, 0);
        draw.fill_rect(rect.x, rect.y, rect.w, rect.h);
        draw.color(40, 40, 40);
        draw.rect(rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10);

        if (locked) {
            // Lock symbol
            draw.color(40, 40, 40);
            draw.fill_rect(rect.x + rect.w / 2 - 8, rect.y + rect.h / 2 - 8, 16, 16);
            draw.color(0, 0, 0);
            draw.fill_circle(static_cast<int>(rect.x + rect.w / 2),
                static_cast<int>(rect.y + rect.h / 2), 3);
        }
        else {
            // Door handle
            draw.color(40, 40, 40);
            draw.fill_circle(static_cast<int>(rect.x + rect.w - 12),
                static_cast<int>(rect.y + rect.h / 2), 4);
        }

        // Label
        if (!label.empty()) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDebugText(renderer, rect.x + rect.w / 2 - label.length() * 4,
                rect.y - 25, label.c_str());
        }
    }
};

// ===== PLAYER CLASS =====
class Player {
public:
    SDL_FRect rect;
    float vel_x, vel_y;
    bool on_ground;
    bool on_drop_platform;
    bool dropping;
    int drop_timer;
    bool drop_key_pressed;
    std::vector<DustParticle> particles;

    // Animation states
    std::string animation_state;
    int animation_timer;
    float walk_cycle;
    int land_timer;
    float idle_timer;
    bool facing_right;
    float head_offset;
    float arm_swing;
    float leg_spread;

    // Abilities
    std::unordered_map<std::string, bool> abilities;
    bool jump_available;
    bool double_jump_available;
    bool can_double_jump;
    bool jump_pressed;
    bool can_fireball;
    std::vector<Fireball> fireballs;
    int fireball_cooldown;

    // Keys
    int keys;

    // Level reference for collision with breakable boxes
    Level* current_level;

    Player(float x, float y)
        : rect{ x, y, 24, 36 }, vel_x(0), vel_y(0), on_ground(false),
        on_drop_platform(false), dropping(false), drop_timer(0),
        drop_key_pressed(false), animation_state("idle"),
        animation_timer(0), walk_cycle(0), land_timer(0),
        idle_timer(0), facing_right(true), head_offset(0),
        arm_swing(0), leg_spread(0), jump_available(false),
        double_jump_available(false), can_double_jump(false),
        jump_pressed(false), can_fireball(false),
        fireball_cooldown(0), keys(0), current_level(nullptr) {
    }

    void set_position(float x, float y) {
        rect.x = x;
        rect.y = y;
    }

    void set_abilities(const std::unordered_map<std::string, bool>& abs) {
        for(auto &e:abs){
            abilities[e.first] = e.second;
        }
        jump_available = abilities["jump"];
        double_jump_available = abilities["double_jump"];
        can_fireball = abilities["fireball"];
    }

    void update(const std::vector<SDL_FRect>& platforms,
        const std::vector<bool>& solid_flags,
        std::vector<BreakableBox>& boxes,
        float mouse_x, float mouse_y,
        AudioManager& audio);

    void check_collisions(const std::vector<SDL_FRect>& platforms,
        const std::vector<bool>& solid_flags,
        char direction);

    void draw(Draw& draw);
};

// ===== LEVEL CLASS =====
class Level {
public:
    int level_number;
    std::vector<SDL_FRect> platforms;
    std::vector<bool> platform_solid;
    std::pair<float, float> player_start;
    std::vector<Door> doors;
    std::vector<BreakableBox> breakable_boxes;
    std::vector<NPC> npcs;
    std::unordered_map<std::string, bool> player_abilities;
    int keys_required;
    std::vector<FogParticle> fog_particles;
    bool lift_blur;
    std::vector<std::pair<float, float>> lights; // Light positions

    Level(int num) : level_number(num), player_start(100, 400),
        keys_required(0), lift_blur(false) {
        // Initialize fog
        for (int i = 0; i < 4; ++i) {
            fog_particles.push_back(FogParticle(
                static_cast<float>(random_int(-200, SCREEN_WIDTH)),
                static_cast<float>(random_int(0, SCREEN_HEIGHT))
            ));
        }

        // Initialize abilities defaults
        /*player_abilities["jump"] = false;
        player_abilities["double_jump"] = false;
        player_abilities["fireball"] = false;*/

        load_level();
    }

    void load_level() {
        // Clear existing data
        platforms.clear();
        platform_solid.clear();
        doors.clear();
        breakable_boxes.clear();
        npcs.clear();
        lights.clear();

        switch (level_number) {
        case 0: // Level 1 - Starting level
        {
            // Platforms
            platforms.push_back({ 0, 0, 1200, 150 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 150, 150, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 700, 1200, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 80 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 300, 200, 450 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 200, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false); // Drop-through

            player_start = { 250, 660 };

            // Doors
            doors.push_back(Door(950, 630, 1, ""));
            doors.push_back(Door(0, 230, -1, "Exit", true)); // Locked exit door

            // Breakable boxes
            breakable_boxes.push_back(BreakableBox(130, 230, true, true)); // Has key and special flag

            // Lights
            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            // NPC
            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "Hey there, sorry for summoning you but I am stuck in this dungeon",
                "I did have my summoning magic which I used...",
                "So you got summoned, now help me.......",
                "You can't do anything right now can you?",
                "Try moving to the next door",
                "Still here? Don't you want to get out of here?",
                "Go on...",
                "...",
                "....",
                "......",
                "Ok fine here is the hint for the next floor... choose door 1"
            };
            npc_dialogues["from_1"] = {
                "I told you to jump,,, you came back now...",
                "Try to break free",
                "...",
                "....",
                "......",
                "Ok fine here is the hint for the next floor... choose door 1"
            };
            npc_dialogues["from_7"] = {
                "That option was wrong too?",
                "We are back I guess to square 1",
                "top left looks suspiciously like floor 7s crack",
                "maybe try throwing a fireball",
                "or maybe not...",
                "..",
                "...."
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            // Abilities - none for level 0
            break;
        }

        case 1: // Level 2 - Jump ability
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 0, "2"));
            doors.push_back(Door(950, 630, 2, "1"));

            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "Ok.. you can jump really high now, use that",
                "Press SPACE or w to defy gravity.",
                "That's it",
                ".",
                "..",
                "...",
                "....",
                "Ok you got me again..",
                "In the next floor the correct door is 2"
            };
            npc_dialogues["from_10"] = {
                "You've taken your first steps.",
                "This power is yours now - jumping.",
                "But greater challenges await ahead."
            };
            npc_dialogues["from_20"] = {
                "Running from what lies ahead?",
                "The double jump proved too much?",
                "Sometimes retreat is wisdom."
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            player_abilities["jump"] = true;
            break;
        }

        case 2: // Level 3
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 600, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 500, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 3, "2"));
            doors.push_back(Door(950, 630, 1, "1"));

            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "The dungeon is unique...",
                "There are total 8 floors, but I have only reached till 7",
                ".",
                "..",
                "...",
                "You want the hint again?",
                "Fine,,, in the next floor go to door 1"
            };
            npc_dialogues["from_0"] = {
                "Such a long journey from the start...",
                "You've skipped many trials to reach here.",
                "Impressive, but dangerous.",
                ".",
                "..",
                "...",
                "You want the hint again?",
                "Fine,,, in the next floor go to door 1"
            };
            npc_dialogues["from_3"] = {
                "Jumped a bit too high huh?.",
                "Remember this floor you need to choose door 2.",
                ".",
                "..",
                "...",
                "You want the hint again?",
                "Fine,,, in the next floor go to door 1"
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));
            break;
        }

        case 3: // Level 4 - Double Jump
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 600, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 500, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 250, 50, 250 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 2, "2"));
            doors.push_back(Door(950, 630, 4, "1"));

            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "You've gained new strength. Jump twice, shadow walker.",
                "Ok I am sorry that was cringe.",
                "This power will help you reach new heights... If you get my pun",
                "...",
                "....",
                "Yeah that was not funny",
                "Next floor choose door 2"
            };
            npc_dialogues["from_0"] = {
                "Such a long journey from the start...",
                "You've skipped many trials to reach here.",
                "Impressive, but dangerous."
            };
            npc_dialogues["from_4"] = {
                "...",
                "....",
                "Next floor choose door 2"
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            player_abilities["jump"] = true;
            player_abilities["double_jump"] = true;
            break;
        }

        case 4: // Level 5
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 600, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 500, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 250, 50, 250 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 300, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 700, 500, 300, 20 }); platform_solid.push_back(false);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 5, "2"));
            doors.push_back(Door(950, 630, 3, "1"));

            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "Go on",
                "This floor is pretty simple...",
                "You dont need more hints",
                "...",
                "..",
                "Fine this is the last hint any ways.. next floor choose 1"
            };
            npc_dialogues["from_0"] = {
                "Such a long journey from the start...",
                "You've skipped many trials to reach here.",
                "Impressive, but dangerous."
            };
            npc_dialogues["from_5"] = {
                "So foolish,... "
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            player_abilities["jump"] = true;
            player_abilities["double_jump"] = true;
            break;
        }

        case 5: // Level 6 - Fireball
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 600, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 500, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 250, 50, 250 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 300, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 700, 500, 300, 20 }); platform_solid.push_back(false);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 4, "2"));
            doors.push_back(Door(950, 630, 6, "1", true)); // Locked door

            lights.push_back({ 300, 200 });
            lights.push_back({ 600, 200 });
            lights.push_back({ 900, 200 });

            // Breakable boxes
            breakable_boxes.push_back(BreakableBox(580, 630, false, false));
            breakable_boxes.push_back(BreakableBox(200, 530, false, false));
            breakable_boxes.push_back(BreakableBox(550, 430, true, false)); // Has key

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "Light can shatter darkness. Press F to cast.",
                "Aim with your mouse, click F to fire.",
                "Break the boxes to find the key.",
                "...",
                "....",
                "......",
                "I have already told you right I have never gone past the next floor",
                "But maybe you see the pattern already?"
            };
            npc_dialogues["from_2"] = {
                "You've come to face the final challenge.",
                "The power of light is yours now.",
                "Use it to unlock your path home.",
                "...",
                "....",
                "......",
                "I have already told you right I have never gone past the next floor"
            };
            npc_dialogues["from_6"] = {
                "So this was the wrong choice huh?",
                "Maybe try going through the other door",
                "I never expected you to cross the next floor too"
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            player_abilities["jump"] = true;
            player_abilities["double_jump"] = true;
            player_abilities["fireball"] = true;
            break;
        }

        case 6: // Level 7
        {
            platforms.push_back({ 0, 700, 1200, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 0, 50, 300 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 0, 1200, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 1150, 0, 50, 300 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 80 }); platform_solid.push_back(true);
            platforms.push_back({ 0, 300, 200, 450 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 80 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 300, 200, 450 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 600, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 500, 200, 20 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 500, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 500, 250, 50, 250 }); platform_solid.push_back(true);
            platforms.push_back({ 650, 300, 50, 200 }); platform_solid.push_back(true);
            platforms.push_back({ 550, 200, 150, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);
            platforms.push_back({ 700, 500, 300, 20 }); platform_solid.push_back(false);
            platforms.push_back({ 350, 450, 150, 20 }); platform_solid.push_back(false);

            player_start = { 250, 660 };

            doors.push_back(Door(850, 630, 7, "2"));
            doors.push_back(Door(950, 630, 5, "1", true)); // Locked

            lights.push_back({ 300, 200 });
            lights.push_back({ 600, 200 });
            lights.push_back({ 900, 200 });

            // Breakable boxes
            breakable_boxes.push_back(BreakableBox(580, 630, false, false));
            breakable_boxes.push_back(BreakableBox(200, 530, false, false));
            breakable_boxes.push_back(BreakableBox(550, 430, true, false)); // Has key
            breakable_boxes.push_back(BreakableBox(130, 230, true, true)); // Has key and special flag
            breakable_boxes.push_back(BreakableBox(1000, 230, false, false));

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "I always thought that the top left corner of this floor looks suspicious",
                "Maybe a fireball would do?",
                "BAaahh, staying in this dungeon is making me go crazy.",
                "..",
                "...",
                "Dont do it,, we might get buried alive!!"
            };
            npc_dialogues["from_2"] = {
                "You've come to face the final challenge.",
                "The power of light is yours now.",
                "Use it to unlock your path home."
            };
            npc_dialogues["from_7"] = {
                "So that door was wrong huh",
                "Maybe the other door??",
                "Perhaps we can be free soon..."
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));

            player_abilities["jump"] = true;
            player_abilities["double_jump"] = true;
            player_abilities["fireball"] = true;
            break;
        }

        case 7: // Level 8 - Final decision
        {
            platforms.push_back({ 150, 700, 900, 100 }); platform_solid.push_back(true);
            platforms.push_back({ 150, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 1000, 150, 50, 600 }); platform_solid.push_back(true);
            platforms.push_back({ 200, 150, 850, 50 }); platform_solid.push_back(true);

            player_start = { 250, 660 };

            doors.push_back(Door(500, 630, 6, "2"));
            doors.push_back(Door(630, 630, 0, "1"));

            lights.push_back({ 600, 200 });
            lights.push_back({ 200, 300 });
            lights.push_back({ 1000, 250 });

            std::unordered_map<std::string, std::vector<std::string>> npc_dialogues;
            npc_dialogues["default"] = {
                "I never came this far...",
                "",
                "Try going through one of the door,,,"
            };
            npc_dialogues["from_0"] = {
                "You've taken your first steps.",
                "This power is yours now - jumping.",
                "But greater challenges await ahead."
            };
            npc_dialogues["from_2"] = {
                "Running from what lies ahead?",
                "The double jump proved too much?",
                "Sometimes retreat is wisdom."
            };

            npcs.push_back(NPC(350, 700, npc_dialogues));
            break;
        }
        }
    }

    void update(Player& player, int from_level, AudioManager& audio) {
        // Update fog
        for (auto& fog : fog_particles) {
            fog.update();
        }

        // Update doors
        for (auto& door : doors) {
            door.update();
        }

        // Update boxes
        for (auto& box : breakable_boxes) {
            box.update();
            if (box.is_special_flag && box.broken) {
                lift_blur = true;
            }

            // Check key collection
            if (box.broken && box.has_key && !box.key_collected) {
                SDL_FRect player_rect = player.rect;
                SDL_FRect box_rect = box.rect;
                if (std::abs(player_rect.x + player_rect.w / 2 - box_rect.x - box_rect.w / 2) < 30 &&
                    std::abs(player_rect.y + player_rect.h / 2 - box_rect.y - box_rect.h / 2) < 30) {
                    if (box.collect_key()) {
                        player.keys++;
                    }
                }
            }
        }

        // Update NPCs
        for (auto& npc : npcs) {
            npc.update(player.rect, from_level);
        }

        // Check door unlocking
        for (auto& door : doors) {
            if (door.locked && player.keys > 0) {
                door.locked = false;
                player.keys--;
            }
        }
    }

    void draw_background(Draw& draw) {
        // Gradient background
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            float ratio = y / static_cast<float>(SCREEN_HEIGHT);
            int gray = static_cast<int>(180 * (1 - ratio * 0.3f));
            draw.color(gray, gray, gray);
            draw.line(0, static_cast<float>(y), SCREEN_WIDTH, static_cast<float>(y));
        }

        // Fog particles
        for (auto& fog : fog_particles) {
            fog.draw(draw);
        }
    }

    void draw_platforms(Draw& draw) {
        for (size_t i = 0; i < platforms.size(); ++i) {
            const auto& platform = platforms[i];
            bool is_solid = platform_solid[i];

            if (is_solid) {
                draw.color(0, 0, 0); // Silhouette
                draw.fill_rect(platform.x, platform.y, platform.w, platform.h);
                draw.color(40, 40, 40);
                draw.line(platform.x, platform.y, platform.x + platform.w, platform.y);
            }
            else {
                // Drop-through platform
                draw.color(0, 0, 0, 180);
                draw.fill_rect(platform.x, platform.y, platform.w, 8);
                draw.color(40, 40, 40);
                draw.line(platform.x, platform.y, platform.x + platform.w, platform.y);
            }
        }

        // Draw blur overlay if needed
        if (!lift_blur && level_number == 0) {
            // Draw covering platforms for the secret area
            std::vector<SDL_FRect> cover_platforms = {
                {0, 0, 150, 800},
                {150, 0, 900, 150},
                {1050, 0, 150, 800}
            };

            for (const auto& platform : cover_platforms) {
                draw.color(0, 0, 0);
                draw.fill_rect(platform.x, platform.y, platform.w, platform.h);
            }
        }
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        draw_background(draw);
        draw_platforms(draw);

        for (auto& box : breakable_boxes) {
            box.draw(draw);
        }

        for (auto& door : doors) {
            door.draw(draw, renderer);
        }

        for (auto& npc : npcs) {
            npc.draw(draw, renderer);
        }

        // Draw lights (simple glow effect)
        for (const auto& light : lights) {
            for (int i = 100; i > 0; i -= 10) {
                int alpha = 30 * (i / 100);
                draw.color(255, 255, 255, alpha);
                draw.fill_circle(static_cast<int>(light.first), static_cast<int>(light.second), i);
            }
        }
    }
};

// Implement Fireball methods
void Fireball::update(const std::vector<SDL_FRect>& platforms, std::vector<BreakableBox*>& boxes) {
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const DustParticle& p) { return p.life <= 0; }),
        particles.end()
    );

    for (auto& particle : particles) {
        particle.update();
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
    for (const auto& platform : platforms) {
        if (SDL_HasRectIntersectionFloat(&rect, &platform)) {
            explode();
            return;
        }
    }

    // Check box collisions
    for (auto* box : boxes) {
        if (box && SDL_HasRectIntersectionFloat(&rect, &box->rect) && !box->broken) {
            box->break_box();
            explode();
            return;
        }
    }

    // Add trail particles
    if (random_float(0, 1) < 0.8f) {
        particles.push_back(DustParticle(
            rect.x + rect.w / 2 + random_float(-3, 3),
            rect.y + rect.h / 2 + random_float(-3, 3)
        ));
    }

    // Remove if off screen
    if (rect.x < -50 || rect.x > SCREEN_WIDTH + 50 ||
        rect.y < -50 || rect.y > SCREEN_HEIGHT + 50) {
        alive = false;
    }
}

void Fireball::explode() {
    alive = false;
    for (int i = 0; i < 4; ++i) {
        float angle = random_float(0, PI * 2);
        float speed = random_float(2, 5);
        DustParticle particle(rect.x + rect.w / 2, rect.y + rect.h / 2);
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;
        particles.push_back(particle);
    }
}

void Fireball::draw(Draw& draw) {
    for (auto& particle : particles) {
        particle.draw(draw);
    }

    if (alive) {
        // White glowing orb
        for (int i = 16; i > 0; i -= 2) {
            int alpha = 150 * (i / 16);
            draw.color(255, 255, 255, alpha);
            draw.fill_circle(static_cast<int>(rect.x + rect.w / 2),
                static_cast<int>(rect.y + rect.h / 2), i);
        }
    }
}

// Implement Player methods
void Player::update(const std::vector<SDL_FRect>& platforms,
    const std::vector<bool>& solid_flags,
    std::vector<BreakableBox>& boxes,
    float mouse_x, float mouse_y,
    AudioManager& audio) {
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
        audio.startWalking(0.4f);
    }
    else {
        audio.stopWalking();
    }

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

    // Animation updates
    animation_timer++;
    if (animation_state == "walking") {
        walk_cycle += std::abs(vel_x) * 0.15f;
        arm_swing = std::sin(walk_cycle) * 12;
        head_offset = std::abs(std::sin(walk_cycle * 2)) * 0.5f;
    }
    else {
        walk_cycle = 0;
        arm_swing *= 0.85f;
    }

    if (animation_state == "idle") {
        idle_timer += 0.04f;
        head_offset = std::sin(idle_timer) * 0.3f;
    }

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
            audio.playSound("jump", 0.3f);
            vel_y = JUMP_STRENGTH;
            can_double_jump = double_jump_available;

            for (int i = 0; i < 3; ++i) {
                particles.push_back(DustParticle(
                    rect.x + rect.w / 2 + random_float(-8, 8),
                    rect.y + rect.h
                ));
            }
        }
        else if (can_double_jump) {
            audio.playSound("jump", 0.3f);
            vel_y = JUMP_STRENGTH * 0.85f;
            can_double_jump = false;

            for (int i = 0; i < 4; ++i) {
                float angle = random_float(0, PI * 2);
                float speed = random_float(2, 4);
                DustParticle particle(rect.x + rect.w / 2, rect.y + rect.h / 2);
                particle.vx = std::cos(angle) * speed;
                particle.vy = std::sin(angle) * speed;
                particles.push_back(particle);
            }
        }
    }
    jump_pressed = jump_key;

    // Fireball
    if (can_fireball && fireball_cooldown <= 0) {
        if (keys_state[SDL_SCANCODE_F] || keys_state[SDL_SCANCODE_LSHIFT]) {
            fireballs.push_back(Fireball(
                rect.x + rect.w / 2, rect.y + rect.h / 2,
                mouse_x, mouse_y));
            fireball_cooldown = 20;
            audio.playSound("fireball", 0.3f);
        }
    }

    if (fireball_cooldown > 0) {
        fireball_cooldown--;
    }

    // Apply gravity
    vel_y += GRAVITY;
    if (vel_y > 20) vel_y = 20;

    bool was_falling = !on_ground && vel_y > 5;

    // Move vertically
    rect.y += vel_y;
    on_ground = false;
    on_drop_platform = false;
    check_collisions(platforms, solid_flags, 'v');

    // Move horizontally
    rect.x += vel_x;
    rect.x = std::max(0.0f, std::min(rect.x, SCREEN_WIDTH - rect.w));
    check_collisions(platforms, solid_flags, 'h');
    

    // Landing effect
    if (on_ground && was_falling) {
        land_timer = 8;
        for (int i = 0; i < 6; ++i) {
            particles.push_back(DustParticle(
                rect.x + rect.w / 2 + random_float(-12, 12),
                rect.y + rect.h
            ));
        }
    }

    // Update particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const DustParticle& p) { return p.life <= 0; }),
        particles.end()
    );

    for (auto& particle : particles) {
        particle.update();
    }

    // Update fireballs
    fireballs.erase(
        std::remove_if(fireballs.begin(), fireballs.end(),
            [](const Fireball& f) { return !f.alive && f.particles.empty(); }),
        fireballs.end()
    );

    // Create box pointers for fireball collision
    std::vector<BreakableBox*> box_ptrs;
    for (auto& box : boxes) {
        box_ptrs.push_back(&box);
    }

    for (auto& fireball : fireballs) {
        fireball.update(platforms, box_ptrs);
    }
}

void Player::check_collisions(const std::vector<SDL_FRect>& platforms,
    const std::vector<bool>& solid_flags,
    char direction) {
    for (size_t i = 0; i < platforms.size(); ++i) {
        const SDL_FRect& platform = platforms[i];
        bool is_solid = solid_flags[i];
        if (SDL_HasRectIntersectionFloat(&rect, &platform)) {
            if (direction == 'h') {
                if (is_solid) {
                    // Check vertical overlap to determine if this is a wall or walkable platform
                    float vertical_overlap = std::min(rect.y + rect.h, platform.y + platform.h) -
                        std::max(rect.y, platform.y);

                    // Only block if there's significant vertical overlap (hitting a wall)
                    // Allow movement if player's feet are near platform top (can step up/down)
                    if (vertical_overlap > 10) { // Threshold for step height
                        float overlap_left = (rect.x + rect.w) - platform.x;
                        float overlap_right = (platform.x + platform.w) - rect.x;

                        if (vel_x > 0 && overlap_left > 0 && overlap_left < rect.w) {
                            // Moving right and hitting a wall
                            rect.x = platform.x - rect.w;
                        }
                        else if (vel_x < 0 && overlap_right > 0 && overlap_right < rect.w) {
                            // Moving left and hitting a wall
                            rect.x = platform.x + platform.w;
                        }
                    }
                }
            }
            else { // vertical
                if (!is_solid) { // drop-through platform
                    if (vel_y > 0 && !dropping) {
                        if (rect.y + rect.h - vel_y <= platform.y + 5) {
                            rect.y = platform.y - rect.h;
                            vel_y = 0;
                            on_ground = true;
                            on_drop_platform = true;
                        }
                    }
                }
                else {
                    float overlap_top = (rect.y + rect.h) - platform.y;
                    float overlap_bottom = (platform.y + platform.h) - rect.y;

                    if (vel_y > 0 && overlap_top > 0 && overlap_top < rect.h) {
                        // Moving down and colliding from top
                        rect.y = platform.y - rect.h;
                        vel_y = 0;
                        on_ground = true;
                    }
                    else if (vel_y < 0 && overlap_bottom > 0 && overlap_bottom < rect.h) {
                        // Moving up and colliding from bottom
                        rect.y = platform.y + platform.h;
                        vel_y = 0;
                    }
                }
            }
        }
    }
}

void Player::draw(Draw& draw) {
    // Draw particles
    for (auto& particle : particles) {
        particle.draw(draw);
    }

    // Draw fireballs
    for (auto& fireball : fireballs) {
        fireball.draw(draw);
    }

    // Draw player with detailed stick figure
    float cx = rect.x + rect.w / 2;
    float cy = rect.y + rect.h / 2;
    float head_y = rect.y + 5 + head_offset;

    if (animation_state == "landing") {
        head_y += 2;
    }

    // White outline effect
    int outline_width = 1;

    // Head with outline
    draw.color(255, 255, 255);
    draw.circle(static_cast<int>(cx), static_cast<int>(head_y + 5), 6);
    draw.color(0, 0, 0);
    draw.fill_circle(static_cast<int>(cx), static_cast<int>(head_y + 5), 5);

    // Neck
    draw.color(255, 255, 255);
    draw.line(cx, head_y + 10, cx, rect.y + 16);

    // Torso
    float torso_lean = (animation_state == "walking") ? vel_x * 0.015f : 0;
    float torso_top_x = cx + torso_lean * 3;
    float torso_bottom_x = cx - torso_lean * 2;

    std::vector<SDL_FPoint> torso_points = {
        {torso_top_x - 5, rect.y + 16},
        {torso_top_x + 5, rect.y + 16},
        {torso_bottom_x + 4, rect.y + 28},
        {torso_bottom_x - 4, rect.y + 28}
    };

    draw.color(255, 255, 255);
    draw.polygon(torso_points);
    draw.color(0, 0, 0);
    // Fill torso interior
    for (float y = rect.y + 17; y < rect.y + 27; y++) {
        float ratio = (y - rect.y - 16) / 12;
        float width = 10 - ratio * 2;
        float x = torso_top_x + (torso_bottom_x - torso_top_x) * ratio;
        draw.line(x - width / 2, y, x + width / 2, y);
    }

    // Arms
    if (can_fireball && fireball_cooldown > 10) {
        // Casting animation
        if (facing_right) {
            draw.color(255, 255, 255);
            draw.lines({ {cx + 4, rect.y + 18},
                       {cx + 10, rect.y + 20},
                       {cx + 16, rect.y + 19} });
            draw.lines({ {cx - 4, rect.y + 18},
                       {cx - 6, rect.y + 24},
                       {cx - 5, rect.y + 30} });
        }
        else {
            draw.color(255, 255, 255);
            draw.lines({ {cx - 4, rect.y + 18},
                       {cx - 10, rect.y + 20},
                       {cx - 16, rect.y + 19} });
            draw.lines({ {cx + 4, rect.y + 18},
                       {cx + 6, rect.y + 24},
                       {cx + 5, rect.y + 30} });
        }
    }
    else {
        // Normal arms
        float left_shoulder_x = cx - 4;
        float right_shoulder_x = cx + 4;
        float shoulder_y = rect.y + 18;

        float left_elbow_x = cx - 5 - arm_swing * 0.2f;
        float left_hand_x = cx - 4 - arm_swing * 0.4f;
        float right_elbow_x = cx + 5 + arm_swing * 0.2f;
        float right_hand_x = cx + 4 + arm_swing * 0.4f;

        draw.color(255, 255, 255);
        draw.lines({ {left_shoulder_x, shoulder_y},
                   {left_elbow_x, rect.y + 24},
                   {left_hand_x, rect.y + 30} });
        draw.lines({ {right_shoulder_x, shoulder_y},
                   {right_elbow_x, rect.y + 24},
                   {right_hand_x, rect.y + 30} });
    }

    // Legs
    float hip_y = rect.y + 28;

    if (animation_state == "landing") {
        // Bent legs on landing
        draw.color(255, 255, 255);
        draw.lines({ {cx - 3, hip_y}, {cx - 5, hip_y + 4}, {cx - 6, rect.y + rect.h} });
        draw.lines({ {cx + 3, hip_y}, {cx + 5, hip_y + 4}, {cx + 6, rect.y + rect.h} });
    }
    else if (animation_state == "jumping") {
        // Tucked legs
        draw.color(255, 255, 255);
        draw.lines({ {cx - 3, hip_y}, {cx - 4, hip_y + 5}, {cx - 3, hip_y + 8} });
        draw.lines({ {cx + 3, hip_y}, {cx + 4, hip_y + 5}, {cx + 3, hip_y + 8} });
    }
    else if (animation_state == "falling") {
        // Extended legs
        draw.color(255, 255, 255);
        draw.lines({ {cx - 3, hip_y}, {cx - 5, hip_y + 6}, {cx - 6, hip_y + 10} });
        draw.lines({ {cx + 3, hip_y}, {cx + 5, hip_y + 6}, {cx + 6, hip_y + 10} });
    }
    else if (animation_state == "walking") {
        // Walking animation
        float left_phase = std::sin(walk_cycle);
        float right_phase = std::sin(walk_cycle + PI);

        float left_knee_x = cx - 3 + std::max(0.0f, left_phase) * 4;
        float left_knee_y = hip_y + 6 - std::abs(left_phase) * 2;
        float left_foot_x = cx - 3 + left_phase * 6;

        float right_knee_x = cx + 3 + std::max(0.0f, right_phase) * 4;
        float right_knee_y = hip_y + 6 - std::abs(right_phase) * 2;
        float right_foot_x = cx + 3 + right_phase * 6;

        draw.color(255, 255, 255);
        draw.lines({ {cx - 3, hip_y}, {left_knee_x, left_knee_y}, {left_foot_x, rect.y + rect.h} });
        draw.lines({ {cx + 3, hip_y}, {right_knee_x, right_knee_y}, {right_foot_x, rect.y + rect.h} });
    }
    else {
        // Standing
        draw.color(255, 255, 255);
        draw.lines({ {cx - 3, hip_y}, {cx - 3, hip_y + 6}, {cx - 4, rect.y + rect.h} });
        draw.lines({ {cx + 3, hip_y}, {cx + 3, hip_y + 6}, {cx + 4, rect.y + rect.h} });
    }

    // Double jump indicator
    if (double_jump_available && can_double_jump && !on_ground) {
        for (int i = 15; i > 0; i -= 3) {
            int alpha = 100 * (i / 15);
            draw.color(255, 255, 255, alpha);
            draw.circle(static_cast<int>(cx), static_cast<int>(rect.y - 35), i);
        }
    }
}

// ===== MENU CLASS =====
class Menu {
private:
    SDL_FRect start_button;
    SDL_FRect quit_button;
    std::string hover;
    std::vector<DustParticle> particles;
    float bg_phase;
    std::vector<FogParticle> fog_particles;

public:
    Menu() : start_button{ SCREEN_WIDTH / 2.0f - 120, 400, 240, 50 },
        quit_button{ SCREEN_WIDTH / 2.0f - 120, 480, 240, 50 },
        bg_phase(0) {
        for (int i = 0; i < 4; ++i) {
            fog_particles.push_back(FogParticle(
                static_cast<float>(random_int(-200, SCREEN_WIDTH)),
                static_cast<float>(random_int(0, SCREEN_HEIGHT))
            ));
        }
    }

    void update(float mouse_x, float mouse_y) {
        hover = "";

        SDL_FPoint mouse_pt = { mouse_x, mouse_y };
        if (SDL_PointInRectFloat(&mouse_pt, &start_button)) {
            hover = "start";
            if (random_float(0, 1) < 0.1f) {
                particles.push_back(DustParticle(
                    start_button.x + start_button.w / 2 + random_float(-40, 40),
                    start_button.y + start_button.h / 2
                ));
            }
        }
        else if (SDL_PointInRectFloat(&mouse_pt, &quit_button)) {
            hover = "quit";
            if (random_float(0, 1) < 0.1f) {
                particles.push_back(DustParticle(
                    quit_button.x + quit_button.w / 2 + random_float(-40, 40),
                    quit_button.y + quit_button.h / 2
                ));
            }
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const DustParticle& p) { return p.life <= 0; }),
            particles.end()
        );

        for (auto& particle : particles) {
            particle.update();
        }

        for (auto& fog : fog_particles) {
            fog.update();
        }

        bg_phase += 0.01f;
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Background gradient
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            int gray = static_cast<int>(160 - (y / static_cast<float>(SCREEN_HEIGHT)) * 60);
            draw.color(gray, gray, gray);
            draw.line(0, static_cast<float>(y), SCREEN_WIDTH, static_cast<float>(y));
        }

        // Fog
        for (auto& fog : fog_particles) {
            fog.draw(draw);
        }

        // Particles
        for (auto& particle : particles) {
            particle.draw(draw);
        }

        // Title with shadow effect
        const char* title = "TTIGSBAMTGOOTD";

        // Shadow
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_SetRenderScale(renderer, 4.0f, 4.0f);
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 8 - strlen(title) + 1, 41, title);

        // Main title
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 8 - strlen(title), 40, title);
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);

        // Subtitle
        const char* subtitle = "That Time I Got Summoned By A Mage To Get Out Of The Dungeon";
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - strlen(subtitle) * 4, 250, subtitle);

        // Start button
        if (hover == "start") {
            for (int i = 10; i > 0; i -= 2) {
                int alpha = 50 * (i / 10);
                draw.color(255, 255, 255, alpha);
                draw.rect(start_button.x - i, start_button.y - i,
                    start_button.w + i * 2, start_button.h + i * 2);
            }
        }
        draw.color(0, 0, 0);
        draw.fill_rect(start_button.x, start_button.y, start_button.w, start_button.h);
        draw.color(40, 40, 40);
        draw.rect(start_button.x, start_button.y, start_button.w, start_button.h);

        SDL_SetRenderDrawColor(renderer, hover == "start" ? 255 : 180,
            hover == "start" ? 255 : 180,
            hover == "start" ? 255 : 180, 255);
        SDL_SetRenderScale(renderer, 2.0f, 2.0f);
        SDL_RenderDebugText(renderer, (start_button.x + start_button.w / 2) / 2 - 25,
            (start_button.y + start_button.h / 2) / 2 - 8, "START");
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);

        // Quit button
        if (hover == "quit") {
            for (int i = 10; i > 0; i -= 2) {
                int alpha = 50 * (i / 10);
                draw.color(255, 255, 255, alpha);
                draw.rect(quit_button.x - i, quit_button.y - i,
                    quit_button.w + i * 2, quit_button.h + i * 2);
            }
        }
        draw.color(0, 0, 0);
        draw.fill_rect(quit_button.x, quit_button.y, quit_button.w, quit_button.h);
        draw.color(40, 40, 40);
        draw.rect(quit_button.x, quit_button.y, quit_button.w, quit_button.h);

        SDL_SetRenderDrawColor(renderer, hover == "quit" ? 255 : 180,
            hover == "quit" ? 255 : 180,
            hover == "quit" ? 255 : 180, 255);
        SDL_SetRenderScale(renderer, 2.0f, 2.0f);
        SDL_RenderDebugText(renderer, (quit_button.x + quit_button.w / 2) / 2 - 20,
            (quit_button.y + quit_button.h / 2) / 2 - 8, "QUIT");
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    }

    std::string handle_click(float x, float y) {
        SDL_FPoint pt = { x, y };
        if (SDL_PointInRectFloat(&pt, &start_button)) {
            return "start";
        }
        else if (SDL_PointInRectFloat(&pt, &quit_button)) {
            return "quit";
        }
        return "";
    }
};

// ===== ENDING SCREEN CLASS =====
class EndingScreen {
private:
    struct Star {
        float x, y, z;
        float speedz;
    };

    std::vector<Star> stars;
    int timer;
    std::vector<std::string> story_texts;
    size_t current_text_index;
    int text_display_timer;
    bool fade_to_menu;
    int fade_timer;

public:
    EndingScreen() : timer(0), current_text_index(0),
        text_display_timer(0), fade_to_menu(false), fade_timer(0) {
        // Initialize stars
        for (int i = 0; i < 150; ++i) {
            stars.push_back(create_star());
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

    Star create_star() {
        Star s;
        s.x = static_cast<float>(random_int(-SCREEN_WIDTH / 2, SCREEN_WIDTH / 2));
        s.y = static_cast<float>(random_int(-SCREEN_HEIGHT / 2, SCREEN_HEIGHT / 2));
        s.z = static_cast<float>(random_int(SCREEN_WIDTH / 2, SCREEN_WIDTH));
        s.speedz = 3;
        return s;
    }

    bool update() {
        timer++;

        // Update stars
        for (auto& star : stars) {
            star.z -= star.speedz;
            if (star.z <= 20) {
                star = create_star();
            }
        }

        // Handle text display
        text_display_timer++;
        if (text_display_timer > 180) { // 3 seconds per text
            text_display_timer = 0;
            current_text_index++;

            if (current_text_index >= story_texts.size()) {
                fade_to_menu = true;
            }
        }

        if (fade_to_menu) {
            fade_timer += 2;
            if (fade_timer > 255) {
                return true; // Signal to return to menu
            }
        }

        return false;
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Black background
        draw.color(0, 0, 0);
        draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Draw stars
        for (const auto& star : stars) {
            float sx = (star.x / star.z) * (SCREEN_WIDTH / 2);
            float sy = (star.y / star.z) * (SCREEN_HEIGHT / 2);
            float radius = ((SCREEN_WIDTH - star.z) / SCREEN_WIDTH) * 4;

            if (radius > 0) {
                // Glowing effect
                for (int i = static_cast<int>(radius * 2); i > 0; i--) {
                    int alpha = static_cast<int>(255 * (i / (radius * 2)) * 0.5f);
                    draw.color(255, 255, 255, alpha);
                    draw.circle(static_cast<int>(SCREEN_WIDTH / 2 + sx),
                        static_cast<int>(SCREEN_HEIGHT / 2 + sy), i);
                }

                // Star core
                draw.color(255, 255, 255);
                draw.fill_circle(static_cast<int>(SCREEN_WIDTH / 2 + sx),
                    static_cast<int>(SCREEN_HEIGHT / 2 + sy),
                    static_cast<int>(radius));
            }
        }

        // Draw current text
        if (current_text_index < story_texts.size()) {
            const std::string& text = story_texts[current_text_index];

            // Calculate opacity
            int opacity;
            if (text_display_timer < 30) {
                opacity = (text_display_timer * 255) / 30;
            }
            else if (text_display_timer > 150) {
                opacity = ((180 - text_display_timer) * 255) / 30;
            }
            else {
                opacity = 255;
            }

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, opacity);

            // Scale text based on importance
            float scale = 1.0f;
            if (text.find("Thank you") != std::string::npos) {
                scale = 2.0f;
            }

            SDL_SetRenderScale(renderer, scale, scale);

            // Center text
            int text_len = static_cast<int>(text.length());
            int text_x = static_cast<int>((SCREEN_WIDTH / 2 - (text_len * 4 * scale)) / scale);
            int text_y = static_cast<int>(SCREEN_HEIGHT / 2 / scale);

            SDL_RenderDebugText(renderer, text_x, text_y, text.c_str());
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        }

        // Fade to black
        if (fade_to_menu) {
            draw.color(0, 0, 0, std::min(255, fade_timer));
            draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
};

// ===== TRANSITION STATE =====
class TransitionState {
public:
    enum Phase { SWIPE, FADE };
    Phase phase;
    float progress;
    SDL_Texture* old_level_texture;
    SDL_Texture* new_level_texture;
    int start_level;
    int target_level;
    float offset_x;

    TransitionState() : phase(SWIPE), progress(0), old_level_texture(nullptr),
        new_level_texture(nullptr), start_level(0),
        target_level(0), offset_x(0) {
    }

    void cleanup(SDL_Renderer* renderer) {
        if (old_level_texture) {
            SDL_DestroyTexture(old_level_texture);
            old_level_texture = nullptr;
        }
        if (new_level_texture) {
            SDL_DestroyTexture(new_level_texture);
            new_level_texture = nullptr;
        }
    }
};

// ===== MAIN GAME CLASS =====
class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Draw draw;
    AudioManager audio;

    GameState state;
    Menu menu;
    Level* level;
    Player player;
    EndingScreen ending_screen;
    TransitionState transition;

    int current_level;
    int from_level;

    bool running;

    // Level transition surfaces
    SDL_Surface* level_surface;

public:
    Game() : window(nullptr), renderer(nullptr),
        state(GameState::MENU), level(nullptr),
        player(100, 400), current_level(0),
        from_level(0), running(true), level_surface(nullptr) {
    }

    ~Game() {
        cleanup();
    }

    bool init() {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        window = SDL_CreateWindow(
            "That time I got summoned by a mage to use my intellect and break free from the dungeon",
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            SDL_Log("Window creation failed: %s", SDL_GetError());
            return false;
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            SDL_Log("Renderer creation failed: %s", SDL_GetError());
            return false;
        }

        SDL_SetRenderVSync(renderer, 1);
        draw.set_renderer(renderer);

        if (!audio.init()) {
            SDL_Log("Audio initialization failed");
            // Continue without audio
        }

        // Load sounds
        audio.loadSound("jump", "sounds/jump.wav");
        audio.loadSound("walk", "sounds/walk.wav");
        audio.loadSound("fireball", "sounds/fireball.wav");
        audio.loadSound("menu_theme", "sounds/menu_theme.wav");
        audio.loadSound("game_theme", "sounds/game_theme.wav");
        audio.loadSound("ending_theme", "sounds/ending_theme.wav");

        // Play menu music
        audio.playMusic("menu_theme", 0.4f);

        return true;
    }

    void cleanup() {
        transition.cleanup(renderer);

        if (level_surface) {
            SDL_DestroySurface(level_surface);
            level_surface = nullptr;
        }

        if (level) {
            delete level;
            level = nullptr;
        }

        audio.cleanup();

        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }

    void start_level(int level_index) {
        if (level) {
            delete level;
        }

        level = new Level(level_index);
        auto [x, y] = level->player_start;
        player.set_position(x, y);
        player.set_abilities(level->player_abilities);
        player.current_level = level;
        current_level = level_index;
        state = GameState::PLAYING;
    }

    SDL_Texture* capture_screen() {
        SDL_Texture* texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            SCREEN_WIDTH, SCREEN_HEIGHT);

        if (!texture) return nullptr;

        // Set render target to texture
        SDL_SetRenderTarget(renderer, texture);

        // Draw current scene
        draw_level();

        // Reset render target
        SDL_SetRenderTarget(renderer, nullptr);

        return texture;
    }

    void start_transition(int target_level) {
        // Stop walking sound if playing
        audio.stopWalking();

        transition.start_level = current_level;
        transition.target_level = target_level;
        from_level = current_level;

        // Capture old level
        transition.old_level_texture = capture_screen();

        // Load new level
        start_level(target_level);

        // Capture new level
        transition.new_level_texture = capture_screen();

        transition.phase = TransitionState::SWIPE;
        transition.progress = 0;
        transition.offset_x = 0;
        state = GameState::TRANSITIONING;
    }

    void handle_events() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    if (state == GameState::PLAYING) {
                        // Return to menu
                        state = GameState::MENU;
                        audio.fadeOutMusic(500);
                        audio.playMusic("menu_theme", 0.4f);
                    }
                }
                else if (state == GameState::PLAYING) {
                    if (event.key.key == SDLK_E) {
                        // Interact with NPCs
                        for (auto& npc : level->npcs) {
                            if (npc.show_prompt) {
                                npc.interact(from_level, current_level);
                            }
                        }
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (state == GameState::MENU) {
                    float x, y;
                    SDL_GetMouseState(&x, &y);
                    std::string action = menu.handle_click(x, y);
                    if (action == "start") {
                        audio.fadeOutMusic(500);
                        audio.playMusic("game_theme", 0.4f);
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
        if (state == GameState::MENU) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            menu.update(mx, my);

        }
        else if (state == GameState::PLAYING) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);

            player.update(level->platforms, level->platform_solid,
                level->breakable_boxes, mx, my, audio);
            level->update(player, from_level, audio);

            // Check door collisions
            SDL_FRect player_rect = player.rect;
            for (auto& door : level->doors) {
                SDL_FRect door_rect = door.rect;
                if (SDL_HasRectIntersectionFloat(&player_rect, &door_rect) && !door.locked) {
                    int target = door.target_level;
                    if (target == -1) {
                        // Exit door - go to ending
                        state = GameState::ENDING;
                        ending_screen = EndingScreen();
                        audio.fadeOutMusic(1000);
                        audio.playMusic("ending_theme", 0.3f);
                    }
                    else {
                        start_transition(target);
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
                // Return to menu
                state = GameState::MENU;
                menu = Menu();
                audio.fadeOutMusic(500);
                audio.playMusic("menu_theme", 0.4f);
            }
        }
    }

    void update_transition() {
        const float TRANSITION_SPEED = 0.02f;

        if (transition.phase == TransitionState::SWIPE) {
            transition.progress += TRANSITION_SPEED;
            transition.offset_x = transition.progress * SCREEN_WIDTH;

            if (transition.progress >= 1.0f) {
                // Transition complete
                state = GameState::PLAYING;
                transition.cleanup(renderer);
            }
        }
    }

    void draw_level() {
        if (!level) return;

        level->draw(draw, renderer);
        player.draw(draw);

        // UI overlay
        draw_ui();
    }

    void draw_ui() {
        // Crosshair for fireball
        if (player.can_fireball) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);

            draw.color(255, 255, 255, 100);
            draw.circle(static_cast<int>(mx), static_cast<int>(my), 8);
            draw.line(mx - 10, my, mx + 10, my);
            draw.line(mx, my - 10, mx, my + 10);
        }

        // UI text
        int ui_y = 20;

        // Abilities
        if (player.abilities["jump"]) {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            if (player.abilities["double_jump"]) {
                SDL_RenderDebugText(renderer, 20, ui_y, "Double Jump: Space");
                ui_y += 20;
            }
            else {
                SDL_RenderDebugText(renderer, 20, ui_y, "Jump: Space");
                ui_y += 20;
            }
        }

        if (player.abilities["fireball"]) {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_RenderDebugText(renderer, 20, ui_y, "Fireball: F (aim with mouse)");
            ui_y += 20;
        }

        // Keys
        if (player.keys > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            char keys_text[32];
            SDL_snprintf(keys_text, sizeof(keys_text), "Keys: %d", player.keys);
            SDL_RenderDebugText(renderer, 20, ui_y, keys_text);
            ui_y += 20;
        }

        // Level indicator
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        char level_text[32];
        SDL_snprintf(level_text, sizeof(level_text), "Floor %d", current_level + 1);
        SDL_RenderDebugText(renderer, SCREEN_WIDTH - 80, 20, level_text);

        // Drop hint
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 100);
        SDL_RenderDebugText(renderer, 20, SCREEN_HEIGHT - 30, "S: Drop through platforms");
    }

    void draw_transition() {
        if (transition.phase == TransitionState::SWIPE) {
            // Clear screen
            draw.color(40, 40, 40);
            draw.clear();

            if (transition.old_level_texture && transition.new_level_texture) {
                // Draw old level sliding out
                SDL_FRect old_rect = {
                    -transition.offset_x, 0,
                    static_cast<float>(SCREEN_WIDTH),
                    static_cast<float>(SCREEN_HEIGHT)
                };
                SDL_RenderTexture(renderer, transition.old_level_texture, nullptr, &old_rect);

                // Draw new level sliding in
                SDL_FRect new_rect = {
                    SCREEN_WIDTH - transition.offset_x, 0,
                    static_cast<float>(SCREEN_WIDTH),
                    static_cast<float>(SCREEN_HEIGHT)
                };
                SDL_RenderTexture(renderer, transition.new_level_texture, nullptr, &new_rect);

                // Draw transition effect (vertical line)
                float line_x = SCREEN_WIDTH - transition.offset_x;
                for (int i = 20; i > 0; i--) {
                    int alpha = 255 * (i / 20);
                    draw.color(255, 255, 255, alpha);
                    draw.line(line_x - i, 0, line_x - i, SCREEN_HEIGHT);
                    draw.line(line_x + i, 0, line_x + i, SCREEN_HEIGHT);
                }
            }
        }
    }

    void render() {
        draw.color(180, 180, 180);
        draw.clear();

        if (state == GameState::MENU) {
            menu.draw(draw, renderer);

        }
        else if (state == GameState::PLAYING) {
            draw_level();

        }
        else if (state == GameState::TRANSITIONING) {
            draw_transition();

        }
        else if (state == GameState::ENDING) {
            ending_screen.draw(draw, renderer);
        }

        draw.present();
    }

    void run() {
        Uint64 frame_start, frame_time;
        const Uint64 frame_delay = 1000 / FPS;

        while (running) {
            frame_start = SDL_GetTicks();

            handle_events();
            update();
            render();

            frame_time = SDL_GetTicks() - frame_start;
            if (frame_delay > frame_time) {
                SDL_Delay(static_cast<Uint32>(frame_delay - frame_time));
            }
        }
    }
};

// ===== MAIN ENTRY POINT =====
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    SDL_SetAppMetadata("Limbo Dungeon", "1.0", "com.example.limbo");

    Game game;
    if (!game.init()) {
        SDL_Log("Failed to initialize game");
        return -1;
    }

    game.run();
    return 0;
}