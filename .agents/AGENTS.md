# FloppyDiskShootingGame GEMINI.md

## 1. ファイルサイズの制約
* 最終的なビルドサイズはゲームに必要なリソースを含めて1.44MB、つまりフロッピーディスクの容量である必要があります。
* メモリやGPUなどに制限はありません。

## 2. アーキテクチャ
### Applicationレイヤー
* DomainとInfrastructureの実装を用いて、ゲーム内の挙動を実装する。

### Domainレイヤー
* ゲーム内キャラクターの定義などを実装する

### Infrastructureレイヤー
* 外部APIやゲームの動作に直接関係ないGraphicsAPIなどを実装する。

### Presentationレイヤー
* ApplicationレイヤーとViweレイヤー間でデータの受け渡しをする

### Viewレイヤー
* キャラクターやUIを描画する