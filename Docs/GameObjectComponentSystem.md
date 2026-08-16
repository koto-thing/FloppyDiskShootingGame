# ゲームオブジェクト＆コンポーネントシステム仕様書

本書は、`FloppyDiskShootingGame` プロジェクトに導入された **Unity 風コンポーネント指向ゲームオブジェクトシステム** の設計コンセプト、主要クラス、および具体的な使い方について解説します。

---

## 💡 1. 設計コンセプト

1.44MBの容量制約（極小サイズ制限）をクリアしつつ、拡張性の高い本格的な 3D/2D ゲームを構築するため、**「Unityライクなコンポーネント指向（Component Pattern）」** を採用しています。

* **データの局所化と Transform 管理の統一**:  
  `GameObject` 自体は「位置・回転・スケール（Transform）」および「所属するコンポーネント」のみを保持し、自律的な描画やロジックの実装はすべて `Component` 派生クラスに委ねます。
* **ランタイム（実行時）での動的アタッチ**:  
  ゲーム実行中に、オブジェクトの状態変化（パワーアップ、状態異常、一時エフェクトなど）に合わせて、随時コンポーネントを **動的（ランタイム）に追加・取得** することができます。
* **描画リソースの競合防止と効率化**:  
  各 `MeshRenderer` コンポーネントが自身専用の 256 バイトの定数バッファ（Upload Heap）を Map 管理するため、同一フレームで大量のオブジェクトを安全かつ同時に描画することができます。

---

## 🛠️ 2. 主要クラス構成

### ① GameObject ([GameObject.h](file:///D:/PandD/FloppyDiskShootingGame/Domain/Entities/GameObject.h))
エンティティの「器」です。Transform（SRT行列）の管理と、コンポーネントの追加（`AddComponent`）および取得（`GetComponent`）を行います。

### ② Component ([Component.h](file:///D:/PandD/FloppyDiskShootingGame/Domain/Entities/Component.h))
すべてのコンポーネントの抽象基底クラスです。
* `Initialize(renderer)`: コンポーネントの初期化（定数バッファやシェーダーリソースの生成）
* `Update()`: 毎フレームの更新ロジック
* `Render(...)`: 毎フレームの描画処理

### ③ Mesh ([Mesh.h](file:///D:/PandD/FloppyDiskShootingGame/Domain/Entities/Mesh.h))
頂点バッファレス（頂点ID描画）仕様に準拠した、形状タイプ（`shapeType`）と頂点数（`vertexCount`）のみを保持する極小のメッシュデータ表現です。

### ④ MeshRenderer ([MeshRenderer.h](file:///D:/PandD/FloppyDiskShootingGame/Domain/Entities/MeshRenderer.h))
`Component` 派生クラスの実装例です。指定されたメッシュと色情報、および親 GameObject の Transform を使って WVP 行列を合成し、DirectX 12 での描画命令を実行します。

---

## 🎮 3. 基本的な使い方

### ① 基本的なオブジェクトの構築と描画
```cpp
// 1. レンダラーを取得
D3D12RenderingService& renderer = ...;

// 2. GameObject の作成
auto myObj = std::make_shared<GameObject>();

// 3. GameObject の初期化 (レンダラーへのポインタを保持)
myObj->Initialize(renderer);

// 4. メッシュ描画機能 (MeshRenderer) をアタッチ
auto rendererComp = myObj->AddComponent<MeshRenderer>();

// 5. 形状 (Cube) と色を設定
auto cubeMesh = std::make_shared<Mesh>(1, 36); // shapeType=1, vertexCount=36
rendererComp->SetMesh(cubeMesh);
rendererComp->SetColor(DirectX::XMFLOAT4(1.0f, 0.4f, 0.1f, 1.0f)); // オレンジ色

// 6. トランスフォームの操作
myObj->SetPosition(DirectX::XMFLOAT3(0.0f, 10.0f, 60.0f));
myObj->SetScale(DirectX::XMFLOAT3(5.0f, 5.0f, 5.0f));
```

### ② ランタイムでのコンポーネントの追加・取得
`AddComponent` は、ゲーム起動時（初期化時）だけでなく、**ゲームの実行中（ランタイム）に動的に呼び出す**ことが可能です。

```cpp
// 例：パワーアップアイテムを取得した瞬間に、バリア描画コンポーネントを動的に追加する
void OnGetShieldItem(std::shared_ptr<GameObject> playerObj) {
    // すでにシールドコンポーネントがあるか確認
    auto shield = playerObj->GetComponent<ShieldBarrierComponent>();
    
    if (!shield) {
        // ランタイムで動的に追加！ (アタッチされた瞬間に Initialize() が走りバッファが作られます)
        shield = playerObj->AddComponent<ShieldBarrierComponent>();
        shield->SetMaxEnergy(100.0f);
    } else {
        // すでに存在していればエネルギーを回復
        shield->Replenish();
    }
}
```

