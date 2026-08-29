#include "ModeSelectionStateController.h"

ModeSelectionStateController::ModeSelectionStateController() {
    
}

void ModeSelectionStateController::Initialize(D3D12RenderingService& renderer) {
    
}

void ModeSelectionStateController::Tick() {
    switch (GetCurrentState()) {
        case DifficultySelect:
            // Handle difficulty selection logic
            break;
        case PlayerTypeSelect:
            // Handle player type selection logic
            break;
    }
}