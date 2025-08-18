// particle_system.cpp - Complete Particle System with Testbed
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <chrono>
#include "renderer2d.cpp"  // Your Draw struct
#include "utils.cpp"        // Utils struct we just created

// Particle system enums
enum class ParticleShape {
    CIRCLE,
    SQUARE,
    TRIANGLE,
    STAR,
    HEXAGON,
    RING,
    HEART,
    DIAMOND,
    CROSS,
    SPIRAL,
    LIGHTNING,
    SMOKE_PUFF,
    FLAME,
    SPARKLE,
    BUBBLE,
    CUSTOM
};

enum class BlendMode {
    NORMAL,
    ADD,
    MULTIPLY,
    SCREEN,
    OVERLAY,
    SOFT_LIGHT,
    COLOR_DODGE
};

enum class EmissionPattern {
    POINT,
    CIRCLE,
    RING,
    CONE,
    BOX,
    SPHERE,
    LINE,
    SPIRAL,
    BURST,
    WAVE,
    FOUNTAIN
};

enum class ParticleBehavior {
    NONE,
    GRAVITY,
    WIND,
    TURBULENCE,
    ATTRACT,
    REPEL,
    ORBIT,
    SWIRL,
    FLOCK,
    SEEK,
    FLEE,
    WANDER,
    FLOW_FIELD,
    MAGNETIC
};

// Color ramp point for gradients
struct ColorRampPoint {
    float t;  // Time (0-1)
    Color color;

    ColorRampPoint(float time, const Color& col) : t(time), color(col) {}
};

// Force field for particle physics
struct ForceField {
    Vec2 position;
    float radius;
    float strength;
    float falloff = 2.0f;

    enum Type {
        ATTRACT,
        REPEL,
        TURBULENCE,
        VORTEX
    } type;

    Vec2 getForce(const Vec2& particlePos) const {
        Vec2 diff = position - particlePos;
        float distance = diff.length();

        if (distance > radius || distance < 0.001f) return { 0, 0 };

        float forceMagnitude = strength * std::pow(1.0f - distance / radius, falloff);

        switch (type) {
        case ATTRACT:
            return diff.normalized() * forceMagnitude;
        case REPEL:
            return diff.normalized() * -forceMagnitude;
        case TURBULENCE:
            return Vec2::fromAngle(Utils::randomFloat(0, TWO_PI), forceMagnitude);
        case VORTEX: {
            Vec2 tangent = diff.perpendicular();
            return tangent.normalized() * forceMagnitude;
        }
        }
        return { 0, 0 };
    }
};

// Main Particle struct
struct Particle {
    // Physics
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    Vec2 previousPos;
    float mass = 1.0f;
    float drag = 0.98f;
    float bounce = 0.8f;

    // Visual
    float size;
    float startSize;
    float endSize;
    float rotation = 0;
    float angularVelocity = 0;
    Color color;
    std::vector<ColorRampPoint> colorRamp;
    ParticleShape shape = ParticleShape::CIRCLE;
    BlendMode blendMode = BlendMode::ADD;
    float glowIntensity = 0;
    float distortionAmount = 0;

    // Lifetime
    float age = 0;
    float lifetime = 1.0f;
    float fadeInTime = 0.1f;
    float fadeOutTime = 0.2f;

    // Trail
    std::deque<Vec2> trail;
    int maxTrailLength = 0;
    float trailFadeRate = 0.9f;

    // Behaviors
    std::vector<ParticleBehavior> behaviors;
    Vec2 target;
    float behaviorStrength = 1.0f;

    // Special effects
    bool hasGlow = false;
    bool hasDistortion = false;
    bool hasShadow = false;
    float pulseRate = 0;
    float pulseAmount = 0;
    float shimmerRate = 0;

    // Collision
    bool collides = false;
    float collisionRadius = 0;

    // Custom data
    std::unordered_map<std::string, float> customData;

    // Constructor
    Particle() {
        reset();
    }

    // Reset particle to initial state
    void reset() {
        position = { 0, 0 };
        velocity = { 0, 0 };
        acceleration = { 0, 0 };
        previousPos = { 0, 0 };
        age = 0;
        rotation = 0;
        angularVelocity = 0;
        size = startSize = endSize = 10;
        trail.clear();
        behaviors.clear();
        customData.clear();
        mass = 1.0f;
        drag = 0.98f;
        bounce = 0.8f;
        hasGlow = false;
        hasDistortion = false;
        hasShadow = false;
        pulseRate = 0;
        pulseAmount = 0;
        shimmerRate = 0;
        collides = false;
        collisionRadius = 5;
        lifetime = 1.0f;
        fadeInTime = 0.1f;
        fadeOutTime = 0.2f;
        maxTrailLength = 0;
        trailFadeRate = 0.9f;
        behaviorStrength = 1.0f;
        target = { 0, 0 };
        shape = ParticleShape::CIRCLE;
        blendMode = BlendMode::ADD;
        glowIntensity = 0;
        distortionAmount = 0;
    }

    // Update particle
    void update(float dt) {
        age += dt;

        // Store previous position for motion blur
        previousPos = position;

        // Apply behaviors
        applyBehaviors(dt);

        // Physics integration
        velocity += acceleration * dt;
        velocity *= drag;
        position += velocity * dt;

        // Clear acceleration for next frame
        acceleration = { 0, 0 };

        // Update rotation
        rotation += angularVelocity * dt;

        // Update trail
        if (maxTrailLength > 0) {
            trail.push_back(position);
            while (trail.size() > static_cast<size_t>(maxTrailLength)) {
                trail.pop_front();
            }
        }

        // Update size with pulse effect
        if (pulseRate > 0) {
            float pulse = std::sin(age * pulseRate * TWO_PI) * pulseAmount;
            size = getCurrentSize() * (1.0f + pulse);
        }
        else {
            size = getCurrentSize();
        }
    }

