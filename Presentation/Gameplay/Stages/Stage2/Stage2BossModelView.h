#pragma once

#include <cmath>

#include "../Common/BossModelTransform.h"
#include "../../../../Engine/Math/Math.h"

/** @brief Stage2ボスの破壊済み部位を描画へ反映する */
struct BossModelDamageState {
    bool mainGun = true;
    bool secondaryGuns[3] = {true, true, true};
    bool connectionCore = true;
    bool connectionCoreExposed = false;
    bool mainGunHit = false;
    bool secondaryGunsHit[3] = {};
    bool connectionCoreHit = false;
    bool funnelHatches[12] = {true, true, true, true, true, true, true, true, true, true, true, true};
    bool funnelHatchesHit[12] = {};
};

/** @brief 砂中潜航艦ユニットのプロシージャル描画 */
class SandSubmarineView {
public:
    static constexpr int PrimitiveCount = 64;

    /**
     * @brief 砂中潜航艦をローカル部品の組み合わせで描画する
     * @param transform 潜航艦ユニットの親Transform
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param damage 破壊済み接続コアの描画状態
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, DrawPart&& drawPart,
        const BossModelDamageState& damage = {}) {
        constexpr float Hull[] = {0.20f, 0.24f, 0.19f, 1.0f};
        constexpr float Armor[] = {0.12f, 0.14f, 0.11f, 1.0f};
        constexpr float LightArmor[] = {0.27f, 0.29f, 0.23f, 1.0f};
        constexpr float Dark[] = {0.055f, 0.065f, 0.055f, 1.0f};
        constexpr float Core[] = {0.95f, 0.28f, 0.055f, 1.0f};
        constexpr float Hit[] = {1.0f, 0.03f, 0.03f, 1.0f};

        // 中央を長い楕円、前後を小さめの丸い部品でつないでカプセル状の主船体を作る
        DrawLocal(transform, drawPart, 5, {0.0f, 0.0f, 0.0f}, {8.8f, 2.05f, 3.15f}, Hull);
        DrawLocal(transform, drawPart, 5, {-4.25f, 0.00f, 0.0f}, {2.65f, 1.95f, 3.00f}, Hull);
        DrawLocal(transform, drawPart, 5, {4.25f, 0.00f, 0.0f}, {2.65f, 1.90f, 2.95f}, Hull);

        // 側面の長い装甲帯で潜水艦らしい厚みと水平ラインを作る
        for (float side : {-1.0f, 1.0f}) {
            DrawLocal(transform, drawPart, 1, {-2.65f, 0.05f, side * 1.54f}, {3.25f, 0.72f, 0.26f}, Armor);
            DrawLocal(transform, drawPart, 1, {1.10f, 0.05f, side * 1.56f}, {3.10f, 0.72f, 0.26f}, Armor);
        }

        // 艦首は垂直な壁を残さず、段階的に細くなる積層装甲でまとめる
        DrawLocal(transform, drawPart, 4, {-5.05f, 0.10f, 0.0f}, {1.70f, 1.65f, 2.55f}, Armor);
        DrawLocal(transform, drawPart, 4, {-5.62f, 0.14f, 0.0f}, {0.95f, 1.30f, 1.95f}, LightArmor);
        DrawLocal(transform, drawPart, 3, {-6.08f, 0.10f, 0.0f}, {0.72f, 0.82f, 0.82f}, Dark);

        // 側面ファンノズルを左右に6基ずつ配置し、ユーザー案の四角い開口部を再現する
        int hatchIndex = 0;
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 6; ++i) {
                const float x = -2.65f + static_cast<float>(i) * 1.05f;
                if (damage.funnelHatches[hatchIndex]) {
                    const float* color = damage.funnelHatchesHit[hatchIndex] ? Hit : Core;
                    DrawLocal(transform, drawPart, 1, {x, -0.22f, side * 1.68f}, {0.62f, 0.58f, 0.20f}, Dark);
                    DrawLocal(transform, drawPart, 1, {x, -0.22f, side * 1.80f}, {0.38f, 0.34f, 0.08f}, color);
                }
                ++hatchIndex;
            }
        }

        // 下部の掘削フィンは中央を大きく、前後を少し小さくして船体の丸みに追従させる
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 8; ++i) {
                const float x = -3.65f + static_cast<float>(i) * 1.05f;
                const float edge = std::fabs(static_cast<float>(i) - 3.5f) / 3.5f;
                const float finHeight = 0.98f - edge * 0.24f;
                DrawLocal(transform, drawPart, 4, {x, -1.48f + edge * 0.10f, side * 1.30f},
                    {0.56f, finHeight, 0.36f}, i % 2 == 0 ? Armor : Dark);
            }
        }

        // 艦尾中央に大型の砂中推進ファン/ドリルを配置する
        DrawLocal(transform, drawPart, 2, {5.12f, -0.06f, 0.0f}, {1.35f, 1.35f, 1.35f}, Armor);
        DrawLocal(transform, drawPart, 2, {5.78f, -0.06f, 0.0f}, {0.48f, 1.10f, 1.10f}, Dark);
        DrawLocal(transform, drawPart, 3, {6.15f, -0.06f, 0.0f}, {0.80f, 0.92f, 0.92f}, LightArmor);
        DrawLocal(transform, drawPart, 2, {5.88f, -0.06f, 0.0f}, {0.18f, 0.66f, 0.66f}, Core);

        // 全武装の破壊前は接続装甲でコアを覆い、破壊後だけ発光コアを露出する
        if (!damage.connectionCoreExposed) {
            DrawLocal(transform, drawPart, 1, {0.45f, 1.22f, 0.0f}, {5.00f, 0.30f, 2.25f}, Dark);
            for (float side : {-1.0f, 1.0f}) {
                DrawLocal(transform, drawPart, 1, {-1.65f, 1.50f, side * 0.84f}, {0.58f, 0.50f, 0.30f}, Armor);
                DrawLocal(transform, drawPart, 1, {2.00f, 1.50f, side * 0.84f}, {0.58f, 0.50f, 0.30f}, Armor);
            }
        } else if (damage.connectionCore) {
            DrawLocal(transform, drawPart, 2, {0.25f, 1.42f, 0.0f}, {1.35f, 0.22f, 1.35f},
                damage.connectionCoreHit ? Hit : Core);
        }

        // 上面のサービスハッチで巨大な一枚物に見えるのを避ける
        for (float x : {-3.15f, -1.05f, 1.05f, 3.15f}) {
            DrawLocal(transform, drawPart, 1, {x, 1.12f, 0.0f}, {0.72f, 0.16f, 1.15f}, LightArmor);
        }
    }

private:
    /**
     * @brief ローカル部品を親Transformへ合成して描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param shape 既存PrimitiveShapeに対応する番号
     * @param localPosition ローカル座標
     * @param scale ローカル寸法
     * @param color 部品色
     * @param localYaw 親Yawへ加算する部品固有のYaw
     * @return なし
     */
    template<class DrawPart>
    static void DrawLocal(const BossModelTransform& transform, DrawPart& drawPart, int shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4], float localYaw = 0.0f) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 position {
            transform.position.x + (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z + (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
        drawPart(shape, position, scale * transform.scale, color, transform.yaw + localYaw, 0.0f);
    }

    friend class LandBattleshipView;
};

/** @brief 陸上戦艦ユニットのプロシージャル描画 */
class LandBattleshipView {
public:
    static constexpr int PrimitiveCount = 53;

    /**
     * @brief 陸上戦艦をローカル部品の組み合わせで描画する
     * @param transform 陸上戦艦ユニットの親Transform
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param damage 破壊済み武装の描画状態
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, DrawPart&& drawPart,
        const BossModelDamageState& damage = {}) {
        constexpr float Hull[] = {0.28f, 0.24f, 0.17f, 1.0f};
        constexpr float Armor[] = {0.36f, 0.31f, 0.22f, 1.0f};
        constexpr float LightArmor[] = {0.43f, 0.38f, 0.28f, 1.0f};
        constexpr float Metal[] = {0.48f, 0.45f, 0.36f, 1.0f};
        constexpr float Dark[] = {0.07f, 0.065f, 0.055f, 1.0f};
        constexpr float Sensor[] = {0.72f, 0.22f, 0.08f, 1.0f};
        constexpr float Hit[] = {1.0f, 0.03f, 0.03f, 1.0f};

        // 前方を低く、中央を厚く、艦橋を最高点、後方を再び低くする段階的な上部船体
        Part(transform, drawPart, 4, {-3.10f, 0.38f, 0.0f}, {2.00f, 0.72f, 2.25f}, Armor);
        Part(transform, drawPart, 4, {-1.65f, 0.70f, 0.0f}, {2.70f, 1.08f, 2.55f}, Hull);
        Part(transform, drawPart, 1, {0.20f, 0.95f, 0.0f}, {2.65f, 1.30f, 2.70f}, Hull);
        Part(transform, drawPart, 4, {1.90f, 0.88f, 0.0f}, {2.25f, 1.12f, 2.40f}, Armor);
        Part(transform, drawPart, 4, {3.40f, 0.55f, 0.0f}, {1.45f, 0.78f, 2.05f}, Hull);
        Part(transform, drawPart, 1, {0.30f, 1.62f, 0.0f}, {2.20f, 0.58f, 1.72f}, Armor);

        // 前部と肩周りに斜面装甲を足して箱の横並び感を消す
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, 4, {-2.70f, 1.05f, side * 0.92f}, {1.55f, 0.72f, 0.55f}, LightArmor, side * 0.10f);
            Part(transform, drawPart, 4, {1.65f, 1.42f, side * 1.04f}, {1.50f, 0.60f, 0.46f}, Armor, side * -0.08f);
        }

        // 主砲は破壊後に砲塔ごと消し、攻撃停止を輪郭でも示す
        if (damage.mainGun) {
            const float* color = damage.mainGunHit ? Hit : Dark;
            Part(transform, drawPart, 2, {-1.55f, 2.03f, 0.0f}, {1.75f, 1.10f, 1.75f}, color);
            GunPart(transform, drawPart, transform.mainGunTracksTarget, transform.aimTarget, {-1.55f, 2.08f, 0.0f}, 4, 0.75f, {1.85f, 1.08f, 1.55f}, damage.mainGunHit ? Hit : Armor);
            GunPart(transform, drawPart, transform.mainGunTracksTarget, transform.aimTarget, {-1.55f, 2.08f, 0.0f}, 1, 1.27f, {0.82f, 0.82f, 1.22f}, damage.mainGunHit ? Hit : LightArmor);
            GunPart(transform, drawPart, transform.mainGunTracksTarget, transform.aimTarget, {-1.55f, 2.08f, 0.0f}, 2, 2.70f, {2.95f, 0.54f, 0.54f}, damage.mainGunHit ? Hit : Metal);
            GunPart(transform, drawPart, transform.mainGunTracksTarget, transform.aimTarget, {-1.55f, 2.08f, 0.0f}, 2, 4.17f, {0.58f, 0.72f, 0.72f}, color);
            GunPart(transform, drawPart, transform.mainGunTracksTarget, transform.aimTarget, {-1.55f, 2.08f, 0.0f}, 2, 4.43f, {0.16f, 0.58f, 0.58f}, damage.mainGunHit ? Hit : Sensor);
        }

        // 三基の副砲を独立描画し、破壊した砲だけ輪郭から取り除く
        constexpr Vector3 SecondaryGunPositions[] = {
            {-0.55f, 2.34f, -0.92f}, {0.65f, 2.34f, 0.0f}, {2.85f, 1.92f, 0.92f}
        };
        for (int i = 0; i < 3; ++i) {
            if (!damage.secondaryGuns[i]) continue;
            const Vector3& position = SecondaryGunPositions[i];
            const float* darkColor = damage.secondaryGunsHit[i] ? Hit : Dark;
            Part(transform, drawPart, 2, position, {0.76f, 0.46f, 0.76f}, darkColor);
            const Vector3 pivot {position.x, position.y + 0.26f, position.z};
            GunPart(transform, drawPart, transform.secondaryGunsTrackTarget, transform.secondaryAimTarget, pivot, 1, 0.20f, {0.82f, 0.46f, 0.64f}, damage.secondaryGunsHit[i] ? Hit : Armor);
            GunPart(transform, drawPart, transform.secondaryGunsTrackTarget, transform.secondaryAimTarget, pivot, 2, 1.10f, {1.35f, 0.20f, 0.20f}, damage.secondaryGunsHit[i] ? Hit : Metal);
        }

        // 艦橋は複数段の装甲塊として積み上げ、上部シルエットの最高点にする
        Part(transform, drawPart, 4, {1.45f, 2.08f, 0.0f}, {1.45f, 0.78f, 1.55f}, Armor);
        Part(transform, drawPart, 1, {1.58f, 2.62f, 0.0f}, {1.15f, 0.64f, 1.18f}, Hull);
        Part(transform, drawPart, 4, {1.62f, 3.10f, 0.0f}, {0.92f, 0.50f, 0.94f}, LightArmor);
        Part(transform, drawPart, 1, {1.62f, 3.48f, 0.0f}, {0.72f, 0.30f, 0.72f}, Dark);
        Part(transform, drawPart, 2, {1.62f, 3.90f, 0.0f}, {0.12f, 0.82f, 0.12f}, Metal);
        Part(transform, drawPart, 5, {1.62f, 4.32f, 0.0f}, {0.25f, 0.25f, 0.25f}, Sensor);

        // 後部は艦橋から段階的に低くし、最後は潜航艦へ自然につながる
        Part(transform, drawPart, 4, {2.65f, 1.62f, 0.0f}, {1.45f, 0.76f, 1.78f}, Armor);
        Part(transform, drawPart, 1, {3.55f, 1.22f, 0.0f}, {1.20f, 0.58f, 1.90f}, Hull);
        Part(transform, drawPart, 4, {4.20f, 0.86f, 0.0f}, {0.82f, 0.52f, 1.65f}, Armor);
        Part(transform, drawPart, 1, {4.55f, 0.48f, 0.0f}, {0.42f, 0.36f, 1.48f}, Dark);

        // 側面装甲パネルで細部を増やしつつ、輪郭を壊さない程度のごつごつ感を追加する
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 6; ++i) {
                const float x = -2.35f + static_cast<float>(i) * 1.05f;
                const float y = 0.98f + (i == 2 || i == 3 ? 0.22f : 0.0f);
                Part(transform, drawPart, 1, {x, y, side * 1.38f}, {0.72f, 0.50f, 0.16f},
                    i % 2 == 0 ? LightArmor : Armor);
            }
        }

        // 後方の排気塔と短いセンサーで機械的な密度を補う
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, 2, {2.85f, 2.12f, side * 0.72f}, {0.38f, 1.02f, 0.38f}, Dark);
            Part(transform, drawPart, 2, {3.48f, 1.78f, side * 0.78f}, {0.46f, 0.72f, 0.46f}, Dark);
            Part(transform, drawPart, 2, {2.25f, 2.46f, side * 0.92f}, {0.22f, 0.44f, 0.22f}, Sensor);
        }
    }

private:
    /**
     * @brief 砲塔基部を支点に照準先へ向けた部品を描画する
     * @param transform 親Transformと照準先
     * @param drawPart 描画関数
     * @param tracksTarget 照準先へ向ける場合true
     * @param aimTarget 照準先
     * @param localPivot 砲塔基部のローカル座標
     * @param shape 既存PrimitiveShapeに対応する番号
     * @param distance 基部から砲口方向への距離
     * @param scale ローカル寸法
     * @param color 部品色
     * @return なし
     */
    template<class DrawPart>
    static void GunPart(const BossModelTransform& transform, DrawPart& drawPart, bool tracksTarget,
        const Vector3& aimTarget,
        const Vector3& localPivot, int shape, float distance, const Vector3& scale, const float color[4]) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 pivot {
            transform.position.x + (localPivot.x * cosine + localPivot.z * sine) * transform.scale,
            transform.position.y + localPivot.y * transform.scale,
            transform.position.z + (-localPivot.x * sine + localPivot.z * cosine) * transform.scale
        };
        const Vector3 delta = tracksTarget ? aimTarget - pivot : Vector3 {-cosine, 0.0f, sine};
        const float horizontal = (std::max)(0.001f, std::sqrt(delta.x * delta.x + delta.z * delta.z));
        const float length = (std::max)(0.001f, std::sqrt(horizontal * horizontal + delta.y * delta.y));
        const float gunYaw = std::atan2(delta.z, -delta.x);
        const float gunPitch = -std::asin(delta.y / length);
        const Vector3 position = pivot + delta / length * (distance * transform.scale);
        drawPart(shape, position, scale * transform.scale, color, gunYaw, gunPitch);
    }

    /**
     * @brief ローカル部品を共通の親Transform合成処理へ渡す
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param shape 既存PrimitiveShapeに対応する番号
     * @param localPosition ローカル座標
     * @param scale ローカル寸法
     * @param color 部品色
     * @param localYaw 親Yawへ加算する部品固有のYaw
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, int shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4], float localYaw = 0.0f) {
        SandSubmarineView::DrawLocal(transform, drawPart, shape, localPosition, scale, color, localYaw);
    }
};

static_assert(SandSubmarineView::PrimitiveCount + LandBattleshipView::PrimitiveCount == 117);
