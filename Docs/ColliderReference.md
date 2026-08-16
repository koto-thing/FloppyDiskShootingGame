# コライダー API リファレンス

このドキュメントでは、本プロジェクトの2D衝突判定機能の使い方と公開APIを説明します。

## 概要

コライダーは `GameObject` に追加する `Component` です。衝突判定は X/Y 平面上で行われ、Z座標は使用しません。

対応する形状は次の2種類です。

| クラス | 形状 | 主な用途 |
|---|---|---|
| `CircleCollider` | 円 | 弾、円形のキャラクター |
| `AABBCollider` | 軸平行境界ボックス | 壁、矩形のキャラクター |

対応する組み合わせは、円同士、AABB同士、円とAABBです。境界が接している場合も衝突として扱われます。

## 必要なヘッダー

```cpp
#include "Domain/Entities/CircleCollider.h"
#include "Domain/Entities/AABBCollider.h"
#include "Domain/Interfaces/ICollisionReceiver.h"
#include "Domain/ValueObjects/CollisionLayer.h"
#include "Infrastructure/Services/CollisionService.h"
```

## 基本的な使用例

```cpp
CollisionService collisionService;

auto player = std::make_shared<GameObject>();
auto playerCollider = player->AddComponent<CircleCollider>(16.0f);
playerCollider->SetLayer(CollisionLayer::PLAYER);
playerCollider->SetCollisionMask(
    static_cast<std::uint32_t>(CollisionLayer::ENEMY_SHOT)
);
collisionService.RegisterCollider(playerCollider);

auto shot = std::make_shared<GameObject>();
auto shotCollider = shot->AddComponent<CircleCollider>(4.0f);
shotCollider->SetLayer(CollisionLayer::ENEMY_SHOT);
shotCollider->SetCollisionMask(
    static_cast<std::uint32_t>(CollisionLayer::PLAYER)
);
collisionService.RegisterCollider(shotCollider);

// GameObject の位置更新後に、毎フレーム1回呼び出す
collisionService.UpdateService();
```

判定対象になるには、両方のコライダーが相手のレイヤーを衝突マスクに含める必要があります。上の例では、プレイヤーが `ENEMY_SHOT` を、敵弾が `PLAYER` を許可しています。

## `Collider`

すべてのコライダーの基底クラスです。通常は直接生成せず、`CircleCollider` または `AABBCollider` を使用します。

### `ColliderType GetColliderType() const`

形状の種類を返します。

| 戻り値 | 意味 |
|---|---|
| `ColliderType::CIRCLE` | 円形コライダー |
| `ColliderType::AABB` | AABBコライダー |

### `void SetLayer(CollisionLayer layer)`

コライダー自身が所属するレイヤーを設定します。初期値は `CollisionLayer::NONE` です。

```cpp
collider->SetLayer(CollisionLayer::PLAYER);
```

### `CollisionLayer GetLayer() const`

現在の所属レイヤーを返します。

### `void SetCollisionMask(std::uint32_t mask)`

衝突を許可する相手レイヤーをビットマスクで設定します。初期値は `0` で、そのままではどのコライダーとも衝突しません。

```cpp
const auto targets = CollisionLayer::ENEMY | CollisionLayer::ENEMY_SHOT;
collider->SetCollisionMask(static_cast<std::uint32_t>(targets));
```

衝突条件は次のとおりです。

```text
(Aのマスク & Bのレイヤー) != 0
かつ
(Bのマスク & Aのレイヤー) != 0
```

### `std::uint32_t GetCollisionMask() const`

現在の衝突マスクを返します。

### `void SetEnabled(bool enabled)`

衝突判定の有効／無効を設定します。初期値は `true` です。`false` のコライダーは `CollisionService::UpdateService()` で無視されます。

```cpp
collider->SetEnabled(false);
```

### `bool IsEnabled() const`

コライダーが有効なら `true` を返します。

### `void SetOffset(const DirectX::XMFLOAT2& offset)`

親 `GameObject` の位置からコライダー中心をずらします。初期値は `(0, 0)` です。オフセットはスケールや回転の影響を受けません。

```cpp
collider->SetOffset({ 0.0f, 12.0f });
```

### `DirectX::XMFLOAT2 GetWorldPosition() const`

`GameObject` の X/Y 座標にオフセットを加えたコライダー中心を返します。親が設定されていない場合はオフセットだけを返します。

## `CircleCollider`

### コンストラクター

```cpp
explicit CircleCollider(float radius);
```

ローカル半径を指定します。

```cpp
auto collider = gameObject->AddComponent<CircleCollider>(8.0f);
```

### `void SetRadius(float radius)`

ローカル半径を変更します。

### `float GetRadius() const`

スケール適用前のローカル半径を返します。

### `float GetWorldRadius() const`

判定に使用するワールド半径を返します。計算式は次のとおりです。

```text
ワールド半径 = ローカル半径 × max(abs(scale.x), abs(scale.y))
```

X/Yで異なるスケールを設定しても楕円にはならず、大きい方の倍率を使った円として判定されます。

## `AABBCollider`

### コンストラクター

```cpp
explicit AABBCollider(const DirectX::XMFLOAT2& halfSize);
```

