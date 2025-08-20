// stickman_fighter.cpp - Complete Stickman Fighting Game with Katana Zero Mechanics
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <string>
#include <deque>
#include <unordered_map>
#include <random>
#include <array>
#include <functional>
#include "renderer2d.cpp"  // Your Draw struct
#include "utils.cpp"       // Your Utils struct

// Constants
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr float GRAVITY = 0.5f;
constexpr float GROUND_Y = 600.0f;
constexpr float TIME_SCALE_SLOW = 0.2f;
constexpr float DASH_SPEED = 25.0f;
constexpr float SWORD_REACH = 80.0f;

// Forward declarations
class Entity;
class Player;
class Enemy;
class Projectile;
class SlashEffect;
class BloodParticle;
class SparkParticle;
class GameWorld;

// ===== ENUMS =====
enum class AttackType {
    HORIZONTAL_SLASH,
    VERTICAL_SLASH,
    DIAGONAL_SLASH,
    THRUST,
    SPIN_ATTACK,
    UPPERCUT,
    DOWNWARD_STRIKE,
    DASH_ATTACK
};

enum class AbilityType {
    TIME_SLOW,
    TELEPORT,
    DEFLECT,
    SHOCKWAVE,
    LIGHTNING_STRIKE,
    SHADOW_CLONE,
    BERSERK_MODE,
    HEAL
};

enum class EnemyType {
    BASIC_GRUNT,
    HEAVY_BRUTE,
    NINJA_ASSASSIN,
    RANGED_ARCHER,
    SHIELD_KNIGHT,
    DUAL_WIELDER,
    BOSS_SAMURAI
};

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY
};

// ===== INPUT MANAGER =====
class InputManager {
public:
    struct MouseGesture {
        std::vector<Vec2> points;
        float totalDistance;
        float startTime;
        Vec2 direction;
        float speed;
    };

private:
    const bool* keyboardState;
    bool prevMouseLeft, prevMouseRight;
    bool mouseLeft, mouseRight;
    float mouseX, mouseY;
    float prevMouseX, prevMouseY;
    MouseGesture currentGesture;
    bool recordingGesture;
    std::deque<Vec2> recentMousePositions;
    static constexpr size_t MOUSE_HISTORY_SIZE = 10;

public:
    InputManager() : keyboardState(nullptr), prevMouseLeft(false),
        prevMouseRight(false), mouseLeft(false), mouseRight(false),
        mouseX(0), mouseY(0), prevMouseX(0), prevMouseY(0),
        recordingGesture(false) {
        keyboardState = SDL_GetKeyboardState(nullptr);
    }

    void update() {
        prevMouseLeft = mouseLeft;
        prevMouseRight = mouseRight;
        prevMouseX = mouseX;
        prevMouseY = mouseY;

        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        mouseLeft = (mouseState & SDL_BUTTON_LMASK) != 0;
        mouseRight = (mouseState & SDL_BUTTON_RMASK) != 0;

        // Update mouse history
        recentMousePositions.push_back({ mouseX, mouseY });
        if (recentMousePositions.size() > MOUSE_HISTORY_SIZE) {
            recentMousePositions.pop_front();
        }

        // Handle gesture recording
        if (mouseLeft && !prevMouseLeft) {
            startGesture();
        }
        else if (recordingGesture && mouseLeft) {
            updateGesture();
        }
        else if (recordingGesture && !mouseLeft) {
            endGesture();
        }
    }

    void startGesture() {
        currentGesture.points.clear();
        currentGesture.points.push_back({ mouseX, mouseY });
        currentGesture.totalDistance = 0;
        currentGesture.startTime = SDL_GetTicks() / 1000.0f;
        recordingGesture = true;
    }

    void updateGesture() {
        Vec2 currentPos(mouseX, mouseY);
        if (!currentGesture.points.empty()) {
            Vec2 lastPos = currentGesture.points.back();
            float dist = (currentPos - lastPos).length();
            if (dist > 2.0f) { // Minimum distance threshold
                currentGesture.points.push_back(currentPos);
                currentGesture.totalDistance += dist;
            }
        }
    }

    void endGesture() {
        if (currentGesture.points.size() >= 2) {
            Vec2 start = currentGesture.points.front();
            Vec2 end = currentGesture.points.back();
            currentGesture.direction = (end - start).normalized();

            float duration = SDL_GetTicks() / 1000.0f - currentGesture.startTime;
            currentGesture.speed = duration > 0 ? currentGesture.totalDistance / duration : 0;
        }
        recordingGesture = false;
    }

    AttackType getAttackFromGesture() {
        if (currentGesture.points.size() < 2) return AttackType::HORIZONTAL_SLASH;

        Vec2 start = currentGesture.points.front();
        Vec2 end = currentGesture.points.back();
        Vec2 delta = end - start;

        float angle = std::atan2(delta.y, delta.x);
        float speed = currentGesture.speed;

        // Fast circular motion = Spin attack
        if (currentGesture.totalDistance > 200 && speed > 1000) {
            return AttackType::SPIN_ATTACK;
        }

        // Categorize by angle
        if (std::abs(angle) < PI / 6) {
            return AttackType::HORIZONTAL_SLASH;
        }
        else if (std::abs(angle - PI) < PI / 6) {
            return AttackType::HORIZONTAL_SLASH;
        }
        else if (std::abs(angle - PI / 2) < PI / 6) {
            return AttackType::DOWNWARD_STRIKE;
        }
        else if (std::abs(angle + PI / 2) < PI / 6) {
            return AttackType::UPPERCUT;
        }
        else if (currentGesture.totalDistance < 50) {
            return AttackType::THRUST;
        }
        else {
            return AttackType::DIAGONAL_SLASH;
        }
    }

    Vec2 getMouseVelocity() const {
        return Vec2(mouseX - prevMouseX, mouseY - prevMouseY);
    }

    Vec2 getAverageMouseDirection() const {
        if (recentMousePositions.size() < 2) return Vec2(0, 0);

        Vec2 avgDir(0, 0);
        for (size_t i = 1; i < recentMousePositions.size(); ++i) {
            avgDir += recentMousePositions[i] - recentMousePositions[i - 1];
        }
        return avgDir.normalized();
    }

    // Getters
    bool isKeyPressed(SDL_Scancode key) const { return keyboardState[key]; }
    bool isMouseLeftPressed() const { return mouseLeft; }
    bool isMouseRightPressed() const { return mouseRight; }
    bool isMouseLeftJustPressed() const { return mouseLeft && !prevMouseLeft; }
    bool isMouseRightJustPressed() const { return mouseRight && !prevMouseRight; }
    float getMouseX() const { return mouseX; }
    float getMouseY() const { return mouseY; }
    Vec2 getMousePos() const { return Vec2(mouseX, mouseY); }
    const MouseGesture& getGesture() const { return currentGesture; }
};

// ===== PARTICLE EFFECTS =====
class Particle {
public:
    Vec2 position;
    Vec2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
    float rotation;
    float rotationSpeed;
    bool gravity;

    Particle(Vec2 pos, Vec2 vel, Color col, float lifeTime, float particleSize = 2.0f)
        : position(pos), velocity(vel), color(col), life(lifeTime),
        maxLife(lifeTime), size(particleSize), rotation(0),
        rotationSpeed(Utils::randomFloat(-10, 10)), gravity(true) {
    }

    virtual void update(float dt, float timeScale = 1.0f) {
        float scaledDt = dt * timeScale;
        position += velocity * scaledDt;
        if (gravity) {
            velocity.y += GRAVITY * scaledDt;
        }
        velocity *= std::pow(0.98f, scaledDt * 60); // Drag
        life -= scaledDt;
        rotation += rotationSpeed * scaledDt;
    }

    virtual void draw(Draw& draw) {
        if (life <= 0) return;

        float alpha = life / maxLife;
        Color drawColor = color;
        drawColor.a *= alpha;

        SDL_Color c = drawColor.toSDL();
        draw.color(c.r, c.g, c.b, c.a);
        draw.fill_circle(position.x, position.y, size * alpha);
    }

    bool isAlive() const { return life > 0; }
};

class BloodParticle : public Particle {
public:
    std::deque<Vec2> trail;
    static constexpr size_t MAX_TRAIL_LENGTH = 10;

    BloodParticle(Vec2 pos, Vec2 vel)
        : Particle(pos, vel, Color(200, 0, 0), Utils::randomFloat(0.5f, 1.5f),
            Utils::randomFloat(2, 5)) {
    }

    void update(float dt, float timeScale = 1.0f) override {
        Particle::update(dt, timeScale);

        trail.push_back(position);
        if (trail.size() > MAX_TRAIL_LENGTH) {
            trail.pop_front();
        }

        // Blood splatter on ground
        if (position.y >= GROUND_Y && velocity.y > 0) {
            velocity.y *= -0.3f;
            velocity.x *= 0.7f;
            if (std::abs(velocity.y) < 1.0f) {
                velocity.y = 0;
                gravity = false;
            }
        }
    }

    void draw(Draw& draw) override {
        if (life <= 0) return;

        // Draw trail
        for (size_t i = 1; i < trail.size(); ++i) {
            float alpha = (life / maxLife) * (i / (float)trail.size());
            Color trailColor = color;
            trailColor.a *= alpha;

            SDL_Color c = trailColor.toSDL();
            draw.color(c.r, c.g, c.b, c.a);
            draw.line(trail[i - 1].x, trail[i - 1].y, trail[i].x, trail[i].y);
        }

        // Draw main particle
        Particle::draw(draw);
    }
};

class SparkParticle : public Particle {
public:
    SparkParticle(Vec2 pos, Vec2 vel)
        : Particle(pos, vel, Color(255, 255, 200), Utils::randomFloat(0.2f, 0.5f),
            Utils::randomFloat(1, 3)) {
        gravity = false;
    }

    void draw(Draw& draw) override {
        if (life <= 0) return;

        float alpha = life / maxLife;

        // Glowing effect
        for (int i = 3; i > 0; i--) {
            Color glowColor = color;
            glowColor.a *= alpha * (i / 3.0f) * 0.3f;

            SDL_Color c = glowColor.toSDL();
            draw.color(c.r, c.g, c.b, c.a);
            draw.fill_circle(position.x, position.y, size * i);
        }

        // Core
        Color coreColor(255, 255, 255);
        coreColor.a = alpha;
        SDL_Color c = coreColor.toSDL();
        draw.color(c.r, c.g, c.b, c.a);
        draw.fill_circle(position.x, position.y, size);
    }
};

