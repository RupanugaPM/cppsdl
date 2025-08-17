// particle_system.cpp - Implementation
#include "particle_system.hpp"
#include <chrono>

namespace ParticleSystem {

    // Static texture cache
    std::unordered_map<std::string, SDL_Texture*> ParticleEmitter::textureCache;

    // ===== Particle Implementation =====
    void Particle::update(float dt) {
        age += dt;

        // Store previous position for motion blur
        previousPos = position;

        // Apply behaviors
        applyBehaviors(dt);

        // Physics integration
        velocity += acceleration * dt;
        velocity *= drag;
        position += velocity * dt;

        // Clear acceleration
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

        // Pulse effect
        if (pulseRate > 0) {
            float pulse = std::sin(age * pulseRate * TWO_PI) * pulseAmount;
            size = getCurrentSize() * (1.0f + pulse);
        }
    }

    void Particle::applyForce(const Vec2& force) {
        acceleration += force / mass;
    }

    void Particle::applyBehaviors(float dt) {
        for (auto behavior : behaviors) {
            switch (behavior) {
            case ParticleBehavior::GRAVITY:
                applyForce({ 0, 98 * mass });
                break;

            case ParticleBehavior::WIND:
                applyForce({ random_float(-10, 10), 0 });
                break;

            case ParticleBehavior::TURBULENCE: {
                float noise = ParticleUtils::perlinNoise(position.x * 0.01f, position.y * 0.01f);
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
                Vec2 tangent = { -toTarget.y, toTarget.x };
                tangent = tangent.normalized() * 50;
                applyForce(tangent * behaviorStrength);
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
                float wanderAngle = random_float(0, TWO_PI);
                applyForce(Vec2::fromAngle(wanderAngle, 20));
                break;
            }

            default:
                break;
            }
        }
    }

    Color Particle::getCurrentColor() const {
        if (colorRamp.empty()) return color;

        float t = age / lifetime;

        // Find surrounding color points
        for (size_t i = 0; i < colorRamp.size() - 1; ++i) {
            if (t >= colorRamp[i].t && t <= colorRamp[i + 1].t) {
                float localT = (t - colorRamp[i].t) / (colorRamp[i + 1].t - colorRamp[i].t);
                return Color::lerp(colorRamp[i].color, colorRamp[i + 1].color, localT);
            }
        }

        return colorRamp.back().color;
    }

    float Particle::getCurrentSize() const {
        float t = age / lifetime;
        float easedT = ParticleUtils::easeInOut(t);
        return startSize + (endSize - startSize) * easedT;
    }

    float Particle::getCurrentAlpha() const {
        float alpha = 1.0f;

        // Fade in
        if (age < fadeInTime) {
            alpha *= age / fadeInTime;
        }

        // Fade out
        float fadeOutStart = lifetime - fadeOutTime;
        if (age > fadeOutStart) {
            alpha *= 1.0f - (age - fadeOutStart) / fadeOutTime;
        }

        return alpha;
    }

    // ===== ForceField Implementation =====
    Vec2 ForceField::getForce(const Vec2& particlePos) const {
        Vec2 diff = position - particlePos;
        float distance = diff.length();

        if (distance > radius) return { 0, 0 };

        float forceMagnitude = strength * std::pow(1.0f - distance / radius, falloff);

        switch (type) {
        case ATTRACT:
            return diff.normalized() * forceMagnitude;

        case REPEL:
            return diff.normalized() * -forceMagnitude;

        case TURBULENCE:
            return Vec2::fromAngle(random_float(0, TWO_PI), forceMagnitude);

        case VORTEX: {
            Vec2 tangent = { -diff.y, diff.x };
            return tangent.normalized() * forceMagnitude;
        }
        }

        return { 0, 0 };
    }

    // ===== ParticleEmitter Implementation =====
    ParticleEmitter::ParticleEmitter() {
        // Initialize particle pool
        for (size_t i = 0; i < maxParticles; ++i) {
            particlePool.push_back(std::make_unique<Particle>());
        }

        // Default color ramp
        colorRamp = {
            {0.0f, Color(255, 255, 255)},
            {1.0f, Color(255, 255, 255, 0)}
        };
    }

    ParticleEmitter::~ParticleEmitter() {
        clear();
    }

