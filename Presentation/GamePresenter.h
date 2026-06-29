#pragma once
#include "ConstantBufferData.h"
#include "../Domain/GameState.h"

class GamePresenter {
public:
    static int PrepareRenderData(const GameState& state, ConstantBufferData* cbvCpuData);
};
