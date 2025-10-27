````markdown
# GitHub Copilot カスタム指示 - HEW_GAME プロジェクト

このファイルは、GitHub Copilot が本プロジェクト（**HEW_GAME**）でコードを生成・修正する際に従うべき **技術規約・運用フロー** を定義します。  
**大規模変更前には必ず「プロジェクト読込 → 計画立案 → レビュー承認」を経ること。**
ユーザーへの説明回答・ソースコメントなどはすべて日本語で行うこと

---

## プロジェクト概要

- **プロジェクト名**: HEW_GAME
- **目的**: Entity Component System (ECS) を活用したチームゲーム開発
- **言語**: C++14（厳守）
- **プラットフォーム**: Windows / DirectX 11
- **アーキテクチャ**: Entity Component System
- **開発スタイル**: Git/GitHub（Draft PR + 承認ゲート運用）

---

## 重要な制約事項（C++ 標準）

### 使用可能（C++14）
```cpp
// auto 型推論
auto entity = world.CreateEntity();

// ラムダ式
world.ForEach<Transform>([](Entity e, Transform& t) {
    t.position.x += 1.0f;
});

// スマートポインタ
std::unique_ptr<Component> component;
std::shared_ptr<Resource> resource;

// 範囲 for
for (const auto& entity : entities) { }

// 初期化リスト
DirectX::XMFLOAT3 position{0.0f, 0.0f, 0.0f};
````

### 使用禁止（C++17 以降）

```cpp
// std::optional（C++17）
std::optional<Transform> GetTransform(Entity e);  // NG

// if constexpr（C++17）
if constexpr (std::is_same_v<T, Transform>) { }   // NG

// std::filesystem（C++17）
std::filesystem::path filePath;                   // NG

// 構造化束縛（C++17）
auto [x, y, z] = GetPosition();                   // NG

// インライン変数（C++17）
inline constexpr int MAX_ENTITIES = 1000;         // NG
```

**対処方針（C++14 互換）**

```cpp
// 可空はポインタで代替（所有権はWorld）
Transform* GetTransform(Entity e) { return world.TryGet<Transform>(e); }

// 型分岐はテンプレート特殊化
template<typename T> void Process(T& component);

// 旧来のWin32 API or boost::filesystem（利用可なら）を使用
#include <windows.h>
```

---

## ECS アーキテクチャの原則

### Entity（識別子のみ）

```cpp
struct Entity {
    uint32_t id;   // エンティティID
    uint32_t gen;  // 世代番号（削除/再利用管理）
};
```

**禁止**: Entity にロジックや状態を持たせない。

### Component（2 種類）

1. **データコンポーネント（IComponent 継承）**: データ保持＋軽いヘルパのみ

```cpp
struct Health : IComponent {
    float current = 100.0f;
    float max = 100.0f;

    void TakeDamage(float dmg) {
        current -= dmg;
        if (current < 0.0f) current = 0.0f;
    }
    bool IsDead() const { return current <= 0.0f; }
};
```

2. **Behaviour コンポーネント（Behaviour 継承）**: 毎フレーム更新されるロジック

```cpp
struct Rotator : Behaviour {
    float speedDegY = 45.0f;
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (t) t->rotation.y += speedDegY * dt;
    }
};
```

### System 実装パターン

* **Behaviour パターン（推奨）**: 各 Entity に振る舞いを装着
* **ForEach パターン**: データ指向に一括処理

```cpp
void UpdateMovementSystem(World& world, float dt) {
    world.ForEach<Transform, Velocity>([dt](Entity, Transform& t, Velocity& v) {
        t.position.x += v.velocity.x * dt;
        t.position.y += v.velocity.y * dt;
        t.position.z += v.velocity.z * dt;
    });
}
```

---

## コーディング規約

| 要素      | 規約                     | 例                                          |
| ------- | ---------------------- | ------------------------------------------ |
| クラス/構造体 | PascalCase             | `Transform`, `MeshRenderer`, `World`       |
| 関数      | PascalCase             | `CreateEntity()`, `TryGet()`, `OnUpdate()` |
| 変数      | camelCase              | `deltaTime`, `entityId`, `speed`           |
| メンバ変数   | camelCase + `_` サフィックス | `world_`, `nextId_`                        |
| 定数      | UPPER_SNAKE_CASE       | `MAX_ENTITIES`, `DEFAULT_SPEED`            |

```cpp
class PlayerController : public Behaviour {
public:
    float speed = 5.0f;
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* transform = w.TryGet<Transform>(self);
        if (transform) transform->position.x += speed * dt;
    }
private:
    InputSystem* input_ = nullptr;
};
```

---

## World クラスの使用

### エンティティ作成（ビルダーパターン推奨）

```cpp
Entity player = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0,0,0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0,1,0})
    .With<Rotator>(45.0f)
    .With<PlayerTag>()
    .Build();
```

### コンポーネント操作

```cpp
// 安全取得
if (auto* t = world.TryGet<Transform>(entity)) { t->position.x += 1.0f; }