    // Apply force to particle
    void applyForce(const Vec2& force) {
        acceleration += force / mass;
    }

    // Apply behaviors
    void applyBehaviors(float dt) {
        for (auto behavior : behaviors) {
            switch (behavior) {
            case ParticleBehavior::GRAVITY:
                applyForce({ 0, 98 * mass });
                break;

            case ParticleBehavior::WIND:
                applyForce({ Utils::randomFloat(-10, 10), 0 });
                break;

            case ParticleBehavior::TURBULENCE: {
                float noise = Utils::perlinNoise(position.x * 0.01f, position.y * 0.01f);
                applyForce({ noise * 50, noise * 50 });
                break;
            }

            case ParticleBehavior::ATTRACT:
                if (target.lengthSq() > 0) {
                    Vec2 desired = (target - position).normalized() * 100;
                    applyForce((desired - velocity) * behaviorStrength);
                }
                break;

            case ParticleBehavior::REPEL:
                if (target.lengthSq() > 0) {
                    Vec2 desired = (position - target).normalized() * 100;
                    applyForce((desired - velocity) * behaviorStrength);
                }
                break;

            case ParticleBehavior::ORBIT: {
                Vec2 toTarget = target - position;
                Vec2 tangent = toTarget.perpendicular();
                tangent = tangent.normalized() * 50;
                applyForce(tangent * behaviorStrength);

                // Add slight attraction to maintain orbit
                float dist = toTarget.length();
                if (dist > 100) {
                    applyForce(toTarget.normalized() * 10);
                }
                else if (dist < 50) {
                    applyForce(toTarget.normalized() * -10);
                }
                break;
            }

            case ParticleBehavior::SWIRL: {
                float angle = std::atan2(position.y - target.y, position.x - target.x);
                angle += dt * behaviorStrength;
                float dist = (position - target).length();
                position = target + Vec2::fromAngle(angle, dist);
                break;
            }

            case ParticleBehavior::WANDER: {
                float wanderAngle = Utils::randomFloat(0, TWO_PI);
                applyForce(Vec2::fromAngle(wanderAngle, 20));
                break;
            }

            case ParticleBehavior::FLOW_FIELD: {
                float angle = Utils::perlinNoise(position.x * 0.005f, position.y * 0.005f) * TWO_PI;
                applyForce(Vec2::fromAngle(angle, 30));
                break;
            }

            default:
                break;
            }
        }
    }

    // Get current color based on lifetime
    Color getCurrentColor() const {
        if (colorRamp.empty()) return color;

        float t = age / lifetime;

        // Find surrounding color points
        for (size_t i = 0; i < colorRamp.size() - 1; ++i) {
            if (t >= colorRamp[i].t && t <= colorRamp[i + 1].t) {
                float localT = (t - colorRamp[i].t) / (colorRamp[i + 1].t - colorRamp[i].t);
                return Color::lerp(colorRamp[i].color, colorRamp[i + 1].color, localT);
            }
        }

        // Return last color if beyond range
        return colorRamp.back().color;
    }

    // Get current size based on lifetime
    float getCurrentSize() const {
        float t = age / lifetime;
        float easedT = Utils::easeInOutCubic(t);
        return startSize + (endSize - startSize) * easedT;
    }

    // Get current alpha based on fade in/out
    float getCurrentAlpha() const {
        float alpha = 1.0f;

        // Fade in
        if (age < fadeInTime && fadeInTime > 0) {
            alpha *= age / fadeInTime;
        }

        // Fade out
        float fadeOutStart = lifetime - fadeOutTime;
        if (age > fadeOutStart && fadeOutTime > 0) {
            alpha *= 1.0f - (age - fadeOutStart) / fadeOutTime;
        }

        // Apply shimmer
        if (shimmerRate > 0) {
            float shimmer = std::sin(age * shimmerRate * TWO_PI) * 0.5f + 0.5f;
            alpha *= 0.5f + shimmer * 0.5f;
        }

        return Utils::clamp(alpha, 0.0f, 1.0f);
    }

    // Check if particle is still alive
    bool isAlive() const {
        return age < lifetime;
    }
};

// Particle Emitter struct
struct ParticleEmitter {
    // Particle management
    std::vector<std::unique_ptr<Particle>> activeParticles;
    std::vector<std::unique_ptr<Particle>> particlePool;
    size_t maxParticles = 5000;

    // Transform
    Vec2 position;
    float rotation = 0;
    Vec2 scale = { 1, 1 };

    // Emission
    bool active = true;
    float emissionRate = 100;
    float emissionAccumulator = 0;
    EmissionPattern pattern = EmissionPattern::POINT;
    float patternRadius = 50;
    float patternAngle = TWO_PI;

    // Burst emission
    bool burstMode = false;
    int burstCount = 100;
    float burstInterval = 1.0f;
    float burstTimer = 0;

    // Particle property ranges
    std::pair<float, float> lifetimeRange = { 1.0f, 2.0f };
    std::pair<float, float> sizeRange = { 4.0f, 8.0f };
    std::pair<float, float> speedRange = { 50.0f, 150.0f };
    std::pair<float, float> angleRange = { 0, TWO_PI };
    std::pair<float, float> angularVelRange = { -180.0f, 180.0f };
    std::pair<float, float> massRange = { 0.8f, 1.2f };

