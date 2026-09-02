#pragma once

#include <algorithm>
#include <cmath>

#include "../Common/BossModelTransform.h"
#include "../../../../Engine/Math/Math.h"

/** @brief Stage3ボスの可動・破壊可能部位種別 */
enum class Stage3BossPartType {
    TopMachineGun,
    GondolaMachineGun,
    HeavyCannon,
    MissilePod,
    FunnelPod
};

/** @brief Stage3ボス武装のモデルローカル取付情報 */
struct Stage3BossWeaponMount {
    Vector3 localPosition;
    Vector3 localRotation;
    Stage3BossPartType type;
    int index;
};

/** @brief 超重装飛行戦艦のプロシージャル描画 */
class Stage3BossModelView final {
public:
    static constexpr int TopGunCount = 6;
    static constexpr int Phase1TurretsPerSection = 2;
    static constexpr int Phase1SectionCount = TopGunCount / Phase1TurretsPerSection;
    static constexpr int GondolaMachineGunCount = 6;
    static constexpr int HeavyCannonCount = 2;
    static constexpr int MissilePodCount = 2;
    static constexpr int FunnelPodCount = 3;
    static constexpr int StaticBodyPrimitiveCount = 30;
    static constexpr int GondolaBodyPrimitiveCount = 32;
    static constexpr int WeaponPrimitiveCount = 73;
    static constexpr int PrimitiveCount =
        StaticBodyPrimitiveCount + GondolaBodyPrimitiveCount + WeaponPrimitiveCount;

    /**
     * @brief Phase1の区画と枠番号から艦尾順の上部砲台番号を取得する
     * @param section 艦尾を0とする区画番号
     * @param slot 区画内の砲台番号
     * @return 描画モデル上の上部砲台番号
     */
    static constexpr int Phase1TopGunIndex(int section, int slot) {
        return TopGunCount - 1 -
            (section * Phase1TurretsPerSection + slot);
    }

    /**
     * @brief Phase1の区画と枠番号からボス部位格納番号を取得する
     * @param section 艦尾を0とする区画番号
     * @param slot 区画内の砲台番号
     * @return bossPartHp配列へ使用する番号
     */
    static constexpr int Phase1PartIndex(int section, int slot) {
        return section * Phase1TurretsPerSection + slot;
    }

    /**
     * @brief 上部砲台番号からPhase1部位格納番号を取得する
     * @param topGunIndex モデル上の上部砲台番号
     * @return bossPartHp配列へ使用する番号
     */
    static constexpr int Phase1PartIndexForTopGun(int topGunIndex) {
        return TopGunCount - 1 - topGunIndex;
    }

    /**
     * @brief 指定区画の上部砲台がすべて破壊済みか判定する
     * @param section 艦尾を0とする区画番号
     * @param hp ボス部位HP配列
     * @return 区画内の全砲台HPが0以下ならtrue
     */
    template<class HpArray>
    static constexpr bool IsPhase1SectionDestroyed(int section, const HpArray& hp) {
        return hp[Phase1PartIndex(section, 0)] <= 0 &&
            hp[Phase1PartIndex(section, 1)] <= 0;
    }

    /**
     * @brief 上部機銃の取付情報を取得する
     * @param index 0以上TopGunCount未満の部位番号
     * @return 指定した上部機銃の取付情報
     */
    static const Stage3BossWeaponMount& TopGunMount(int index) {
        return TopGunMounts[index];
    }

    /**
     * @brief 上部砲台のワールド取付位置を取得する
     * @param index 上部砲台番号
     * @param transform 親Transform
     * @return 砲台旋回支点のワールド座標
     */
    static Vector3 TopGunWorldPosition(int index, const BossModelTransform& transform) {
        return TransformLocalPosition(transform, TopGunMount(index).localPosition);
    }

    /**
     * @brief 上部砲台を目標へ向けるローカル回転を取得する
     * @param index 上部砲台番号
     * @param transform 親Transform
     * @param targetWorldPosition 照準するワールド座標
     * @return XをPitch、YをYawとするローカル回転
     */
    static Vector3 TopGunAimRotation(int index, const BossModelTransform& transform,
        const Vector3& targetWorldPosition) {
        const Vector3 mount = TopGunWorldPosition(index, transform);
        const float dx = targetWorldPosition.x - mount.x;
        const float dy = targetWorldPosition.y - mount.y;
        const float dz = targetWorldPosition.z - mount.z;
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const float localX = dx * cosine - dz * sine;
        const float localZ = dx * sine + dz * cosine;
        const float horizontal = (std::max)(0.001f,
            std::sqrt(localX * localX + localZ * localZ));
        return {
            (std::clamp)(-std::atan2(dy, horizontal),
                -Math::ToRadians(55.0f), Math::ToRadians(35.0f)),
            std::atan2(localZ, -localX),
            0.0f
        };
    }