// ===== SLASH EFFECT =====
class SlashEffect {
public:
    struct SlashPoint {
        Vec2 position;
        float width;
        float alpha;
    };

    std::vector<SlashPoint> points;
    Color color;
    float lifetime;
    float maxLifetime;
    bool active;

    SlashEffect() : color(255, 255, 255), lifetime(0), maxLifetime(0.3f), active(false) {}

    void start(Vec2 startPos, Color slashColor = Color(255, 255, 255)) {
        points.clear();
        points.push_back({ startPos, 15.0f, 1.0f });
        color = slashColor;
        lifetime = maxLifetime;
        active = true;
    }

    void addPoint(Vec2 pos) {
        if (!active) return;

        if (!points.empty()) {
            Vec2 lastPos = points.back().position;
            if ((pos - lastPos).length() > 5.0f) {
                float width = 15.0f * (lifetime / maxLifetime);
                points.push_back({ pos, width, lifetime / maxLifetime });
            }
        }
    }

    void update(float dt) {
        if (!active) return;

        lifetime -= dt;
        if (lifetime <= 0) {
            active = false;
            points.clear();
        }

        // Fade out points
        for (auto& point : points) {
            point.alpha *= 0.95f;
        }
    }

    void draw(Draw& draw) {
        if (!active || points.size() < 2) return;

        // Draw slash trail
        for (size_t i = 1; i < points.size(); ++i) {
            SlashPoint& p1 = points[i - 1];
            SlashPoint& p2 = points[i];

            float alpha = (p1.alpha + p2.alpha) / 2.0f * (lifetime / maxLifetime);

            // Glow effect
            for (int g = 3; g > 0; g--) {
                Color glowColor = color;
                glowColor.a *= alpha * (g / 3.0f) * 0.3f;

                SDL_Color c = glowColor.toSDL();
                draw.color(c.r, c.g, c.b, c.a);

                float width = (p1.width + p2.width) / 2.0f * g;
                // Draw thick line between points
                for (float t = 0; t <= 1.0f; t += 0.1f) {
                    Vec2 pos = Vec2::lerp(p1.position, p2.position, t);
                    draw.fill_circle(pos.x, pos.y, width / 2);
                }
            }

            // Core line
            Color coreColor(255, 255, 255);
            coreColor.a = alpha;
            SDL_Color c = coreColor.toSDL();
            draw.color(c.r, c.g, c.b, c.a);
            draw.line(p1.position.x, p1.position.y, p2.position.x, p2.position.y);
        }
    }
};

// ===== HITBOX SYSTEM =====
class Hitbox {
public:
    Vec2 position;
    Vec2 size;
    Vec2 offset;
    bool active;
    float damage;
    float knockback;
    int hitStun;

    Hitbox() : position(0, 0), size(0, 0), offset(0, 0),
        active(false), damage(0), knockback(0), hitStun(0) {
    }

    Hitbox(Vec2 pos, Vec2 sz, float dmg = 10.0f, float kb = 5.0f, int stun = 10)
        : position(pos), size(sz), offset(0, 0), active(true),
        damage(dmg), knockback(kb), hitStun(stun) {
    }

    SDL_FRect getRect() const {
        return { position.x + offset.x - size.x / 2,
                position.y + offset.y - size.y / 2,
                size.x, size.y };
    }

    bool intersects(const Hitbox& other) const {
        if (!active || !other.active) return false;
        SDL_FRect r1 = getRect();
        SDL_FRect r2 = other.getRect();
        return SDL_HasRectIntersectionFloat(&r1, &r2);
    }

    void draw(Draw& draw, Color color = Color(255, 0, 0, 100)) {
        if (!active) return;
        SDL_FRect rect = getRect();
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);
        draw.rect(rect.x, rect.y, rect.w, rect.h);
    }
};

// ===== COMBAT SYSTEM =====
class CombatStats {
public:
    float maxHealth;
    float health;
    float attack;
    float defense;
    float speed;
    float critChance;
    float critDamage;

    CombatStats(float hp = 100, float atk = 10, float def = 5, float spd = 5)
        : maxHealth(hp), health(hp), attack(atk), defense(def), speed(spd),
        critChance(0.1f), critDamage(2.0f) {
    }

    float calculateDamage(float baseDamage) {
        float damage = baseDamage * (attack / 10.0f);
        if (Utils::randomFloat(0, 1) < critChance) {
            damage *= critDamage;
        }
        return damage;
    }

    void takeDamage(float damage) {
        float reducedDamage = damage * (1.0f - (defense / (defense + 50.0f)));
        health -= reducedDamage;
        health = std::max(0.0f, health);
    }

    bool isAlive() const { return health > 0; }
    float getHealthPercent() const { return health / maxHealth; }
};

// ===== ANIMATION SYSTEM =====
class AnimationFrame {
public:
    std::string name;
    int duration;
    std::function<void(Entity*)> onEnter;
    std::function<void(Entity*)> onUpdate;
    std::function<void(Entity*)> onExit;

    AnimationFrame(const std::string& n, int d)
        : name(n), duration(d), onEnter(nullptr), onUpdate(nullptr), onExit(nullptr) {
    }
};

class AnimationController {
private:
    std::unordered_map<std::string, std::vector<AnimationFrame>> animations;
    std::string currentAnimation;
    size_t currentFrame;
    int frameTimer;
    bool looping;
    Entity* owner;

public:
    AnimationController(Entity* entity)
        : currentAnimation("idle"), currentFrame(0), frameTimer(0),
        looping(true), owner(entity) {
    }

    void addAnimation(const std::string& name, const std::vector<AnimationFrame>& frames) {
        animations[name] = frames;
    }

    void play(const std::string& name, bool loop = true) {
        if (currentAnimation != name) {
            if (hasAnimation(currentAnimation) && currentFrame < animations[currentAnimation].size()) {
                auto& frame = animations[currentAnimation][currentFrame];
                if (frame.onExit) frame.onExit(owner);
            }

            currentAnimation = name;
            currentFrame = 0;
            frameTimer = 0;
            looping = loop;

            if (hasAnimation(name) && !animations[name].empty()) {
                auto& frame = animations[name][0];
                if (frame.onEnter) frame.onEnter(owner);
            }
        }
    }

    void update(float dt) {
        if (!hasAnimation(currentAnimation)) return;

        auto& anim = animations[currentAnimation];
        if (anim.empty()) return;

        frameTimer++;

        auto& frame = anim[currentFrame];
        if (frame.onUpdate) frame.onUpdate(owner);

        if (frameTimer >= frame.duration) {
            if (frame.onExit) frame.onExit(owner);

            frameTimer = 0;
            currentFrame++;

            if (currentFrame >= anim.size()) {
                if (looping) {
                    currentFrame = 0;
                }
                else {
                    currentFrame = anim.size() - 1;
                    return;
                }
            }

            auto& nextFrame = anim[currentFrame];
            if (nextFrame.onEnter) nextFrame.onEnter(owner);
        }
    }

    bool hasAnimation(const std::string& name) const {
        return animations.find(name) != animations.end();
    }

    bool isPlaying(const std::string& name) const {
        return currentAnimation == name;
    }

    bool isFinished() const {
        if (!hasAnimation(currentAnimation) || looping) return false;
        auto& anim = animations.at(currentAnimation);
        return currentFrame >= anim.size() - 1;
    }

    std::string getCurrentAnimation() const { return currentAnimation; }
    size_t getCurrentFrame() const { return currentFrame; }
};

// ===== ENTITY BASE CLASS =====
class Entity {
public:
    Vec2 position;
    Vec2 velocity;
    Vec2 size;
    bool facingRight;
    bool onGround;

    CombatStats stats;
    Hitbox hurtbox;
    Hitbox attackHitbox;
    AnimationController* animator;

    int hitStun;
    int invulnerableFrames;
    bool blocking;
    float blockMeter;

    std::vector<std::unique_ptr<Particle>> particles;

    Entity(Vec2 pos, Vec2 sz = Vec2(30, 60))
        : position(pos), velocity(0, 0), size(sz), facingRight(true),
        onGround(false), hitStun(0), invulnerableFrames(0),
        blocking(false), blockMeter(100.0f) {

        hurtbox = Hitbox(position, size);
        attackHitbox = Hitbox(position, Vec2(0, 0));
        animator = new AnimationController(this);
        setupAnimations();
    }

    virtual ~Entity() {
        delete animator;
    }

    virtual void setupAnimations() {
        // Override in derived classes
    }

    virtual void update(float dt, float timeScale = 1.0f) {
        float scaledDt = dt * timeScale;

        // Update physics
        if (!onGround) {
            velocity.y += GRAVITY * scaledDt * 60;
        }

        // Apply friction
        if (onGround) {
            velocity.x *= std::pow(0.85f, scaledDt * 60);
        }
        else {
            velocity.x *= std::pow(0.98f, scaledDt * 60);
        }

        // Update position
        position += velocity * scaledDt * 60;

        // Ground collision
        if (position.y + size.y / 2 > GROUND_Y) {
            position.y = GROUND_Y - size.y / 2;
            velocity.y = 0;
            onGround = true;
        }
        else {
            onGround = false;
        }

        // Screen boundaries
        position.x = std::max(size.x / 2, std::min(position.x, SCREEN_WIDTH - size.x / 2));

        // Update hitboxes
        hurtbox.position = position;
        attackHitbox.position = position;
        if (!facingRight) {
            attackHitbox.offset.x = -std::abs(attackHitbox.offset.x);
        }
        else {
            attackHitbox.offset.x = std::abs(attackHitbox.offset.x);
        }

        // Update timers
        if (hitStun > 0) hitStun--;
        if (invulnerableFrames > 0) invulnerableFrames--;

        // Update block meter
        if (!blocking && blockMeter < 100.0f) {
            blockMeter += scaledDt * 20;
            blockMeter = std::min(100.0f, blockMeter);
        }

        // Update animation
        animator->update(scaledDt);

        // Update particles
        updateParticles(scaledDt);
    }

