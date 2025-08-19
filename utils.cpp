// utils.cpp - Fixed to avoid multiple definition errors
#pragma once
#include <SDL3/SDL.h>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <array>

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float HALF_PI = 1.57079632679489661923f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Vector2 structure
struct Vec2 {
    float x, y;

    Vec2(float x = 0, float y = 0) : x(x), y(y) {}

    Vec2 operator-() const { return { -x, -y }; }
    Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2 operator/(float s) const { return { x / s, y / s }; }
    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float len = length();
        return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
    }

    float dot(const Vec2& v) const { return x * v.x + y * v.y; }
    float cross(const Vec2& v) const { return x * v.y - y * v.x; }

    Vec2 rotate(float angle) const {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return { x * c - y * s, x * s + y * c };
    }

    Vec2 perpendicular() const { return { -y, x }; }

    float angle() const { return std::atan2(y, x); }

    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
        return a + (b - a) * t;
    }

    static Vec2 fromAngle(float angle, float magnitude = 1.0f) {
        return { std::cos(angle) * magnitude, std::sin(angle) * magnitude };
    }

    static Vec2 random(float minX, float maxX, float minY, float maxY) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distX(minX, maxX);
        std::uniform_real_distribution<float> distY(minY, maxY);
        return { distX(gen), distY(gen) };
    }
};

// Color structure with HDR support
struct Color {
    float r, g, b, a;

    Color(float r = 1, float g = 1, float b = 1, float a = 1)
        : r(r), g(g), b(b), a(a) {
    }

    Color(int r, int g, int b, int a = 255)
        : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {
    }

    Color operator*(float s) const { return { r * s, g * s, b * s, a * s }; }
    Color operator+(const Color& c) const { return { r + c.r, g + c.g, b + c.b, a + c.a }; }

    static Color lerp(const Color& a, const Color& b, float t) {
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        };
    }

    static Color hsv(float h, float s, float v, float a = 1.0f) {
        float c = v * s;
        float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
        float m = v - c;

        float r = 0, g = 0, b = 0;
        if (h < 60) { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }

        return { r + m, g + m, b + m, a };
    }

    SDL_Color toSDL() const {
        return {
            static_cast<Uint8>(std::min(255.0f, std::max(0.0f, r * 255))),
            static_cast<Uint8>(std::min(255.0f, std::max(0.0f, g * 255))),
            static_cast<Uint8>(std::min(255.0f, std::max(0.0f, b * 255))),
            static_cast<Uint8>(std::min(255.0f, std::max(0.0f, a * 255)))
        };
    }
};

// Main Utils struct - all inline to avoid multiple definitions
struct Utils {
    // Random number generator - inline statics
    static std::random_device& getRD() {
        static std::random_device rd;
        return rd;
    }

    static std::mt19937& getGen() {
        static std::mt19937 gen(getRD()());
        return gen;
    }

    // Initialize random generator
    static inline void initRandom() {
        getGen().seed(getRD()());
    }