// 追加/存在確認/削除
world.Add<Health>(entity, Health{100.0f, 100.0f});
if (world.Has<Transform>(entity)) { /* ... */ }
world.Remove<Health>(entity);

// 削除（原因付きログ）
world.DestroyEntityWithCause(entity, World::Cause::Collision);
```

### ForEach 利用

```cpp
world.ForEach<Transform>([](Entity, Transform& t) { t.position.y += 0.1f; });
world.ForEach<PlayerTag, Transform>([](Entity, PlayerTag&, Transform& t) { /* プレイヤのみ */ });
```

---

## DirectXMath の使用

**保持は `XMFLOAT3/4`、計算は `XMVECTOR`**

```cpp
void MoveTowards(Transform& t, const DirectX::XMFLOAT3& target, float speed) {
    using namespace DirectX;
    XMVECTOR pos = XMLoadFloat3(&t.position);
    XMVECTOR tgt = XMLoadFloat3(&target);
    XMVECTOR dir = XMVectorSubtract(tgt, pos);
    dir = XMVector3Normalize(dir);
    XMVECTOR move = XMVectorScale(dir, speed);
    XMStoreFloat3(&t.position, XMVectorAdd(pos, move));
}
```

---

## ドキュメンテーション規約（Doxygen）

```cpp
/**
 * @file MyComponent.h
 * @brief コンポーネントの説明
 * @author ...
 * @date 2025
 * @version 6.0
 */
```

関数・構造体には `@brief`, `@details`, `@note`, `@warning`, `@param`, `@return` を適宜付与。

---

## 禁止事項（アーキ破壊/非推奨パターン）

* Entity にロジック/状態を追加しない
* グローバルでエンティティ管理しない（World 管理）
* コンポーネントから別コンポーネントへの**直接生ポインタ保持**禁止（World 経由で取得）
* Update 内で同期的に大量スポーン/破棄しない（**Enqueue** を使用）
* 毎フレームの不要な動的確保を避け、メンバ再利用

---

## デバッグとログ（規約）

```cpp
#include "app/DebugLog.h"

// レベル別
DEBUGLOG("Entity created: ID=%u", entity.id);
DEBUGLOG_WARNING("Transform not found on entity %u", entity.id);
DEBUGLOG_ERROR("Failed to load resource: %s", resourceName.c_str());