    void updateParticles(float dt) {
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const std::unique_ptr<Particle>& p) { return !p->isAlive(); }),
            particles.end()
        );

        for (auto& particle : particles) {
            particle->update(dt);
        }
    }

    virtual void takeDamage(float damage, Vec2 knockbackDir, int stun) {
        if (invulnerableFrames > 0) return;

        if (blocking && blockMeter > 0) {
            // Reduced damage and knockback when blocking
            damage *= 0.3f;
            knockbackDir *= 0.3f;
            blockMeter -= damage * 2;

            // Spark particles on block
            for (int i = 0; i < 5; ++i) {
                float angle = Utils::randomFloat(-PI / 4, PI / 4);
                Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(2, 5));
                particles.push_back(std::make_unique<SparkParticle>(position, vel));
            }

            if (blockMeter <= 0) {
                blocking = false;
                blockMeter = 0;
                hitStun = stun * 2; // Guard break stun
            }
        }
        else {
            stats.takeDamage(damage);
            velocity += knockbackDir;
            hitStun = stun;
            invulnerableFrames = 30;

            // Blood particles
            for (int i = 0; i < 10; ++i) {
                float angle = Utils::randomFloat(0, TWO_PI);
                Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(2, 8));
                particles.push_back(std::make_unique<BloodParticle>(position, vel));
            }
        }
    }

    virtual void draw(Draw& draw) = 0;

    void drawParticles(Draw& draw) {
        for (auto& particle : particles) {
            particle->draw(draw);
        }
    }

    void drawHealthBar(Draw& draw) {
        float barWidth = 50;
        float barHeight = 6;
        Vec2 barPos = position + Vec2(0, -size.y / 2 - 20);

        // Background
        draw.color(50, 50, 50);
        draw.fill_rect(barPos.x - barWidth / 2, barPos.y - barHeight / 2, barWidth, barHeight);

        // Health
        float healthPercent = stats.getHealthPercent();
        Color healthColor = Color::lerp(Color(255, 0, 0), Color(0, 255, 0), healthPercent);
        SDL_Color c = healthColor.toSDL();
        draw.color(c.r, c.g, c.b);
        draw.fill_rect(barPos.x - barWidth / 2, barPos.y - barHeight / 2,
            barWidth * healthPercent, barHeight);

        // Border
        draw.color(0, 0, 0);
        draw.rect(barPos.x - barWidth / 2, barPos.y - barHeight / 2, barWidth, barHeight);

        // Block meter (if blocking)
        if (blocking && blockMeter > 0) {
            float blockBarY = barPos.y + barHeight;
            draw.color(100, 100, 255, 180);
            draw.fill_rect(barPos.x - barWidth / 2, blockBarY,
                barWidth * (blockMeter / 100.0f), 3);
        }
    }
};

// ===== PLAYER CLASS =====
class Player : public Entity {
private:
    InputManager* input;
    SlashEffect slashEffect;
    std::vector<std::unique_ptr<SlashEffect>> activeSlashes;

    // Abilities
    std::unordered_map<AbilityType, float> abilityCooldowns;
    bool timeSlowActive;
    float timeSlowDuration;
    float dashCooldown;
    bool isDashing;
    Vec2 dashDirection;
    float dashTime;

    // Combat
    int comboCount;
    float comboTimer;
    AttackType lastAttack;
    bool canCancelAttack;

    // Animation state
    float armAngle;
    float legAngle;
    float bodyLean;
    float swordAngle;
    bool showAfterImage;
    std::deque<std::pair<Vec2, float>> afterImages;

public:
    Player(Vec2 pos, InputManager* inputMgr)
        : Entity(pos, Vec2(30, 60)), input(inputMgr), timeSlowActive(false),
        timeSlowDuration(0), dashCooldown(0), isDashing(false),
        dashTime(0), comboCount(0), comboTimer(0),
        lastAttack(AttackType::HORIZONTAL_SLASH), canCancelAttack(false),
        armAngle(0), legAngle(0), bodyLean(0), swordAngle(0),
        showAfterImage(false) {

        stats = CombatStats(150, 15, 8, 7);
        stats.critChance = 0.25f;

        // Initialize ability cooldowns
        for (int i = 0; i < 8; ++i) {
            abilityCooldowns[static_cast<AbilityType>(i)] = 0;
        }
    }

    void setupAnimations() override {
        // Idle animation
        std::vector<AnimationFrame> idle = {
            AnimationFrame("idle", 60)
        };
        idle[0].onUpdate = [](Entity* e) {
            Player* p = static_cast<Player*>(e);
            p->swordAngle = std::sin(SDL_GetTicks() * 0.002f) * 5;
            p->bodyLean = std::sin(SDL_GetTicks() * 0.001f) * 2;
            };
        animator->addAnimation("idle", idle);

        // Attack animations
        std::vector<AnimationFrame> hSlash = {
            AnimationFrame("windup", 5),
            AnimationFrame("slash", 8),
            AnimationFrame("recovery", 10)
        };
        hSlash[0].onEnter = [](Entity* e) {
            Player* p = static_cast<Player*>(e);
            p->swordAngle = -45;
            p->armAngle = -30;
            };
        hSlash[1].onEnter = [](Entity* e) {
            Player* p = static_cast<Player*>(e);
            p->swordAngle = 90;
            p->armAngle = 45;
            p->attackHitbox.active = true;
            p->attackHitbox.size = Vec2(SWORD_REACH, 30);
            p->attackHitbox.offset = Vec2(SWORD_REACH / 2, 0);
            p->canCancelAttack = true;
            };
        hSlash[2].onExit = [](Entity* e) {
            Player* p = static_cast<Player*>(e);
            p->attackHitbox.active = false;
            p->canCancelAttack = false;
            };
        animator->addAnimation("horizontal_slash", hSlash);
    }

    void update(float dt, float timeScale = 1.0f) override {
        Entity::update(dt, timeScale);

        // Handle input
        if (hitStun <= 0) {
            handleMovement(dt, timeScale);
            handleCombat(dt, timeScale);
            handleAbilities(dt, timeScale);
        }

        // Update abilities
        updateAbilities(dt);

        // Update combo
        if (comboTimer > 0) {
            comboTimer -= dt;
            if (comboTimer <= 0) {
                comboCount = 0;
            }
        }

        // Update dash
        if (isDashing) {
            updateDash(dt, timeScale);
        }

        // Update after images
        if (showAfterImage || isDashing || timeSlowActive) {
            afterImages.push_back({ position, 0.5f });
            if (afterImages.size() > 10) {
                afterImages.pop_front();
            }
        }

        // Fade after images
        for (auto& [pos, alpha] : afterImages) {
            alpha -= dt * 2;
        }
        afterImages.erase(
            std::remove_if(afterImages.begin(), afterImages.end(),
                [](const std::pair<Vec2, float>& img) { return img.second <= 0; }),
            afterImages.end()
        );

        // Update active slashes
        activeSlashes.erase(
            std::remove_if(activeSlashes.begin(), activeSlashes.end(),
                [](const std::unique_ptr<SlashEffect>& s) { return !s->active; }),
            activeSlashes.end()
        );

        for (auto& slash : activeSlashes) {
            slash->update(dt);
        }
    }

    void handleMovement(float dt, float timeScale) {
        // Horizontal movement
        float moveSpeed = stats.speed;
        if (timeSlowActive) moveSpeed *= 2; // Move faster during time slow

        if (input->isKeyPressed(SDL_SCANCODE_A)) {
            velocity.x = -moveSpeed;
            facingRight = false;
        }
        else if (input->isKeyPressed(SDL_SCANCODE_D)) {
            velocity.x = moveSpeed;
            facingRight = true;
        }

        // Jumping
        if (input->isKeyPressed(SDL_SCANCODE_SPACE) && onGround) {
            velocity.y = -12;
            onGround = false;

            // Jump particles
            for (int i = 0; i < 5; ++i) {
                Vec2 vel(Utils::randomFloat(-2, 2), Utils::randomFloat(-1, 0));
                particles.push_back(std::make_unique<Particle>(
                    position + Vec2(0, size.y / 2), vel, Color(200, 200, 200), 0.5f));
            }
        }

        // Dash
        if (input->isKeyPressed(SDL_SCANCODE_LSHIFT) && dashCooldown <= 0) {
            startDash();
        }

        // Block
        blocking = input->isMouseRightPressed() && blockMeter > 0;
    }

    void handleCombat(float dt, float timeScale) {
        if (input->isMouseLeftJustPressed()) {
            AttackType attack = input->getAttackFromGesture();
            performAttack(attack);
        }

        // Update slash effect with mouse position
        if (attackHitbox.active && input->isMouseLeftPressed()) {
            slashEffect.addPoint(input->getMousePos());
        }
    }

    void handleAbilities(float dt, float timeScale) {
        // Time slow (Q)
        if (input->isKeyPressed(SDL_SCANCODE_Q) &&
            abilityCooldowns[AbilityType::TIME_SLOW] <= 0) {
            activateTimeSlow();
        }

        // Shockwave (E)
        if (input->isKeyPressed(SDL_SCANCODE_E) &&
            abilityCooldowns[AbilityType::SHOCKWAVE] <= 0) {
            performShockwave();
        }

        // Teleport to mouse (R)
        if (input->isKeyPressed(SDL_SCANCODE_R) &&
            abilityCooldowns[AbilityType::TELEPORT] <= 0) {
            teleportToMouse();
        }
    }

