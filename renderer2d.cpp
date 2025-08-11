// renderer2d.cpp
#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Draw {
    SDL_Renderer* renderer;

    Draw(SDL_Renderer* ren = nullptr) : renderer(ren) {
        if (renderer) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }
    }

    void set_renderer(SDL_Renderer* ren) {
        renderer = ren;
        if (renderer) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }
    }

    void color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
    }

    void clear() {
        SDL_RenderClear(renderer);
    }

    void present() {
        SDL_RenderPresent(renderer);
    }

    void point(float x, float y) {
        SDL_RenderPoint(renderer, x, y);
    }

    void points(const SDL_FPoint* pts, int count) {
        SDL_RenderPoints(renderer, pts, count);
    }

    void points(const std::vector<SDL_FPoint>& pts) {
        SDL_RenderPoints(renderer, pts.data(), static_cast<int>(pts.size()));
    }

    void line(float x1, float y1, float x2, float y2) {
        SDL_RenderLine(renderer, x1, y1, x2, y2);
    }

    void lines(const SDL_FPoint* pts, int count) {
        SDL_RenderLines(renderer, pts, count);
    }

    void lines(const std::vector<SDL_FPoint>& pts) {
        SDL_RenderLines(renderer, pts.data(), static_cast<int>(pts.size()));
    }

    void polygon(const std::vector<SDL_FPoint>& pts) {
        if (pts.size() < 2) return;
        SDL_RenderLines(renderer, pts.data(), static_cast<int>(pts.size()));
        SDL_RenderLine(renderer, pts.back().x, pts.back().y, pts.front().x, pts.front().y);
    }

    void rect(float x, float y, float w, float h) {
        SDL_FRect r{ x, y, w, h };
        SDL_RenderRect(renderer, &r);
    }

    void rects(const SDL_FRect* rects, int count) {
        SDL_RenderRects(renderer, rects, count);
    }

    void fill_rect(float x, float y, float w, float h) {
        SDL_FRect r{ x, y, w, h };
        SDL_RenderFillRect(renderer, &r);
    }

    void fill_rects(const SDL_FRect* rects, int count) {
        SDL_RenderFillRects(renderer, rects, count);
    }

    void circle(int cx, int cy, int radius) {
        if (radius <= 0) return;

        std::vector<SDL_FPoint> pts;
        pts.reserve(radius * 8);

        int x = radius, y = 0;
        int err = 1 - radius;

        while (x >= y) {
            pts.push_back({ static_cast<float>(cx + x), static_cast<float>(cy + y) });
            pts.push_back({ static_cast<float>(cx + y), static_cast<float>(cy + x) });
            pts.push_back({ static_cast<float>(cx - y), static_cast<float>(cy + x) });
            pts.push_back({ static_cast<float>(cx - x), static_cast<float>(cy + y) });
            pts.push_back({ static_cast<float>(cx - x), static_cast<float>(cy - y) });
            pts.push_back({ static_cast<float>(cx - y), static_cast<float>(cy - x) });
            pts.push_back({ static_cast<float>(cx + y), static_cast<float>(cy - x) });
            pts.push_back({ static_cast<float>(cx + x), static_cast<float>(cy - y) });

            ++y;
            if (err <= 0) {
                err += 2 * y + 1;
            }
            else {
                --x;
                err += 2 * (y - x) + 1;
            }
        }

        SDL_RenderPoints(renderer, pts.data(), static_cast<int>(pts.size()));
    }

    void fill_circle(int cx, int cy, int radius) {
        if (radius <= 0) return;

        int rsquared = radius * radius;
        for (int dy = -radius; dy <= radius; ++dy) {
            int dx = static_cast<int>(std::sqrt(rsquared - dy * dy));
            SDL_RenderLine(renderer,
                static_cast<float>(cx - dx), static_cast<float>(cy + dy),
                static_cast<float>(cx + dx), static_cast<float>(cy + dy));
        }
    }

    void ellipse(int cx, int cy, int rx, int ry, int segments = 64) {
        if (segments < 8) segments = 8;

        std::vector<SDL_FPoint> pts;
        pts.reserve(segments + 1);

        float step = (2.0f * 3.14159265f) / segments;
        for (int i = 0; i <= segments; ++i) {
            float angle = i * step;
            pts.push_back({
                cx + rx * std::cos(angle),
                cy + ry * std::sin(angle)
                });
        }

        SDL_RenderLines(renderer, pts.data(), static_cast<int>(pts.size()));
    }

    void fill_polygon(const std::vector<SDL_FPoint>& pts, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
        if (pts.size() < 3) return;

        std::vector<SDL_Vertex> verts;
        verts.reserve(pts.size());

        SDL_FColor color{ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
        for (const auto& p : pts) {
            verts.push_back({ p, color, {0, 0} });
        }

        std::vector<int> indices;
        indices.reserve((pts.size() - 2) * 3);

        for (size_t i = 1; i < pts.size() - 1; ++i) {
            indices.push_back(0);
            indices.push_back(static_cast<int>(i));
            indices.push_back(static_cast<int>(i + 1));
        }

        SDL_RenderGeometry(renderer, nullptr,
            verts.data(), static_cast<int>(verts.size()),
            indices.data(), static_cast<int>(indices.size()));
    }
};