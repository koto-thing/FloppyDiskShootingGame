#include "GamePresenter.h"

int GamePresenter::PrepareRenderData(const GameState& state, ConstantBufferData* cbvCpuData) {
    int cbOffset = 0;

    auto FormatObject = [&](const GameObject& obj, const float color[4]) {
        if (!obj.active) return;

        float ndcX = (obj.pos.x / 800.0f) * 2.0f - 1.0f;
        float ndcY = 1.0f - (obj.pos.y / 600.0f) * 2.0f;
        float sizeX = obj.size / 800.0f;
        float sizeY = obj.size / 600.0f;

        ConstantBufferData& cbData = cbvCpuData[cbOffset];
        cbData.position[0] = ndcX;
        cbData.position[1] = ndcY;
        cbData.size[0] = sizeX;
        cbData.size[1] = sizeY;
        cbData.color[0] = color[0];
        cbData.color[1] = color[1];
        cbData.color[2] = color[2];
        cbData.color[3] = color[3];

        cbOffset++;
    };

    // 1. Player (Green)
    float playerColor[4] = { 0.0f, 1.0f, 0.4f, 1.0f };
    FormatObject(state.player, playerColor);

    // 2. Bullets (Yellow)
    float bulletColor[4] = { 1.0f, 0.9f, 0.0f, 1.0f };
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (state.playerBullets[i].active) {
            FormatObject(state.playerBullets[i], bulletColor);
        }
    }

    // 3. Enemies (Pink-Red)
    float enemyColor[4] = { 1.0f, 0.2f, 0.4f, 1.0f };
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (state.enemies[i].active) {
            FormatObject(state.enemies[i], enemyColor);
        }
    }

    return cbOffset;
}