    void performAttack(AttackType type) {
        if (animator->isPlaying("attack") && !canCancelAttack) return;

        // Create slash effect
        auto slash = std::make_unique<SlashEffect>();
        slash->start(position + Vec2(facingRight ? 30 : -30, 0));
        activeSlashes.push_back(std::move(slash));

        // Combo system
        if (comboTimer > 0 && type != lastAttack) {
            comboCount++;
            comboCount = std::min(comboCount, 10);
        }
        else {
            comboCount = 1;
        }
        comboTimer = 1.0f;
        lastAttack = type;

        // Apply combo multiplier to damage
        float comboMultiplier = 1.0f + (comboCount - 1) * 0.2f;

        switch (type) {
        case AttackType::HORIZONTAL_SLASH:
            animator->play("horizontal_slash", false);
            attackHitbox.damage = stats.attack * comboMultiplier;
            attackHitbox.knockback = 5 + comboCount;
            attackHitbox.hitStun = 10 + comboCount * 2;
            break;

        case AttackType::UPPERCUT:
            attackHitbox.damage = stats.attack * 1.5f * comboMultiplier;
            attackHitbox.knockback = 8;
            attackHitbox.hitStun = 15;
            attackHitbox.size = Vec2(60, 80);
            attackHitbox.offset = Vec2(30, -40);
            velocity.y = -8; // Jump with uppercut
            break;

        case AttackType::SPIN_ATTACK:
            attackHitbox.damage = stats.attack * 2 * comboMultiplier;
            attackHitbox.knockback = 10;
            attackHitbox.hitStun = 20;
            attackHitbox.size = Vec2(120, 60);
            attackHitbox.offset = Vec2(0, 0);
            showAfterImage = true;
            break;

        default:
            attackHitbox.damage = stats.attack * comboMultiplier;
            attackHitbox.knockback = 5;
            attackHitbox.hitStun = 10;
            break;
        }

        attackHitbox.active = true;
    }

    void startDash() {
        isDashing = true;
        dashTime = 0.2f;
        dashCooldown = 0.5f;

        // Dash towards mouse or movement direction
        Vec2 mousePos = input->getMousePos();
        dashDirection = (mousePos - position).normalized();

        if (dashDirection.length() < 0.1f) {
            dashDirection = Vec2(facingRight ? 1 : -1, 0);
        }

        invulnerableFrames = 12; // Brief invulnerability during dash
    }

    void updateDash(float dt, float timeScale) {
        if (dashTime > 0) {
            velocity = dashDirection * DASH_SPEED;
            dashTime -= dt;

            // Dash particles
            for (int i = 0; i < 2; ++i) {
                Vec2 vel = -dashDirection * Utils::randomFloat(2, 5);
                particles.push_back(std::make_unique<Particle>(
                    position, vel, Color(100, 100, 255), 0.3f));
            }
        }
        else {
            isDashing = false;
            velocity *= 0.5f; // Slow down after dash
        }
    }

    void activateTimeSlow() {
        timeSlowActive = true;
        timeSlowDuration = 3.0f;
        abilityCooldowns[AbilityType::TIME_SLOW] = 10.0f;

        // Visual effect
        for (int i = 0; i < 20; ++i) {
            float angle = (i / 20.0f) * TWO_PI;
            Vec2 vel = Vec2::fromAngle(angle, 5);
            particles.push_back(std::make_unique<Particle>(
                position, vel, Color(100, 100, 255), 1.0f));
        }
    }

    void performShockwave() {
        abilityCooldowns[AbilityType::SHOCKWAVE] = 5.0f;

        // Create shockwave hitbox
        attackHitbox.active = true;
        attackHitbox.size = Vec2(300, 100);
        attackHitbox.offset = Vec2(0, 0);
        attackHitbox.damage = stats.attack * 3;
        attackHitbox.knockback = 15;
        attackHitbox.hitStun = 30;

        // Visual effect
        for (int i = 0; i < 30; ++i) {
            float angle = Utils::randomFloat(0, TWO_PI);
            Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(5, 15));
            particles.push_back(std::make_unique<SparkParticle>(position, vel));
        }
    }

    void teleportToMouse() {
        Vec2 mousePos = input->getMousePos();
        Vec2 oldPos = position;
        position = mousePos;
        position.y = std::min(position.y, GROUND_Y - size.y / 2);

        abilityCooldowns[AbilityType::TELEPORT] = 3.0f;

        // Teleport particles at both locations
        for (int i = 0; i < 10; ++i) {
            float angle = Utils::randomFloat(0, TWO_PI);
            Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(2, 5));

            particles.push_back(std::make_unique<Particle>(
                oldPos, vel, Color(150, 0, 255), 0.5f));
            particles.push_back(std::make_unique<Particle>(
                position, -vel, Color(150, 0, 255), 0.5f));
        }
    }

    void updateAbilities(float dt) {
        // Update cooldowns
        for (auto& [ability, cooldown] : abilityCooldowns) {
            if (cooldown > 0) cooldown -= dt;
        }

        if (dashCooldown > 0) dashCooldown -= dt;

        // Update time slow
        if (timeSlowActive) {
            timeSlowDuration -= dt;
            if (timeSlowDuration <= 0) {
                timeSlowActive = false;
            }
        }
    }

    void draw(Draw& draw) override {
        // Draw after images
        for (const auto& [imgPos, alpha] : afterImages) {
            drawStickman(draw, imgPos, Color(100, 100, 255, int(alpha * 255)));
        }

        // Draw main character
        Color mainColor = invulnerableFrames > 0 && (invulnerableFrames % 4 < 2) ?
            Color(255, 100, 100) : Color(0, 0, 0);
        drawStickman(draw, position, mainColor);

        // Draw sword
        drawSword(draw);

        // Draw slashes
        for (auto& slash : activeSlashes) {
            slash->draw(draw);
        }

        // Draw particles
        drawParticles(draw);

        // Draw UI elements
        drawHealthBar(draw);
        drawComboCounter(draw);
        drawAbilityCooldowns(draw);

        // Debug hitboxes
#ifdef DEBUG
        hurtbox.draw(draw, Color(0, 255, 0, 100));
        if (attackHitbox.active) {
            attackHitbox.draw(draw, Color(255, 0, 0, 100));
        }
#endif
    }

    void drawStickman(Draw& draw, Vec2 pos, Color color) {
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);

        float headY = pos.y - size.y / 2 + 10;

        // Head
        draw.circle(pos.x, headY, 8);
        draw.fill_circle(pos.x, headY, 7);

        // Body
        float bodyTop = headY + 8;
        float bodyBottom = pos.y + size.y / 2 - 15;
        draw.line(pos.x, bodyTop, pos.x + bodyLean, bodyBottom);

        // Arms
        float shoulderY = bodyTop + 5;
        float armLength = 20;

        // Sword arm
        float swordArmAngle = facingRight ? armAngle : PI - armAngle;
        Vec2 swordElbow = Vec2(pos.x, shoulderY) +
            Vec2::fromAngle(swordArmAngle + PI / 4, armLength / 2);
        Vec2 swordHand = swordElbow +
            Vec2::fromAngle(swordArmAngle, armLength / 2);
        draw.line(pos.x, shoulderY, swordElbow.x, swordElbow.y);
        draw.line(swordElbow.x, swordElbow.y, swordHand.x, swordHand.y);

        // Other arm
        float otherArmAngle = -armAngle * 0.5f;
        Vec2 otherElbow = Vec2(pos.x, shoulderY) +
            Vec2::fromAngle(otherArmAngle + PI * 0.75f, armLength / 2);
        Vec2 otherHand = otherElbow +
            Vec2::fromAngle(otherArmAngle + PI, armLength / 2);
        draw.line(pos.x, shoulderY, otherElbow.x, otherElbow.y);
        draw.line(otherElbow.x, otherElbow.y, otherHand.x, otherHand.y);

        // Legs
        float hipY = bodyBottom;
        float legLength = 25;

        if (isDashing || std::abs(velocity.x) > 3) {
            // Running animation
            float runCycle = SDL_GetTicks() * 0.01f;
            float leg1Angle = std::sin(runCycle) * 0.5f;
            float leg2Angle = std::sin(runCycle + PI) * 0.5f;

            Vec2 knee1 = Vec2(pos.x, hipY) +
                Vec2::fromAngle(PI / 2 + leg1Angle, legLength / 2);
            Vec2 foot1 = knee1 +
                Vec2::fromAngle(PI / 2 + leg1Angle * 0.5f, legLength / 2);

            Vec2 knee2 = Vec2(pos.x, hipY) +
                Vec2::fromAngle(PI / 2 + leg2Angle, legLength / 2);
            Vec2 foot2 = knee2 +
                Vec2::fromAngle(PI / 2 + leg2Angle * 0.5f, legLength / 2);

            draw.line(pos.x, hipY, knee1.x, knee1.y);
            draw.line(knee1.x, knee1.y, foot1.x, foot1.y);
            draw.line(pos.x, hipY, knee2.x, knee2.y);
            draw.line(knee2.x, knee2.y, foot2.x, foot2.y);
        }
        else {
            // Standing
            draw.line(pos.x, hipY, pos.x - 5, pos.y + size.y / 2);
            draw.line(pos.x, hipY, pos.x + 5, pos.y + size.y / 2);
        }
    }

    void drawSword(Draw& draw) {
        // Calculate sword position based on arm
        float swordArmAngle = facingRight ? armAngle : PI - armAngle;
        Vec2 shoulderPos = position + Vec2(0, -size.y / 2 + 15);
        Vec2 elbowPos = shoulderPos + Vec2::fromAngle(swordArmAngle + PI / 4, 10);
        Vec2 handPos = elbowPos + Vec2::fromAngle(swordArmAngle, 10);

        // Sword extends from hand
        float swordLength = SWORD_REACH;
        float currentSwordAngle = swordArmAngle + Utils::degToRad(swordAngle);
        Vec2 swordTip = handPos + Vec2::fromAngle(currentSwordAngle, swordLength);

        // Draw sword with glow effect
        if (comboCount > 3) {
            // Combo glow
            for (int i = 3; i > 0; i--) {
                int alpha = 50 * (comboCount - 3) * i / 3;
                draw.color(255, 100, 100, alpha);
                float width = 3 + i;

                // Draw thick glowing line
                Vec2 perpendicular = (swordTip - handPos).perpendicular().normalized();
                std::vector<SDL_FPoint> quad = {
                    {handPos.x - perpendicular.x * width / 2,
                     handPos.y - perpendicular.y * width / 2},
                    {handPos.x + perpendicular.x * width / 2,
                     handPos.y + perpendicular.y * width / 2},
                    {swordTip.x + perpendicular.x * width / 2,
                     swordTip.y + perpendicular.y * width / 2},
                    {swordTip.x - perpendicular.x * width / 2,
                     swordTip.y - perpendicular.y * width / 2},
                    {handPos.x - perpendicular.x * width / 2,
                     handPos.y - perpendicular.y * width / 2}
                };
                draw.polygon(quad);
            }
        }

        // Main sword
        draw.color(200, 200, 200);
        for (int i = -1; i <= 1; i++) {
            Vec2 offset = Vec2(i, 0);
            draw.line(handPos.x + offset.x, handPos.y + offset.y,
                swordTip.x + offset.x, swordTip.y + offset.y);
        }

        // Hilt
        Vec2 hiltPerpendicular = (swordTip - handPos).perpendicular().normalized();
        draw.color(100, 50, 0);
        draw.line(handPos.x - hiltPerpendicular.x * 8,
            handPos.y - hiltPerpendicular.y * 8,
            handPos.x + hiltPerpendicular.x * 8,
            handPos.y + hiltPerpendicular.y * 8);

        // Hand guard
        draw.color(150, 150, 150);
        Vec2 guardPos = handPos + Vec2::fromAngle(currentSwordAngle, 10);
        draw.line(guardPos.x - hiltPerpendicular.x * 10,
            guardPos.y - hiltPerpendicular.y * 10,
            guardPos.x + hiltPerpendicular.x * 10,
            guardPos.y + hiltPerpendicular.y * 10);
    }

    void drawComboCounter(Draw& draw) {
        if (comboCount <= 1) return;

        Vec2 comboPos = position + Vec2(0, -size.y / 2 - 40);

        // Combo text with scaling effect
        float scale = 1.0f + (comboTimer > 0.8f ? (comboTimer - 0.8f) * 2 : 0);
        int fontSize = 12 * scale;

        // Shadow
        draw.color(0, 0, 0, 200);
        draw.fill_rect(comboPos.x - 20, comboPos.y - 10, 40, 20);

        // Text (would need proper text rendering in real implementation)
        Color comboColor = Utils::rainbow(comboCount / 10.0f);
        SDL_Color c = comboColor.toSDL();
        draw.color(c.r, c.g, c.b);

        // Draw combo count
        std::string comboText = std::to_string(comboCount) + "x";
        draw.fill_circle(comboPos.x, comboPos.y, fontSize / 2);
    }

    void drawAbilityCooldowns(Draw& draw) {
        float startX = 20;
        float startY = SCREEN_HEIGHT - 100;
        float iconSize = 40;
        float spacing = 50;

        int abilityIndex = 0;
        for (const auto& [ability, cooldown] : abilityCooldowns) {
            Vec2 iconPos(startX + abilityIndex * spacing, startY);

            // Background
            draw.color(50, 50, 50);
            draw.fill_rect(iconPos.x, iconPos.y, iconSize, iconSize);

            // Cooldown overlay
            if (cooldown > 0) {
                float cooldownPercent = cooldown / 10.0f; // Max 10 second cooldown
                draw.color(0, 0, 0, 180);
                draw.fill_rect(iconPos.x, iconPos.y, iconSize, iconSize * cooldownPercent);
            }

            // Border
            draw.color(100, 100, 100);
            draw.rect(iconPos.x, iconPos.y, iconSize, iconSize);

            // Ability icon (simplified)
            draw.color(255, 255, 255);
            switch (ability) {
            case AbilityType::TIME_SLOW:
                draw.circle(iconPos.x + iconSize / 2, iconPos.y + iconSize / 2, 15);
                break;
            case AbilityType::SHOCKWAVE:
                for (int i = 0; i < 3; i++) {
                    draw.circle(iconPos.x + iconSize / 2, iconPos.y + iconSize / 2, 5 + i * 5);
                }
                break;
            case AbilityType::TELEPORT:
                draw.line(iconPos.x + 10, iconPos.y + 10,
                    iconPos.x + iconSize - 10, iconPos.y + iconSize - 10);
                draw.line(iconPos.x + iconSize - 10, iconPos.y + 10,
                    iconPos.x + 10, iconPos.y + iconSize - 10);
                break;
            }

            abilityIndex++;
            if (abilityIndex >= 3) break; // Only show first 3 abilities
        }
    }

    // Getters
    bool isTimeSlowActive() const { return timeSlowActive; }
    float getTimeScale() const { return timeSlowActive ? TIME_SCALE_SLOW : 1.0f; }
    int getCombo() const { return comboCount; }
};