中心から各辺までの距離、つまり幅と高さの半分を指定します。`{ 20.0f, 10.0f }` は幅40、高さ20の矩形です。

```cpp
auto collider = gameObject->AddComponent<AABBCollider>(
    DirectX::XMFLOAT2{ 20.0f, 10.0f }
);
```

### `void SetHalfSize(const DirectX::XMFLOAT2& halfSize)`

ローカル半サイズを変更します。

### `DirectX::XMFLOAT2 GetWorldHalfSize() const`

親 `GameObject` のX/Yスケールを反映した半サイズを返します。

```text
worldHalfSize.x = halfSize.x × abs(scale.x)
worldHalfSize.y = halfSize.y × abs(scale.y)
```

AABBは常に座標軸に平行です。`GameObject` を回転してもコライダーは回転しません。

## `CollisionLayer`

定義済みのレイヤーは次のとおりです。

| 値 | ビット | 用途 |
|---|---:|---|
| `NONE` | `0` | 未設定 |
| `PLAYER` | `1 << 0` | プレイヤー |
| `PLAYER_SHOT` | `1 << 1` | プレイヤーの弾 |
| `ENEMY` | `1 << 2` | 敵 |
| `ENEMY_SHOT` | `1 << 3` | 敵の弾 |
| `WALL` | `1 << 4` | 壁 |

複数レイヤーは `operator|` で結合できます。

```cpp
const auto mask = CollisionLayer::ENEMY | CollisionLayer::WALL;
collider->SetCollisionMask(static_cast<std::uint32_t>(mask));
```

## `CollisionService`

コライダーの登録、解除、および総当たりの衝突判定を管理します。

### `void RegisterCollider(const std::shared_ptr<Collider>& collider)`

判定対象を登録します。サービス内では `weak_ptr` として保持されるため、所有権は移りません。同じコライダーを複数回登録しないでください。

### `void UnregisterCollider(const Collider* collider)`

指定したコライダーと、すでに破棄された登録を一覧から削除します。

```cpp
collisionService.UnregisterCollider(collider.get());
```

`shared_ptr` が破棄されたコライダーは次回更新時にも自動的に一覧から除去されます。

### `void UpdateService()`

登録済みの有効なコライダーを1組ずつ判定し、衝突した両方の `GameObject` に通知します。通常は各オブジェクトの移動処理が終わった後、毎フレーム1回呼び出します。

登録数を N としたとき、判定候補の走査量は O(N²) です。

### `void Clear()`

すべての登録を解除します。シーン終了時やシーン切り替え時に使用します。

## 衝突イベント

衝突通知を受けるコンポーネントは、`Component` と `ICollisionReceiver` の両方を継承します。

```cpp
class PlayerDamageReceiver final
    : public Component,
      public ICollisionReceiver {
public:
    void OnCollisionEnter(Collider& self, Collider& other) override {
        if (other.GetLayer() == CollisionLayer::ENEMY_SHOT) {
            // ダメージ処理
            other.SetEnabled(false);
        }
    }
};

player->AddComponent<PlayerDamageReceiver>();
```

通知先は、コライダー自身ではなく、コライダーと同じ `GameObject` に追加されたすべての `ICollisionReceiver` 実装コンポーネントです。

### `OnCollisionEnter(Collider& self, Collider& other)`

`self` は通知を受ける側のコライダー、`other` は相手側のコライダーです。

現在の `CollisionService` は、重なっているすべてのフレームで `OnCollisionEnter` を呼び出します。接触開始時の1回だけに限定する接触履歴は保持していません。

### `OnCollisionStay(Collider& self, Collider& other)`

インターフェースには定義されていますが、現在の `CollisionService` からは呼び出されません。

### `OnCollisionExit(Collider& self, Collider& other)`

インターフェースには定義されていますが、現在の `CollisionService` からは呼び出されません。

## ライフサイクルの推奨順序

```cpp
// シーン初期化
auto collider = object->AddComponent<CircleCollider>(8.0f);
collider->SetLayer(CollisionLayer::PLAYER_SHOT);
collider->SetCollisionMask(
    static_cast<std::uint32_t>(CollisionLayer::ENEMY)
);
collisionService.RegisterCollider(collider);

// 毎フレーム
object->UpdateObject();
collisionService.UpdateService();

// 個別に取り除く場合
collisionService.UnregisterCollider(collider.get());

// シーン終了
collisionService.Clear();
```

## 注意事項

- 判定は2DのX/Y平面だけを使用し、Z座標を無視します。
- `GameObject` の回転は、どのコライダー形状にも反映されません。
- レイヤーとマスクは初期状態では衝突しない値です。両方を明示的に設定してください。
- `CircleCollider` の非等方スケールは楕円ではなく、最大スケールの円になります。
- `AABBCollider` のコンストラクター引数は全体サイズではなく半サイズです。
- コライダーはサービスに自動登録されません。`RegisterCollider()` を明示的に呼び出してください。
- 同じコライダーを重複登録すると通知も重複する可能性があります。
- 衝突応答（押し戻し、速度変更、オブジェクト破棄など）は自動では行われません。イベント受信側で実装してください。
- コールバック中にコライダーを無効化しても、そのフレームですでに走査される別の組み合わせには即座に反映されない場合があります。