    // Random functions
    static inline float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dis(min, max);
        return dis(getGen());
    }

    static inline int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dis(min, max);
        return dis(getGen());
    }

    static inline bool randomBool(float probability = 0.5f) {
        return randomFloat(0, 1) < probability;
    }

    static inline Vec2 randomPointInCircle(float radius) {
        float angle = randomFloat(0, TWO_PI);
        float r = std::sqrt(randomFloat(0, 1)) * radius;
        return Vec2::fromAngle(angle, r);
    }

    static inline Vec2 randomPointOnCircle(float radius) {
        float angle = randomFloat(0, TWO_PI);
        return Vec2::fromAngle(angle, radius);
    }

    static inline Vec2 randomDirection() {
        float angle = randomFloat(0, TWO_PI);
        return Vec2::fromAngle(angle);
    }

    // Noise functions
    static inline float perlinNoise(float x, float y) {
        int xi = static_cast<int>(std::floor(x)) & 255;
        int yi = static_cast<int>(std::floor(y)) & 255;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);

        float u = fade(xf);
        float v = fade(yf);

        int a = p()[xi] + yi;
        int aa = p()[a];
        int ab = p()[a + 1];
        int b = p()[xi + 1] + yi;
        int ba = p()[b];
        int bb = p()[b + 1];

        float res = lerp(v,
            lerp(u, grad(p()[aa], xf, yf), grad(p()[ba], xf - 1, yf)),
            lerp(u, grad(p()[ab], xf, yf - 1), grad(p()[bb], xf - 1, yf - 1))
        );

        return res;
    }

    static inline float simplexNoise(float x, float y, float z) {
        const float F3 = 1.0f / 3.0f;
        const float G3 = 1.0f / 6.0f;

        float s = (x + y + z) * F3;
        int i = fastFloor(x + s);
        int j = fastFloor(y + s);
        int k = fastFloor(z + s);

        float t = (i + j + k) * G3;
        float X0 = i - t;
        float Y0 = j - t;
        float Z0 = k - t;
        float x0 = x - X0;
        float y0 = y - Y0;
        float z0 = z - Z0;

        int i1, j1, k1;
        int i2, j2, k2;

        if (x0 >= y0) {
            if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
            else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
            else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
        }
        else {
            if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
            else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
            else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        }

        float n0 = contributionSimplex(x0, y0, z0, i, j, k);
        float n1 = contributionSimplex(x0 - i1 + G3, y0 - j1 + G3, z0 - k1 + G3,
            i + i1, j + j1, k + k1);
        float n2 = contributionSimplex(x0 - i2 + 2 * G3, y0 - j2 + 2 * G3, z0 - k2 + 2 * G3,
            i + i2, j + j2, k + k2);
        float n3 = contributionSimplex(x0 - 1 + 3 * G3, y0 - 1 + 3 * G3, z0 - 1 + 3 * G3,
            i + 1, j + 1, k + 1);

        return 32.0f * (n0 + n1 + n2 + n3);
    }

    static inline float voronoiNoise(float x, float y) {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        float xf = x - xi;
        float yf = y - yi;

        float minDist = 1.0f;

        for (int j = -1; j <= 1; ++j) {
            for (int i = -1; i <= 1; ++i) {
                float cellX = i + hash2D(xi + i, yi + j);
                float cellY = j + hash2D(xi + i + 89, yi + j + 37);
                float dist = std::sqrt((cellX - xf) * (cellX - xf) +
                    (cellY - yf) * (cellY - yf));
                minDist = std::min(minDist, dist);
            }
        }

        return minDist;
    }

    static inline float turbulence(float x, float y, int octaves = 4) {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            value += perlinNoise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        return value / maxValue;
    }

    // All easing functions as inline
    static inline float linear(float t) { return t; }
    static inline float easeInQuad(float t) { return t * t; }
    static inline float easeOutQuad(float t) { return t * (2 - t); }
    static inline float easeInOutQuad(float t) {
        return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }

    static inline float easeInCubic(float t) { return t * t * t; }
    static inline float easeOutCubic(float t) {
        float t1 = t - 1;
        return t1 * t1 * t1 + 1;
    }
    static inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
    }

    static inline float easeInQuart(float t) { return t * t * t * t; }
    static inline float easeOutQuart(float t) {
        float t1 = t - 1;
        return 1 - t1 * t1 * t1 * t1;
    }
    static inline float easeInOutQuart(float t) {
        return t < 0.5f ? 8 * t * t * t * t : 1 - 8 * (--t) * t * t * t;
    }

    static inline float easeInQuint(float t) { return t * t * t * t * t; }
    static inline float easeOutQuint(float t) {
        float t1 = t - 1;
        return 1 + t1 * t1 * t1 * t1 * t1;
    }
    static inline float easeInOutQuint(float t) {
        return t < 0.5f ? 16 * t * t * t * t * t : 1 + 16 * (--t) * t * t * t * t;
    }

    static inline float easeInSine(float t) { return 1 - std::cos(t * HALF_PI); }
    static inline float easeOutSine(float t) { return std::sin(t * HALF_PI); }
    static inline float easeInOutSine(float t) { return 0.5f * (1 - std::cos(PI * t)); }

    static inline float easeInExpo(float t) {
        return t == 0 ? 0 : std::pow(2, 10 * (t - 1));
    }
    static inline float easeOutExpo(float t) {
        return t == 1 ? 1 : 1 - std::pow(2, -10 * t);
    }
    static inline float easeInOutExpo(float t) {
        if (t == 0 || t == 1) return t;
        if (t < 0.5f) return 0.5f * std::pow(2, 20 * t - 10);
        return 1 - 0.5f * std::pow(2, -20 * t + 10);
    }

    static inline float easeInCirc(float t) { return 1 - std::sqrt(1 - t * t); }
    static inline float easeOutCirc(float t) { return std::sqrt(1 - (--t) * t); }
    static inline float easeInOutCirc(float t) {
        return t < 0.5f
            ? (1 - std::sqrt(1 - 4 * t * t)) / 2
            : (std::sqrt(1 - (-2 * t + 2) * (-2 * t + 2)) + 1) / 2;
    }

    static inline float easeInElastic(float t) {
        if (t == 0 || t == 1) return t;
        float p = 0.3f;
        float s = p / 4;
        float postFix = std::pow(2, 10 * (t -= 1));
        return -(postFix * std::sin((t - s) * TWO_PI / p));
    }

    static inline float easeOutElastic(float t) {
        if (t == 0 || t == 1) return t;
        float p = 0.3f;
        float s = p / 4;
        return std::pow(2, -10 * t) * std::sin((t - s) * TWO_PI / p) + 1;
    }

    static inline float easeInOutElastic(float t) {
        if (t == 0 || t == 1) return t;
        float p = 0.45f;
        float s = p / 4;

        if (t < 0.5f) {
            float postFix = std::pow(2, 10 * (t *= 2 - 1));
            return -0.5f * (postFix * std::sin((t - s) * TWO_PI / p));
        }

        float postFix = std::pow(2, -10 * (t *= 2 - 1));
        return postFix * std::sin((t - s) * TWO_PI / p) * 0.5f + 1;
    }

    static inline float easeInBack(float t) {
        const float s = 1.70158f;
        return t * t * ((s + 1) * t - s);
    }

    static inline float easeOutBack(float t) {
        const float s = 1.70158f;
        t -= 1;
        return t * t * ((s + 1) * t + s) + 1;
    }

    static inline float easeInOutBack(float t) {
        const float s = 1.70158f * 1.525f;
        if (t < 0.5f) {
            return 0.5f * ((2 * t) * (2 * t) * ((s + 1) * (2 * t) - s));
        }
        t = 2 * t - 2;
        return 0.5f * (t * t * ((s + 1) * t + s) + 2);
    }

    static inline float bounce(float t) {
        if (t < 1 / 2.75f) {
            return 7.5625f * t * t;
        }
        else if (t < 2 / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        }
        else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }

    static inline float easeInBounce(float t) { return 1 - bounce(1 - t); }
    static inline float easeOutBounce(float t) { return bounce(t); }
    static inline float easeInOutBounce(float t) {
        return t < 0.5f
            ? (1 - bounce(1 - 2 * t)) / 2
            : (1 + bounce(2 * t - 1)) / 2;
    }

    // Shape generation functions - all inline
    static inline std::vector<Vec2> generateStarPoints(int numPoints, float innerRadius, float outerRadius) {
        std::vector<Vec2> points;
        float angleStep = TWO_PI / (numPoints * 2);

        for (int i = 0; i < numPoints * 2; ++i) {
            float radius = (i % 2 == 0) ? outerRadius : innerRadius;
            float angle = i * angleStep - HALF_PI;
            points.push_back(Vec2::fromAngle(angle, radius));
        }
        points.push_back(points[0]);

        return points;
    }

    static inline std::vector<Vec2> generatePolygonPoints(int sides, float radius) {
        std::vector<Vec2> points;
        float angleStep = TWO_PI / sides;

        for (int i = 0; i <= sides; ++i) {
            float angle = i * angleStep - HALF_PI;
            points.push_back(Vec2::fromAngle(angle, radius));
        }

        return points;
    }

    static inline std::vector<Vec2> generateHeartPoints(float size) {
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

    static inline std::vector<Vec2> generateSpiralPoints(float size, int turns, int pointsPerTurn = 16) {
        std::vector<Vec2> points;
        int totalPoints = turns * pointsPerTurn;

        for (int i = 0; i <= totalPoints; ++i) {
            float t = (i / static_cast<float>(totalPoints));
            float angle = t * turns * TWO_PI;
            float radius = t * size;
            points.push_back(Vec2::fromAngle(angle, radius));
        }

        return points;
    }

    static inline std::vector<Vec2> generateGearPoints(int teeth, float innerRadius, float outerRadius) {
        std::vector<Vec2> points;
        float angleStep = TWO_PI / teeth;
        float toothWidth = angleStep * 0.3f;

        for (int i = 0; i < teeth; ++i) {
            float baseAngle = i * angleStep;

            points.push_back(Vec2::fromAngle(baseAngle - toothWidth / 2, innerRadius));
            points.push_back(Vec2::fromAngle(baseAngle - toothWidth / 2, outerRadius));
            points.push_back(Vec2::fromAngle(baseAngle + toothWidth / 2, outerRadius));
            points.push_back(Vec2::fromAngle(baseAngle + toothWidth / 2, innerRadius));
            points.push_back(Vec2::fromAngle(baseAngle + angleStep - toothWidth / 2, innerRadius));
        }
        points.push_back(points[0]);

        return points;
    }

    static inline std::vector<Vec2> generateLightningPoints(Vec2 start, Vec2 end, int segments = 5, float chaos = 20) {
        std::vector<Vec2> points;
        points.push_back(start);

        Vec2 direction = (end - start).normalized();
        Vec2 perpendicular = direction.perpendicular();
        float segmentLength = (end - start).length() / segments;

        Vec2 current = start;
        for (int i = 1; i < segments; ++i) {
            float offset = randomFloat(-chaos, chaos);
            current = current + direction * segmentLength;
            points.push_back(current + perpendicular * offset);
        }
        points.push_back(end);

        return points;
    }

    static inline std::vector<Vec2> generateCloudPoints(float width, float height, int bumps = 6) {
        std::vector<Vec2> points;

        for (int i = 0; i <= bumps * 4; ++i) {
            float t = i / static_cast<float>(bumps * 4);
            float angle = t * TWO_PI;

            float rx = width / 2;
            float ry = height / 2;

            float bumpiness = 0.3f;
            float r = 1.0f + bumpiness * std::sin(angle * bumps);

            float x = rx * r * std::cos(angle);
            float y = ry * r * std::sin(angle);

            points.push_back({ x, y });
        }

        return points;
    }

    // Color utility functions - all inline
    static inline Color rainbow(float t) {
        t = std::fmod(t, 1.0f);
        return Color::hsv(t * 360, 1.0f, 1.0f);
    }

    static inline Color temperatureToColor(float kelvin) {
        float temp = kelvin / 100.0f;
        float r, g, b;

        if (temp <= 66) {
            r = 255;
        }
        else {
            r = temp - 60;
            r = 329.698727446f * std::pow(r, -0.1332047592f);
            r = std::max(0.0f, std::min(255.0f, r));
        }

        if (temp <= 66) {
            g = temp;
            g = 99.4708025861f * std::log(g) - 161.1195681661f;
            g = std::max(0.0f, std::min(255.0f, g));
        }
        else {
            g = temp - 60;
            g = 288.1221695283f * std::pow(g, -0.0755148492f);
            g = std::max(0.0f, std::min(255.0f, g));
        }

        if (temp >= 66) {
            b = 255;
        }
        else if (temp <= 19) {
            b = 0;
        }
        else {
            b = temp - 10;
            b = 138.5177312231f * std::log(b) - 305.0447927307f;
            b = std::max(0.0f, std::min(255.0f, b));
        }

        return Color(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    static inline Color plasmaColor(float t) {
        float r = std::sin(t * TWO_PI) * 0.5f + 0.5f;
        float g = std::sin(t * TWO_PI + TWO_PI / 3) * 0.5f + 0.5f;
        float b = std::sin(t * TWO_PI + 2 * TWO_PI / 3) * 0.5f + 0.5f;
        return Color(r, g, b);
    }

    static inline Color fireColor(float t) {
        if (t < 0.2f) {
            float s = t / 0.2f;
            return Color::lerp(Color(1.0f, 1.0f, 1.0f), Color(1.0f, 1.0f, 0.8f), s);
        }
        else if (t < 0.4f) {
            float s = (t - 0.2f) / 0.2f;
            return Color::lerp(Color(1.0f, 1.0f, 0.8f), Color(1.0f, 0.8f, 0.4f), s);
        }
        else if (t < 0.6f) {
            float s = (t - 0.4f) / 0.2f;
            return Color::lerp(Color(1.0f, 0.8f, 0.4f), Color(1.0f, 0.4f, 0.2f), s);
        }
        else if (t < 0.8f) {
            float s = (t - 0.6f) / 0.2f;
            return Color::lerp(Color(1.0f, 0.4f, 0.2f), Color(0.4f, 0.0f, 0.0f), s);
        }
        else {
            float s = (t - 0.8f) / 0.2f;
            return Color::lerp(Color(0.4f, 0.0f, 0.0f), Color(0.0f, 0.0f, 0.0f), s);
        }
    }

    static inline Color iceColor(float t) {
        if (t < 0.33f) {
            float s = t / 0.33f;
            return Color::lerp(Color(1.0f, 1.0f, 1.0f), Color(0.8f, 0.9f, 1.0f), s);
        }
        else if (t < 0.66f) {
            float s = (t - 0.33f) / 0.33f;
            return Color::lerp(Color(0.8f, 0.9f, 1.0f), Color(0.4f, 0.6f, 1.0f), s);
        }
        else {
            float s = (t - 0.66f) / 0.34f;
            return Color::lerp(Color(0.4f, 0.6f, 1.0f), Color(0.1f, 0.2f, 0.4f), s);
        }
    }

    static inline Color poisonColor(float t) {
        if (t < 0.5f) {
            float s = t / 0.5f;
            return Color::lerp(Color(0.4f, 1.0f, 0.2f), Color(0.2f, 0.6f, 0.1f), s);
        }
        else {
            float s = (t - 0.5f) / 0.5f;
            return Color::lerp(Color(0.2f, 0.6f, 0.1f), Color(0.4f, 0.2f, 0.6f), s);
        }
    }

    static inline Color electricColor(float t) {
        if (t < 0.3f) {
            float s = t / 0.3f;
            return Color::lerp(Color(1.0f, 1.0f, 1.0f), Color(0.6f, 0.8f, 1.0f), s);
        }
        else if (t < 0.7f) {
            float s = (t - 0.3f) / 0.4f;
            return Color::lerp(Color(0.6f, 0.8f, 1.0f), Color(0.4f, 0.4f, 1.0f), s);
        }
        else {
            float s = (t - 0.7f) / 0.3f;
            return Color::lerp(Color(0.4f, 0.4f, 1.0f), Color(0.6f, 0.2f, 0.8f), s);
        }
    }

    // Math utility functions - all inline
    static inline float clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }

    static inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    static inline float inverseLerp(float a, float b, float value) {
        return (value - a) / (b - a);
    }

    static inline float remap(float value, float fromMin, float fromMax, float toMin, float toMax) {
        float t = inverseLerp(fromMin, fromMax, value);
        return lerp(toMin, toMax, t);
    }

    static inline float smoothstep(float edge0, float edge1, float x) {
        float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static inline float smootherstep(float edge0, float edge1, float x) {
        float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    static inline float pingPong(float t, float length) {
        t = std::fmod(t, length * 2);
        return length - std::abs(t - length);
    }

    static inline float repeat(float t, float length) {
        return std::fmod(t, length);
    }

    static inline float distance(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }

    static inline float angleBetween(float x1, float y1, float x2, float y2) {
        return std::atan2(y2 - y1, x2 - x1);
    }

    static inline float normalizeAngle(float angle) {
        while (angle > PI) angle -= TWO_PI;
        while (angle < -PI) angle += TWO_PI;
        return angle;
    }

    static inline float angleDifference(float a, float b) {
        float diff = b - a;
        return normalizeAngle(diff);
    }

    static inline float lerpAngle(float a, float b, float t) {
        float diff = angleDifference(a, b);
        return a + diff * t;
    }

    static inline float degToRad(float degrees) {
        return degrees * PI / 180.0f;
    }

private:
    // Helper functions for noise generation - all inline
    static inline float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    static inline float grad(int hash, float x, float y) {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    static inline int fastFloor(float x) {
        return x > 0 ? static_cast<int>(x) : static_cast<int>(x) - 1;
    }

    static inline float hash2D(int x, int y) {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    }

    static inline float contributionSimplex(float x, float y, float z, int i, int j, int k) {
        float t = 0.6f - x * x - y * y - z * z;
        if (t < 0) return 0.0f;

        t *= t;
        int gi = p()[(i + p()[(j + p()[k & 255]) & 255]) & 255] % 12;
        return t * t * dot3D(grad3()[gi], x, y, z);
    }

    static inline float dot3D(const float g[], float x, float y, float z) {
        return g[0] * x + g[1] * y + g[2] * z;
    }

    // Permutation table for noise - as function to avoid multiple definition
    static const std::array<int, 512>& p() {
        static const std::array<int, 512> perm = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
            49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,
            236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
            49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,
            236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };
        return perm;
    }

    // Gradient table for 3D simplex noise
    static const float(&grad3())[12][3]{
        static const float gradients[12][3] = {
            {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
            {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
            {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
        };
        return gradients;
    }
};

// Remove the global initialization function
// No need for initializeUtils() anymore