    // Visual properties
    std::vector<ColorRampPoint> colorRamp;
    ParticleShape shape = ParticleShape::CIRCLE;
    BlendMode blendMode = BlendMode::ADD;
    bool enableGlow = true;
    float glowIntensity = 1.0f;
    bool enableTrails = false;
    int trailLength = 10;
    float trailFadeRate = 0.9f;

    // Physics
    Vec2 gravity = { 0, 98 };
    Vec2 wind = { 0, 0 };
    float turbulence = 0;
    float drag = 0.98f;
    std::vector<ForceField> forceFields;

    // Behaviors
    std::vector<ParticleBehavior> behaviors;
    Vec2 targetPosition;

    // Special effects
    bool enablePulse = false;
    float pulseRate = 2.0f;
    float pulseAmount = 0.2f;
    bool enableShimmer = false;
    float shimmerRate = 10.0f;
    bool enableDistortion = false;
    float distortionAmount = 0.1f;

    // Collision
    bool enableCollision = false;
    std::vector<SDL_FRect> collisionRects;

    // Callbacks
    std::function<void(Particle&)> onParticleSpawn;
    std::function<void(Particle&)> onParticleUpdate;
    std::function<void(Particle&)> onParticleDeath;

    // Constructor
    ParticleEmitter() {
        init();
    }