    void ParticleEmitter::update(float dt) {
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
                float noise = ParticleUtils::perlinNoise(
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
                        p->velocity.y *= -p->bounce;
                        p->position.y = rect.y - p->collisionRadius;
                    }
                }
            }

            // Check if particle is dead
            if (!p->isAlive()) {
                if (onParticleDeath) {
                    onParticleDeath(*p);
                }

                // Return to pool
                (*it)->reset();
                returnToPool(std::move(*it));
                it = activeParticles.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void ParticleEmitter::emit(int count) {
        for (int i = 0; i < count && activeParticles.size() < maxParticles; ++i) {
            Particle* p = getPooledParticle();
            if (!p) break;

            // Initialize particle
            p->position = getEmissionPosition();
            p->velocity = getEmissionVelocity();
            p->lifetime = random_float(lifetimeRange.first, lifetimeRange.second);
            p->startSize = random_float(sizeRange.first, sizeRange.second);
            p->endSize = p->startSize * 0.1f;
            p->size = p->startSize;
            p->rotation = random_float(0, TWO_PI);
            p->angularVelocity = random_float(angularVelRange.first, angularVelRange.second);
            p->mass = random_float(massRange.first, massRange.second);
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

    void ParticleEmitter::burst() {
        emit(burstCount);
    }

    void ParticleEmitter::clear() {
        for (auto& p : activeParticles) {
            p->reset();
            returnToPool(std::move(p));
        }
        activeParticles.clear();
    }

    void ParticleEmitter::draw(SDL_Renderer* renderer) {
        // Sort particles by blend mode for proper rendering
        std::stable_sort(activeParticles.begin(), activeParticles.end(),
            [](const auto& a, const auto& b) {
                return static_cast<int>(a->blendMode) < static_cast<int>(b->blendMode);
            });

        // Draw particles
        for (auto& p : activeParticles) {
            drawParticle(renderer, *p);
        }
    }

    void ParticleEmitter::drawParticle(SDL_Renderer* renderer, Particle& particle) {
        Color color = particle.getCurrentColor();
        float size = particle.getCurrentSize();
        float alpha = particle.getCurrentAlpha();

        // Apply shimmer
        if (particle.shimmerRate > 0) {
            float shimmer = std::sin(particle.age * particle.shimmerRate * TWO_PI) * 0.5f + 0.5f;
            alpha *= 0.5f + shimmer * 0.5f;
        }

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
            drawTrail(renderer, particle);
        }

        // Draw glow
        if (particle.hasGlow) {
            drawGlow(renderer, particle.position, size * 2, color, particle.glowIntensity);
        }

        // Draw main shape
        drawShape(renderer, particle.shape, particle.position, size, particle.rotation, color);

        // Reset blend mode
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    void ParticleEmitter::drawGlow(SDL_Renderer* renderer, const Vec2& pos, float size,
        const Color& color, float intensity) {
        int layers = static_cast<int>(5 * intensity);
        for (int i = layers; i > 0; --i) {
            float t = static_cast<float>(i) / layers;
            Color glowColor = color;
            glowColor.a *= t * 0.2f;
            float glowSize = size * (1.0f + t);

            SDL_Color c = glowColor.toSDL();
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

            // Draw glow circle
            int segments = 32;
            std::vector<SDL_FPoint> points;
            for (int j = 0; j <= segments; ++j) {
                float angle = (j / static_cast<float>(segments)) * TWO_PI;
                points.push_back({
                    pos.x + std::cos(angle) * glowSize,
                    pos.y + std::sin(angle) * glowSize
                    });
            }
            SDL_RenderLines(renderer, points.data(), static_cast<int>(points.size()));
        }
    }

    void ParticleEmitter::drawTrail(SDL_Renderer* renderer, Particle& particle) {
        if (particle.trail.size() < 2) return;

        for (size_t i = 0; i < particle.trail.size() - 1; ++i) {
            float t = static_cast<float>(i) / particle.trail.size();
            Color trailColor = particle.getCurrentColor();
            trailColor.a *= t * particle.trailFadeRate;

            SDL_Color c = trailColor.toSDL();
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

            SDL_RenderLine(renderer,
                particle.trail[i].x, particle.trail[i].y,
                particle.trail[i + 1].x, particle.trail[i + 1].y);
        }
    }

    void ParticleEmitter::drawShape(SDL_Renderer* renderer, ParticleShape shape, const Vec2& pos,
        float size, float rotation, const Color& color) {
        SDL_Color c = color.toSDL();
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

        switch (shape) {
        case ParticleShape::CIRCLE: {
            int segments = std::max(8, static_cast<int>(size * 2));
            for (int i = 0; i < segments; ++i) {
                float angle1 = (i / static_cast<float>(segments)) * TWO_PI;
                float angle2 = ((i + 1) / static_cast<float>(segments)) * TWO_PI;
                SDL_RenderLine(renderer,
                    pos.x + std::cos(angle1) * size,
                    pos.y + std::sin(angle1) * size,
                    pos.x + std::cos(angle2) * size,
                    pos.y + std::sin(angle2) * size);
            }
            break;
        }

        case ParticleShape::SQUARE: {
            SDL_FRect rect = { pos.x - size, pos.y - size, size * 2, size * 2 };
            SDL_RenderFillRect(renderer, &rect);
            break;
        }

        case ParticleShape::STAR: {
            auto points = ParticleUtils::generateStarPoints(5, size * 0.4f, size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            SDL_RenderLines(renderer, sdlPoints.data(), static_cast<int>(sdlPoints.size()));
            break;
        }

        case ParticleShape::HEART: {
            auto points = ParticleUtils::generateHeartPoints(size);
            std::vector<SDL_FPoint> sdlPoints;
            for (const auto& p : points) {
                Vec2 rotated = p.rotate(rotation);
                sdlPoints.push_back({ pos.x + rotated.x, pos.y + rotated.y });
            }
            SDL_RenderLines(renderer, sdlPoints.data(), static_cast<int>(sdlPoints.size()));
            break;
        }

        case ParticleShape::HEXAGON: {
            std::vector<SDL_FPoint> points;
            for (int i = 0; i <= 6; ++i) {
                float angle = (i / 6.0f) * TWO_PI + rotation;
                points.push_back({
                    pos.x + std::cos(angle) * size,
                    pos.y + std::sin(angle) * size
                    });
            }
            SDL_RenderLines(renderer, points.data(), static_cast<int>(points.size()));
            break;
        }

        case ParticleShape::RING: {
            int segments = 32;
            float innerRadius = size * 0.6f;
            for (int i = 0; i < segments; ++i) {
                float angle = (i / static_cast<float>(segments)) * TWO_PI;
                float nextAngle = ((i + 1) / static_cast<float>(segments)) * TWO_PI;

                // Outer ring
                SDL_RenderLine(renderer,
                    pos.x + std::cos(angle) * size,
                    pos.y + std::sin(angle) * size,
                    pos.x + std::cos(nextAngle) * size,
                    pos.y + std::sin(nextAngle) * size);

                // Inner ring
                SDL_RenderLine(renderer,
                    pos.x + std::cos(angle) * innerRadius,
                    pos.y + std::sin(angle) * innerRadius,
                    pos.x + std::cos(nextAngle) * innerRadius,
                    pos.y + std::sin(nextAngle) * innerRadius);
            }
            break;
        }

        case ParticleShape::LIGHTNING: {
            // Draw jagged lightning bolt
            std::vector<SDL_FPoint> points;
            int segments = 5;
            for (int i = 0; i <= segments; ++i) {
                float t = i / static_cast<float>(segments);
                float x = pos.x + random_float(-size * 0.3f, size * 0.3f);
                float y = pos.y - size + t * size * 2;
                points.push_back({ x, y });
            }
            SDL_RenderLines(renderer, points.data(), static_cast<int>(points.size()));
            break;
        }

        case ParticleShape::FLAME: {
            // Draw flame-like shape
            for (int i = 0; i < 3; ++i) {
                float offset = random_float(-size * 0.2f, size * 0.2f);
                float height = size * (1.0f - i * 0.3f);
                SDL_RenderLine(renderer,
                    pos.x + offset, pos.y + size,
                    pos.x + offset * 0.5f, pos.y - height);
            }
            break;
        }

        default:
            // Fallback to circle
            SDL_RenderPoint(renderer, pos.x, pos.y);
            break;
        }
    }

    Particle* ParticleEmitter::getPooledParticle() {
        if (particlePool.empty()) return nullptr;

        Particle* p = particlePool.back().release();
        particlePool.pop_back();
        return p;
    }

    void ParticleEmitter::returnToPool(std::unique_ptr<Particle> particle) {
        if (particlePool.size() < maxParticles) {
            particlePool.push_back(std::move(particle));
        }
    }

    Vec2 ParticleEmitter::getEmissionPosition() const {
        switch (pattern) {
        case EmissionPattern::POINT:
            return position;

        case EmissionPattern::CIRCLE: {
            float angle = random_float(0, TWO_PI);
            float radius = random_float(0, patternRadius);
            return position + Vec2::fromAngle(angle, radius);
        }

        case EmissionPattern::RING: {
            float angle = random_float(0, TWO_PI);
            return position + Vec2::fromAngle(angle, patternRadius);
        }

        case EmissionPattern::CONE: {
            float angle = random_float(-patternAngle / 2, patternAngle / 2) + rotation;
            float distance = random_float(0, patternRadius);
            return position + Vec2::fromAngle(angle, distance);
        }

        case EmissionPattern::BOX: {
            float x = random_float(-patternRadius, patternRadius);
            float y = random_float(-patternRadius, patternRadius);
            return position + Vec2(x, y);
        }

        case EmissionPattern::LINE: {
            float t = random_float(-1, 1);
            Vec2 dir = Vec2::fromAngle(rotation);
            return position + dir * (t * patternRadius);
        }

        case EmissionPattern::SPIRAL: {
            static float spiralAngle = 0;
            spiralAngle += 0.5f;
            float radius = patternRadius * (spiralAngle / TWO_PI);
            return position + Vec2::fromAngle(spiralAngle, radius);
        }

        default:
            return position;
        }
    }

    Vec2 ParticleEmitter::getEmissionVelocity() const {
        float angle = random_float(angleRange.first, angleRange.second);
        float speed = random_float(speedRange.first, speedRange.second);
        return Vec2::fromAngle(angle, speed);
    }

    // ===== ParticleSystemManager Implementation =====
    ParticleEmitter* ParticleSystemManager::createEmitter() {
        emitters.push_back(std::make_unique<ParticleEmitter>());
        return emitters.back().get();
    }

    void ParticleSystemManager::removeEmitter(ParticleEmitter* emitter) {
        emitters.erase(
            std::remove_if(emitters.begin(), emitters.end(),
                [emitter](const auto& e) { return e.get() == emitter; }),
            emitters.end()
        );
    }

    void ParticleSystemManager::update(float dt) {
        auto start = std::chrono::high_resolution_clock::now();

        totalParticles = 0;
        for (auto& emitter : emitters) {
            emitter->update(dt);
            totalParticles += static_cast<int>(emitter->getParticleCount());
        }

        auto end = std::chrono::high_resolution_clock::now();
        updateTime = std::chrono::duration<float, std::milli>(end - start).count();
    }

    void ParticleSystemManager::draw() {
        auto start = std::chrono::high_resolution_clock::now();

        for (auto& emitter : emitters) {
            emitter->draw(renderer);
        }

        auto end = std::chrono::high_resolution_clock::now();
        drawTime = std::chrono::duration<float, std::milli>(end - start).count();
    }

    void ParticleSystemManager::clear() {
        emitters.clear();
    }

    // Preset effects implementation
    ParticleEmitter* ParticleSystemManager::createFireEffect(const Vec2& pos) {
        auto emitter = createEmitter();
        emitter->position = pos;
        emitter->emissionRate = 100;
        emitter->pattern = EmissionPattern::CONE;
        emitter->patternAngle = HALF_PI / 2;
        emitter->patternRadius = 10;
        emitter->rotation = -HALF_PI;

        emitter->lifetimeRange = { 0.5f, 1.5f };
        emitter->sizeRange = { 8.0f, 15.0f };
        emitter->speedRange = { 50.0f, 150.0f };
        emitter->angleRange = { -HALF_PI - 0.3f, -HALF_PI + 0.3f };

        emitter->colorRamp = {
            {0.0f, Color(255, 255, 200)},
            {0.2f, Color(255, 200, 100)},
            {0.5f, Color(255, 100, 50)},
            {1.0f, Color(100, 0, 0, 0)}
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

        return emitter;
    }

    ParticleEmitter* ParticleSystemManager::createExplosionEffect(const Vec2& pos) {
        auto emitter = createEmitter();
        emitter->position = pos;
        emitter->burstMode = true;
        emitter->burstCount = 200;
        emitter->active = false; // Single burst

        emitter->pattern = EmissionPattern::SPHERE;
        emitter->patternRadius = 5;

        emitter->lifetimeRange = { 0.5f, 1.0f };
        emitter->sizeRange = { 5.0f, 15.0f };
        emitter->speedRange = { 200.0f, 500.0f };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            {0.0f, Color(255, 255, 255)},
            {0.1f, Color(255, 200, 100)},
            {0.3f, Color(255, 100, 50)},
            {1.0f, Color(50, 50, 50, 0)}
        };

        emitter->shape = ParticleShape::CIRCLE;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 2.0f;

        emitter->drag = 0.95f;

        // Add shockwave
        auto shockwave = createEmitter();
        shockwave->position = pos;
        shockwave->burstMode = true;
        shockwave->burstCount = 1;
        shockwave->active = false;

        shockwave->lifetimeRange = { 0.5f, 0.5f };
        shockwave->sizeRange = { 10.0f, 10.0f };
        shockwave->speedRange = { 0, 0 };

        shockwave->colorRamp = {
            {0.0f, Color(255, 255, 255, 128)},
            {1.0f, Color(255, 255, 255, 0)}
        };

        shockwave->shape = ParticleShape::RING;
        shockwave->blendMode = BlendMode::ADD;

        // Custom update to expand ring
        shockwave->onParticleUpdate = [](Particle& p) {
            p.size = p.startSize + p.age * 500;
            };

        emitter->burst();
        shockwave->burst();

        return emitter;
    }

    ParticleEmitter* ParticleSystemManager::createMagicEffect(const Vec2& pos) {
        auto emitter = createEmitter();
        emitter->position = pos;
        emitter->emissionRate = 50;
        emitter->pattern = EmissionPattern::CIRCLE;
        emitter->patternRadius = 30;

        emitter->lifetimeRange = { 1.0f, 2.0f };
        emitter->sizeRange = { 3.0f, 8.0f };
        emitter->speedRange = { 20.0f, 50.0f };
        emitter->angleRange = { 0, TWO_PI };

        emitter->colorRamp = {
            {0.0f, Color::hsv(random_float(0, 360), 1.0f, 1.0f)},
            {0.5f, Color::hsv(random_float(0, 360), 0.8f, 0.8f)},
            {1.0f, Color(100, 100, 255, 0)}
        };

        emitter->shape = ParticleShape::STAR;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 1.0f;
        emitter->enablePulse = true;
        emitter->pulseRate = 2.0f;
        emitter->pulseAmount = 0.3f;
        emitter->enableShimmer = true;
        emitter->shimmerRate = 5.0f;

        emitter->behaviors.push_back(ParticleBehavior::ORBIT);
        emitter->targetPosition = pos;

        // Add force field for swirling effect
        ForceField field;
        field.position = pos;
        field.radius = 100;
        field.strength = 50;
        field.type = ForceField::VORTEX;
        emitter->addForceField(field);

        return emitter;
    }

    ParticleEmitter* ParticleSystemManager::createElectricityEffect(const Vec2& start, const Vec2& end) {
        auto emitter = createEmitter();
        emitter->position = start;
        emitter->emissionRate = 200;
        emitter->pattern = EmissionPattern::LINE;
        emitter->patternRadius = (end - start).length();
        emitter->rotation = std::atan2(end.y - start.y, end.x - start.x);

        emitter->lifetimeRange = { 0.1f, 0.3f };
        emitter->sizeRange = { 2.0f, 4.0f };
        emitter->speedRange = { 0, 50.0f };

        emitter->colorRamp = {
            {0.0f, Color(200, 200, 255)},
            {0.5f, Color(150, 150, 255)},
            {1.0f, Color(100, 100, 200, 0)}
        };

        emitter->shape = ParticleShape::LIGHTNING;
        emitter->blendMode = BlendMode::ADD;
        emitter->enableGlow = true;
        emitter->glowIntensity = 2.0f;

        emitter->turbulence = 100;

        return emitter;
    }

    // ===== Utility Functions =====
    namespace ParticleUtils {

        float perlinNoise(float x, float y) {
            // Simple pseudo-random noise (replace with proper Perlin noise for production)
            int n = static_cast<int>(x + y * 57);
            n = (n << 13) ^ n;
            float noise = (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
            return noise;
        }

        float easeInOut(float t) {
            return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
        }

        float easeInCubic(float t) {
            return t * t * t;
        }

        float easeOutCubic(float t) {
            return 1 + (--t) * t * t;
        }

        float easeInOutElastic(float t) {
            if (t < 0.5f) {
                return 0.5f * std::sin(13 * HALF_PI * (2 * t)) * std::pow(2, 10 * ((2 * t) - 1));
            }
            else {
                return 0.5f * (std::sin(-13 * HALF_PI * ((2 * t - 1) + 1)) * std::pow(2, -10 * (2 * t - 1)) + 2);
            }
        }

        float bounce(float t) {
            if (t < 0.363636f) {
                return 7.5625f * t * t;
            }
            else if (t < 0.727272f) {
                t -= 0.545454f;
                return 7.5625f * t * t + 0.75f;
            }
            else if (t < 0.909090f) {
                t -= 0.818181f;
                return 7.5625f * t * t + 0.9375f;
            }
            else {
                t -= 0.954545f;
                return 7.5625f * t * t + 0.984375f;
            }
        }

        std::vector<Vec2> generateStarPoints(int numPoints, float innerRadius, float outerRadius) {
            std::vector<Vec2> points;
            float angleStep = TWO_PI / (numPoints * 2);

            for (int i = 0; i < numPoints * 2; ++i) {
                float radius = (i % 2 == 0) ? outerRadius : innerRadius;
                float angle = i * angleStep;
                points.push_back({ std::cos(angle) * radius, std::sin(angle) * radius });
            }
            points.push_back(points[0]); // Close the shape

            return points;
        }

        std::vector<Vec2> generateHeartPoints(float size) {
            std::vector<Vec2> points;
            int segments = 32;

            for (int i = 0; i <= segments; ++i) {
                float t = (i / static_cast<float>(segments)) * TWO_PI;
                float x = 16 * std::pow(std::sin(t), 3);
                float y = -(13 * std::cos(t) - 5 * std::cos(2 * t) - 2 * std::cos(3 * t) - std::cos(4 * t));
                points.push_back({ x * size / 20, y * size / 20 });
            }

            return points;
        }

        std::vector<Vec2> generateSpiralPoints(float size, int turns) {
            std::vector<Vec2> points;
            int segments = 32 * turns;

            for (int i = 0; i <= segments; ++i) {
                float t = (i / static_cast<float>(segments)) * turns * TWO_PI;
                float radius = (i / static_cast<float>(segments)) * size;
                points.push_back({ std::cos(t) * radius, std::sin(t) * radius });
            }

            return points;
        }

        Color rainbow(float t) {
            return Color::hsv(t * 360, 1.0f, 1.0f);
        }

        Color temperatureToColor(float kelvin) {
            // Simplified color temperature conversion
            float temp = kelvin / 100;
            float r, g, b;

            if (temp <= 66) {
                r = 255;
                g = temp;
                g = 99.4708025861f * std::log(g) - 161.1195681661f;
                if (temp <= 19) {
                    b = 0;
                }
                else {
                    b = temp - 10;
                    b = 138.5177312231f * std::log(b) - 305.0447927307f;
                }
            }
            else {
                r = temp - 60;
                r = 329.698727446f * std::pow(r, -0.1332047592f);
                g = temp - 60;
                g = 288.1221695283f * std::pow(g, -0.0755148492f);
                b = 255;
            }

            return Color(
                std::max(0.0f, std::min(1.0f, r / 255)),
                std::max(0.0f, std::min(1.0f, g / 255)),
                std::max(0.0f, std::min(1.0f, b / 255))
            );
        }

        Color plasmaColor(float t) {
            // Generate plasma-like colors
            float r = std::sin(t * TWO_PI) * 0.5f + 0.5f;
            float g = std::sin(t * TWO_PI + TWO_PI / 3) * 0.5f + 0.5f;
            float b = std::sin(t * TWO_PI + 2 * TWO_PI / 3) * 0.5f + 0.5f;
            return Color(r, g, b);
        }

    } // namespace ParticleUtils

} // namespace ParticleSystem