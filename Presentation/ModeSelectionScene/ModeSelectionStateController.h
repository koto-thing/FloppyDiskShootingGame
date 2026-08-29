#pragma once

#include "../../Domain/Entities/Component.h"

enum ModeSelectionState {
    DifficultySelect,
    PlayerTypeSelect,
};

class ModeSelectionStateController : public Component {
public:
    ModeSelectionStateController();
    ~ModeSelectionStateController() override = default;
    
    void Initialize(D3D12RenderingService& renderer) override;
    void Tick() override;
    
private:
    ModeSelectionState GetCurrentState() const;
};