    // Initialize emitter
    void init() {
        // Initialize particle pool
        for (size_t i = 0; i < maxParticles; ++i) {
            particlePool.push_back(std::make_unique<Particle>());
        }

        // Default color ramp
        colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };
    }

    // Get particle from pool
    Particle* getPooledParticle() {
        if (particlePool.empty()) return nullptr;

        Particle* p = particlePool.back().release();
        particlePool.pop_back();
        return p;
    }

    // Return particle to pool
    void returnToPool(std::unique_ptr<Particle> particle) {
        if (particlePool.size() < maxParticles) {
            particle->reset();
            particlePool.push_back(std::move(particle));
        }
    }

    // Get emission position based on pattern
    Vec2 getEmissionPosition() const {
        switch (pattern) {
        case EmissionPattern::POINT:
            return position;

        case EmissionPattern::CIRCLE: {
            float angle = Utils::randomFloat(0, TWO_PI);
            float radius = Utils::randomFloat(0, patternRadius);
            return position + Vec2::fromAngle(angle, radius);
        }

        case EmissionPattern::RING: {
            float angle = Utils::randomFloat(0, TWO_PI);
            return position + Vec2::fromAngle(angle, patternRadius);
        }

        case EmissionPattern::CONE: {
            float angle = Utils::randomFloat(-patternAngle / 2, patternAngle / 2) + rotation;
            float distance = Utils::randomFloat(0, patternRadius);
            return position + Vec2::fromAngle(angle, distance);
        }

        case EmissionPattern::BOX: {
            float x = Utils::randomFloat(-patternRadius, patternRadius);
            float y = Utils::randomFloat(-patternRadius, patternRadius);
            return position + Vec2(x, y);
        }

        case EmissionPattern::LINE: {
            float t = Utils::randomFloat(-1, 1);
            Vec2 dir = Vec2::fromAngle(rotation);
            return position + dir * (t * patternRadius);
        }

        case EmissionPattern::SPIRAL: {
            static float spiralAngle = 0;
            spiralAngle += 0.5f;
            float radius = patternRadius * std::fmod(spiralAngle / TWO_PI, 1.0f);
            return position + Vec2::fromAngle(spiralAngle, radius);
        }

        case EmissionPattern::FOUNTAIN: {
            float spreadAngle = Utils::randomFloat(-0.2f, 0.2f);
            return position + Vec2(spreadAngle * patternRadius, 0);
        }

        default:
            return position;
        }
    }

    // Get emission velocity
    Vec2 getEmissionVelocity() const {
        float angle = Utils::randomFloat(angleRange.first, angleRange.second);
        float speed = Utils::randomFloat(speedRange.first, speedRange.second);
        return Vec2::fromAngle(angle, speed);
    }

    // Emit particles
    void emit(int count = 1) {
        for (int i = 0; i < count && activeParticles.size() < maxParticles; ++i) {
            Particle* p = getPooledParticle();
            if (!p) break;

            // Initialize particle properties
            p->position = getEmissionPosition();
            p->velocity = getEmissionVelocity();
            p->lifetime = Utils::randomFloat(lifetimeRange.first, lifetimeRange.second);
            p->startSize = Utils::randomFloat(sizeRange.first, sizeRange.second);
            p->endSize = p->startSize * 0.1f;
            p->size = p->startSize;
            p->rotation = Utils::randomFloat(0, TWO_PI);
            p->angularVelocity = Utils::randomFloat(angularVelRange.first, angularVelRange.second);
            p->mass = Utils::randomFloat(massRange.first, massRange.second);
            p->drag = drag;

            // Visual properties
            p->colorRamp = colorRamp;
            p->shape = shape;
            p->blendMode = blendMode;
            p->hasGlow = enableGlow;
            p->glowIntensity = glowIntensity;
            p->hasDistortion = enableDistortion;
            p->distortionAmount = distortionAmount;

            // Trail
            p->maxTrailLength = enableTrails ? trailLength : 0;
            p->trailFadeRate = trailFadeRate;

            // Behaviors
            p->behaviors = behaviors;
            p->target = targetPosition;

            // Pulse/shimmer
            p->pulseRate = enablePulse ? pulseRate : 0;
            p->pulseAmount = pulseAmount;
            p->shimmerRate = enableShimmer ? shimmerRate : 0;

            // Collision
            p->collides = enableCollision;
            p->collisionRadius = p->startSize / 2;

            // Custom spawn callback
            if (onParticleSpawn) {
                onParticleSpawn(*p);
            }

            activeParticles.push_back(std::unique_ptr<Particle>(p));
        }
    }

    // Burst emission
    void burst() {
        emit(burstCount);
    }

    // Update emitter and particles
    void update(float dt) {
        // Update burst timer
        if (burstMode && active) {
            burstTimer += dt;
            if (burstTimer >= burstInterval) {
                burstTimer = 0;
                burst();
            }
        }

        // Continuous emission
        if (!burstMode && active) {
            emissionAccumulator += dt * emissionRate;
            int numToEmit = static_cast<int>(emissionAccumulator);
            emissionAccumulator -= numToEmit;
            emit(numToEmit);
        }

        // Update particles
        auto it = activeParticles.begin();
        while (it != activeParticles.end()) {
            Particle* p = it->get();

            // Apply force fields
            for (const auto& field : forceFields) {
                p->applyForce(field.getForce(p->position));
            }

            // Apply global forces
            p->applyForce(gravity * p->mass);
            p->applyForce(wind);

            // Apply turbulence
            if (turbulence > 0) {
                float noise = Utils::perlinNoise(
                    p->position.x * 0.01f + p->age,
                    p->position.y * 0.01f + p->age
                );
                p->applyForce({ noise * turbulence, noise * turbulence });
            }

            // Update particle
            p->update(dt);

            // Custom update callback
            if (onParticleUpdate) {
                onParticleUpdate(*p);
            }

            // Handle collision
            if (enableCollision && p->collides) {
                for (const auto& rect : collisionRects) {
                    SDL_FPoint particlePoint = { p->position.x, p->position.y };
                    if (SDL_PointInRectFloat(&particlePoint, &rect)) {
                        // Simple bounce
                        if (p->position.y < rect.y + rect.h / 2) {
                            p->velocity.y *= -p->bounce;
                            p->position.y = rect.y - p->collisionRadius;
                        }
                        else {
                            p->velocity.y *= -p->bounce;
                            p->position.y = rect.y + rect.h + p->collisionRadius;
                        }
                    }
                }
            }

            // Check if particle is dead
            if (!p->isAlive()) {
                if (onParticleDeath) {
                    onParticleDeath(*p);
                }

                // Return to pool
                returnToPool(std::move(*it));
                it = activeParticles.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Clear all particles
    void clear() {
        for (auto& p : activeParticles) {
            returnToPool(std::move(p));
        }
        activeParticles.clear();
    }

    // Draw particles
    void draw(SDL_Renderer* renderer, Draw& draw) {
        // Sort particles by blend mode for proper rendering
        std::stable_sort(activeParticles.begin(), activeParticles.end(),
            [](const auto& a, const auto& b) {
                return static_cast<int>(a->blendMode) < static_cast<int>(b->blendMode);
            });

        // Draw each particle
        for (auto& p : activeParticles) {
            drawParticle(renderer, draw, *p);
        }
    }

    // Draw individual particle
    void drawParticle(SDL_Renderer* renderer, Draw& draw, Particle& particle) {
        Color color = particle.getCurrentColor();
        float size = particle.size;
        float alpha = particle.getCurrentAlpha();

        color.a *= alpha;

        // Set blend mode
        switch (particle.blendMode) {
        case BlendMode::ADD:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            break;
        case BlendMode::MULTIPLY:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MUL);
            break;
        default:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            break;
        }

        // Draw trail
        if (!particle.trail.empty()) {
            drawTrail(draw, particle);
        }

        // Draw glow
        if (particle.hasGlow) {
            drawGlow(draw, particle.position, size * 2, color, particle.glowIntensity);
        }

        // Draw main shape
        drawShape(draw, particle.shape, particle.position, size, particle.rotation, color);

        // Reset blend mode
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    // Draw glow effect
    void drawGlow(Draw& draw, const Vec2& pos, float size, const Color& color, float intensity) {
        int layers = static_cast<int>(5 * intensity);
        for (int i = layers; i > 0; --i) {
            float t = static_cast<float>(i) / layers;
            Color glowColor = color;
            glowColor.a *= t * 0.2f;
            float glowSize = size * (1.0f + t);

            SDL_Color c = glowColor.toSDL();
            draw.color(c.r, c.g, c.b, c.a);

            // Draw glow circle
            draw.fill_circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(glowSize));
        }
    }

    // Draw particle trail
    void drawTrail(Draw& draw, Particle& particle) {
        if (particle.trail.size() < 2) return;

        for (size_t i = 0; i < particle.trail.size() - 1; ++i) {
            float t = static_cast<float>(i) / particle.trail.size();
            Color trailColor = particle.getCurrentColor();
            trailColor.a *= t * particle.trailFadeRate * particle.getCurrentAlpha();

            SDL_Color c = trailColor.toSDL();
            draw.color(c.r, c.g, c.b, c.a);

            float trailSize = particle.size * (1.0f - t * 0.5f);
            draw.fill_circle(static_cast<int>(particle.trail[i].x),
                static_cast<int>(particle.trail[i].y),
                static_cast<int>(trailSize));
        }
    }

    // Draw particle shape
    void drawShape(Draw& draw, ParticleShape shape, const Vec2& pos, float size,
        float rotation, const Color& color) {
        SDL_Color c = color.toSDL();
        draw.color(c.r, c.g, c.b, c.a);

        switch (shape) {
        case ParticleShape::CIRCLE: {
            draw.fill_circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size));
            break;
        }

        case ParticleShape::SQUARE: {
            draw.fill_rect(pos.x - size, pos.y - size, size * 2, size * 2);
            break;
        }

        case ParticleShape::STAR: {
            auto points = Utils::generateStarPoints(5, size * 0.4f, size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            draw.polygon(sdlPoints);
            break;
        }

        case ParticleShape::HEXAGON: {
            auto points = Utils::generatePolygonPoints(6, size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            draw.polygon(sdlPoints);
            break;
        }

        case ParticleShape::RING: {
            draw.circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size));
            draw.circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size * 0.6f));
            break;
        }

        case ParticleShape::HEART: {
            auto points = Utils::generateHeartPoints(size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            draw.polygon(sdlPoints);
            break;
        }

        case ParticleShape::TRIANGLE: {
            auto points = Utils::generatePolygonPoints(3, size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            draw.polygon(sdlPoints);
            break;
        }

        case ParticleShape::DIAMOND: {
            std::vector<SDL_FPoint> points = {
                {pos.x, pos.y - size},
                {pos.x + size * 0.7f, pos.y},
                {pos.x, pos.y + size},
                {pos.x - size * 0.7f, pos.y},
                {pos.x, pos.y - size}
            };
            draw.polygon(points);
            break;
        }

        case ParticleShape::CROSS: {
            float thickness = size * 0.3f;
            draw.fill_rect(pos.x - thickness / 2, pos.y - size, thickness, size * 2);
            draw.fill_rect(pos.x - size, pos.y - thickness / 2, size * 2, thickness);
            break;
        }

        case ParticleShape::LIGHTNING: {
            // Draw jagged lightning bolt
            std::vector<SDL_FPoint> points;
            int segments = 5;
            for (int i = 0; i <= segments; ++i) {
                float t = i / static_cast<float>(segments);
                float x = pos.x + Utils::randomFloat(-size * 0.3f, size * 0.3f);
                float y = pos.y - size + t * size * 2;
                points.push_back({ x, y });
            }
            draw.lines(points);
            break;
        }

        case ParticleShape::FLAME: {
            // Draw flame-like shape
            for (int i = 0; i < 3; ++i) {
                float offset = Utils::randomFloat(-size * 0.2f, size * 0.2f);
                float height = size * (1.0f - i * 0.3f);
                draw.line(pos.x + offset, pos.y + size,
                    pos.x + offset * 0.5f, pos.y - height);
            }
            break;
        }

        case ParticleShape::SPARKLE: {
            // Draw sparkle with radiating lines
            for (int i = 0; i < 8; ++i) {
                float angle = (i / 8.0f) * TWO_PI;
                float innerRadius = size * 0.3f;
                float outerRadius = size;
                Vec2 inner = Vec2::fromAngle(angle, innerRadius);
                Vec2 outer = Vec2::fromAngle(angle, outerRadius);
                draw.line(pos.x + inner.x, pos.y + inner.y,
                    pos.x + outer.x, pos.y + outer.y);
            }
            draw.fill_circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size * 0.3f));
            break;
        }

        case ParticleShape::BUBBLE: {
            // Draw bubble with highlight
            draw.circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size));
            // Highlight
            draw.fill_circle(static_cast<int>(pos.x - size * 0.3f),
                static_cast<int>(pos.y - size * 0.3f),
                static_cast<int>(size * 0.2f));
            break;
        }

        case ParticleShape::SMOKE_PUFF: {
            // Draw cloud-like shape
            for (int i = 0; i < 5; ++i) {
                float angle = (i / 5.0f) * TWO_PI;
                float offsetRadius = size * 0.5f;
                Vec2 offset = Vec2::fromAngle(angle, offsetRadius);
                draw.fill_circle(static_cast<int>(pos.x + offset.x),
                    static_cast<int>(pos.y + offset.y),
                    static_cast<int>(size * 0.6f));
            }
            draw.fill_circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size * 0.7f));
            break;
        }

        default:
            // Fallback to circle
            draw.fill_circle(static_cast<int>(pos.x), static_cast<int>(pos.y),
                static_cast<int>(size));
            break;
        }
    }

    // Get particle count
    size_t getParticleCount() const {
        return activeParticles.size();
    }
};

