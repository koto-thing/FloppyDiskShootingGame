#pragma once
#include "GameObject.h"

const int MAX_BULLETS = 100;
const int MAX_ENEMIES = 20;

struct GameState {
    GameObject player;
    GameObject playerBullets[MAX_BULLETS];
    GameObject enemies[MAX_ENEMIES];
    int score;
};