    /**
     * @brief ゴンドラ機銃の取付情報を取得する
     * @param index 0以上GondolaMachineGunCount未満の部位番号
     * @return 指定したゴンドラ機銃の取付情報
     */
    static const Stage3BossWeaponMount& GondolaMachineGunMount(int index) {
        return GondolaMachineGunMounts[index];
    }

    /**
     * @brief 大口径砲の取付情報を取得する
     * @param index 0以上HeavyCannonCount未満の部位番号
     * @return 指定した大口径砲の取付情報
     */
    static const Stage3BossWeaponMount& HeavyCannonMount(int index) {
        return HeavyCannonMounts[index];
    }

    /**
     * @brief 大口径砲の砲身先端ワールド座標を取得する
     * @param index 0以上HeavyCannonCount未満の部位番号
     * @param transform 親Transform
     * @param localRotation XをPitch、YをYawとする追加回転
     * @return 砲身中心線上の先端ワールド座標
     */
    static Vector3 HeavyCannonMuzzleWorldPosition(int index,
        const BossModelTransform& transform, const Vector3& localRotation) {
        return TransformLocalPosition(transform,
            MountedLocalPosition(HeavyCannonMount(index), localRotation,
                {-2.63f, -0.48f, 0.0f}));
    }

    /**
     * @brief ミサイルポッドの取付情報を取得する
     * @param index 0以上MissilePodCount未満の部位番号
     * @return 指定したミサイルポッドの取付情報
     */
    static const Stage3BossWeaponMount& MissilePodMount(int index) {
        return MissilePodMounts[index];
    }

    /**
     * @brief ファンネル射出ポッドの取付情報を取得する
     * @param index 0以上FunnelPodCount未満の部位番号
     * @return 指定したファンネル射出ポッドの取付情報
     */
    static const Stage3BossWeaponMount& FunnelPodMount(int index) {
        return FunnelPodMounts[index];
    }

    /**
     * @brief ファンネル生成に使う射出口のローカル位置を取得する
     * @param index 0以上FunnelPodCount未満の部位番号
     * @return モデル原点基準の射出口位置
     */
    static Vector3 FunnelLaunchLocalPosition(int index) {
        const Stage3BossWeaponMount& mount = FunnelPodMount(index);
        return mount.localPosition + Vector3 {-0.90f, 0.0f, 0.0f};
    }