// ===== TESTBED APPLICATION =====
class ParticleTestbed {
private:
    // SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;
    Draw draw;

    // Application state
    bool running;
    float deltaTime;
    Uint64 lastFrameTime;

    // Particle system
    std::vector<std::unique_ptr<ParticleEmitter>> emitters;
    int currentEffectIndex;
    std::vector<std::string> effectNames;

    // Mouse state
    float mouseX, mouseY;
    bool mousePressed;

    // UI state
    bool showStats;
    bool showHelp;
    bool paused;

    // Performance tracking
    int frameCount;
    float fpsTimer;
    float currentFPS;

    // Screen dimensions
    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int SCREEN_HEIGHT = 720;

public:
    ParticleTestbed() : window(nullptr), renderer(nullptr), running(true),
        deltaTime(0), lastFrameTime(0), currentEffectIndex(0),
        mouseX(0), mouseY(0), mousePressed(false),
        showStats(true), showHelp(false), paused(false),
        frameCount(0), fpsTimer(0), currentFPS(0) {
        initEffectNames();
    }

    ~ParticleTestbed() {
        cleanup();
    }

    void initEffectNames() {
        effectNames = {
            "Fire Effect",
            "Magic Particles",
            "Explosion",
            "Smoke",
            "Rain",
            "Snow",
            "Lightning",
            "Portal",
            "Galaxy",
            "Fountain",
            "Confetti",
            "Mouse Trail"
        };
    }

