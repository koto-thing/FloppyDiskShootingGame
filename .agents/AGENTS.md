# FloppyDiskShootingGame GEMINI.md

## 1. ファイルサイズの制約
* 最終的なビルドサイズはゲームに必要なリソースを含めて1.44MB、つまりフロッピーディスクの容量である必要があります。
* メモリやGPUなどに制限はありません。

## 2. アーキテクチャ
### Applicationレイヤー
* DomainとInfrastructureの実装を用いて、ゲーム内の挙動を実装する。
  * DTOs：Data Transfer Object
  * UseCases：ユースケースを実装する
  * Interfaces：依存性逆転を用いて、Infra層などの実装をインターフェイスとして定義する

### Domainレイヤー
* ゲーム内キャラクターの定義などを実装する
  * Entities：ゲーム内の敵やプレイヤーのパラメータなどを定義
  * ValueObjects：ゲーム内の座標や速度などの値を定義

### Infrastructureレイヤー
* 外部APIやゲームの動作に直接関係ないGraphicsAPIなどを実装する。
  * Repositories：データの永続化や取得を行う
  * ExternalServices：外部APIの呼び出し

### Presentationレイヤー
* ApplicationレイヤーとViweレイヤー間でデータの受け渡しをする

### Viewレイヤー
* キャラクターやUIを描画する