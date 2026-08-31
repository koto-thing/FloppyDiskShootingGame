---
name: stage-2d-3d-rendering
description: Build or repair stage backgrounds and bidirectional 2D/3D camera transitions in this DirectX 12 shooting game, including Cube-based terrain, scenery, stars, clouds, and grids.
---

# Stage 2D/3D Rendering Skill

ステージ背景、2D/3D表示、双方向カメラ遷移を変更するときに使う

主な対象は `Presentation/Gameplay/SideScrollingShooter.cpp` の `Render2D`、`Render3D`、`ConfigureSideCamera`、`ConfigureRailCamera`、`RailBlend`、座標系初期化処理

## 先に確認すること

描画を直す前に、次の流れを往復とも追う

1. `TickViewTransition` が切り替え先と補間率を決める
2. `InitializeRailObjects` または `InitializeSideObjects` がゲーム座標を変換する
3. 遷移中は `Render3D` が描画する
4. 遷移終了後の2Dは `Render2D` が描画する

2D→3Dだけでなく3D→2Dも確認する

片方向だけ自然な場合、補間式そのものより、遷移描画の終点と通常描画の開始点が一致していない可能性が高い

## 2D表示の不変条件

- 安定した2D表示では、敵、弾、アイテム、演出を用途別の固定Zへ置き、ゲーム上の奥行きを画面へ持ち込まない
- 2D中まで奥行き座標からXを逆変換しない。通常の2D座標をそのまま使う
- 2D背景は正面カメラの視錐台全体を覆う
- UIはカメラをリセットしてから描画する

## 双方向遷移の不変条件

遷移中に補間する背景要素は、`railWeight == 0`で`Render2D`と完全に同じになる必要がある

一致させる項目:

- プリミティブ数
- 位置
- 大きさ
- 奥行き
- 色と透明度
- スクロール式

遷移終了時に別の本数や配置へ交換しない。最後の1フレームの交換は、カメラが滑らかでも明確な跳びとして見える

敵や弾を触る場合は、切り替え開始時に保存した`transitionSideX`と`transitionSideY`の意味を往復で確認する

## 大きなCubeを3Dへ展開する方法

2Dの縦グリッド線など、細長いCubeをそのまま回転・奥行き展開すると、途中で厚い壁として画面を塞ぐ

一度に全成分を線形補間せず、二段階に分ける

1. 前半で線を床位置まで下げ、高さを薄く潰す
2. 後半でX/Z座標を移動し、床方向の奥行きと幅を伸ばす

3D→2Dでは同じ補間率を逆向きに使い、床線を縮めてから縦線を持ち上げる

`SmoothStep(Clamp01(weight * 2))`と`SmoothStep(Clamp01(weight * 2 - 1))`のように、下降と座標展開で別の重みを持つと実装を増やさずに済む

## Cubeの接地計算

`ObjectShader.hlsl`のCubeは各軸`-0.5`から`0.5`で定義されている

高さ`h`のCubeを地面上面`groundTopY`へ接地する中心Y:

```text
centerY = groundTopY + h * 0.5
```

地面の上面を動かさず下方向だけへ厚くする場合:

```text
groundCenterY = groundTopY - groundHeight * 0.5
```

見た目だけで値を足し引きせず、地面上面とオブジェクト底面を式で一致させる

## 背景を画面全体へ広げる

`WorldXScale`と`WorldYScale`はゲーム座標用であり、背景全面の寸法には使わない

透視カメラの距離、FOV、アスペクト比から背景面の半サイズを求める

```text
halfHeight = distance * tan(fov * 0.5)
halfWidth = halfHeight * aspectRatio
```

端の欠けを防ぐため少し余白を持たせる

- 砂地は左右の視錐台外まで伸ばす
- 2D地面は上面を維持し、下端の外まで厚みを伸ばす
- 3D地面は手前、左右、遠方の視錐台外まで覆う
- 星と雲はこの表示範囲を基準に配置する

## 星、雲、サボテンの配置

プロシージャル配置は決定的にし、実行ごとの乱数は使わない

剰余配置では乗数と法の最大公約数に注意する。共通因子があると、星が数本の縦列や中央へ固まる

- XとYで異なる互いに素な乗数を使う
- 複数の高さ帯と奥行き帯へ分散する
- 2Dでは全要素を意図した同一Zへ置く
- 3Dのサボテンは地面の横幅と奥行き全体へ分散し、すべて地面上面へ接地する
- 昼の雲と夜の星は同じ空領域を使い、昼夜補間率でフェードする
- 雲、星、サボテンは依頼どおりCubeの組み合わせを優先する

## 検証

最低限、次を確認する

1. 安定した2D
2. 2D→3Dの開始、中間、終了
3. 安定した3D
4. 3D→2Dの開始、中間、終了
5. Stage1の星とグリッドが画面全体を覆う
6. Stage2の地面に空の隙間がなく、全サボテンが接地する
7. 昼の雲、夜の星、昼夜の途中

動画が提供された場合は一定間隔でフレーム抽出し、最後の1フレームの交換、前景を横切る背景物、巨大Cubeによる遮蔽を探す

最後にDebug x64ビルド、既存テスト、`git diff --check`を実行する