// ===== ENEMY CLASS =====
class Enemy : public Entity {
private:
    EnemyType type;
    Entity* target;
    float attackCooldown;
    float detectionRange;
    float attackRange;

    // AI state
    enum AIState { IDLE, PATROL, CHASE, ATTACK, RETREAT, STUNNED };
    AIState aiState;
    float stateTimer;
    Vec2 patrolTarget;

    // Combat patterns
    int attackPattern;
    int attackStep;

public:
    Enemy(Vec2 pos, EnemyType enemyType, Entity* player)
        : Entity(pos), type(enemyType), target(player), attackCooldown(0),
        detectionRange(300), attackRange(100), aiState(IDLE),
        stateTimer(0), attackPattern(0), attackStep(0) {

        setupByType();
    }

    void setupByType() {
        switch (type) {
        case EnemyType::BASIC_GRUNT:
            stats = CombatStats(50, 8, 3, 4);
            size = Vec2(25, 50);
            detectionRange = 200;
            attackRange = 60;
            break;

        case EnemyType::HEAVY_BRUTE:
            stats = CombatStats(120, 15, 8, 2);
            size = Vec2(40, 70);
            detectionRange = 150;
            attackRange = 80;
            break;

        case EnemyType::NINJA_ASSASSIN:
            stats = CombatStats(40, 12, 2, 10);
            size = Vec2(20, 45);
            detectionRange = 400;
            attackRange = 50;
            stats.critChance = 0.4f;
            break;

        case EnemyType::BOSS_SAMURAI:
            stats = CombatStats(300, 20, 10, 6);
            size = Vec2(35, 65);
            detectionRange = 500;
            attackRange = 100;
            break;
        }
    }

    void update(float dt, float timeScale = 1.0f) override {
        Entity::update(dt, timeScale);

        if (hitStun > 0) {
            aiState = STUNNED;
            return;
        }

        updateAI(dt, timeScale);

        if (attackCooldown > 0) {
            attackCooldown -= dt * timeScale;
        }
    }

    void updateAI(float dt, float timeScale) {
        float distanceToTarget = (target->position - position).length();
        Vec2 dirToTarget = (target->position - position).normalized();

        // Face target
        facingRight = target->position.x > position.x;

        switch (aiState) {
        case IDLE:
            if (distanceToTarget < detectionRange) {
                aiState = CHASE;
            }
            else {
                // Start patrolling
                aiState = PATROL;
                patrolTarget = position + Vec2(Utils::randomFloat(-100, 100), 0);
            }
            break;

        case PATROL:
        {
            Vec2 toPatrolTarget = patrolTarget - position;
            if (toPatrolTarget.length() > 10) {
                velocity.x = toPatrolTarget.normalized().x * stats.speed * 0.5f;
            }
            else {
                // Reached patrol point, pick new one
                patrolTarget = position + Vec2(Utils::randomFloat(-150, 150), 0);
            }

            if (distanceToTarget < detectionRange) {
                aiState = CHASE;
            }
        }
        break;

        case CHASE:
            if (distanceToTarget > detectionRange * 1.5f) {
                aiState = IDLE;
            }
            else if (distanceToTarget < attackRange) {
                aiState = ATTACK;
                velocity.x = 0;
            }
            else {
                // Move towards target
                velocity.x = dirToTarget.x * stats.speed;

                // Jump if target is above
                if (target->position.y < position.y - 50 && onGround) {
                    velocity.y = -10;
                }
            }
            break;

        case ATTACK:
            if (distanceToTarget > attackRange * 1.2f) {
                aiState = CHASE;
            }
            else if (attackCooldown <= 0) {
                performAttackPattern();
                attackCooldown = 1.0f / (stats.speed / 5.0f);
            }
            break;

        case RETREAT:
            velocity.x = -dirToTarget.x * stats.speed * 1.5f;
            stateTimer -= dt;
            if (stateTimer <= 0 || distanceToTarget > attackRange * 2) {
                aiState = CHASE;
            }
            break;

        case STUNNED:
            // Handled in main update
            break;
        }
    }

    void performAttackPattern() {
        attackHitbox.active = true;
        attackHitbox.damage = stats.attack;
        attackHitbox.knockback = 5;
        attackHitbox.hitStun = 10;

        switch (type) {
        case EnemyType::BASIC_GRUNT:
            // Simple horizontal attack
            attackHitbox.size = Vec2(60, 30);
            attackHitbox.offset = Vec2(facingRight ? 30 : -30, 0);

            // Attack particles
            for (int i = 0; i < 3; ++i) {
                Vec2 vel = Vec2(facingRight ? 3 : -3, Utils::randomFloat(-2, 2));
                particles.push_back(std::make_unique<Particle>(
                    position + attackHitbox.offset, vel, Color(255, 100, 100), 0.3f));
            }
            break;

        case EnemyType::NINJA_ASSASSIN:
            // Quick multi-hit combo
            if (attackStep == 0) {
                attackHitbox.size = Vec2(50, 20);
                attackHitbox.offset = Vec2(facingRight ? 25 : -25, -10);
                attackStep = 1;
                attackCooldown = 0.2f;
            }
            else if (attackStep == 1) {
                attackHitbox.size = Vec2(50, 20);
                attackHitbox.offset = Vec2(facingRight ? 25 : -25, 10);
                attackStep = 2;
                attackCooldown = 0.2f;
            }
            else {
                // Dash attack
                velocity.x = (facingRight ? 1 : -1) * 15;
                attackHitbox.size = Vec2(70, 30);
                attackHitbox.offset = Vec2(facingRight ? 35 : -35, 0);
                attackStep = 0;
                attackCooldown = 1.5f;
            }
            break;

        case EnemyType::BOSS_SAMURAI:
            // Complex pattern with multiple attacks
            attackPattern = (attackPattern + 1) % 3;
            switch (attackPattern) {
            case 0: // Horizontal sweep
                attackHitbox.size = Vec2(100, 40);
                attackHitbox.offset = Vec2(facingRight ? 50 : -50, 0);
                attackHitbox.damage = stats.attack * 1.5f;
                break;
            case 1: // Uppercut
                velocity.y = -8;
                attackHitbox.size = Vec2(60, 80);
                attackHitbox.offset = Vec2(facingRight ? 30 : -30, -40);
                attackHitbox.damage = stats.attack * 2;
                attackHitbox.knockback = 10;
                break;
            case 2: // Ground slam
                attackHitbox.size = Vec2(150, 60);
                attackHitbox.offset = Vec2(0, 30);
                attackHitbox.damage = stats.attack * 2.5f;
                attackHitbox.knockback = 15;
                attackHitbox.hitStun = 20;

                // Shockwave particles
                for (int i = 0; i < 10; ++i) {
                    float angle = Utils::randomFloat(-PI / 4, PI / 4);
                    Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(5, 10));
                    vel.y = -std::abs(vel.y);
                    particles.push_back(std::make_unique<SparkParticle>(
                        position + Vec2(0, size.y / 2), vel));
                }
                break;
            }
            break;
        }