---

## ✍️ 4. 新しいカスタムコンポーネントの作り方

新しい挙動（自転回転、AI移動、カスタムエフェクト等）を追加するには、`Component` クラスを継承して新しいクラスを作成します。

### 例：毎フレーム自転回転する `RotateComponent` の作成

```cpp
#pragma once
#include "Component.h"
#include "GameObject.h"

// 1. Component を継承したクラスを定義
class RotateComponent : public Component {
public:
    RotateComponent(float speed = 1.0f) : m_rotateSpeed(speed) {}
    virtual ~RotateComponent() {}

    // 2. 毎フレームの更新ロジックをオーバーライド
    virtual void Update() override {
        if (!m_gameObject) return;

        // 親 GameObject の Transform を取得して回転を更新
        DirectX::XMFLOAT3 rotation = m_gameObject->GetRotation();
        rotation.y += m_rotateSpeed; // Y軸周りに回転
        if (rotation.y > 360.0f) rotation.y -= 360.0f;

        m_gameObject->SetRotation(rotation);
    }

private:
    float m_rotateSpeed;
};
```

#### アタッチ方法
```cpp
// 毎フレーム Y 軸周りに 1.5 度ずつ回転するコンポーネントをアタッチ
myObj->AddComponent<RotateComponent>(1.5f);
```

---

## ⚠️ 5. 実装時の注意点

1. **Initialize の重要性**:
   `AddComponent` を呼び出した際、`GameObject::Initialize` が既に呼ばれていれば、コンポーネントの `Initialize` も自動で即座に実行されます。
   そのため、オブジェクト作成時に必ず `myObj->Initialize(renderer);` を一度呼び出してください。
2. **2D対面回転について**:
   `MeshRenderer` では、`shapeType == 3` (Sprite2D) がアタッチされた場合、2D縦/横モードでのカメラの正対状態（XZ平面/YZ平面）を考慮して自動でワールド行列に回転を掛けます。そのため、2Dモードでペラペラになって消える心配はありません。

---

## 🏗️ 6. 設計パターン：静的な追加とキャッシュ

特定のオブジェクト（例：プレイヤーなど）が**「最初から特定のコンポーネントを必ず持っている」**と決まっている（静的な）場合、以下の設計パターンが最も推奨されます。

### 💡 推奨：`Initialize` 内での Add ＆ メンバ変数キャッシュ
派生クラスの `Initialize` 内で `AddComponent` を呼び出しつつ、アクセスが頻繁なコンポーネントへの参照（ポインタ）をメンバ変数に保存（キャッシュ）しておきます。

```cpp
class PlayerObject : public GameObject {
public:
    virtual void Initialize(D3D12RenderingService& renderer) override {
        // 1. 基底の初期化 (必須)
        GameObject::Initialize(renderer);

        // 2. 必要なコンポーネントを自身にアタッチ
        m_meshRenderer = AddComponent<MeshRenderer>();
        m_controller = AddComponent<PlayerController>();

        // 3. メッシュ設定などの初期化
        auto wingMesh = std::make_shared<Mesh>(2, 18);
        m_meshRenderer->SetMesh(wingMesh);
    }

    void SetInvincible(bool val) {
        // キャッシュされたメンバ変数から高速にアクセス！
        m_meshRenderer->SetColor(val ? DirectX::XMFLOAT4(1,0,0,0.5f) : DirectX::XMFLOAT4(1,1,1,1));
    }

private:
    // 頻繁に使うコンポーネントをキャッシュしておく
    std::shared_ptr<MeshRenderer> m_meshRenderer;
    std::shared_ptr<PlayerController> m_controller;
};
```

#### なぜ直接変数として持たせる（AddComponentしない）より良いのか？
1. **自動ライフサイクル管理**:  
   `AddComponent` されたコンポーネントは、`GameObject::UpdateObject()` や `RenderObject()` が自動で巡回して処理するため、派生クラス側で手動の更新・描画コードを書く必要がありません。
2. **GetComponent への応答性**:  
   他のクラスやマネージャから `obj->GetComponent<PlayerController>()` でアクセスできるようになり、コンポーネントシステムの疎結合性が保たれます。
3. **キャッシュによる高速化**:  
   メンバ変数（スマートポインタ）として保持しておくことで、毎フレーム `GetComponent` を呼ぶオーバーヘッドをゼロにできます。
