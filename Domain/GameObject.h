#pragma once

struct Vector2 {
    float x;
    float y;
};

struct GameObject {
    bool active;
    Vector2 pos;
    Vector2 velocity;
    float size;
};
