//// particle_system.hpp - Advanced SDL3 Particle System
//#pragma once
//#include <SDL3/SDL.h>
//#include <vector>
//#include <memory>
//#include <deque>
//#include <unordered_map>
//#include <functional>
//#include <algorithm>
//#include <cmath>
//#include <random>
//#include <string>
//
//namespace ParticleSystem {
//
//    // Constants
//    constexpr float PI = 3.14159265f;
//    constexpr float TWO_PI = 6.28318531f;
//    constexpr float HALF_PI = 1.57079633f;
//
//    // Random utilities
//    inline std::random_device rd;
//    inline std::mt19937 gen(rd());
//
//    inline float random_float(float min, float max) {
//        std::uniform_real_distribution<float> dis(min, max);
//        return dis(gen);
//    }
//
//    inline int random_int(int min, int max) {
//        std::uniform_int_distribution<int> dis(min, max);
//        return dis(gen);
//    }
//
//    // Vector2 for particle physics
//    struct Vec2 {
//        float x, y;
//
//        Vec2(float x = 0, float y = 0) : x(x), y(y) {}
//
//        Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
//        Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
//        Vec2 operator*(float s) const { return { x * s, y * s }; }
//        Vec2 operator/(float s) const { return { x / s, y / s }; }
//        Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
//        Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
//        Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
//
//        float length() const { return std::sqrt(x * x + y * y); }
//        float lengthSq() const { return x * x + y * y; }
//        Vec2 normalized() const {
//            float len = length();
//            return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
//        }
//        float dot(const Vec2& v) const { return x * v.x + y * v.y; }
//        Vec2 rotate(float angle) const {
//            float c = std::cos(angle);
//            float s = std::sin(angle);
//            return { x * c - y * s, x * s + y * c };
//        }
//
//        static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
//            return a + (b - a) * t;
//        }
//
//        static Vec2 fromAngle(float angle, float magnitude = 1.0f) {
//            return { std::cos(angle) * magnitude, std::sin(angle) * magnitude };
//        }
//    };
//
//    // Color with HDR support
//    struct Color {
//        float r, g, b, a;
//
//        Color(float r = 1, float g = 1, float b = 1, float a = 1)
//            : r(r), g(g), b(b), a(a) {
//        }
//
//        Color(int r, int g, int b, int a = 255)
//            : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {
//        }
//
//        static Color lerp(const Color& a, const Color& b, float t) {
//            return {
//                a.r + (b.r - a.r) * t,
//                a.g + (b.g - a.g) * t,
//                a.b + (b.b - a.b) * t,
//                a.a + (b.a - a.a) * t
//            };
//        }
//
//        static Color hsv(float h, float s, float v, float a = 1.0f) {
//            float c = v * s;
//            float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
//            float m = v - c;
//
//            float r = 0, g = 0, b = 0;
//            if (h < 60) { r = c; g = x; b = 0; }
//            else if (h < 120) { r = x; g = c; b = 0; }
//            else if (h < 180) { r = 0; g = c; b = x; }
//            else if (h < 240) { r = 0; g = x; b = c; }
//            else if (h < 300) { r = x; g = 0; b = c; }
//            else { r = c; g = 0; b = x; }
//
//            return { r + m, g + m, b + m, a };
//        }
//
//        SDL_Color toSDL() const {
//            return {
//                static_cast<Uint8>(std::min(255.0f, r * 255)),
//                static_cast<Uint8>(std::min(255.0f, g * 255)),
//                static_cast<Uint8>(std::min(255.0f, b * 255)),
//                static_cast<Uint8>(std::min(255.0f, a * 255))
//            };
//        }
//    };
//
//    // Particle shapes
//    enum class ParticleShape {
//        CIRCLE,
//        SQUARE,
//        TRIANGLE,
//        STAR,
//        HEXAGON,
//        RING,
//        HEART,
//        DIAMOND,
//        CROSS,
//        SPIRAL,
//        LIGHTNING,
//        SMOKE_PUFF,
//        FLAME,
//        SPARKLE,
//        BUBBLE,
//        CUSTOM
//    };
//
//    // Blend modes
//    enum class BlendMode {
//        NORMAL,
//        ADD,
//        MULTIPLY,
//        SCREEN,
//        OVERLAY,
//        SOFT_LIGHT,
//        COLOR_DODGE
//    };
//
//    // Emission patterns
//    enum class EmissionPattern {
//        POINT,
//        CIRCLE,
//        RING,
//        CONE,
//        BOX,
//        SPHERE,
//        LINE,
//        SPIRAL,
//        BURST,
//        WAVE,
//        FOUNTAIN
//    };
//
//    // Particle behaviors
//    enum class ParticleBehavior {
//        NONE,
//        GRAVITY,
//        WIND,
//        TURBULENCE,
//        ATTRACT,
//        REPEL,
//        ORBIT,
//        SWIRL,
//        FLOCK,
//        SEEK,
//        FLEE,
//        WANDER,
//        FLOW_FIELD,
//        MAGNETIC
//    };
//
//    // Color ramp point
//    struct ColorRampPoint {
//        float t;  // Time (0-1)
//        Color color;
//    };
//
//    // Particle class
//    class Particle {
//    public:
//        // Physics
//        Vec2 position;
//        Vec2 velocity;
//        Vec2 acceleration;
//        Vec2 previousPos;  // For motion blur
//        float mass = 1.0f;
//        float drag = 0.98f;
//        float bounce = 0.8f;
//
//        // Visual
//        float size;
//        float startSize;
//        float endSize;
//        float rotation = 0;
//        float angularVelocity = 0;
//        Color color;
//        std::vector<ColorRampPoint> colorRamp;
//        ParticleShape shape = ParticleShape::CIRCLE;
//        BlendMode blendMode = BlendMode::ADD;
//        float glowIntensity = 0;
//        float distortionAmount = 0;
//
//        // Lifetime
//        float age = 0;
//        float lifetime = 1.0f;
//        float fadeInTime = 0.1f;
//        float fadeOutTime = 0.2f;
//
//        // Trail
//        std::deque<Vec2> trail;
//        int maxTrailLength = 0;
//        float trailFadeRate = 0.9f;
//
//        // Behaviors
//        std::vector<ParticleBehavior> behaviors;
//        Vec2 target;  // For seeking behaviors
//        float behaviorStrength = 1.0f;
//
//        // Special effects
//        bool hasGlow = false;
//        bool hasDistortion = false;
//        bool hasShadow = false;
//        float pulseRate = 0;
//        float pulseAmount = 0;
//        float shimmerRate = 0;
//
//        // Collision
//        bool collides = false;
//        float collisionRadius = 0;
//
//        // Custom data
//        std::unordered_map<std::string, float> customData;
//
//    public:
//        Particle() = default;
//
//        void update(float dt);
//        void applyForce(const Vec2& force);
//        void applyBehaviors(float dt);
//        Color getCurrentColor() const;
//        float getCurrentSize() const;
//        float getCurrentAlpha() const;
//        bool isAlive() const { return age < lifetime; }
//
//        void reset() {
//            position = { 0, 0 };
//            velocity = { 0, 0 };
//            acceleration = { 0, 0 };
//            age = 0;
//            trail.clear();
//            behaviors.clear();
//            customData.clear();
//        }
//    };
//
//    // Force field
//    class ForceField {
//    public:
//        Vec2 position;
//        float radius;
//        float strength;
//        float falloff = 2.0f;  // How quickly force decreases with distance
//
//        enum Type {
//            ATTRACT,
//            REPEL,
//            TURBULENCE,
//            VORTEX
//        } type;
//
//        Vec2 getForce(const Vec2& particlePos) const;
//    };
//
//    // Emitter class
//    class ParticleEmitter {
//    private:
//        // Particle management
//        std::vector<std::unique_ptr<Particle>> activeParticles;
//        std::vector<std::unique_ptr<Particle>> particlePool;
//        size_t maxParticles = 5000;
//
//        // Texture cache
//        static std::unordered_map<std::string, SDL_Texture*> textureCache;
//
//    public:
//        // Transform
//        Vec2 position;
//        float rotation = 0;
//        Vec2 scale = { 1, 1 };
//
//        // Emission
//        bool active = true;
//        float emissionRate = 100;  // particles per second
//        float emissionAccumulator = 0;
//        EmissionPattern pattern = EmissionPattern::POINT;
//        float patternRadius = 50;
//        float patternAngle = TWO_PI;
//
//        // Burst emission
//        bool burstMode = false;
//        int burstCount = 100;
//        float burstInterval = 1.0f;
//        float burstTimer = 0;
//
//        // Particle properties (ranges for randomization)
//        std::pair<float, float> lifetimeRange = { 1.0f, 2.0f };
//        std::pair<float, float> sizeRange = { 4.0f, 8.0f };
//        std::pair<float, float> speedRange = { 50.0f, 150.0f };
//        std::pair<float, float> angleRange = { 0, TWO_PI };
//        std::pair<float, float> angularVelRange = { -180.0f, 180.0f };
//        std::pair<float, float> massRange = { 0.8f, 1.2f };
//
//        // Visual properties
//        std::vector<ColorRampPoint> colorRamp;
//        ParticleShape shape = ParticleShape::CIRCLE;
//        BlendMode blendMode = BlendMode::ADD;
//        bool enableGlow = true;
//        float glowIntensity = 1.0f;
//        bool enableTrails = false;
//        int trailLength = 10;
//
//        // Physics
//        Vec2 gravity = { 0, 98 };
//        Vec2 wind = { 0, 0 };
//        float turbulence = 0;
//        float drag = 0.98f;
//        std::vector<ForceField> forceFields;
//
//        // Behaviors
//        std::vector<ParticleBehavior> behaviors;
//        Vec2 targetPosition;  // For seeking behaviors
//
//        // Special effects
//        bool enablePulse = false;
//        float pulseRate = 2.0f;
//        float pulseAmount = 0.2f;
//        bool enableShimmer = false;
//        float shimmerRate = 10.0f;
//        bool enableDistortion = false;
//        float distortionAmount = 0.1f;
//
//        // Collision
//        bool enableCollision = false;
//        std::vector<SDL_FRect> collisionRects;
//
//        // Callbacks
//        std::function<void(Particle&)> onParticleSpawn;
//        std::function<void(Particle&)> onParticleUpdate;
//        std::function<void(Particle&)> onParticleDeath;
//
//    public:
//        ParticleEmitter();
//        ~ParticleEmitter();
//
//        void update(float dt);
//        void emit(int count = 1);
//        void burst();
//        void clear();
//
//        void draw(SDL_Renderer* renderer);
//        void drawParticle(SDL_Renderer* renderer, Particle& particle);
//        void drawGlow(SDL_Renderer* renderer, const Vec2& pos, float size, const Color& color, float intensity);
//        void drawTrail(SDL_Renderer* renderer, Particle& particle);
//        void drawShape(SDL_Renderer* renderer, ParticleShape shape, const Vec2& pos, float size,
//            float rotation, const Color& color);
//
//        void addForceField(const ForceField& field) { forceFields.push_back(field); }
//        void clearForceFields() { forceFields.clear(); }
//
//        size_t getParticleCount() const { return activeParticles.size(); }
//
//    private:
//        Particle* getPooledParticle();
//        void returnToPool(std::unique_ptr<Particle> particle);
//        Vec2 getEmissionPosition() const;
//        Vec2 getEmissionVelocity() const;
//    };
//
//    // Particle system manager
//    class ParticleSystemManager {
//    private:
//        std::vector<std::unique_ptr<ParticleEmitter>> emitters;
//        SDL_Renderer* renderer;
//
//        // Performance monitoring
//        float updateTime = 0;
//        float drawTime = 0;
//        int totalParticles = 0;
//
//    public:
//        ParticleSystemManager(SDL_Renderer* ren) : renderer(ren) {}
//
//        ParticleEmitter* createEmitter();
//        void removeEmitter(ParticleEmitter* emitter);
//        void update(float dt);
//        void draw();
//        void clear();
//
//        // Preset effects
//        ParticleEmitter* createFireEffect(const Vec2& pos);
//        //ParticleEmitter* createSmokeEffect(const Vec2& pos);
//        ParticleEmitter* createExplosionEffect(const Vec2& pos);
//        ParticleEmitter* createMagicEffect(const Vec2& pos);
//        /*ParticleEmitter* createSparkleEffect(const Vec2& pos);
//        ParticleEmitter* createRainEffect(const Vec2& pos);
//        ParticleEmitter* createSnowEffect(const Vec2& pos);*/
//        ParticleEmitter* createElectricityEffect(const Vec2& start, const Vec2& end);
//        /*ParticleEmitter* createPortalEffect(const Vec2& pos);
//        ParticleEmitter* createGalaxyEffect(const Vec2& pos);
//        ParticleEmitter* createFlameJetEffect(const Vec2& pos, float angle);*/
//        /*ParticleEmitter* createWaterfallEffect(const Vec2& pos);
//        ParticleEmitter* createAuroraEffect(const Vec2& pos);*/
//        //ParticleEmitter* createBloodEffect(const Vec2& pos, const Vec2& direction);
//        /*ParticleEmitter* createShockwaveEffect(const Vec2& pos);
//        ParticleEmitter* createConfettiEffect(const Vec2& pos);
//        ParticleEmitter* createFireworksEffect(const Vec2& pos);
//        ParticleEmitter* createCosmicDustEffect(const Vec2& pos);
//        ParticleEmitter* createHealingEffect(const Vec2& pos);
//        ParticleEmitter* createPoisonEffect(const Vec2& pos);*/
//
//        // Performance info
//        void getPerformanceStats(float& updateMs, float& drawMs, int& particleCount) {
//            updateMs = updateTime;
//            drawMs = drawTime;
//            particleCount = totalParticles;
//        }
//    };
//
//    // Utility functions for advanced effects
//    namespace ParticleUtils {
//        // Noise functions for turbulence
//        float perlinNoise(float x, float y);
//        /*float simplexNoise(float x, float y, float z);*/
//
//        // Curve functions for particle properties
//        float easeInOut(float t);
//        float easeInCubic(float t);
//        float easeOutCubic(float t);
//        float easeInOutElastic(float t);
//        float bounce(float t);
//
//        // Shape generation
//        std::vector<Vec2> generateStarPoints(int numPoints, float innerRadius, float outerRadius);
//        std::vector<Vec2> generateHeartPoints(float size);
//        std::vector<Vec2> generateSpiralPoints(float size, int turns);
//
//        // Color utilities
//        Color rainbow(float t);
//        Color temperatureToColor(float kelvin);
//        Color plasmaColor(float t);
//    }
//
//} // namespace ParticleSystem