# FloppyDiskShootingGame GEMINI.md

## 1. ファイルサイズの制約
* The final build size should be 1.44MB, which is the size of a floppy disk, including the resources required for the game.
* There are no restrictions on memory or GPU.
* Data compression within the game is permitted.

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

## 3. Workflow

### Input prompt

* If we input the ambiguous prompt, you have to question the user to clarify the prompt.

### Codespace comment

* Please make comments based on Doxygen
* No need for periods at the end of sentences
* Please write a brief comment for each chunk of processing.