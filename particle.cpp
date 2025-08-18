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
        case 0: 
            createAllEffectsCombined();
            break;
        case 1: 
            createAuroraEffect(); 
            break;
        case 2: 
            createBlackHoleEffect(); 
            break;
        case 3: 
            createBloodSplatterEffect();
            break;
        case 4: 
            createButterflySwarmEffect();;
            break;
        case 5: createCosmicDustEffect(); break;
        case 6: createDisintegrationEffect(); break;
        case 7: createEnergyShieldEffect(); break;
        case 8: createFireworksEffect(); break;
        case 9: createMatrixRainEffect(); break;
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

    // Additional Particle Effect Creation Functions

    void createSparklesEffect() {
        auto emitter = std::make_unique<ParticleEmitter>();
        emitter->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::SPHERE;
        emitter->patternRadius = 200;

        emitter->lifetimeRange = { 1.0f, 2.0f };
        emitter->sizeRange = { 2.0f, 6.0f };
        emitter->speedRange = { 10.0f, 30.0f };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255)),
            ColorRampPoint(0.5f, Color(255, 220, 100)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };

        emitter->shape = ParticleShape::SPARKLE;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->enablePulse = true;
        emitter->pulseRate = 5.0f;
        emitter->enableShimmer = true;
        emitter->shimmerRate = 10.0f;

        emitters.push_back(std::move(emitter));
    }

    void createWaterfallEffect() {
        // Water stream
        auto water = std::make_unique<ParticleEmitter>();
        water->position = { SCREEN_WIDTH / 2.0f, 100 };
        water->emissionRate = 500;
        water->pattern = EmissionPattern::LINE;
        water->patternRadius = 50;

        water->lifetimeRange = { 2.0f, 3.0f };
        water->sizeRange = { 2.0f, 5.0f };
        water->speedRange = { 50.0f, 100.0f };
        water->angleRange = { HALF_PI - 0.1f, HALF_PI + 0.1f };

        water->colorRamp = {
            ColorRampPoint(0.0f, Color(150, 200, 255, 150)),
            ColorRampPoint(1.0f, Color(200, 220, 255, 50))
        };

        water->shape = ParticleShape::CIRCLE;
        water->blendMode = BlendMode::NORMAL;
        water->gravity = { 0, 300 };
        water->enableTrails = true;
        water->trailLength = 5;

        // Mist at bottom
        auto mist = std::make_unique<ParticleEmitter>();
        mist->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100 };
        mist->emissionRate = 100;
        mist->pattern = EmissionPattern::CIRCLE;
        mist->patternRadius = 100;

        mist->lifetimeRange = { 2.0f, 4.0f };
        mist->sizeRange = { 20.0f, 40.0f };
        mist->speedRange = { 20.0f, 50.0f };
        mist->angleRange = { -PI, 0 };

        mist->colorRamp = {
            ColorRampPoint(0.0f, Color(220, 240, 255, 100)),
            ColorRampPoint(1.0f, Color(200, 220, 255, 0))
        };

        mist->shape = ParticleShape::SMOKE_PUFF;
        mist->blendMode = BlendMode::NORMAL;
        mist->gravity = { 0, -50 };

        emitters.push_back(std::move(water));
        emitters.push_back(std::move(mist));
    }

    void createAuroraEffect() {
        // Aurora waves
        for (int i = 0; i < 1; ++i) {
            auto aurora = std::make_unique<ParticleEmitter>();
            aurora->position = { SCREEN_WIDTH / 2.0f, 200.0f + i * 50 };
            aurora->emissionRate = 100;
            aurora->pattern = EmissionPattern::LINE;
            aurora->patternRadius = SCREEN_WIDTH / 2;

            aurora->lifetimeRange = { 3.0f, 5.0f };
            aurora->sizeRange = { 30.0f, 60.0f };
            aurora->speedRange = { 10.0f, 30.0f };
            aurora->angleRange = { 0, TWO_PI };

            float hue = 120 + i * 60; // Green to blue
            aurora->colorRamp = {
                ColorRampPoint(0.0f, Color::hsv(hue, 0.8f, 0.8f, 0.0f)),
                ColorRampPoint(0.2f, Color::hsv(hue, 0.8f, 0.8f, 0.3f)),
                ColorRampPoint(0.5f, Color::hsv(hue + 30, 0.7f, 1.0f, 0.2f)),
                ColorRampPoint(1.0f, Color::hsv(hue + 60, 0.6f, 0.6f, 0.0f))
            };

            aurora->shape = ParticleShape::SMOKE_PUFF;
            aurora->blendMode = BlendMode::ADD;
            aurora->behaviors.push_back(ParticleBehavior::FLOW_FIELD);
            aurora->wind = { 50, 0 };

            emitters.push_back(std::move(aurora));
        }
    }

    void createPoisonCloudEffect() {
        auto poison = std::make_unique<ParticleEmitter>();
        poison->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        poison->emissionRate = 80;
        poison->pattern = EmissionPattern::CIRCLE;
        poison->patternRadius = 50;

        poison->lifetimeRange = { 2.0f, 4.0f };
        poison->sizeRange = { 20.0f, 40.0f };
        poison->speedRange = { 20.0f, 50.0f };
        poison->angleRange = { 0, TWO_PI };

        poison->colorRamp = {
            ColorRampPoint(0.0f, Color(50, 200, 50, 150)),
            ColorRampPoint(0.5f, Color(100, 255, 50, 100)),
            ColorRampPoint(1.0f, Color(50, 150, 50, 0))
        };

        poison->shape = ParticleShape::SMOKE_PUFF;
        poison->blendMode = BlendMode::NORMAL;
        poison->gravity = { 0, -20 };
        poison->turbulence = 40;
        poison->enableDistortion = true;
        poison->distortionAmount = 0.2f;

        // Bubbles
        auto bubbles = std::make_unique<ParticleEmitter>();
        bubbles->position = poison->position;
        bubbles->emissionRate = 20;
        bubbles->pattern = EmissionPattern::CIRCLE;
        bubbles->patternRadius = 30;

        bubbles->lifetimeRange = { 1.0f, 2.0f };
        bubbles->sizeRange = { 5.0f, 15.0f };
        bubbles->speedRange = { 30.0f, 60.0f };
        bubbles->angleRange = { -PI, 0 };

        bubbles->colorRamp = {
            ColorRampPoint(0.0f, Color(150, 255, 150, 100)),
            ColorRampPoint(1.0f, Color(100, 200, 100, 0))
        };

        bubbles->shape = ParticleShape::BUBBLE;
        bubbles->blendMode = BlendMode::ADD;
        bubbles->gravity = { 0, -50 };

        emitters.push_back(std::move(poison));
        emitters.push_back(std::move(bubbles));
    }

    void createHealingAuraEffect() {
        auto heal = std::make_unique<ParticleEmitter>();
        heal->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        heal->emissionRate = 60;
        heal->pattern = EmissionPattern::CIRCLE;
        heal->patternRadius = 80;

        heal->lifetimeRange = { 2.0f, 3.0f };
        heal->sizeRange = { 5.0f, 15.0f };
        heal->speedRange = { 20.0f, 40.0f };
        heal->angleRange = { -PI, 0 };

        heal->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 255, 100, 0)),
            ColorRampPoint(0.3f, Color(200, 255, 200, 200)),
            ColorRampPoint(0.7f, Color(150, 255, 150, 150)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };

        heal->shape = ParticleShape::CROSS;
        heal->blendMode = BlendMode::ADD;
        heal->enableGlow = true;
        heal->glowIntensity = 1.5f;
        heal->gravity = { 0, -30 };
        heal->enablePulse = true;
        heal->pulseRate = 1.0f;
        heal->pulseAmount = 0.3f;

        // Holy light rays
        auto rays = std::make_unique<ParticleEmitter>();
        rays->position = heal->position;
        rays->emissionRate = 10;
        rays->pattern = EmissionPattern::POINT;

        rays->lifetimeRange = { 1.0f, 2.0f };
        rays->sizeRange = { 100.0f, 200.0f };
        rays->speedRange = { 0, 0 };

        rays->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 200, 50)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };

        rays->shape = ParticleShape::STAR;
        rays->blendMode = BlendMode::ADD;
        rays->angularVelRange = { 10, 30 };

        emitters.push_back(std::move(heal));
        emitters.push_back(std::move(rays));
    }

    void createBloodSplatterEffect() {
        auto blood = std::make_unique<ParticleEmitter>();
        blood->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        blood->burstMode = true;
        blood->burstCount = 50;
        blood->active = false;
        blood->pattern = EmissionPattern::CONE;
        blood->patternAngle = HALF_PI;
        blood->rotation = HALF_PI;

        blood->lifetimeRange = { 1.0f, 2.0f };
        blood->sizeRange = { 3.0f, 8.0f };
        blood->speedRange = { 100.0f, 300.0f };
        blood->angleRange = { HALF_PI - 0.5f, HALF_PI + 0.5f };

        blood->colorRamp = {
            ColorRampPoint(0.0f, Color(200, 0, 0)),
            ColorRampPoint(0.5f, Color(150, 0, 0)),
            ColorRampPoint(1.0f, Color(100, 0, 0, 0))
        };

        blood->shape = ParticleShape::CIRCLE;
        blood->blendMode = BlendMode::NORMAL;
        blood->gravity = { 0, 300 };
        blood->enableTrails = true;
        blood->trailLength = 5;

        blood->burst();
        emitters.push_back(std::move(blood));
    }

    void createShockwaveEffect() {
        auto shockwave = std::make_unique<ParticleEmitter>();
        shockwave->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        shockwave->burstMode = true;
        shockwave->burstCount = 1;
        shockwave->active = false;

        shockwave->lifetimeRange = { 1.0f, 1.0f };
        shockwave->sizeRange = { 10.0f, 10.0f };
        shockwave->speedRange = { 0, 0 };

        shockwave->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255, 200)),
            ColorRampPoint(1.0f, Color(255, 255, 255, 0))
        };

        shockwave->shape = ParticleShape::RING;
        shockwave->blendMode = BlendMode::ADD;

        shockwave->onParticleUpdate = [](Particle& p) {
            p.size = p.startSize + p.age * 300;
            };

        shockwave->burst();
        emitters.push_back(std::move(shockwave));
    }

    void createFireworksEffect() {
        // Create multiple bursts with random positions
        for (int i = 0; i < 3; ++i) {
            auto firework = std::make_unique<ParticleEmitter>();
            firework->position = {
                Utils::randomFloat(200, SCREEN_WIDTH - 200),
                Utils::randomFloat(100, SCREEN_HEIGHT / 2)
            };
            firework->burstMode = true;
            firework->burstCount = 100;
            firework->burstInterval = Utils::randomFloat(0.5f, 2.0f);

            firework->pattern = EmissionPattern::SPHERE;
            firework->lifetimeRange = { 1.0f, 2.0f };
            firework->sizeRange = { 3.0f, 8.0f };
            firework->speedRange = { 100.0f, 300.0f };

            float hue = Utils::randomFloat(0, 360);
            firework->colorRamp = {
                ColorRampPoint(0.0f, Color::hsv(hue, 1, 1)),
                ColorRampPoint(0.5f, Color::hsv(hue + 30, 0.8f, 0.8f)),
                ColorRampPoint(1.0f, Color(100, 100, 100, 0))
            };

            firework->shape = ParticleShape::STAR;
            firework->blendMode = BlendMode::ADD;
            firework->enableGlow = true;
            firework->enableTrails = true;
            firework->gravity = { 0, 100 };
            firework->drag = 0.98f;

            emitters.push_back(std::move(firework));
        }
    }

    void createCosmicDustEffect() {
        auto dust = std::make_unique<ParticleEmitter>();
        dust->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        dust->emissionRate = 50;
        dust->pattern = EmissionPattern::SPHERE;
        dust->patternRadius = 300;

        dust->lifetimeRange = { 5.0f, 10.0f };
        dust->sizeRange = { 1.0f, 3.0f };
        dust->speedRange = { 5.0f, 20.0f };
        dust->angleRange = { 0, TWO_PI };

        dust->colorRamp = {
            ColorRampPoint(0.0f, Color(200, 200, 255, 0)),
            ColorRampPoint(0.2f, Color(200, 200, 255, 200)),
            ColorRampPoint(0.8f, Color(255, 200, 200, 200)),
            ColorRampPoint(1.0f, Color(255, 255, 200, 0))
        };

        dust->shape = ParticleShape::CIRCLE;
        dust->blendMode = BlendMode::ADD;
        dust->enableGlow = true;
        dust->behaviors.push_back(ParticleBehavior::WANDER);
        dust->behaviors.push_back(ParticleBehavior::FLOW_FIELD);

        emitters.push_back(std::move(dust));
    }

    void createPlasmaFieldEffect() {
        auto plasma = std::make_unique<ParticleEmitter>();
        plasma->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        plasma->emissionRate = 200;
        plasma->pattern = EmissionPattern::CIRCLE;
        plasma->patternRadius = 100;

        plasma->lifetimeRange = { 1.0f, 2.0f };
        plasma->sizeRange = { 10.0f, 30.0f };
        plasma->speedRange = { 0, 50 };
        plasma->angleRange = { 0, TWO_PI };

        plasma->onParticleUpdate = [](Particle& p) {
            float t = p.age / p.lifetime;
            p.color = Utils::plasmaColor(t + p.position.x * 0.01f);
            };

        plasma->shape = ParticleShape::CIRCLE;
        plasma->blendMode = BlendMode::ADD;
        plasma->enableGlow = true;
        plasma->glowIntensity = 2.0f;
        plasma->turbulence = 50;
        plasma->enableDistortion = true;

        ForceField field;
        field.position = plasma->position;
        field.radius = 200;
        field.strength = 100;
        field.type = ForceField::TURBULENCE;
        plasma->forceFields.push_back(field);

        emitters.push_back(std::move(plasma));
    }

    void createTornadoEffect() {
        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

        auto tornado = std::make_unique<ParticleEmitter>();
        tornado->position = center;
        tornado->emissionRate = 300;
        tornado->pattern = EmissionPattern::CIRCLE;
        tornado->patternRadius = 150;

        tornado->lifetimeRange = { 2.0f, 4.0f };
        tornado->sizeRange = { 5.0f, 20.0f };
        tornado->speedRange = { 50.0f, 100.0f };
        tornado->angleRange = { 0, TWO_PI };

        tornado->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 100, 100, 150)),
            ColorRampPoint(1.0f, Color(50, 50, 50, 0))
        };

        tornado->shape = ParticleShape::SMOKE_PUFF;
        tornado->blendMode = BlendMode::NORMAL;

        ForceField vortex;
        vortex.position = center;
        vortex.radius = 300;
        vortex.strength = 200;
        vortex.type = ForceField::VORTEX;
        tornado->forceFields.push_back(vortex);

        tornado->gravity = { 0, -100 };
        tornado->turbulence = 50;

        emitters.push_back(std::move(tornado));
    }

    void createMatrixRainEffect() {
        auto matrix = std::make_unique<ParticleEmitter>();
        matrix->position = { SCREEN_WIDTH / 2.0f, -50 };
        matrix->emissionRate = 100;
        matrix->pattern = EmissionPattern::LINE;
        matrix->patternRadius = SCREEN_WIDTH / 2;

        matrix->lifetimeRange = { 3.0f, 6.0f };
        matrix->sizeRange = { 8.0f, 12.0f };
        matrix->speedRange = { 50.0f, 150.0f };
        matrix->angleRange = { HALF_PI, HALF_PI };

        matrix->colorRamp = {
            ColorRampPoint(0.0f, Color(0, 255, 0, 0)),
            ColorRampPoint(0.1f, Color(0, 255, 0, 255)),
            ColorRampPoint(0.9f, Color(0, 255, 0, 255)),
            ColorRampPoint(1.0f, Color(0, 100, 0, 0))
        };

        matrix->shape = ParticleShape::SQUARE;
        matrix->blendMode = BlendMode::ADD;
        matrix->enableTrails = true;
        matrix->trailLength = 20;
        matrix->trailFadeRate = 0.8f;

        emitters.push_back(std::move(matrix));
    }

    void createEnergyShieldEffect() {
        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

        auto shield = std::make_unique<ParticleEmitter>();
        shield->position = center;
        shield->emissionRate = 200;
        shield->pattern = EmissionPattern::RING;
        shield->patternRadius = 150;

        shield->lifetimeRange = { 1.0f, 2.0f };
        shield->sizeRange = { 5.0f, 10.0f };
        shield->speedRange = { 0, 20 };
        shield->angleRange = { 0, TWO_PI };

        shield->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 200, 255, 100)),
            ColorRampPoint(0.5f, Color(150, 220, 255, 150)),
            ColorRampPoint(1.0f, Color(200, 240, 255, 0))
        };

        shield->shape = ParticleShape::HEXAGON;
        shield->blendMode = BlendMode::ADD;
        shield->enableGlow = true;
        shield->enablePulse = true;
        shield->pulseRate = 2.0f;
        shield->pulseAmount = 0.2f;

        shield->onParticleUpdate = [center](Particle& p) {
            Vec2 diff = p.position - center;
            float dist = diff.length();
            if (dist > 0) {
                p.position = center + diff.normalized() * 150;
            }
            };

        emitters.push_back(std::move(shield));
    }

    void createDisintegrationEffect() {
        auto disintegrate = std::make_unique<ParticleEmitter>();
        disintegrate->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        disintegrate->burstMode = true;
        disintegrate->burstCount = 100;
        disintegrate->active = false;
        disintegrate->pattern = EmissionPattern::CIRCLE;
        disintegrate->patternRadius = 20;

        disintegrate->lifetimeRange = { 1.0f, 2.0f };
        disintegrate->sizeRange = { 2.0f, 5.0f };
        disintegrate->speedRange = { 50.0f, 150.0f };
        disintegrate->angleRange = { 0, TWO_PI };

        disintegrate->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 200, 255)),
            ColorRampPoint(0.5f, Color(50, 100, 200)),
            ColorRampPoint(1.0f, Color(0, 50, 100, 0))
        };

        disintegrate->shape = ParticleShape::SQUARE;
        disintegrate->blendMode = BlendMode::ADD;
        disintegrate->enableGlow = true;
        disintegrate->behaviors.push_back(ParticleBehavior::TURBULENCE);

        disintegrate->burst();
        emitters.push_back(std::move(disintegrate));
    }

    void createBlackHoleEffect() {
        Vec2 center(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

        // Accretion disk
        auto disk = std::make_unique<ParticleEmitter>();
        disk->position = center;
        disk->emissionRate = 200;
        disk->pattern = EmissionPattern::RING;
        disk->patternRadius = 200;

        disk->lifetimeRange = { 3.0f, 5.0f };
        disk->sizeRange = { 2.0f, 5.0f };
        disk->speedRange = { 50.0f, 100.0f };
        disk->angleRange = { 0, TWO_PI };

        disk->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 200, 100, 200)),
            ColorRampPoint(0.5f, Color(255, 100, 50, 150)),
            ColorRampPoint(1.0f, Color(100, 0, 0, 0))
        };

        disk->shape = ParticleShape::CIRCLE;
        disk->blendMode = BlendMode::ADD;
        disk->enableGlow = true;
        disk->enableTrails = true;
        disk->trailLength = 15;

        ForceField blackHole;
        blackHole.position = center;
        blackHole.radius = 400;
        blackHole.strength = 300;
        blackHole.type = ForceField::ATTRACT;
        blackHole.falloff = 2.5f;
        disk->forceFields.push_back(blackHole);

        // Event horizon
        auto horizon = std::make_unique<ParticleEmitter>();
        horizon->position = center;
        horizon->emissionRate = 1;
        horizon->pattern = EmissionPattern::POINT;

        horizon->lifetimeRange = { 10.0f, 10.0f };
        horizon->sizeRange = { 80.0f, 80.0f };
        horizon->speedRange = { 0, 0 };

        horizon->colorRamp = {
            ColorRampPoint(0.0f, Color(0, 0, 0, 255)),
            ColorRampPoint(1.0f, Color(0, 0, 0, 255))
        };

        horizon->shape = ParticleShape::CIRCLE;
        horizon->blendMode = BlendMode::NORMAL;

        emitters.push_back(std::move(disk));
        emitters.push_back(std::move(horizon));
    }

    void createRainbowWaveEffect() {
        auto wave = std::make_unique<ParticleEmitter>();
        wave->position = { 100, SCREEN_HEIGHT / 2.0f };
        wave->emissionRate = 200;
        wave->pattern = EmissionPattern::POINT;

        wave->lifetimeRange = { 2.0f, 3.0f };
        wave->sizeRange = { 10.0f, 20.0f };
        wave->speedRange = { 200.0f, 300.0f };
        wave->angleRange = { 0, 0 };

        wave->onParticleSpawn = [](Particle& p) {
            float hue = std::fmod(p.age * 100, 360);
            p.colorRamp = {
                ColorRampPoint(0.0f, Color::hsv(hue, 1.0f, 1.0f)),
                ColorRampPoint(0.5f, Color::hsv(hue + 60, 1.0f, 1.0f)),
                ColorRampPoint(1.0f, Color::hsv(hue + 120, 1.0f, 1.0f, 0))
            };
            };

        wave->shape = ParticleShape::CIRCLE;
        wave->blendMode = BlendMode::ADD;
        wave->enableGlow = true;
        wave->behaviors.push_back(ParticleBehavior::TURBULENCE);

        emitters.push_back(std::move(wave));
    }

    void createButterflySwarmEffect() {
        auto butterflies = std::make_unique<ParticleEmitter>();
        butterflies->position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        butterflies->emissionRate = 20;
        butterflies->pattern = EmissionPattern::CIRCLE;
        butterflies->patternRadius = 200;

        butterflies->lifetimeRange = { 10.0f, 15.0f };
        butterflies->sizeRange = { 15.0f, 25.0f };
        butterflies->speedRange = { 30.0f, 60.0f };
        butterflies->angleRange = { 0, TWO_PI };

        butterflies->onParticleSpawn = [](Particle& p) {
            float hue = Utils::randomFloat(0, 360);
            p.colorRamp = {
                ColorRampPoint(0.0f, Color::hsv(hue, 0.8f, 1.0f)),
                ColorRampPoint(1.0f, Color::hsv(hue, 0.8f, 1.0f))
            };
            p.behaviors.push_back(ParticleBehavior::WANDER);
            p.behaviors.push_back(ParticleBehavior::FLOCK);
            p.shape = ParticleShape::HEART;
            p.angularVelocity = Utils::randomFloat(-90, 90);
            };

        butterflies->blendMode = BlendMode::NORMAL;
        butterflies->behaviors.push_back(ParticleBehavior::WANDER);

        emitters.push_back(std::move(butterflies));
    }

    void createAllEffectsCombined() {
        // Create a subset of effects together
        Vec2 positions[] = {
            {200, 200}, {SCREEN_WIDTH - 200, 200},
            {200, SCREEN_HEIGHT - 200}, {SCREEN_WIDTH - 200, SCREEN_HEIGHT - 200},
            {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}
        };

        // Mini fire
        auto fire = std::make_unique<ParticleEmitter>();
        fire->position = positions[0];
        fire->emissionRate = 50;
        fire->pattern = EmissionPattern::CONE;
        fire->patternAngle = HALF_PI / 3;
        fire->rotation = -HALF_PI;
        fire->lifetimeRange = { 0.5f, 1.0f };
        fire->sizeRange = { 5.0f, 10.0f };
        fire->speedRange = { 30.0f, 80.0f };
        fire->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 200)),
            ColorRampPoint(0.5f, Color(255, 100, 50)),
            ColorRampPoint(1.0f, Color(100, 0, 0, 0))
        };
        fire->shape = ParticleShape::FLAME;
        fire->blendMode = BlendMode::ADD;
        fire->enableGlow = true;

        // Mini magic
        auto magic = std::make_unique<ParticleEmitter>();
        magic->position = positions[1];
        magic->emissionRate = 30;
        magic->pattern = EmissionPattern::RING;
        magic->patternRadius = 50;
        magic->lifetimeRange = { 1.0f, 2.0f };
        magic->sizeRange = { 3.0f, 6.0f };
        magic->colorRamp = {
            ColorRampPoint(0.0f, Color(100, 50, 255)),
            ColorRampPoint(1.0f, Color(50, 0, 150, 0))
        };
        magic->shape = ParticleShape::STAR;
        magic->blendMode = BlendMode::ADD;
        magic->enableGlow = true;
        magic->behaviors.push_back(ParticleBehavior::ORBIT);
        magic->targetPosition = magic->position;

        // Mini sparkles
        auto sparkles = std::make_unique<ParticleEmitter>();
        sparkles->position = positions[2];
        sparkles->emissionRate = 40;
        sparkles->pattern = EmissionPattern::SPHERE;
        sparkles->patternRadius = 80;
        sparkles->lifetimeRange = { 0.5f, 1.5f };
        sparkles->sizeRange = { 2.0f, 5.0f };
        sparkles->colorRamp = {
            ColorRampPoint(0.0f, Color(255, 255, 255)),
            ColorRampPoint(1.0f, Color(255, 220, 100, 0))
        };
        sparkles->shape = ParticleShape::SPARKLE;
        sparkles->blendMode = BlendMode::ADD;
        sparkles->enableShimmer = true;

        // Mini smoke
        auto smoke = std::make_unique<ParticleEmitter>();
        smoke->position = positions[3];
        smoke->emissionRate = 20;
        smoke->pattern = EmissionPattern::CONE;
        smoke->patternAngle = HALF_PI / 4;
        smoke->rotation = -HALF_PI;
        smoke->lifetimeRange = { 1.0f, 2.0f };
        smoke->sizeRange = { 15.0f, 25.0f };
        smoke->colorRamp = {
            ColorRampPoint(0.0f, Color(150, 150, 150, 150)),
            ColorRampPoint(1.0f, Color(50, 50, 50, 0))
        };
        smoke->shape = ParticleShape::SMOKE_PUFF;
        smoke->blendMode = BlendMode::NORMAL;

        // Center ambient particles
        auto ambient = std::make_unique<ParticleEmitter>();
        ambient->position = positions[4];
        ambient->emissionRate = 50;
        ambient->pattern = EmissionPattern::BOX;
        ambient->patternRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 3.0f;
        ambient->lifetimeRange = { 3.0f, 6.0f };
        ambient->sizeRange = { 1.0f, 3.0f };
        ambient->speedRange = { 5.0f, 15.0f };
        ambient->colorRamp = {
            ColorRampPoint(0.0f, Color(200, 200, 255, 0)),
            ColorRampPoint(0.5f, Color(200, 200, 255, 100)),
            ColorRampPoint(1.0f, Color(200, 200, 255, 0))
        };
        ambient->shape = ParticleShape::CIRCLE;
        ambient->blendMode = BlendMode::ADD;
        ambient->behaviors.push_back(ParticleBehavior::WANDER);

        emitters.push_back(std::move(fire));
        emitters.push_back(std::move(magic));
        emitters.push_back(std::move(sparkles));
        emitters.push_back(std::move(smoke));
        emitters.push_back(std::move(ambient));
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
            SDL_Delay(30);
        }
    }
};

// Main entry point
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    SDL_SetAppMetadata("Particle System Testbed", "1.0", "com.example.particles");

    ParticleTestbed testbed;
    if (!testbed.init()) {
        SDL_Log("Failed to initialize testbed");
        return -1;
    }

    testbed.run();
    return 0;
}