    bool init() {
        // Initialize SDL
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        // Create window
        window = SDL_CreateWindow(
            "Particle System Testbed",
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            SDL_Log("Window creation failed: %s", SDL_GetError());
            return false;
        }

        // Create renderer
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            SDL_Log("Renderer creation failed: %s", SDL_GetError());
            return false;
        }

        SDL_SetRenderVSync(renderer, 1);
        draw.set_renderer(renderer);

        // Initialize utils
        Utils::initRandom();

        // Load first effect
        loadEffect(0);

        lastFrameTime = SDL_GetTicks();

        return true;
    }

    void cleanup() {
        emitters.clear();

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

    void loadEffect(int index) {
        currentEffectIndex = index;
        emitters.clear();

        switch (index) {
        case 0: createFireEffect(); break;
        case 1: createMagicEffect(); break;
        case 2: createExplosionEffect(); break;
        case 3: createSmokeEffect(); break;
        case 4: createRainEffect(); break;
        case 5: createSnowEffect(); break;
        case 6: createLightningEffect(); break;
        case 7: createPortalEffect(); break;
        case 8: createGalaxyEffect(); break;
        case 9: createFountainEffect(); break;
        case 10: createConfettiEffect(); break;
        case 11: createMouseTrailEffect(); break;
        default: createFireEffect(); break;
        }
    }

    void createFireEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100 };
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::CONE;
        emitter->patternAngle = HALF_PI / 2;
        emitter->rotation = -HALF_PI;

        emitter->lifetimeRange = { 0.5f, 1.5f };
        emitter->sizeRange = { 8.0f, 15.0f };
        emitter->speedRange = { 50.0f, 150.0f };
        emitter->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 200)),
            ColorRampPoint(0.2f, Color(255, 200, 100)),
            ColorRampPoint(0.5f, Color(255, 100, 50)),
            ColorRampPoint(1.0f, Color(100, 0, 0, 0))
        };

        emitter->shape = ParticleShape::FLAME;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 1.5f;
        emitter->enableTrails = true;
        emitter->trailLength = 5;

        emitter->gravity = { 0, -50 };
        emitter->turbulence = 20;

        emitter->behaviors.push_back(ParticleBehavior::TURBULENCE);

        emitters.push_back(std::move(emitter));
    }

    void createMagicEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        emitter->emissionRate = 50;
        emitter->pattern = EmissionPattern::CIRCLE;
        emitter->patternRadius = 30;

        emitter->lifetimeRange = { 1.0f, 2.0f };
        emitter->sizeRange = { 3.0f, 8.0f };
        emitter->speedRange = { 20.0f, 50.0f };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color::hsv(Utils::randomFloat(0, 360), 1.0f, 1.0f)),
            ColorRampPoint(0.5f, Color::hsv(Utils::randomFloat(0, 360), 0.8f, 0.8f)),
            ColorRampPoint(1.0f, Color(100, 100, 255, 0))
        };

        emitter->shape = ParticleShape::STAR;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->enablePulse = true;
        emitter->pulseRate = 2.0f;
        emitter->pulseAmount = 0.3f;
        emitter->enableShimmer = true;
        emitter->shimmerRate = 5.0f;

        emitter->behaviors.push_back(ParticleBehavior::ORBIT);
        emitter->targetPosition = emitter->position;

        ForceField field;
        field.position = emitter->position;
        field.radius = 100;
        field.strength = 50;
        field.type = ForceField::VORTEX;
        emitter->forceFields.push_back(field);

        emitters.push_back(std::move(emitter));
    }

    void createExplosionEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        emitter->burstMode = true;
        emitter->burstCount = 200;
        emitter->active = false;

        emitter->pattern = EmissionPattern::SPHERE;
        emitter->lifetimeRange = { 0.5f, 1.0f };
        emitter->sizeRange = { 5.0f, 15.0f };
        emitter->speedRange = { 200.0f, 500.0f };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255)),
            ColorRampPoint(0.1f, Color(255, 200, 100)),
            ColorRampPoint(0.3f, Color(255, 100, 50)),
            ColorRampPoint(1.0f, Color(50, 50, 50, 0))
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 2.0f;
        emitter->drag = 0.95f;

        emitter->burst();
        emitters.push_back(std::move(emitter));
    }

    void createSmokeEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.7f };
        emitter->emissionRate = 30;
        emitter->pattern = EmissionPattern::CONE;
        emitter->patternAngle = HALF_PI / 3;
        emitter->rotation = -HALF_PI;

        emitter->lifetimeRange = { 2.0f, 4.0f };
        emitter->sizeRange = { 20.0f, 40.0f };
        emitter->speedRange = { 30.0f, 60.0f };
        emitter->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(150, 150, 150, 200)),
            ColorRampPoint(0.5f, Color(100, 100, 100, 150)),
            ColorRampPoint(1.0f, Color(50, 50, 50, 0))
        };

        emitter->shape = ParticleShape::SMOKE_PUFF;
        emitter->blendMode = BlendMode::NORMAL;
        emitter->gravity = { 0, -20 };
        emitter->turbulence = 30;
        emitter->behaviors.push_back(ParticleBehavior::TURBULENCE);

        emitters.push_back(std::move(emitter));
    }

    void createRainEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, -50 };
        emitter->emissionRate = 300;
        emitter->pattern = EmissionPattern::LINE;
        emitter->patternRadius = SCREEN_WIDTH / 2;

        emitter->lifetimeRange = { 2.0f, 3.0f };
        emitter->sizeRange = { 1.0f, 2.0f };
        emitter->speedRange = { 400.0f, 500.0f };
        emitter->angleRange = { HALF_PI - 0.1f, HALF_PI + 0.1f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(150, 150, 255, 100)),
            ColorRampPoint(1.0f, Color(150, 150, 255, 50))
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::NORMAL;
        emitter->gravity = { 0, 200 };
        emitter->enableTrails = true;
        emitter->trailLength = 10;

        emitters.push_back(std::move(emitter));
    }

    void createSnowEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, -50 };
        emitter->emissionRate = 50;
        emitter->pattern = EmissionPattern::LINE;
        emitter->patternRadius = SCREEN_WIDTH / 2;

        emitter->lifetimeRange = { 5.0f, 8.0f };
        emitter->sizeRange = { 2.0f, 6.0f };
        emitter->speedRange = { 30.0f, 60.0f };
        emitter->angleRange = { HALF_PI - 0.3f, HALF_PI + 0.3f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255, 200)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 100))
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::NORMAL;
        emitter->gravity = { 0, 30 };
        emitter->wind = { 20, 0 };
        emitter->behaviors.push_back(ParticleBehavior::WANDER);

        emitters.push_back(std::move(emitter));
    }

    void createLightningEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { 100, SCREEN_HEIGHT / 2.0f };
        emitter->emissionRate = 200;
        emitter->pattern = EmissionPattern::LINE;
        emitter->patternRadius = SCREEN_WIDTH - 200;
        emitter->rotation = 0;

        emitter->lifetimeRange = { 0.1f, 0.3f };
        emitter->sizeRange = { 2.0f, 4.0f };
        emitter->speedRange = { 0, 50.0f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(200, 200, 255)),
            ColorRampPoint(0.5f, Color(150, 150, 255)),
            ColorRampPoint(1.0f, Color(100, 100, 200, 0))
        };

        emitter->shape = ParticleShape::LIGHTNING;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 2.0f;
        emitter->turbulence = 100;

        emitters.push_back(std::move(emitter));
    }

    void createPortalEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::RING;
        emitter->patternRadius = 100;

        emitter->lifetimeRange = { 2.0f, 3.0f };
        emitter->sizeRange = { 3.0f, 8.0f };
        emitter->speedRange = { 0, 20 };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 50, 255)),
            ColorRampPoint(0.5f, Color(200, 100, 255)),
            ColorRampPoint(1.0f, Color(50, 0, 150, 0))
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;

        ForceField vortex;
        vortex.position = emitter->position;
        vortex.radius = 200;
        vortex.strength = 100;
        vortex.type = ForceField::VORTEX;
        emitter->forceFields.push_back(vortex);

        emitters.push_back(std::move(emitter));
    }

    void createGalaxyEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::SPIRAL;
        emitter->patternRadius = 200;

        emitter->lifetimeRange = { 5.0f, 10.0f };
        emitter->sizeRange = { 1.0f, 4.0f };
        emitter->speedRange = { 0, 10 };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255)),
            ColorRampPoint(0.3f, Color(200, 200, 255)),
            ColorRampPoint(0.6f, Color(255, 200, 200)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };

        emitter->shape = ParticleShape::STAR;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->enableShimmer = true;

        emitters.push_back(std::move(emitter));
    }

    void createFountainEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 50 };
        emitter->emissionRate = 150;
        emitter->pattern = EmissionPattern::CONE;
        emitter->patternAngle = HALF_PI / 4;
        emitter->rotation = -HALF_PI;

        emitter->lifetimeRange = { 2.0f, 3.0f };
        emitter->sizeRange = { 3.0f, 8.0f };
        emitter->speedRange = { 200.0f, 400.0f };
        emitter->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 150, 255, 200)),
            ColorRampPoint(0.5f, Color(150, 200, 255, 150)),
            ColorRampPoint(1.0f, Color(200, 220, 255, 0))
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::NORMAL;
        emitter->gravity = { 0, 400 };
        emitter->enableTrails = true;
        emitter->trailLength = 8;
        emitter->drag = 0.99f;

        emitters.push_back(std::move(emitter));
    }

    void createConfettiEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, 50 };
        emitter->emissionRate = 50;
        emitter->pattern = EmissionPattern::CONE;
        emitter->patternAngle = HALF_PI;
        emitter->rotation = HALF_PI;

        emitter->lifetimeRange = { 3.0f, 5.0f };
        emitter->sizeRange = { 5.0f, 10.0f };
        emitter->speedRange = { 100.0f, 300.0f };
        emitter->angleRange = { HALF_PI - 0.5f, HALF_PI + 0.5f };
        emitter->angularVelRange = { -360, 360 };

        emitter->onParticleSpawn = [](Particle& p) {
            float hue = Utils::randomFloat(0, 360);
            p.colorRamp = {
                ColorRampPoint(0.0f, Color::hsv(hue, 1.0f, 1.0f)),
                ColorRampPoint(1.0f, Color::hsv(hue, 1.0f, 1.0f, 0))
            };
            p.shape = static_cast<ParticleShape>(Utils::randomInt(0, 5));
            };

        emitter->blendMode = BlendMode::NORMAL;
        emitter->gravity = { 0, 200 };
        emitter->drag = 0.98f;

        emitters.push_back(std::move(emitter));
    }

    void createMouseTrailEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::POINT;

        emitter->lifetimeRange = { 0.5f, 1.0f };
        emitter->sizeRange = { 5.0f, 15.0f };
        emitter->speedRange = { 0, 20 };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Utils::rainbow(0)),
            ColorRampPoint(0.2f, Utils::rainbow(0.2f)),
            ColorRampPoint(0.4f, Utils::rainbow(0.4f)),
            ColorRampPoint(0.6f, Utils::rainbow(0.6f)),
            ColorRampPoint(0.8f, Utils::rainbow(0.8f)),
            ColorRampPoint(1.0f, Utils::rainbow(1.0f))
        };

        emitter->shape = ParticleShape::STAR;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->enableTrails = true;
        emitter->trailLength = 10;

        emitters.push_back(std::move(emitter));
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                handleKeyPress(event.key.key);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                mousePressed = true;
                handleMouseClick(event.button.x, event.button.y);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                mousePressed = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseX = event.motion.x;
                mouseY = event.motion.y;
            }
        }
    }

    void handleKeyPress(SDL_Keycode key) {
        switch (key) {
        case SDLK_ESCAPE:
            running = false;
            break;
        case SDLK_SPACE:
            paused = !paused;
            break;
        case SDLK_S:
            showStats = !showStats;
            break;
        case SDLK_H:
            showHelp = !showHelp;
            break;
        case SDLK_R:
            loadEffect(currentEffectIndex);
            break;
        case SDLK_LEFT:
            loadEffect((currentEffectIndex - 1 + effectNames.size()) % effectNames.size());
            break;
        case SDLK_RIGHT:
            loadEffect((currentEffectIndex + 1) % effectNames.size());
            break;
        case SDLK_C:
            for (auto& emitter : emitters) {
                emitter->clear();
            }
            break;
        case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
        case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
            loadEffect(key - SDLK_1);
            break;
        }
    }

    void handleMouseClick(float x, float y) {
        // Create explosion at mouse position
        if (currentEffectIndex == 2) {
            emitters[0]->position = { x, y };
            emitters[0]->burst();
        }
    }

    void update() {
        // Calculate delta time
        Uint64 currentTime = SDL_GetTicks();
        deltaTime = (currentTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentTime;

        if (paused) return;

        // Update emitters
        for (auto& emitter : emitters) {
            // Update mouse trail position
            if (currentEffectIndex == 11) {
                emitter->position = { mouseX, mouseY };
            }

            emitter->update(deltaTime);
        }

        // Update FPS
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            currentFPS = frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer = 0;
        }
    }

    void render() {
        // Clear screen with gradient
        for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
            int intensity = 20 + (y * 20 / SCREEN_HEIGHT);
            draw.color(intensity, intensity, intensity + 10);
            draw.fill_rect(0, y, SCREEN_WIDTH, 2);
        }

        // Draw particles
        for (auto& emitter : emitters) {
            emitter->draw(renderer, draw);
        }

        // Draw UI
        drawUI();

        draw.present();
    }

    void drawUI() {
        if (showStats) {
            drawStats();
        }

        if (showHelp) {
            drawHelp();
        }

        // Draw effect name
        draw.color(0, 0, 0, 180);
        draw.fill_rect(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 50, 300, 40);
        draw.color(255, 255, 255);
        draw.rect(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 50, 300, 40);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        std::string name = effectNames[currentEffectIndex];
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - name.length() * 4,
            SCREEN_HEIGHT - 35, name.c_str());
    }

    void drawStats() {
        draw.color(0, 0, 0, 200);
        draw.fill_rect(10, 10, 200, 100);
        draw.color(255, 255, 255);
        draw.rect(10, 10, 200, 100);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        std::stringstream ss;
        ss << "FPS: " << static_cast<int>(currentFPS);
        SDL_RenderDebugText(renderer, 20, 20, ss.str().c_str());

        int totalParticles = 0;
        for (const auto& emitter : emitters) {
            totalParticles += emitter->getParticleCount();
        }

        ss.str("");
        ss << "Particles: " << totalParticles;
        SDL_RenderDebugText(renderer, 20, 40, ss.str().c_str());

        ss.str("");
        ss << "Emitters: " << emitters.size();
        SDL_RenderDebugText(renderer, 20, 60, ss.str().c_str());

        if (paused) {
            SDL_RenderDebugText(renderer, 20, 80, "PAUSED");
        }
    }

    void drawHelp() {
        draw.color(0, 0, 0, 220);
        draw.fill_rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 150, 400, 300);
        draw.color(255, 255, 255);
        draw.rect(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 150, 400, 300);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int y = SCREEN_HEIGHT / 2 - 130;

        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 40, y, "CONTROLS");
        y += 30;

        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "Left/Right - Switch effects");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "1-9 - Jump to effect");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "Space - Pause/Resume");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "R - Restart effect");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "C - Clear particles");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "S - Toggle stats");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "H - Toggle help");
        SDL_RenderDebugText(renderer, SCREEN_WIDTH / 2 - 180, y += 20,
            "ESC - Exit");
    }

    void run() {
        while (running) {
            handleEvents();
            update();
            render();

            // Cap framerate to 60 FPS
            SDL_Delay(16);
        }
    }
};

//// Main entry point
//int main(int argc, char* argv[]) {
//    (void)argc; (void)argv;
//
//    SDL_SetAppMetadata("Particle System Testbed", "1.0", "com.example.particles");
//
//    ParticleTestbed testbed;
//    if (!testbed.init()) {
//        SDL_Log("Failed to initialize testbed");
//        return -1;
//    }
//
//    testbed.run();
//    return 0;
//}