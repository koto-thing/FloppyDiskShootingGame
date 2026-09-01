#include <cassert>

#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stage5ModelView.h"

/**
 * @brief Stage 5の状態順序と共有モデルTransformを検証する
 * @return なし
 */
void RunStage5ModelViewTests() {
    using Phase = SideScrollingShooter::Stage5Phase;
    using Weakpoint = SideScrollingShooter::TayamaWeakpoint;

    // 正規遷移を許可し、TAYAMA攻略順の飛び越しを拒否する
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::EastsourceBattle, Phase::EastsourceFall));
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaCommandCore, Phase::TayamaCollapse));
    assert(!SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaFireControl, Phase::TayamaCommandCore));
    assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::FireControlRadar, Phase::TayamaFireControl));
    assert(!SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::CommandCore, Phase::TayamaLiftEngines));

    // EASTSOURCEは表示列挙と境界計算が同じ26パーツを使用する
    const Stage5ModelTransform eastsourceTransform {{2.0f, 3.0f, 40.0f}, {}, 0.72f};
    EastsourceModelState eastsourceState;
    int eastsourceParts = 0;
    EastsourceModelView::VisitParts(eastsourceTransform, eastsourceState,
        [&](PrimitiveShape, const Matrix4x4&, const ColorF&, EastsourcePartGroup) {
            ++eastsourceParts;
        });
    assert(eastsourceParts == static_cast<int>(EastsourceModelView::PrimitiveCount));
    assert(EastsourceModelView::GroupBounds(eastsourceTransform, eastsourceState,
        EastsourcePartGroup::Nose).valid);
    eastsourceState.destroyed[static_cast<std::size_t>(EastsourcePartGroup::Nose)] = true;
    assert(!EastsourceModelView::GroupBounds(eastsourceTransform, eastsourceState,
        EastsourcePartGroup::Nose).valid);

    // TAYAMAは同じパーツ群を補間し、崩壊Offsetも同じ境界へ反映する
    const Stage5ModelTransform tayamaTransform {{0.0f, 0.0f, 57.0f}, {}, 1.08f};
    TayamaModelState tayamaState;
    int tayamaParts = 0;
    TayamaModelView::VisitParts(tayamaTransform, 1.0f, tayamaState,
        [&](PrimitiveShape, const Matrix4x4&, const ColorF&, TayamaPartGroup) {
            ++tayamaParts;
        });
    assert(tayamaParts == static_cast<int>(TayamaModelView::PrimitiveCount));
    const Stage5GroupBounds before = TayamaModelView::GroupBounds(tayamaTransform, 1.0f,
        tayamaState, TayamaPartGroup::LeftFlightDeck);
    tayamaState.collapseOffsets[static_cast<std::size_t>(TayamaPartGroup::LeftFlightDeck)] =
        {{-8.0f, -4.0f, 2.0f}, {0.0f, 0.0f, 0.6f}, Vector3::One};
    const Stage5GroupBounds after = TayamaModelView::GroupBounds(tayamaTransform, 1.0f,
        tayamaState, TayamaPartGroup::LeftFlightDeck);
    assert(before.valid && after.valid);
    assert((after.center - before.center).LengthSquared() > 1.0f);
}