        // Schedule hitbox deactivation
        stateTimer = 0.2f;
    }

    void takeDamage(float damage, Vec2 knockbackDir, int stun) override {
        Entity::takeDamage(damage, knockbackDir, stun);

        // Retreat after taking damage (for smarter enemies)
        if (type == EnemyType::NINJA_ASSASSIN && stats.getHealthPercent() < 0.3f) {
            aiState = RETREAT;
            stateTimer = 1.0f;
        }
    }

    void draw(Draw& draw) override {
        // Draw based on enemy type
        Color enemyColor = Color(100, 0, 0);
        if (invulnerableFrames > 0 && (invulnerableFrames % 4 < 2)) {
            enemyColor = Color(255, 100, 100);
        }

        switch (type) {
        case EnemyType::BASIC_GRUNT:
            drawBasicEnemy(draw, enemyColor);
            break;
        case EnemyType::NINJA_ASSASSIN:
            drawNinjaEnemy(draw, enemyColor);
            break;
        case EnemyType::BOSS_SAMURAI:
            drawBossEnemy(draw, enemyColor);
            break;
        default:
            drawBasicEnemy(draw, enemyColor);
            break;
        }

        drawParticles(draw);
        drawHealthBar(draw);

        // Draw attack telegraph
        if (attackCooldown > 0.8f && attackCooldown < 1.0f) {
            draw.color(255, 0, 0, 100);
            SDL_FRect telegraphRect = attackHitbox.getRect();
            draw.rect(telegraphRect.x, telegraphRect.y, telegraphRect.w, telegraphRect.h);
        }
    }

    void drawBasicEnemy(Draw& draw, Color color) {
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);

        // Simple stickman
        float headY = position.y - size.y / 2 + 8;

        // Head
        draw.fill_circle(position.x, headY, 6);

        // Body
        draw.line(position.x, headY + 6, position.x, position.y + size.y / 2 - 10);

        // Arms
        float armY = headY + 10;
        draw.line(position.x, armY, position.x - 10, armY + 10);
        draw.line(position.x, armY, position.x + 10, armY + 10);

        // Legs
        float legY = position.y + size.y / 2 - 10;
        draw.line(position.x, legY, position.x - 5, position.y + size.y / 2);
        draw.line(position.x, legY, position.x + 5, position.y + size.y / 2);

        // Weapon (club)
        if (attackHitbox.active) {
            draw.color(100, 50, 0);
            Vec2 weaponPos = position + attackHitbox.offset;
            draw.fill_rect(weaponPos.x - 5, weaponPos.y - 15, 10, 30);
        }
    }

    void drawNinjaEnemy(Draw& draw, Color color) {
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);

        float headY = position.y - size.y / 2 + 6;

        // Head with mask
        draw.fill_circle(position.x, headY, 5);
        draw.color(50, 50, 50);
        draw.fill_rect(position.x - 5, headY - 2, 10, 4);

        // Slim body
        draw.color(c.r, c.g, c.b, c.a);
        draw.line(position.x, headY + 5, position.x, position.y + size.y / 2 - 8);

        // Dynamic arms
        float armY = headY + 8;
        if (attackHitbox.active) {
            // Attack pose
            float attackAngle = facingRight ? -PI / 4 : -3 * PI / 4;
            Vec2 hand = Vec2(position.x, armY) + Vec2::fromAngle(attackAngle, 15);
            draw.line(position.x, armY, hand.x, hand.y);

            // Kunai
            draw.color(150, 150, 150);
            Vec2 kunaiTip = hand + Vec2::fromAngle(attackAngle, 20);
            draw.line(hand.x, hand.y, kunaiTip.x, kunaiTip.y);
        }
        else {
            draw.line(position.x, armY, position.x - 8, armY + 8);
            draw.line(position.x, armY, position.x + 8, armY + 8);
        }

        // Crouched legs
        draw.color(c.r, c.g, c.b, c.a);
        float legY = position.y + size.y / 2 - 8;
        draw.line(position.x, legY, position.x - 6, position.y + size.y / 2);
        draw.line(position.x, legY, position.x + 6, position.y + size.y / 2);
    }

    void drawBossEnemy(Draw& draw, Color color) {
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);

        float headY = position.y - size.y / 2 + 10;

        // Samurai helmet
        draw.fill_circle(position.x, headY, 8);
        draw.color(150, 50, 50);
        std::vector<SDL_FPoint> helmet = {
            {position.x - 10, headY - 5},
            {position.x + 10, headY - 5},
            {position.x + 12, headY},
            {position.x - 12, headY},
            {position.x - 10, headY - 5}
        };
        draw.polygon(helmet);

        // Armored body
        draw.color(c.r, c.g, c.b, c.a);
        std::vector<SDL_FPoint> armor = {
            {position.x - 8, headY + 8},
            {position.x + 8, headY + 8},
            {position.x + 10, position.y},
            {position.x - 10, position.y},
            {position.x - 8, headY + 8}
        };
        draw.polygon(armor);

        // Arms with katana
        float armY = headY + 12;
        if (attackHitbox.active) {
            // Katana strike pose
            float swordAngle = 0;
            switch (attackPattern) {
            case 0: swordAngle = facingRight ? PI / 4 : 3 * PI / 4; break;
            case 1: swordAngle = facingRight ? -PI / 3 : -2 * PI / 3; break;
            case 2: swordAngle = PI / 2; break;
            }

            Vec2 hand = Vec2(position.x, armY) + Vec2::fromAngle(swordAngle - PI / 2, 15);
            draw.line(position.x, armY, hand.x, hand.y);

            // Katana
            Vec2 swordTip = hand + Vec2::fromAngle(swordAngle, 60);
            draw.color(200, 200, 200);
            for (int i = -1; i <= 1; i++) {
                draw.line(hand.x + i, hand.y, swordTip.x + i, swordTip.y);
            }

            // Katana glow during attack
            draw.color(255, 100, 100, 50);
            for (int i = 1; i <= 3; i++) {
                draw.line(hand.x, hand.y, swordTip.x, swordTip.y);
            }
        }
        else {
            // Idle pose
            draw.color(c.r, c.g, c.b, c.a);
            draw.line(position.x, armY, position.x - 10, armY + 12);
            draw.line(position.x, armY, position.x + 10, armY + 12);
        }

        // Legs
        draw.color(c.r, c.g, c.b, c.a);
        float legY = position.y;
        draw.line(position.x - 5, legY, position.x - 7, position.y + size.y / 2);
        draw.line(position.x + 5, legY, position.x + 7, position.y + size.y / 2);
    }
};

// ===== GAME WORLD =====
class GameWorld {
private:
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<Particle>> worldParticles;
    InputManager input;

    GameState gameState;
    int wave;
    int enemiesKilled;
    float waveTimer;
    bool showingWaveText;
    float waveTextTimer;

    // Camera shake
    Vec2 cameraOffset;
    float cameraShakeIntensity;

public:
    GameWorld() : gameState(GameState::PLAYING), wave(1), enemiesKilled(0),
        waveTimer(0), showingWaveText(true), waveTextTimer(2.0f),
        cameraOffset(0, 0), cameraShakeIntensity(0) {

        player = std::make_unique<Player>(Vec2(SCREEN_WIDTH / 2, GROUND_Y - 30), &input);
        spawnWave();
    }

    void spawnWave() {
        enemies.clear();
        showingWaveText = true;
        waveTextTimer = 2.0f;

        int enemyCount = 2 + wave;

        for (int i = 0; i < enemyCount; ++i) {
            float x = Utils::randomFloat(100, SCREEN_WIDTH - 100);
            // Avoid spawning on player
            while (std::abs(x - player->position.x) < 100) {
                x = Utils::randomFloat(100, SCREEN_WIDTH - 100);
            }

            Vec2 spawnPos(x, GROUND_Y - 30);

            // Spawn different enemy types based on wave
            EnemyType type;
            if (wave % 5 == 0) {
                // Boss wave
                type = EnemyType::BOSS_SAMURAI;
            }
            else if (wave > 3 && Utils::randomFloat(0, 1) < 0.3f) {
                type = EnemyType::NINJA_ASSASSIN;
            }
            else if (wave > 2 && Utils::randomFloat(0, 1) < 0.4f) {
                type = EnemyType::HEAVY_BRUTE;
            }
            else {
                type = EnemyType::BASIC_GRUNT;
            }

            enemies.push_back(std::make_unique<Enemy>(spawnPos, type, player.get()));
        }
    }

