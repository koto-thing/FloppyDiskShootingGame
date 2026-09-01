#pragma once

/** @brief ゲーム設定値 */
struct GameSettings {
    float masterVolume = 1.0f;
    float bgmVolume = 1.0f;
    float seVolume = 1.0f;
};

/** @brief ゲーム設定をユーザーの永続データ領域へ保存するリポジトリ */
class SettingsRepository {
public:
    /**
     * @brief 保存済み設定を取得する
     * @return 保存済み設定、未保存または破損時は既定値
     */
    GameSettings Load() const;

    /**
     * @brief ゲーム設定を保存する
     * @param settings 保存する設定
     */
    void Save(const GameSettings& settings) const;

    /**
     * @brief 設定値を有効範囲へ補正する
     * @param settings 補正前の設定
     * @return 補正後の設定
     */
    static GameSettings Sanitize(GameSettings settings);
};
