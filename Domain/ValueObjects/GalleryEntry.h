#pragma once

#include <cstdint>

/** @brief ギャラリーで展示するモデルの固定識別子 */
enum class GalleryEntry : std::uint32_t {
    Player,
    LightEnemy,
    HeavyEnemy,
    ArmoredEnemy,
    Stage1Boss,
    Stage2Boss,
    Stage3Boss,
    Stage3BarrierFunnel,
    Stage3ReflectFunnel,
    Stage4Boss,
    Eastsource,
    Tayama,
    Stage1Enemy,
    Stage2Enemy,
    Stage3Enemy,
    Stage4Enemy,
    Stage4WeaponDrone,
    WallSecurityDrone,
    NeoAizuBuildings,
    Count
};

/**
 * @brief 展示識別子に対応する解放ビットを取得する
 * @param entry 展示識別子
 * @return 展示識別子に対応する1ビット
 */
constexpr std::uint32_t GalleryEntryBit(GalleryEntry entry) {
    return 1u << static_cast<std::uint32_t>(entry);
}

/** @brief 初回起動時から解放する展示ビット */
// ponytail: 検証用展示は初期解放し、進行連動展示が必要になったら出現時Unlockへ移行する
inline constexpr std::uint32_t DefaultGalleryUnlocks = GalleryEntryBit(GalleryEntry::Player) |
    GalleryEntryBit(GalleryEntry::WallSecurityDrone) |
    GalleryEntryBit(GalleryEntry::NeoAizuBuildings);

/** @brief 保存データで受け付ける展示ビット */
inline constexpr std::uint32_t ValidGalleryUnlocks =
    (1u << static_cast<std::uint32_t>(GalleryEntry::Count)) - 1u;

static_assert(static_cast<std::uint32_t>(GalleryEntry::Count) == 19u);
static_assert((DefaultGalleryUnlocks & GalleryEntryBit(GalleryEntry::Player)) != 0u);
static_assert((DefaultGalleryUnlocks & GalleryEntryBit(GalleryEntry::WallSecurityDrone)) != 0u);
static_assert((DefaultGalleryUnlocks & GalleryEntryBit(GalleryEntry::NeoAizuBuildings)) != 0u);
