#pragma once

// 自動生成ファイル: 直接編集せず EnemyScripts.json を Tools/StageEditor.html で編集すること
// CommandとDefinitionの定義は SideScrollingShooterEnemies.h にある

namespace EnemyScript {
    inline constexpr Command SampleWaveCommands[] = {
        {MoveTo, 90, 1.3f, 0.4f, 0.0f}, // MoveTo
        {FireFan, 0, 3.0f, 30.0f, 0.018f}, // FireFan
        {Move, 40, 0.0f, 0.0f, 0.0f}, // Wait
        {Repeat, 0, 1.0f, 2.0f, 0.0f}, // Repeat
        {Sine, 240, -0.012f, 0.3f, 120.0f}, // Sine
        {FireRing, 0, 8.0f, 0.012f, 0.0f}, // FireRing
        {Move, 0, -0.02f, 0.0f, 0.0f}, // Move
    };

    /** @brief スクリプト番号順の行動スクリプト一覧 */
    inline constexpr Definition Scripts[] = {
        {SampleWaveCommands, 7, 12, 200},
    };
    inline constexpr int ScriptCount = 1;
}