    void update(float dt) {
        if (gameState != GameState::PLAYING) return;

        input.update();

        // Get time scale from player
        float timeScale = player->getTimeScale();

        // Update entities
        player->update(dt, timeScale);

        for (auto& enemy : enemies) {
            enemy->update(dt, timeScale);
        }

        // Handle combat collisions
        handleCombat();

        // Remove dead enemies
        int killedThisFrame = 0;
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [&killedThisFrame](const std::unique_ptr<Enemy>& e) {
                    if (!e->stats.isAlive()) {
                        killedThisFrame++;
                        return true;
                    }
                    return false;
                }),
            enemies.end()
        );

        enemiesKilled += killedThisFrame;

        // Add screen shake for kills
        if (killedThisFrame > 0) {
            addCameraShake(5.0f * killedThisFrame);
        }

        // Check wave completion
        if (enemies.empty() && !showingWaveText) {
            wave++;
            spawnWave();
        }

        // Update wave text
        if (showingWaveText) {
            waveTextTimer -= dt;
            if (waveTextTimer <= 0) {
                showingWaveText = false;
            }
        }

        // Update world particles
        updateWorldParticles(dt * timeScale);

        // Update camera shake
        updateCameraShake(dt);

        // Check game over
        if (!player->stats.isAlive()) {
            gameState = GameState::GAME_OVER;
        }
    }

    void handleCombat() {
        // Player attacks hitting enemies
        if (player->attackHitbox.active) {
            for (auto& enemy : enemies) {
                if (player->attackHitbox.intersects(enemy->hurtbox)) {
                    Vec2 knockback = (enemy->position - player->position).normalized() *
                        player->attackHitbox.knockback;
                    knockback.y = -5; // Add upward component

                    enemy->takeDamage(player->attackHitbox.damage, knockback,
                        player->attackHitbox.hitStun);

                    // Hit effects
                    createHitEffect(enemy->position);
                    addCameraShake(3.0f);

                    // Disable hitbox after hit (no multi-hit)
                    player->attackHitbox.active = false;
                }
            }
        }

        // Enemy attacks hitting player
        for (auto& enemy : enemies) {
            if (enemy->attackHitbox.active &&
                enemy->attackHitbox.intersects(player->hurtbox)) {

                Vec2 knockback = (player->position - enemy->position).normalized() *
                    enemy->attackHitbox.knockback;
                knockback.y = -3;

                player->takeDamage(enemy->attackHitbox.damage, knockback,
                    enemy->attackHitbox.hitStun);

                createHitEffect(player->position);
                addCameraShake(5.0f);

                enemy->attackHitbox.active = false;
            }
        }
    }

    void createHitEffect(Vec2 pos) {
        // Sparks
        for (int i = 0; i < 8; ++i) {
            float angle = Utils::randomFloat(0, TWO_PI);
            Vec2 vel = Vec2::fromAngle(angle, Utils::randomFloat(3, 8));
            worldParticles.push_back(std::make_unique<SparkParticle>(pos, vel));
        }

        // Impact ring
        for (int i = 0; i < 16; ++i) {
            float angle = (i / 16.0f) * TWO_PI;
            Vec2 vel = Vec2::fromAngle(angle, 4);
            auto particle = std::make_unique<Particle>(pos, vel,
                Color(255, 255, 255), 0.3f, 2);
            particle->gravity = false;
            worldParticles.push_back(std::move(particle));
        }
    }

    void addCameraShake(float intensity) {
        cameraShakeIntensity = std::max(cameraShakeIntensity, intensity);
    }

    void updateCameraShake(float dt) {
        if (cameraShakeIntensity > 0) {
            cameraOffset.x = Utils::randomFloat(-cameraShakeIntensity, cameraShakeIntensity);
            cameraOffset.y = Utils::randomFloat(-cameraShakeIntensity, cameraShakeIntensity);
            cameraShakeIntensity -= dt * 30;
            cameraShakeIntensity = std::max(0.0f, cameraShakeIntensity);
        }
        else {
            cameraOffset = Vec2(0, 0);
        }
    }

    void updateWorldParticles(float dt) {
        worldParticles.erase(
            std::remove_if(worldParticles.begin(), worldParticles.end(),
                [](const std::unique_ptr<Particle>& p) { return !p->isAlive(); }),
            worldParticles.end()
        );

        for (auto& particle : worldParticles) {
            particle->update(dt);
        }
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Apply camera shake
        SDL_SetRenderViewport(renderer, nullptr);
        if (cameraShakeIntensity > 0) {
            SDL_FRect viewport = { cameraOffset.x, cameraOffset.y,
                                 SCREEN_WIDTH, SCREEN_HEIGHT };
            // Note: SDL3 doesn't have SetRenderViewport with FRect, 
            // so we'd need to translate all drawing instead
        }

        // Draw background
        drawBackground(draw);

        // Draw ground
        draw.color(50, 50, 50);
        draw.fill_rect(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y);
        draw.color(30, 30, 30);
        draw.line(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y);

        // Draw world particles
        for (auto& particle : worldParticles) {
            particle->draw(draw);
        }

        // Draw entities
        for (auto& enemy : enemies) {
            enemy->draw(draw);
        }
        player->draw(draw);

        // Draw UI
        drawUI(draw, renderer);

        // Draw wave text
        if (showingWaveText) {
            drawWaveText(draw, renderer);
        }

        // Draw game over screen
        if (gameState == GameState::GAME_OVER) {
            drawGameOver(draw, renderer);
        }
    }

    void drawBackground(Draw& draw) {
        // Gradient background
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            float t = y / (float)SCREEN_HEIGHT;
            int r = 20 + t * 30;
            int g = 20 + t * 30;
            int b = 30 + t * 40;

            // Add time slow effect
            if (player->isTimeSlowActive()) {
                b += 30;
                g += 10;
            }

            draw.color(r, g, b);
            draw.line(0, y, SCREEN_WIDTH, y);
        }

        // Background particles
        static std::vector<Vec2> bgParticles;
        static bool initialized = false;
        if (!initialized) {
            for (int i = 0; i < 50; ++i) {
                bgParticles.push_back(Vec2(Utils::randomFloat(0, SCREEN_WIDTH),
                    Utils::randomFloat(0, GROUND_Y)));
            }
            initialized = true;
        }

        // Draw and update background particles
        for (auto& p : bgParticles) {
            p.x += 0.5f;
            if (p.x > SCREEN_WIDTH) p.x = 0;

            draw.color(100, 100, 100, 50);
            draw.fill_circle(p.x, p.y, 2);
        }
    }

    void drawUI(Draw& draw, SDL_Renderer* renderer) {
        // Wave and score
        draw.color(255, 255, 255);
        std::string waveText = "Wave: " + std::to_string(wave);
        std::string killText = "Kills: " + std::to_string(enemiesKilled);
        std::string comboText = "Combo: " + std::to_string(player->getCombo()) + "x";

        // These would need proper text rendering
        draw.color(0, 0, 0, 180);
        draw.fill_rect(10, 10, 150, 80);
        draw.color(255, 255, 255);
        draw.rect(10, 10, 150, 80);

        // Controls hint
        draw.color(255, 255, 255, 100);
        std::string controls[] = {
            "WASD: Move",
            "Space: Jump",
            "LMB: Attack (draw slash)",
            "RMB: Block",
            "Shift: Dash",
            "Q: Time Slow",
            "E: Shockwave",
            "R: Teleport"
        };

        int yOffset = SCREEN_HEIGHT - 150;
        for (const auto& control : controls) {
            // Would render text here
            yOffset += 15;
        }
    }

    void drawWaveText(Draw& draw, SDL_Renderer* renderer) {
        float alpha = std::min(1.0f, waveTextTimer);

        // Background fade
        draw.color(0, 0, 0, 100 * alpha);
        draw.fill_rect(0, SCREEN_HEIGHT / 2 - 50, SCREEN_WIDTH, 100);

        // Wave text
        std::string text = "WAVE " + std::to_string(wave);
        if (wave % 5 == 0) {
            text += " - BOSS";
        }

        // Would render large text here
        draw.color(255, 255, 255, 255 * alpha);
        float textX = SCREEN_WIDTH / 2 - text.length() * 10;
        float textY = SCREEN_HEIGHT / 2;

        // Simple text visualization
        for (char c : text) {
            draw.fill_rect(textX, textY, 15, 20);
            textX += 20;
        }
    }

    void drawGameOver(Draw& draw, SDL_Renderer* renderer) {
        // Dark overlay
        draw.color(0, 0, 0, 200);
        draw.fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Game over text
        draw.color(255, 0, 0);
        std::string gameOverText = "GAME OVER";
        float textX = SCREEN_WIDTH / 2 - gameOverText.length() * 15;
        float textY = SCREEN_HEIGHT / 2 - 50;

        for (char c : gameOverText) {
            draw.fill_rect(textX, textY, 25, 35);
            textX += 30;
        }

        // Stats
        draw.color(255, 255, 255);
        std::string statsText = "Waves Survived: " + std::to_string(wave) +
            "  Enemies Killed: " + std::to_string(enemiesKilled);
        // Would render stats text

        // Restart prompt
        std::string restartText = "Press R to Restart";
        // Would render restart text
    }

    void restart() {
        player = std::make_unique<Player>(Vec2(SCREEN_WIDTH / 2, GROUND_Y - 30), &input);
        enemies.clear();
        worldParticles.clear();
        wave = 1;
        enemiesKilled = 0;
        gameState = GameState::PLAYING;
        spawnWave();
    }

    bool isGameOver() const { return gameState == GameState::GAME_OVER; }
    InputManager& getInput() { return input; }
};

// ===== MAIN GAME CLASS =====
class StickmanFighter {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Draw draw;
    std::unique_ptr<GameWorld> world;
    bool running;

public:
    StickmanFighter() : window(nullptr), renderer(nullptr), running(true) {}

    ~StickmanFighter() {
        cleanup();
    }

    bool init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        window = SDL_CreateWindow(
            "Stickman Fighter - Katana Zero Style",
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

        Utils::initRandom();

        world = std::make_unique<GameWorld>();

        return true;
    }

    void cleanup() {
        // ... continuing from cleanup() function ...

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_Quit();
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                else if (event.key.key == SDLK_R && world->isGameOver()) {
                    world->restart();
                }
                else if (event.key.key == SDLK_F11) {
                    // Toggle fullscreen
                    Uint32 flags = SDL_GetWindowFlags(window);
                    if (flags & SDL_WINDOW_FULLSCREEN) {
                        SDL_SetWindowFullscreen(window, 0);
                    }
                    else {
                        SDL_SetWindowFullscreen(window, 1);
                    }
                }
            }
        }
    }

    void update(float dt) {
        world->update(dt);
    }

    void render() {
        // Clear screen
        draw.color(30, 30, 40);
        draw.clear();

        // Draw world
        world->draw(draw, renderer);

        // Draw FPS counter
        drawFPS();

        // Present
        draw.present();
    }

    void drawFPS() {
        static int frameCount = 0;
        static float fpsTimer = 0;
        static int currentFPS = 60;
        static Uint64 lastTime = SDL_GetTicks();

        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        frameCount++;
        fpsTimer += deltaTime;

        if (fpsTimer >= 1.0f) {
            currentFPS = frameCount;
            frameCount = 0;
            fpsTimer = 0;
        }

        // Draw FPS
        draw.color(0, 0, 0, 150);
        draw.fill_rect(SCREEN_WIDTH - 80, 10, 70, 25);
        draw.color(255, 255, 255);
        draw.rect(SCREEN_WIDTH - 80, 10, 70, 25);

        // Simple FPS visualization (would need proper text rendering)
        std::string fpsText = "FPS: " + std::to_string(currentFPS);
        float x = SCREEN_WIDTH - 75;
        float y = 18;

        // Draw simple rectangles to represent digits
        for (char c : fpsText) {
            if (c >= '0' && c <= '9') {
                draw.fill_rect(x, y, 3, 5);
                x += 5;
            }
            else if (c == ':' || c == ' ') {
                x += 3;
            }
            else {
                draw.fill_rect(x, y, 4, 5);
                x += 6;
            }
        }
    }

    void run() {
        Uint64 lastTime = SDL_GetTicks();
        const float targetFPS = 60.0f;
        const float targetFrameTime = 1000.0f / targetFPS;

        while (running) {
            Uint64 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            handleEvents();
            update(deltaTime);
            render();

            // Frame rate limiting
            Uint64 frameTime = SDL_GetTicks() - currentTime;
            if (frameTime < targetFrameTime) {
                SDL_Delay(static_cast<Uint32>(targetFrameTime - frameTime));
            }
        }
    }
};