// _DEBUG 節での条件付きログ
#ifdef _DEBUG
DEBUGLOG("Delta time = %f", deltaTime);
#endif
```

**セキュリティ/運用**

* PII/シークレット（鍵/トークン/メール）を出力しない
* スロットリング：同一メッセージは 1 秒に 1 回まで（呼び出し側で制御）
* `INFO`, `WARN`, `ERROR` 以外は `_DEBUG` ビルド限定

---

## マクロ活用

### DEFINE_DATA_COMPONENT

```cpp
DEFINE_DATA_COMPONENT(Score,
    int points = 0;
    void AddPoints(int p) { points += p; }
    void Reset() { points = 0; }
);
```

### DEFINE_BEHAVIOUR

```cpp
DEFINE_BEHAVIOUR(CircularMotion,
    float radius = 3.0f;
    float speed = 1.0f;
    float angle = 0.0f;
,
    angle += speed * dt;
    if (auto* t = w.TryGet<Transform>(self)) {
        t->position.x = cosf(angle) * radius;
        t->position.z = sinf(angle) * radius;
    }
);
```

---

## 変更計画と承認フロー（Large Change Gate）

**目的**: 影響の大きい修正に対し、**着手前の「計画→レビュー→承認」を必須化**し、設計崩壊や差し戻しを防止。

### 大規模変更の判定基準（以下のいずれか）

* 変更行数（加算+削除） > **300**
* **新規ファイル 3 個以上** または **公開 API/コンポーネントのシグネチャ変更**
* **Core 領域**（`include/ecs/*`, `include/components/*`）に触れる
* **スレッドモデル/ライフサイクル**へ影響（スポーン/破棄/更新順序/同期）
* **ビルド設定/依存追加** を含む

### 実行手順

1. **コードベース読込**（関連 `.h/.cpp`、呼び出し関係、依存）
2. **計画作成（PLAN.md 生成）** ? 本書テンプレ使用
3. **ドラフト PR 作成（Draft 指定）** ? PR 本文に PLAN.md を貼付、レビューア指名
4. **承認ゲート** ? レビューアが `APPROVED: <氏名 or チーム>` コメントを付けるまで、**実装はスケルトン/スタブ最小限**
5. **実装開始** ? 計画から逸れる場合は PLAN.md 更新 → **再承認** 必須

### 禁止事項

* `APPROVED:` コメント **前** に大規模改変をマージ/Push
* Core 領域の無断編集（Hotfix は `HOTFIX:` チケット + 最小差分に限る）

---

## PLAN.md テンプレート（貼付用）

```md
# PLAN.md ? 変更計画

## 概要
- タイトル: <変更名>
- 目的: <ユーザ価値/性能/保守性>
- スコープ: <対象シーン/システム/コンポーネント>

## 影響範囲
- 既存 API/ABI: <有/無 + 詳細>
- 変更予定ファイル:
  - include/...
  - src/...

## 設計方針
- アーキテクチャ: <ECS 原則に沿った分離>
- データフロー/更新順序: <OnStart/OnUpdate/ForEach>
- 代替案比較: <案A/案B/不採用理由>

## 失敗モードと緩和策
- 競合/並列: <EnqueueSpawn/Destroy、イテレータ無効化防止>
- ライフサイクル: <Entity 世代/参照失効>
- 数値安定性: <正規化前の長さチェック/NaN対策>

## マイルストーン
- M1: スケルトン導入（テスト通過）
- M2: 機能A
- M3: 機能B/負荷試験

## 計測/検証
- 性能指標: <フレーム時間、ForEach 走査件数>
- テスト: <単体/統合/シーン回帰>

## ロールバック
- 手順: <Revert/Feature Flag>

## レビュー依頼
- レビューア: <担当者>
- 承認トークン: `APPROVED: <氏名>`
```

---

## Copilot 前段タスク（大規模変更時は必須）

* プロジェクトを走査し、関連ファイル/呼び出し関係/依存を列挙
* 上記 **PLAN.md** を自動生成（テンプレ厳守）
* Draft PR 本文に PLAN.md を埋め込むテキストを出力
* 承認待ちコメントを自動挿入:
  `READY FOR REVIEW ? Reply with "APPROVED: <name>" to proceed.`
* `APPROVED:` が付くまで**実装コード生成はスケルトン/インタフェースのみに限定**

---

## 変更凍結領域（要承認）

* `include/ecs/World.h`
* `include/ecs/Entity.h`
* `include/components/Component.h`
* `include/components/Transform.h`
* `include/components/MeshRenderer.h`

**規則**: 上記に触れる PR は **無条件で PLAN.md 必須**。`APPROVED:` 付与前は Draft のまま。

---

## C++14 代替パターン（関数ブロック）

```cpp
// Optional の代替（借用明示）
template<typename T>
T* TryGetBorrowed(World& w, Entity e) { return w.TryGet<T>(e); }

struct MaybeTransform { Transform* ptr; bool has; };
inline MaybeTransform GetMaybeTransform(World& w, Entity e) {
    Transform* t = w.TryGet<Transform>(e);
    return { t, t != nullptr };
}
```

---

## DirectXMath 安全ガード（完全形）

```cpp
static inline void MoveTowardsSafe(Transform& t, const DirectX::XMFLOAT3& target, float speed, float dt) {
    using namespace DirectX;
    XMVECTOR pos = XMLoadFloat3(&t.position);
    XMVECTOR tgt = XMLoadFloat3(&target);
    XMVECTOR delta = XMVectorSubtract(tgt, pos);

    // ゼロ長回避
    XMVECTOR lenSq = XMVector3LengthSq(delta);
    if (XMVectorGetX(lenSq) < 1e-12f) return;

    // 単位系: speed[m/s] * dt[s]
    XMVECTOR dir = XMVector3Normalize(delta);
    XMVECTOR step = XMVectorScale(dir, speed * dt);
    XMStoreFloat3(&t.position, XMVectorAdd(pos, step));
}
```

---

## チーム開発ルール

### ファイル編集の優先順位

* **コア（触らない/要承認）**: 前節「変更凍結領域」参照
* **自由に編集**: `include/scenes/`, `include/components/Custom*.h`, `src/`
* **要相談**: `include/graphics/`, `include/input/`, `include/app/`

### Git コミットメッセージ

```bash
# 良い例（型＋内容）
git commit -m "feat: Add player shooting system"
git commit -m "fix: Resolve collision detection NaN at zero-length"
git commit -m "docs: Update README with component guide"
git commit -m "perf: Optimize render loop"
git commit -m "refactor: Restructure component stores"
```

---

## 参考ファイル

* `include/samples/ComponentSamples.h` ? コンポーネント実装例
* `include/samples/SampleScenes.h` ? シーン実装例
* `include/scenes/MiniGame.h` ? 小規模ゲーム実装
* `include/ecs/World.h` ? 利用ガイド/インタフェース

---

## コード生成チェックリスト（Copilot 用）

* [ ] C++14 準拠（C++17 機能は不使用）
* [ ] ECS の分離（Entity / Component / System）
* [ ] コンポーネントは `IComponent` または `Behaviour` を継承
* [ ] エンティティ作成はビルダーパターン（推奨）
* [ ] ポインタ取得は `TryGet` を用いて null チェック
* [ ] 命名規約（PascalCase / camelCase / `_` 接尾辞）遵守
* [ ] Doxygen コメント付与
* [ ] DirectXMath の型（XMFLOAT3 等）と計算手順の分離
* [ ] グローバル管理を避け、World 管理
* [ ] リーク無し（World が所有権を持つ）/ ライフサイクル安全
* [ ] **大規模変更時は PLAN.md + Draft PR + `APPROVED:` 承認を確認**

---

## 作成者・メタ情報

* **作成者**: 山内陽
* **最終更新**: 2025
* **バージョン**: v6.0 ? チームゲーム開発フレームワーク（HEW_GAME 対応）

---