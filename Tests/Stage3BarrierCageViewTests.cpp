#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage3/Stage3BarrierCageView.h"

/**
 * @brief Stage3封鎖バリアの面分離、開度、スクロールを検証する
 * @return なし
 */
void RunStage3BarrierCageViewTests() {
    // 全面表示時の固定部品数とPLAY BALLドット数を確認する
    int primitiveCount = 0;
    auto countPart = [&](int, const Vector3&, const Vector3&, const float[4], float, float) {
        ++primitiveCount;
    };
    Stage3BarrierCageView::Draw({}, {}, countPart);
    assert(primitiveCount == Stage3BarrierCageView::FramePrimitiveCount +
        Stage3BarrierCageView::FieldPrimitiveCount +
        Stage3BarrierCageView::MarqueeStripPrimitiveCount +
        Stage3BarrierCageView::MarqueeDotPrimitiveCount);

    // 開度0では描画せずFrontのみなら1面分だけ描画する
    Stage3BarrierCagePose hidden;
    hidden.openAmount = 0.0f;
    primitiveCount = 0;
    Stage3BarrierCageView::Draw({}, hidden, countPart);
    assert(primitiveCount == 0);

    Stage3BarrierCagePose frontOnly;
    frontOnly.visibleFaces = Stage3BarrierCageView::FaceBit(Stage3BarrierFace::Front);
    primitiveCount = 0;
    Stage3BarrierCageView::Draw({}, frontOnly, countPart);
    assert(primitiveCount == 5 + 1 + Stage3BarrierCageView::MarqueeDotPrimitiveCountPerFace);

    // 半展開時は上端を固定したまま高さだけ半分になることを確認する
    Stage3BarrierCagePose halfOpen;
    halfOpen.openAmount = 0.5f;
    halfOpen.visibleFaces = Stage3BarrierCageView::FaceBit(Stage3BarrierFace::Front);
    Vector3 fieldPosition {};
    Vector3 fieldScale {};
    bool firstPrimitive = true;
    Stage3BarrierCageView::Draw({}, halfOpen,
        [&](int, const Vector3& position, const Vector3& scale, const float[4], float, float) {
            if (!firstPrimitive) return;
            firstPrimitive = false;
            fieldPosition = position;
            fieldScale = scale;
        });
    assert(std::fabs(fieldScale.y - Stage3BarrierCageView::BarrierHeight * 0.5f) < 0.0001f);
    assert(std::fabs(fieldPosition.y -
        (Stage3BarrierCageView::BarrierTopY - fieldScale.y * 0.5f)) < 0.0001f);

    // スクロール値がドット列を実際に移動させることを確認する
    auto firstDotPosition = [](float scrollOffset) {
        Stage3BarrierCagePose pose;
        pose.scrollOffset = scrollOffset;
        Vector3 result {};
        int drawIndex = 0;
        Stage3BarrierCageView::DrawFace(Stage3BarrierFace::Front, {}, pose,
            [&](int, const Vector3& position, const Vector3&, const float[4], float, float) {
                if (drawIndex++ == 6) result = position;
            });
        return result;
    };
    assert(std::fabs(firstDotPosition(0.0f).z - firstDotPosition(1.0f).z) > 0.0001f);
}