    /**
     * @brief 固定船体を描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawStaticBody(const BossModelTransform& transform, DrawPart&& drawPart) {
        // 長い中央胴体へ大小の箱型胴体を重ねて重厚な区画感を作る
        Part(transform, drawPart, 1, {0.0f, 0.0f, 0.0f}, {13.8f, 5.8f, 5.9f}, Hull);
        Part(transform, drawPart, 1, {-6.15f, 0.0f, 0.0f}, {4.8f, 4.8f, 4.9f}, Hull);
        Part(transform, drawPart, 1, {6.10f, -0.02f, 0.0f}, {4.6f, 4.5f, 4.6f}, Hull);
        Part(transform, drawPart, 1, {-8.20f, -0.05f, 0.0f}, {2.9f, 3.5f, 3.6f}, Armor);
        Part(transform, drawPart, 1, {8.05f, -0.08f, 0.0f}, {2.7f, 3.1f, 3.2f}, Armor);

        // 艦首を丸く尖らせ、艦尾は細い尾端へ長く絞る
        Part(transform, drawPart, 5, {-9.65f, -0.05f, 0.0f}, {1.85f, 2.25f, 2.35f}, LightArmor);
        Part(transform, drawPart, 4, {-8.85f, -0.08f, 0.0f}, {2.15f, 2.85f, 2.95f}, Armor);
        Part(transform, drawPart, 3, {9.55f, -0.10f, 0.0f}, {2.8f, 2.0f, 2.1f}, Dark);

        // 船体の太さへ追従する帯状装甲で滑らかな外形を崩さず区画を示す
        constexpr Vector3 ArmorBands[] = {
            {-5.8f, 5.05f, 5.15f}, {-2.9f, 5.75f, 5.85f}, {0.0f, 5.95f, 6.05f},
            {2.9f, 5.70f, 5.80f}, {5.8f, 4.85f, 4.95f}
        };
        for (const Vector3& band : ArmorBands) {
            Part(transform, drawPart, 2, {band.x, 0.0f, 0.0f}, {0.24f, band.y, band.z}, Armor);
        }
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 4; ++i) {
                const float x = -4.8f + static_cast<float>(i) * 3.2f;
                const float surface = i == 0 || i == 3 ? 2.55f : 2.88f;
                Part(transform, drawPart, 1, {x, 0.10f, side * surface},
                    {2.45f, 0.90f, 0.16f}, i % 2 == 0 ? Armor : LightArmor);
            }
        }

        // 艦尾上側は砲台の視認性を優先し、下側と左右の安定翼だけを残す
        Part(transform, drawPart, 4, {8.05f, -2.38f, 0.0f}, {1.90f, 1.45f, 0.32f}, Armor);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, 4, {7.85f, -0.05f, side * 3.00f}, {2.05f, 0.34f, 1.75f}, Armor);
            Part(transform, drawPart, 1, {5.9f, -2.30f, side * 1.58f}, {1.45f, 0.20f, 0.28f}, Dark);
        }

        // 固定警告灯を前後へ置いて巨大な輪郭の端を読みやすくする
        Part(transform, drawPart, 5, {-8.7f, 0.9f, -1.55f}, {0.34f, 0.34f, 0.34f}, Warning);
        Part(transform, drawPart, 5, {-8.7f, 0.9f, 1.55f}, {0.34f, 0.34f, 0.34f}, Warning);
        Part(transform, drawPart, 5, {7.5f, 1.0f, -1.45f}, {0.28f, 0.28f, 0.28f}, Warning);
        Part(transform, drawPart, 5, {7.5f, 1.0f, 1.45f}, {0.28f, 0.28f, 0.28f}, Warning);
    }

    /**
     * @brief 撃破演出の損傷段階に応じて上部船体メッシュを描画する
     * @param transform 親Transform
     * @param damageStage 0を無傷、1を中央破孔、2以上を全損とする損傷段階
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawDamagedStaticBody(const BossModelTransform& transform,
        int damageStage, DrawPart&& drawPart) {
        if (damageStage >= 2) return;

        // 一回目の突進後は中央Primitive群を除外して背景まで抜ける大穴を作る
        auto DrawRemainingPart = [&](int shape, const Vector3& position, const Vector3& scale,
            const float color[4], float yaw, float pitch) {
            const float cosine = std::cos(transform.yaw);
            const float sine = std::sin(transform.yaw);
            const Vector3 offset = position - transform.position;
            const float localX = transform.scale > Math::Epsilon ?
                (offset.x * cosine - offset.z * sine) / transform.scale : 0.0f;
            if (damageStage == 1 && std::abs(localX) < 3.25f) return;
            drawPart(shape, position, scale, color, yaw, pitch);
        };
        DrawStaticBody(transform, DrawRemainingPart);
    }

    /**
     * @brief 武装を含まないゴンドラ外装を描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawGondolaBody(const BossModelTransform& transform, DrawPart&& drawPart) {
        // 吊り下げ支柱で主船体と長大なゴンドラを接続する
        for (float x : {-5.5f, -3.3f, -1.1f, 1.1f, 3.3f, 5.5f}) {
            Part(transform, drawPart, 1, {x, -3.05f, 0.0f}, {0.32f, 2.35f, 0.42f}, Dark);
        }

        // 前後で高さが変わる装甲区画を連結して戦艦型の下部構造を作る
        Part(transform, drawPart, 4, {-6.6f, -4.25f, 0.0f}, {2.8f, 1.65f, 3.5f}, Armor);
        Part(transform, drawPart, 1, {-4.2f, -4.18f, 0.0f}, {2.6f, 1.85f, 3.7f}, Hull);
        Part(transform, drawPart, 1, {-1.4f, -4.10f, 0.0f}, {2.8f, 2.0f, 3.9f}, Hull);
        Part(transform, drawPart, 1, {1.5f, -4.05f, 0.0f}, {2.8f, 2.1f, 3.9f}, Hull);
        Part(transform, drawPart, 1, {4.3f, -4.12f, 0.0f}, {2.7f, 1.9f, 3.7f}, Hull);
        Part(transform, drawPart, 4, {6.6f, -4.20f, 0.0f}, {2.5f, 1.65f, 3.4f}, Armor);

        // 艦橋と暗い窓列を側面に独立した固定装飾として追加する
        Part(transform, drawPart, 4, {0.8f, -2.85f, 0.0f}, {3.0f, 1.0f, 2.5f}, LightArmor);
        Part(transform, drawPart, 1, {0.8f, -2.38f, 0.0f}, {1.7f, 0.34f, 2.15f}, Armor);
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 4; ++i) {
                Part(transform, drawPart, 1,
                    {-0.35f + static_cast<float>(i) * 0.76f, -2.40f, side * 1.10f},
                    {0.48f, 0.20f, 0.10f}, Window);
            }
        }

        // 大きな側面装甲パネルで武装区画のまとまりを示す
        for (float side : {-1.0f, 1.0f}) {
            for (int i = 0; i < 5; ++i) {
                Part(transform, drawPart, 1,
                    {-5.3f + static_cast<float>(i) * 2.65f, -4.15f, side * 1.94f},
                    {1.75f, 1.05f, 0.18f}, i % 2 == 0 ? LightArmor : Armor);
            }
        }
    }

    /**
     * @brief 上部機銃を個別に描画する
     * @param index 部位番号
     * @param transform 親Transform
     * @param localRotation XをPitch、YをYawとする追加回転
     * @param active 現在攻略中の砲台ならtrue
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawTopGun(int index, const BossModelTransform& transform,
        const Vector3& localRotation, bool active, DrawPart&& drawPart) {
        // 黒い取付基部は船体へ固定し、上部装甲と連装砲身だけを照準へ追従させる
        const Stage3BossWeaponMount& mount = TopGunMount(index);
        Part(transform, drawPart, 2, mount.localPosition, {0.72f, 0.32f, 0.72f}, Dark,
            mount.localRotation.y, mount.localRotation.x);
        MountedPart(transform, drawPart, mount, localRotation, 4, {-0.12f, 0.32f, 0.0f}, {0.88f, 0.55f, 0.82f}, active ? Active : Armor);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-0.82f, 0.38f, -0.20f}, {1.25f, 0.15f, 0.15f}, Metal);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-0.82f, 0.38f, 0.20f}, {1.25f, 0.15f, 0.15f}, Metal);
    }

    /**
     * @brief ゴンドラ機銃を個別に描画する
     * @param index 部位番号
     * @param transform 親Transform
     * @param localRotation XをPitch、YをYawとする追加回転
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawGondolaMachineGun(int index, const BossModelTransform& transform,
        const Vector3& localRotation, DrawPart&& drawPart) {
        // 小型旋回砲塔と連装砲身を独立して描画する
        const Stage3BossWeaponMount& mount = GondolaMachineGunMount(index);
        MountedPart(transform, drawPart, mount, localRotation, 2, {0.0f, 0.0f, 0.0f}, {0.64f, 0.30f, 0.64f}, Dark);
        MountedPart(transform, drawPart, mount, localRotation, 1, {-0.08f, -0.30f, 0.0f}, {0.80f, 0.48f, 0.72f}, Armor);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-0.78f, -0.36f, -0.17f}, {1.15f, 0.14f, 0.14f}, Metal);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-0.78f, -0.36f, 0.17f}, {1.15f, 0.14f, 0.14f}, Metal);
    }

    /**
     * @brief ゴンドラ大口径砲を個別に描画する
     * @param index 部位番号
     * @param transform 親Transform
     * @param localRotation XをPitch、YをYawとする追加回転
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawHeavyCannon(int index, const BossModelTransform& transform,
        const Vector3& localRotation, DrawPart&& drawPart) {
        // 大型砲塔本体と後退可能な砲身を別Primitiveにする
        const Stage3BossWeaponMount& mount = HeavyCannonMount(index);
        MountedPart(transform, drawPart, mount, localRotation, 2, {0.0f, 0.0f, 0.0f}, {1.20f, 0.42f, 1.20f}, Dark);
        MountedPart(transform, drawPart, mount, localRotation, 4, {-0.12f, -0.42f, 0.0f}, {1.45f, 0.82f, 1.25f}, Armor);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-1.35f, -0.48f, 0.0f}, {2.30f, 0.34f, 0.34f}, Metal);
        MountedPart(transform, drawPart, mount, localRotation, 2, {-2.45f, -0.48f, 0.0f}, {0.36f, 0.46f, 0.46f}, Dark);
    }

    /**
     * @brief ミサイルポッドを個別に描画する
     * @param index 部位番号
     * @param transform 親Transform
     * @param openAmount 扉の開度
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMissilePod(int index, const BossModelTransform& transform,
        float openAmount, DrawPart&& drawPart) {
        // 本体、発射口、扉を分けて将来の開閉状態へ接続可能にする
        const Stage3BossWeaponMount& mount = MissilePodMount(index);
        const float amount = Math::Clamp01(openAmount);
        MountedPart(transform, drawPart, mount, {}, 1, {0.0f, 0.0f, 0.0f}, {1.65f, 1.00f, 1.35f}, Armor);
        for (float z : {-0.38f, 0.38f}) {
            MountedPart(transform, drawPart, mount, {}, 2, {-0.84f, -0.20f, z}, {0.16f, 0.32f, 0.32f}, Dark);
        }
        MountedPart(transform, drawPart, mount, {amount * -Math::ToRadians(70.0f), 0.0f, 0.0f},
            1, {-0.88f, 0.42f, 0.0f}, {0.14f, 0.76f, 1.28f}, LightArmor);
    }

    /**
     * @brief ファンネル射出ポッドを個別に描画する
     * @param index 部位番号
     * @param transform 親Transform
     * @param openAmount 0を閉、1を全開とする扉開度
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawFunnelPod(int index, const BossModelTransform& transform,
        float openAmount, DrawPart&& drawPart) {
        // PodBody、PodInterior、PodDoorを独立Primitiveとして描画する
        const Stage3BossWeaponMount& mount = FunnelPodMount(index);
        const float amount = Math::Clamp01(openAmount);
        MountedPart(transform, drawPart, mount, {}, 1, {0.0f, 0.0f, 0.0f}, {1.75f, 1.15f, 1.45f}, Armor);
        MountedPart(transform, drawPart, mount, {}, 1, {-0.89f, 0.0f, 0.0f}, {0.12f, 0.80f, 1.05f}, Warning);
        MountedPart(transform, drawPart, mount, {amount * -Math::ToRadians(95.0f), 0.0f, 0.0f},
            1, {-0.96f, 0.54f, 0.0f}, {0.14f, 0.95f, 1.35f}, LightArmor);
    }

private:
    inline static constexpr float Hull[4] = {0.20f, 0.21f, 0.18f, 1.0f};
    inline static constexpr float Armor[4] = {0.14f, 0.15f, 0.14f, 1.0f};
    inline static constexpr float LightArmor[4] = {0.29f, 0.30f, 0.27f, 1.0f};
    inline static constexpr float Dark[4] = {0.055f, 0.060f, 0.055f, 1.0f};
    inline static constexpr float Metal[4] = {0.35f, 0.34f, 0.30f, 1.0f};
    inline static constexpr float Window[4] = {0.03f, 0.08f, 0.10f, 1.0f};
    inline static constexpr float Warning[4] = {0.65f, 0.22f, 0.05f, 1.0f};
    inline static constexpr float Active[4] = {1.00f, 0.30f, 0.04f, 1.0f};

    inline static constexpr Stage3BossWeaponMount TopGunMounts[TopGunCount] = {
        {{-6.8f, 3.06f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 0},
        {{-4.2f, 3.06f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 1},
        {{-1.4f, 3.06f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 2},
        {{1.4f, 3.06f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 3},
        {{4.2f, 3.06f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 4},
        {{8.8f, 1.63f, 0.0f}, {}, Stage3BossPartType::TopMachineGun, 5}
    };
    inline static constexpr Stage3BossWeaponMount GondolaMachineGunMounts[GondolaMachineGunCount] = {
        {{-6.2f, -5.08f, -1.20f}, {}, Stage3BossPartType::GondolaMachineGun, 0},
        {{-3.8f, -5.18f, 1.20f}, {}, Stage3BossPartType::GondolaMachineGun, 1},
        {{-1.3f, -5.22f, -1.25f}, {}, Stage3BossPartType::GondolaMachineGun, 2},
        {{1.4f, -5.22f, 1.25f}, {}, Stage3BossPartType::GondolaMachineGun, 3},
        {{3.9f, -5.16f, -1.20f}, {}, Stage3BossPartType::GondolaMachineGun, 4},
        {{6.1f, -5.04f, 1.20f}, {}, Stage3BossPartType::GondolaMachineGun, 5}
    };
    inline static constexpr Stage3BossWeaponMount HeavyCannonMounts[HeavyCannonCount] = {
        {{-2.7f, -5.30f, 0.0f}, {}, Stage3BossPartType::HeavyCannon, 0},
        {{2.8f, -5.30f, 0.0f}, {}, Stage3BossPartType::HeavyCannon, 1}
    };
    inline static constexpr Stage3BossWeaponMount MissilePodMounts[MissilePodCount] = {
        {{-5.1f, -4.05f, -2.05f}, {}, Stage3BossPartType::MissilePod, 0},
        {{-5.1f, -4.05f, 2.05f}, {}, Stage3BossPartType::MissilePod, 1}
    };
    inline static constexpr Stage3BossWeaponMount FunnelPodMounts[FunnelPodCount] = {
        {{0.0f, -4.00f, -2.08f}, {}, Stage3BossPartType::FunnelPod, 0},
        {{2.7f, -4.05f, 2.08f}, {}, Stage3BossPartType::FunnelPod, 1},
        {{5.2f, -4.10f, -2.00f}, {}, Stage3BossPartType::FunnelPod, 2}
    };

    /**
     * @brief モデルローカル位置を親Transformへ合成する
     * @param transform 親Transform
     * @param localPosition モデルローカル座標
     * @return 合成後のワールド座標
     */
    static Vector3 TransformLocalPosition(
        const BossModelTransform& transform, const Vector3& localPosition) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        return {
            transform.position.x + (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z + (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
    }

    /**
     * @brief 武装支点基準の位置へPitchとYawを適用する
     * @param mount 武装取付情報
     * @param localRotation XをPitch、YをYawとする追加回転
     * @param partPosition 武装支点基準の座標
     * @return モデル原点基準の回転後座標
     */
    static Vector3 MountedLocalPosition(const Stage3BossWeaponMount& mount,
        const Vector3& localRotation, const Vector3& partPosition) {
        const float pitch = mount.localRotation.x + localRotation.x;
        const float yaw = mount.localRotation.y + localRotation.y;
        const float pitchCosine = std::cos(pitch);
        const float pitchSine = std::sin(pitch);
        const Vector3 pitched {
            partPosition.x * pitchCosine - partPosition.y * pitchSine,
            partPosition.x * pitchSine + partPosition.y * pitchCosine,
            partPosition.z
        };
        const float yawCosine = std::cos(yaw);
        const float yawSine = std::sin(yaw);
        const Vector3 rotated {
            pitched.x * yawCosine + pitched.z * yawSine,
            pitched.y,
            -pitched.x * yawSine + pitched.z * yawCosine
        };
        return mount.localPosition + rotated;
    }

    /**
     * @brief モデルローカル部品を親Transformへ合成して描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param shape PrimitiveShape番号
     * @param localPosition モデルローカル座標
     * @param scale ローカル寸法
     * @param color RGBA色
     * @param localYaw 親Yawへ加算するYaw
     * @param localPitch 親へ加算するPitch
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, int shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4],
        float localYaw = 0.0f, float localPitch = 0.0f) {
        // Stage2と同じ親Yaw合成でローカル座標をワールドへ変換する
        const Vector3 position = TransformLocalPosition(transform, localPosition);
        drawPart(shape, position, scale * transform.scale, color,
            transform.yaw + localYaw, localPitch);
    }

    /**
     * @brief 武装支点からの相対部品をYawとPitchへ追従させて描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param mount 武装取付情報
     * @param localRotation XをPitch、YをYawとする追加回転
     * @param shape PrimitiveShape番号
     * @param partPosition 武装支点基準の座標
     * @param scale ローカル寸法
     * @param color RGBA色
     * @return なし
     */
    template<class DrawPart>
    static void MountedPart(const BossModelTransform& transform, DrawPart& drawPart,
        const Stage3BossWeaponMount& mount, const Vector3& localRotation, int shape,
        const Vector3& partPosition, const Vector3& scale, const float color[4]) {
        // PitchとYawを支点周りの部品位置にも反映する
        const float pitch = mount.localRotation.x + localRotation.x;
        const float yaw = mount.localRotation.y + localRotation.y;
        Part(transform, drawPart, shape,
            MountedLocalPosition(mount, localRotation, partPosition), scale, color, yaw, pitch);
    }
};

static_assert(Stage3BossModelView::PrimitiveCount == 135);