// ===== MENU SYSTEM =====
class MainMenu {
private:
    struct MenuItem {
        std::string text;
        SDL_FRect rect;
        std::function<void()> action;
        bool hovered;
    };

    std::vector<MenuItem> items;
    std::vector<std::unique_ptr<Particle>> particles;
    int selectedIndex;

public:
    MainMenu() : selectedIndex(0) {
        // Initialize menu items
        float centerX = SCREEN_WIDTH / 2;
        float startY = SCREEN_HEIGHT / 2 - 100;
        float itemHeight = 60;
        float itemWidth = 200;

        items = {
            {"START GAME", {centerX - itemWidth / 2, startY, itemWidth, itemHeight}, nullptr, false},
            {"CONTROLS", {centerX - itemWidth / 2, startY + itemHeight + 20, itemWidth, itemHeight}, nullptr, false},
            {"QUIT", {centerX - itemWidth / 2, startY + (itemHeight + 20) * 2, itemWidth, itemHeight}, nullptr, false}
        };

        // Create background particles
        for (int i = 0; i < 30; ++i) {
            Vec2 pos(Utils::randomFloat(0, SCREEN_WIDTH),
                Utils::randomFloat(0, SCREEN_HEIGHT));
            Vec2 vel(Utils::randomFloat(-0.5f, 0.5f),
                Utils::randomFloat(-0.5f, 0.5f));
            particles.push_back(std::make_unique<Particle>(
                pos, vel, Color(100, 100, 150), 999999.0f, 3));
        }
    }

    void update(float dt, float mouseX, float mouseY) {
        // Update particles
        for (auto& particle : particles) {
            particle->update(dt);

            // Wrap around screen
            if (particle->position.x < 0) particle->position.x = SCREEN_WIDTH;
            if (particle->position.x > SCREEN_WIDTH) particle->position.x = 0;
            if (particle->position.y < 0) particle->position.y = SCREEN_HEIGHT;
            if (particle->position.y > SCREEN_HEIGHT) particle->position.y = 0;
        }

        // Update hover states
        SDL_FPoint mousePoint = { mouseX, mouseY };
        for (size_t i = 0; i < items.size(); ++i) {
            items[i].hovered = SDL_PointInRectFloat(&mousePoint, &items[i].rect);
            if (items[i].hovered) {
                selectedIndex = i;
            }
        }
    }

    void draw(Draw& draw, SDL_Renderer* renderer) {
        // Draw background
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            float t = y / (float)SCREEN_HEIGHT;
            int gray = 20 + t * 30;
            draw.color(gray, gray, gray + 10);
            draw.line(0, y, SCREEN_WIDTH, y);
        }

        // Draw particles
        for (auto& particle : particles) {
            particle->draw(draw);
        }

        // Draw title
        drawTitle(draw);

        // Draw menu items
        for (size_t i = 0; i < items.size(); ++i) {
            drawMenuItem(draw, items[i], i == selectedIndex);
        }

        // Draw decorative elements
        drawDecorations(draw);
    }

    void drawTitle(Draw& draw) {
        std::string title = "STICKMAN FIGHTER";
        float titleX = SCREEN_WIDTH / 2 - title.length() * 15;
        float titleY = 100;

        // Title shadow
        draw.color(0, 0, 0, 150);
        for (int i = 0; i < 5; ++i) {
            float offset = i * 2;
            draw.fill_rect(titleX + offset, titleY + offset,
                title.length() * 30, 50);
        }

        // Main title (simulated large text)
        draw.color(255, 255, 255);
        float charX = titleX;
        for (char c : title) {
            if (c != ' ') {
                // Draw stylized character
                draw.fill_rect(charX, titleY, 25, 40);

                // Add some detail
                draw.color(200, 200, 200);
                draw.rect(charX + 2, titleY + 2, 21, 36);
                draw.color(255, 255, 255);
            }
            charX += 30;
        }

        // Subtitle
        std::string subtitle = "Katana Zero Style Combat";
        draw.color(150, 150, 150);
        float subX = SCREEN_WIDTH / 2 - subtitle.length() * 5;
        float subY = titleY + 60;

        for (char c : subtitle) {
            if (c != ' ') {
                draw.fill_rect(subX, subY, 8, 12);
            }
            subX += 10;
        }
    }

    void drawMenuItem(Draw& draw, const MenuItem& item, bool selected) {
        SDL_FRect rect = item.rect;

        if (selected) {
            // Glow effect
            for (int i = 10; i > 0; i--) {
                int alpha = 100 * (10 - i) / 10;
                draw.color(100, 100, 255, alpha);
                draw.rect(rect.x - i, rect.y - i,
                    rect.w + i * 2, rect.h + i * 2);
            }

            // Animated selection indicator
            float time = SDL_GetTicks() / 1000.0f;
            float arrowX = rect.x - 30 + std::sin(time * 5) * 5;

            // Draw arrow
            std::vector<SDL_FPoint> arrow = {
                {arrowX, rect.y + rect.h / 2},
                {arrowX - 15, rect.y + rect.h / 2 - 10},
                {arrowX - 15, rect.y + rect.h / 2 + 10},
                {arrowX, rect.y + rect.h / 2}
            };
            draw.color(255, 255, 255);
            draw.polygon(arrow);
        }

        // Button background
        if (item.hovered) {
            draw.color(60, 60, 80);
        }
        else {
            draw.color(40, 40, 50);
        }
        draw.fill_rect(rect.x, rect.y, rect.w, rect.h);

        // Button border
        draw.color(100, 100, 120);
        draw.rect(rect.x, rect.y, rect.w, rect.h);

        // Text (simplified)
        draw.color(255, 255, 255);
        float textX = rect.x + rect.w / 2 - item.text.length() * 4;
        float textY = rect.y + rect.h / 2 - 6;

        for (char c : item.text) {
            if (c != ' ') {
                draw.fill_rect(textX, textY, 6, 10);
            }
            textX += 8;
        }
    }

    void drawDecorations(Draw& draw) {
        // Draw fighting stickmen silhouettes
        float time = SDL_GetTicks() / 1000.0f;

        // Left fighter
        Vec2 leftPos(150, 400);
        drawFighterSilhouette(draw, leftPos, true,
            std::sin(time * 3) * 30);

        // Right fighter
        Vec2 rightPos(SCREEN_WIDTH - 150, 400);
        drawFighterSilhouette(draw, rightPos, false,
            -std::sin(time * 3 + PI) * 30);

        // Clash effect between them
        if (std::sin(time * 3) > 0.8f) {
            Vec2 clashPos = (leftPos + rightPos) * 0.5f;
            for (int i = 0; i < 5; ++i) {
                float angle = Utils::randomFloat(0, TWO_PI);
                Vec2 sparkPos = clashPos + Vec2::fromAngle(angle,
                    Utils::randomFloat(10, 30));
                draw.color(255, 255, 200, 150);
                draw.fill_circle(sparkPos.x, sparkPos.y, 3);
            }
        }
    }

    void drawFighterSilhouette(Draw& draw, Vec2 pos, bool facingRight, float swordAngle) {
        draw.color(80, 80, 80);

        // Head
        draw.fill_circle(pos.x, pos.y - 25, 6);

        // Body
        draw.line(pos.x, pos.y - 19, pos.x, pos.y);

        // Arms with sword
        float armAngle = facingRight ? swordAngle * DEG_TO_RAD :
            (180 - swordAngle) * DEG_TO_RAD;
        Vec2 hand = pos + Vec2::fromAngle(armAngle, 20);
        draw.line(pos.x, pos.y - 10, hand.x, hand.y);

        // Sword
        Vec2 swordTip = hand + Vec2::fromAngle(armAngle, 40);
        draw.color(120, 120, 120);
        draw.line(hand.x, hand.y, swordTip.x, swordTip.y);

        // Other arm
        draw.color(80, 80, 80);
        draw.line(pos.x, pos.y - 10, pos.x - (facingRight ? -10 : 10), pos.y);

        // Legs
        draw.line(pos.x, pos.y, pos.x - 8, pos.y + 20);
        draw.line(pos.x, pos.y, pos.x + 8, pos.y + 20);
    }

    int handleClick(float mouseX, float mouseY) {
        SDL_FPoint mousePoint = { mouseX, mouseY };
        for (size_t i = 0; i < items.size(); ++i) {
            if (SDL_PointInRectFloat(&mousePoint, &items[i].rect)) {
                return i;
            }
        }
        return -1;
    }

    int handleKeyPress(SDL_Keycode key) {
        switch (key) {
        case SDLK_UP:
        case SDLK_W:
            selectedIndex = (selectedIndex - 1 + items.size()) % items.size();
            break;
        case SDLK_DOWN:
        case SDLK_S:
            selectedIndex = (selectedIndex + 1) % items.size();
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            return selectedIndex;
        }
        return -1;
    }
};

// ===== MAIN FUNCTION =====
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    SDL_SetAppMetadata("Stickman Fighter", "1.0", "com.example.stickfighter");

    StickmanFighter game;
    if (!game.init()) {
        SDL_Log("Failed to initialize game");
        return -1;
    }

    game.run();
    return 0;
}