# Collision System Documentation

## ?? 目次

- [概要](#概要)
- [主な特徴](#主な特徴)
- [クイックスタート](#クイックスタート)
- [コンポーネントリファレンス](#コンポーネントリファレンス)
- [衝突判定の仕組み](#衝突判定の仕組み)
- [使用例](#使用例)
- [パフォーマンス最適化](#パフォーマンス最適化)
- [今後の拡張予定](#今後の拡張予定)

---

## 概要

HEW_ECSフレームワークの衝突判定システムは、ECS(Entity Component System)設計原則に完全準拠した、型安全で拡張性の高いシステムです。

### 設計思想

1. **データとロジックの完全分離**: 衝突形状はデータコンポーネント、検出処理はSystemとして実装
2. **型安全性**: C++17の `std::variant` と `std::optional` を活用
3. **パフォーマンス重視**: Broad-phase/Narrow-phase分離による最適化
4. **使いやすさ**: 直感的なAPIとコールバックシステム

---

## 主な特徴

### ? 実装済み

- ? **AABB (Axis-Aligned Bounding Box)** - 軸平行境界ボックス
- ? **Sphere** - 球体
- ? **Capsule** - カプセル(実装完了、判定は今後追加)
- ? **Box vs Box** 衝突判定
- ? **Sphere vs Sphere** 衝突判定
- ? **Box vs Sphere** 衝突判定
- ? 衝突イベントコールバックシステム
- ? レイヤーベースの衝突フィルタリング
- ? デバッグ描画機能(Debugビルド)

### ?? 今後実装予定

- ?? **OBB (Oriented Bounding Box)** - 回転可能なボックス
- ?? **Capsule衝突判定** (Capsule vs Sphere, Capsule vs Box, Capsule vs Capsule)
- ?? **Ray Casting** - レイによる交差判定
- ?? **空間分割** (Octree/Grid) によるパフォーマンス最適化
- ?? **連続衝突検出** (CCD - Continuous Collision Detection)
- ?? **物理エンジン統合** (オプション)

---

## クイックスタート

### 基本的な使い方(3ステップ)

```cpp
// ステップ1: 衝突検出システムを作成
Entity collisionSystem = world.Create()
    .With<CollisionDetectionSystem>()
    .Build();

// ステップ2: 衝突判定を持つエンティティを作成
Entity player = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
    .With<CollisionBox>(DirectX::XMFLOAT3{1, 2, 1})  // 幅1, 高さ2, 奥行き1
    .With<PlayerTag>()
    .Build();

Entity enemy = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{3, 0, 0})
    .With<CollisionSphere>(0.5f)  // 半径0.5
    .With<EnemyTag>()
    .Build();

// ステップ3: 衝突コールバックを登録
auto* colSys = world.TryGet<CollisionDetectionSystem>(collisionSystem);
if (colSys) {
    colSys->OnCollision([&](Entity a, Entity b, const CollisionInfo& info) {
        // プレイヤーと敵の衝突を検出
        if (world.Has<PlayerTag>(a) && world.Has<EnemyTag>(b)) {
        DEBUGLOG("プレイヤーが敵と衝突!");
        }
    });
}
```

**これだけ！** ECSシステムが自動的に衝突を検出し、コールバックを呼び出します。

---

## コンポーネントリファレンス

### CollisionBox

軸平行境界ボックス(AABB)。回転しない矩形の当たり判定。

```cpp
struct CollisionBox : IComponent {
    DirectX::XMFLOAT3 size{ 1.0f, 1.0f, 1.0f };  ///< サイズ(幅, 高さ, 奥行き)
    DirectX::XMFLOAT3 offset{ 0.0f, 0.0f, 0.0f }; ///< Transformからのオフセット
};
```

**使用例**

```cpp
// 均等サイズ
.With<CollisionBox>(1.0f)

// カスタムサイズ
.With<CollisionBox>(DirectX::XMFLOAT3{2.0f, 1.0f, 0.5f})

// オフセット付き
.With<CollisionBox>(
 DirectX::XMFLOAT3{1, 1, 1},    // サイズ
  DirectX::XMFLOAT3{0, 0.5f, 0}    // Y軸に0.5オフセット
)
```

**適用例**
- 箱型のオブジェクト
- 建物、壁
- プラットフォーム

---

### CollisionSphere

球体の当たり判定。すべての方向で均等。

```cpp
struct CollisionSphere : IComponent {
    float radius{ 0.5f };       ///< 半径
    DirectX::XMFLOAT3 offset{ 0.0f, 0.0f, 0.0f }; ///< オフセット
};
```

**使用例**

```cpp
// 標準の半径
.With<CollisionSphere>(0.5f)

// カスタム半径
.With<CollisionSphere>(2.0f)

// オフセット付き
.With<CollisionSphere>(0.5f, DirectX::XMFLOAT3{0, 1, 0})
```

**適用例**
- 球形のオブジェクト
- アイテム(コイン、パワーアップ)
- 弾丸、爆発範囲

---

### CollisionCapsule

カプセル形状。人型キャラクターに最適。

```cpp
struct CollisionCapsule : IComponent {
    float radius{ 0.5f };      ///< 半径
    float height{ 2.0f };      ///< 高さ(中心間距離)
    DirectX::XMFLOAT3 offset{ 0.0f, 0.0f, 0.0f };
};
```

**使用例**

```cpp
// 標準サイズ
.With<CollisionCapsule>(0.5f, 2.0f)

// カスタムサイズ
.With<CollisionCapsule>(0.3f, 1.8f)
```

**適用例**
- 人型キャラクター
- 動物
- NPCs

> ?? **注意**: Capsule vs 他の形状の衝突判定は今後実装予定です。

---

### CollisionDetectionSystem

衝突検出を行うシステムコンポーネント。Worldに1つだけ配置。

```cpp
struct CollisionDetectionSystem : Behaviour {
    void OnCollision(CollisionCallback callback);
    void SetDebugLog(bool enable);
    size_t GetCollisionCount() const;
};
```

**メソッド**

| メソッド | 説明 |
|---------|------|
| `OnCollision(callback)` | 衝突時のコールバックを登録 |
| `SetDebugLog(true/false)` | デバッグログの有効/無効 |
| `GetCollisionCount()` | 現在フレームの衝突数を取得 |

---

### CollisionLayer

レイヤーベースの衝突フィルタリング。

```cpp
struct CollisionLayer : IComponent {
    uint8_t layer{ 0 };      ///< 自身のレイヤー(0-7)
    uint8_t mask{ 0xFF };    ///< 衝突するレイヤーのマスク
};
```

**使用例**

```cpp
// プレイヤー: レイヤー0、敵(レイヤー1)とのみ衝突
.With<CollisionLayer>(0, 0b0010)

// 敵: レイヤー1、プレイヤー(レイヤー0)とのみ衝突
.With<CollisionLayer>(1, 0b0001)

// 壁: レイヤー2、すべてのレイヤーと衝突
.With<CollisionLayer>(2, 0xFF)
```

**レイヤー設計例**

| レイヤー | 名前 | 衝突対象 |
|----------|------|----------|
| 0 | Player | Enemy, Wall, Item |
| 1 | Enemy | Player, Wall |
| 2 | Wall | All |
| 3 | Item | Player |
| 4 | Trigger | Player |

---

## 衝突判定の仕組み

### アルゴリズム概要

1. **Broad Phase (粗い判定)**  
   すべての衝突可能エンティティを収集 (現在はO(n?)の総当たり)

2. **Narrow Phase (詳細判定)**  
   各ペアに対して実際の形状間衝突判定を実行

3. **Collision Response (衝突応答)**  
   登録されたコールバックを実行

### 対応している衝突判定

| 形状A | 形状B | 状態 |
|-------|-------|------|
| Box | Box | ? 実装済み |
| Sphere | Sphere | ? 実装済み |
| Box | Sphere | ? 実装済み |
| Capsule | Capsule | ?? 未実装 |
| Capsule | Box | ?? 未実装 |
| Capsule | Sphere | ?? 未実装 |

### CollisionInfo 構造体

衝突情報を格納する構造体。

```cpp
struct CollisionInfo {
    Entity entityA;         ///< 衝突したエンティティA
    Entity entityB;     ///< 衝突したエンティティB
 DirectX::XMFLOAT3 contactPoint; ///< 接触点
    DirectX::XMFLOAT3 normal;       ///< 衝突法線(A -> B方向)
    float penetrationDepth;       ///< 侵入深度
    bool isColliding;               ///< 衝突しているか
};
```

---

## 使用例

### 例1: アイテム取得

```cpp
Entity item = world.Create()
 .With<Transform>(DirectX::XMFLOAT3{5, 0, 0})
    .With<CollisionSphere>(0.3f)
    .Build();

colSys->OnCollision([&](Entity a, Entity b, const CollisionInfo& info) {
    if ((a == player && b == item) || (b == player && a == item)) {
        DEBUGLOG("アイテム取得!");
        world.DestroyEntity(item);
    }
});
```

### 例2: ダメージ処理

```cpp
colSys->OnCollision([&](Entity a, Entity b, const CollisionInfo& info) {
  // プレイヤーが敵と衝突
    if (world.Has<PlayerTag>(a) && world.Has<EnemyTag>(b)) {
        auto* playerHealth = world.TryGet<Health>(a);
        if (playerHealth) {
            playerHealth->TakeDamage(10.0f);
            DEBUGLOG("プレイヤーがダメージを受けた!");
 }
    }
});
```

### 例3: 壁との衝突で押し出す

```cpp
colSys->OnCollision([&](Entity a, Entity b, const CollisionInfo& info) {
 // プレイヤーが壁と衝突
    if (world.Has<PlayerTag>(a) && world.Has<WallTag>(b)) {
        auto* transform = world.TryGet<Transform>(a);
        if (transform) {
     // 衝突法線方向に押し出す
            transform->position.x -= info.normal.x * info.penetrationDepth;
            transform->position.y -= info.normal.y * info.penetrationDepth;
      transform->position.z -= info.normal.z * info.penetrationDepth;
   }
    }
});
```

### 例4: トリガーゾーン

```cpp
Entity goalZone = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{20, 0, 0})
    .With<CollisionBox>(DirectX::XMFLOAT3{3, 5, 3})
    .With<GoalTag>()
    .Build();

colSys->OnCollision([&](Entity a, Entity b, const CollisionInfo& info) {
if ((world.Has<PlayerTag>(a) && world.Has<GoalTag>(b)) ||
        (world.Has<GoalTag>(a) && world.Has<PlayerTag>(b))) {
     DEBUGLOG("?? ゴール達成!");
        // 次のステージへ...
    }
});
```

---

## パフォーマンス最適化

### 現在の実装

- **Broad Phase**: O(n?) 総当たり
- **Narrow Phase**: 最適化された判定アルゴリズム

### パフォーマンス指標

| エンティティ数 | フレームレート | 備考 |
|---------------|---------------|------|
| ~100 | 60 FPS | 快適 |
| ~500 | 30-60 FPS | やや重い |
| 1000+ | <30 FPS | 空間分割が必要 |

### 最適化のヒント

1. **不要なエンティティを削除**  
   画面外のオブジェクトは削除するか、衝突判定を無効化

2. **レイヤーシステムを活用**  
   必要な衝突のみ検出するよう設定

3. **衝突形状を簡略化**  
   複雑な形状は複数の単純な形状で近似

4. **静的オブジェクトの最適化**  
   (今後実装予定)

これにより、すべての衝突形状がワイヤーフレームで描画されます。

- **CollisionBox**: 緑色の立方体
- **CollisionSphere**: 黄色の球
- **CollisionCapsule**: 黄色の球2つ + 線分

### デバッグログの有効化

```cpp
auto* colSys = world.TryGet<CollisionDetectionSystem>(collisionSystem);
if (colSys) {
    colSys->SetDebugLog(true);  // 衝突をログ出力
}
```

---

## 今後の拡張予定

### Phase 1: 基本形状の完成 (優先度: 高)

- [ ] Capsule vs Sphere 衝突判定
- [ ] Capsule vs Box 衝突判定
- [ ] Capsule vs Capsule 衝突判定
- [ ] OBB (Oriented Bounding Box) の実装

### Phase 2: 最適化 (優先度: 高)

- [ ] 空間分割 (Octree または Uniform Grid)
- [ ] 静的/動的オブジェクトの分離
- [ ] SIMD最適化 (DirectXMath活用)

### Phase 3: 高度な機能 (優先度: 中)

- [ ] Ray Casting (レイキャスト)
- [ ] Sweep Test (連続衝突検出)
- [ ] Trigger/Collider の分離
- [ ] 親子階層のサポート

### Phase 4: 物理エンジン (優先度: 低)

- [ ] 物理応答システム (オプション)
- [ ] PhysX / Bullet 統合 (オプション)

---

## トラブルシューティング

### Q: 衝突が検出されない

**A**: 以下を確認してください

1. ? `CollisionDetectionSystem` が作成されているか
2. ? 両方のエンティティに `Transform` があるか
3. ? 両方のエンティティに衝突形状コンポーネントがあるか
4. ? `CollisionLayer` を使用している場合、マスク設定が正しいか
5. ? 実際にオブジェクトが接触しているか (DebugDraw で確認)

### Q: パフォーマンスが悪い

**A**: 以下を試してください

1. エンティティ数を減らす
2. 不要なエンティティを削除
3. `CollisionLayer` で衝突を制限
4. Debugビルドではなく、Releaseビルドで実行

### Q: 衝突の精度が低い

**A**: より適切な形状を選択してください

- 箱 → `CollisionBox`
- 球 → `CollisionSphere`
- 人型 → `CollisionCapsule`

---

## API リファレンス

### CollisionBox

| メソッド | 説明 |
|---------|------|
| `GetWorldCenter(transform)` | ワールド座標での中心を取得 |
| `GetScaledSize(transform)` | スケール適用後のサイズ |

### CollisionSphere

| メソッド | 説明 |
|---------|------|
| `GetWorldCenter(transform)` | ワールド座標での中心を取得 |
| `GetScaledRadius(transform)` | スケール適用後の半径 |

### CollisionCapsule

| メソッド | 説明 |
|---------|------|
| `GetWorldCenter(transform)` | ワールド座標での中心を取得 |
| `GetTopPoint(transform)` | カプセル上端点 |
| `GetBottomPoint(transform)` | カプセル下端点 |

---

## サンプルコード

詳細なサンプルコードは `include/samples/CollisionSamples.h` を参照してください。

```cpp
#include "samples/CollisionSamples.h"

// Game.cpp で使用
void GameScene::OnEnter(World& world) {
    CollisionSamples::Sample1_BasicCollision(world);
}
```

---

## ライセンス

このコードは HEW_ECS プロジェクトの一部です。  
著作権: **はじけるポップコーン**

---

## 作成者

- **立山悠朔** - 初期実装
- **上手凉太郎** - 初期実装
- **山内陽** - v2.0 リファクタリング、ECS準拠設計

バージョン: **v2.0** (2025)

---

## 関連ドキュメント

- [Core_Architecture.md](../../docs/Core_Architecture.md) - フレームワーク全体の設計
- [README.md](../../README.md) - プロジェクト概要
- [ComponentSamples.h](./ComponentSamples.h) - ECSコンポーネントのサンプル

---

**Happy Collision Detection! ??**
