#pragma once
#include "ecs/Entity.h"
#include "components/Component.h"
#include "app/DebugLog.h" // デバッグビルド/リリースビルド両方で必要
#include "components/Model.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <cstdio>
#include <memory>
#include <unordered_set>
#include <algorithm> // std::remove_if のために追加
#include <limits>
#include <mutex>
#include <string> // std::to_string のために追加

#ifdef _DEBUG
#include <cassert>
#endif

#if !defined(ECS_TRACE_LOG)
#if defined(ENABLE_ECS_TRACE_LOG) && ENABLE_ECS_TRACE_LOG
#define ECS_TRACE_LOG(message) DEBUGLOG(message)
#define ECS_TRACE_CATEGORY(category, message) DEBUGLOG_CATEGORY(category, message)
#else
#define ECS_TRACE_LOG(message) ((void)0)
#define ECS_TRACE_CATEGORY(category, message) ((void)0)
#endif
#endif

/**
 * @file World.h
 * @brief ECSワールド管理システムとエンティティビルダーの定義
 * @author 山内陽
 * @date 2025
 * @version 5.0
 *
 * @details
 * ECSアーキテクチャの中核となるWorldクラスと、
 * エンティティを便利に作成するためのEntityBuilderクラスを定義します。
 */

class World; ///< 前方宣言

/**
 * @class EntityBuilder
 * @brief エンティティ作成用のビルダーパターンクラス
 *
 * @details
 * メソッドチェーンを使用して、複数のコンポーネントを持つエンティティを
 * 直感的に作成できます。Worldクラスと連携して動作します。
 *
 * @par 使用例
 * @code
 * Entity player = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 *     .With<Rotator>(45.0f)
 *     .Build();
 * @endcode
 *
 * @note Build()は省略可能です(暗黙的にEntityへ変換されます)
 * @see World
 *
 * @author 山内陽
 */
class EntityBuilder {
public:
    /**
     * @brief コンストラクタ
     * @param[in] world Worldインスタンスへのポインタ
     * @param[in] entity 作成されたエンティティ
     */
    EntityBuilder(World* world, Entity entity) : world_(world), entity_(entity) {}

    /**
     * @brief メソッドチェーンでコンポーネントを追加
     *
     * @tparam T 追加するコンポーネントの型
     * @tparam Args コンストラクタ引数の型(可変長)
     * @param[in] args コンポーネントのコンストラクタに転送する引数
     * @return EntityBuilder& メソッドチェーン用の自身への参照
     *
     * @details
     * 指定したコンポーネントを作成し、エンティティに追加します。
     * メソッドチェーンで複数のコンポーネントを連続して追加できます。
     *
     * @par 使用例
     * @code
     * world.Create()
     *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
     *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
     *     .Build();
     * @endcode
     */
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args);

    /**
     * @brief メソッドチェーンでコンポーネントを追加（原因付き、int版）
     *
     * @tparam T 追加するコンポーネントの型
     * @tparam CauseType 原因の型（enum class World::Causeまたはint）
     * @tparam Args コンストラクタ引数の型(可変長)
     * @param[in] cause 事象の原因タグ
     * @param[in] args コンポーネントのコンストラクタに転送する引数
     * @return EntityBuilder& メソッドチェーン用の自身への参照
     *
     * @note この宣言はWorldクラス定義後に実装されます
     */
    template<typename T, typename CauseType, typename... Args>
    EntityBuilder& WithCause(CauseType cause, Args&&... args);

    /**
     * @brief エンティティを確定して返す
     * @return Entity 作成されたエンティティ
     *
     * @details
     * ビルダーパターンを完了し、作成されたエンティティを返します。
     * 省略可能で、暗黙的にEntityに変換されます。
     */
    Entity Build() { return entity_; }

    /**
     * @brief Entityへの暗黙的型変換演算子
     * @return Entity 作成されたエンティティ
     *
     * @details
     * Build()を呼ばなくても、自動的にEntityに変換されます。
     */
    operator Entity() const { return entity_; }

private:
    World* world_;    ///< Worldインスタンスへのポインタ
    Entity entity_;   ///< 作成されたエンティティ
};

/**
 * @class World
 * @brief ECSワールド管理クラス
 *
 * @details
 * ECSアーキテクチャにおけるすべてのエンティティとコンポーネントを管理します。
 *
 * ### 主な機能:
 * - エンティティの作成/破棄
 * - コンポーネントの追加/削除/取得
 * - Behaviourコンポーネントの更新
 *
 * @par 基本的な使用方法
 * @code
 * World world;
 *
 * // エンティティを作成してコンポーネントを追加
 * Entity player = world.CreateEntity();
 * world.Add<Transform>(player, Transform{...});
 * world.Add<MeshRenderer>(player, MeshRenderer{...});
 *
 * // コンポーネントを取得して操作
 * auto* transform = world.TryGet<Transform>(player);
 * if (transform) {
 *     transform->position.x += 1.0f;
 * }
 *
 * // 毎フレーム更新
 * world.Tick(deltaTime);
 * @endcode
 *
 * @par ビルダーパターン(推奨)
 * @code
 * Entity player = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 *     .With<Rotator>(45.0f)
 *     .Build();
 * @endcode
 *
 * @see Entity
 * @see IComponent
 * @see Behaviour
 *
 * @author 山内陽
 */
class World {
public:
    // 起因タグ（ログ解析用）
    enum class Cause {
        Unknown = 0,
        Spawner,
        WaveTimer,
        Collision,
        LifetimeExpired,
        StageReset,
        SceneInit,
        SceneTeardown,   // シーン終了時
        SceneUnload,     // シーン切り替え時
        AppShutdown      // アプリケーション終了時
    };

    static const char* CauseToString(Cause c) {
        switch (c) {
        case Cause::Spawner: return "Spawner";
        case Cause::WaveTimer: return "WaveTimer";
        case Cause::Collision: return "Collision";
        case Cause::LifetimeExpired: return "LifetimeExpired";
        case Cause::SceneInit: return "SceneInit";
        case Cause::SceneTeardown: return "SceneTeardown";
        case Cause::SceneUnload: return "SceneUnload";
        case Cause::AppShutdown: return "AppShutdown";
        default: return "Unknown";
        }
    }

    /**
     * @brief 生存エンティティ数を取得
     * @return size_t 生存中のエンティティ数
     */
    size_t GetAliveCount() const {
        return alive_.size();
    }

    /**
     * @brief 総エンティティ数を取得
     * @return size_t 総エンティティ数
     */
    size_t GetEntityCount() const {
        return alive_.size();
    }

    /**
     * @brief デストラクタ
     * @details 確保したコンポーネントストアのメモリを解放します
     */
    ~World() {
        ECS_TRACE_LOG("World::~World() - World破棄中");
        ECS_TRACE_LOG("アクティブエンティティ: " + std::to_string(alive_.size()));
        ECS_TRACE_LOG("アクティブビヘイビア: " + std::to_string(behaviours_.size()));

        // 未処理の破棄キューを先に処理
        FlushDestroyEndOfFrame();

        // ⚠️ 残存エンティティを強制削除（原因をAppShutdownに明確化）
        if (!alive_.empty()) {
            DEBUGLOG_WARNING(std::to_string(alive_.size()) + " 個の残存エンティティを強制破棄 (原因=AppShutdown)");

            // イテレータ無効化を避けるためコピー
            std::vector<uint32_t> aliveIds(alive_.begin(), alive_.end());
            for (uint32_t id : aliveIds) {
                DestroyEntityInternal(id, Cause::AppShutdown);
            }

            ECS_TRACE_LOG("すべてのエンティティを破棄 (最終生存数: " + std::to_string(alive_.size()) + ")");
        }

        for (auto& pair : stores_) {
            delete pair.second;
        }

        ECS_TRACE_LOG("World破棄完了");
    }

    /**
     * @brief 新しいエンティティを作成
     * @return Entity 一意なIDを持つ新規作成されたエンティティ
     *
     * @details
     * 一意なIDを持つ新しいエンティティを作成します。初期状態ではコンポーネントは付いていません。
     * 削除されたエンティティのIDを再利用してメモリ効率を向上させます。
     *
     * @note より便利なエンティティ作成にはCreate()(ビルダー)の使用を推奨してください
     */
    Entity CreateEntity() { return CreateEntityWithCause(Cause::Unknown); }

    /**
     * @brief 新しいエンティティを作成 (原因付き)
     * @param cause 事象の原因
     */
    Entity CreateEntityWithCause(Cause cause) {
        if (enforceNoMutateDuranteUpdate_ && inUpdate_) {
            DEBUGLOG_WARNING(std::string("Update中にエンティティ作成 (原因=") + CauseToString(cause) + ")");

        }

        std::lock_guard<std::mutex> lock(entityMutex_);
        uint32_t id;
        if (!freeIdsReady_.empty()) {
            // 再利用可能なIDがあればそれを使う
            id = freeIdsReady_.back();
            freeIdsReady_.pop_back();
            ECS_TRACE_LOG("エンティティ作成 (再利用ID: " + std::to_string(id) + ")");
        }
        else {
            // なければ新規ID
            id = ++nextId_;
            generations_.resize(std::max<size_t>(generations_.size(), id + 1), 1);
            ECS_TRACE_LOG("エンティティ作成 (新規ID: " + std::to_string(id) + ")");
        }
        alive_.insert(id); // live setへコミット

        // メトリクス更新
        totalCreated_++;
        if (trackFrameAccounting_) { createdThisFrame_++; }
        if (alive_.size() > maxAlive_) maxAlive_ = alive_.size();

        return Entity{ id, generations_[id] };
    }

    /**
     * @brief 並列環境向け: エンティティ生成をキューし、フラッシュ時に生成（メインスレッド）
     * @param cause 起因タグ
     * @param onCreated 生成直後に呼ばれるコールバック（メインスレッド）。引数に生成された Entity。
     */
    void EnqueueSpawn(Cause cause, const std::function<void(Entity)>& onCreated) {
        if (systemsStopped_) {
            DEBUGLOG_WARNING(std::string("システム停止後のスポーン要求を拒否 (原因=") + CauseToString(cause) + ")");
            return;
        }
        std::lock_guard<std::mutex> lock(spawnMutex_);
        pendingSpawn_.push_back({ cause, onCreated });
        ECS_TRACE_LOG(std::string("スポーンをキューに追加 (原因=") + CauseToString(cause) + ")");
    }

    /**
     * @brief ビルダーパターンで新しいエンティティを作成
     * @return EntityBuilder メソッドチェーン用のビルダーオブジェクト
     *
     * @details
     * 直感的なコンポーネント追加を可能にするEntityBuilderを返します。
     * メソッドチェーンで複数のコンポーネントを連続して追加できます。
     *
     * @par 使用例
     * @code
     * Entity enemy = world.Create()
     *     .With<Transform>(DirectX::XMFLOAT3{5, 0, 0})
     *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
     *     .With<Enemy>()
     *     .Build();
     * @endcode
     *
     * @see EntityBuilder
     */
    EntityBuilder Create() {
        return EntityBuilder(this, CreateEntity());
    }

    /**
     * @brief エンティティが生存しているか確認
     *
     * @param[in] e 確認するエンティティ
     * @return true 生存している, false 破棄済み
     */
    bool IsAlive(Entity e) const {
        // IDが生存かつ世代一致
        if (alive_.count(e.id) == 0) return false;
        if (e.id >= generations_.size()) return false;
        return generations_[e.id] == e.gen;
    }

    /**
     * @brief エンティティとそのすべてのコンポーネントを破棄
     *
     * @param[in] e 破棄するエンティティ
     *
     * @details
     * 指定されたエンティティとそれに関連するすべてのコンポーネントを削除します。
     * Behaviourコンポーネントも自動的に登録解除されます。
     * IDは再利用用にプールされます。
     *
     * @warning 破棄されたエンティティを使用するとクラッシュする可能性があります
     */
    void DestroyEntity(Entity e) { DestroyEntityWithCause(e, Cause::Unknown); }

    /**
     * @brief エンティティを破棄 (原因付き)
     * @param e 破棄するエンティティ
     * @param cause 事象の原因
     */
    void DestroyEntityWithCause(Entity e, Cause cause) {
        if (!IsAlive(e)) {
            DEBUGLOG_WARNING("既に死亡/無効なエンティティの破棄を試行 (ID: " + std::to_string(e.id) + ", gen: " + std::to_string(e.gen) + ")");
            return;
        }
        // 破棄要求はEoFで処理（スレッド安全）
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingDestroy_.push_back({ e.id, cause });
        }
        ECS_TRACE_LOG(std::string("破棄をキューに追加 (ID: ") + std::to_string(e.id) + ", 原因=" + CauseToString(cause) + ")");
    }

    /**
     * @brief エンティティにコンポーネントを追加
     *
     * @tparam T 追加するコンポーネントの型
     * @tparam Args コンストラクタ引数の型(可変長)
     * @param[in] e 対象エンティティ
     * @param[in] args コンポーネントのコンストラクタ引数
     * @return T& 追加されたコンポーネントへの参照
     * @throws std::runtime_error エンティティが死んでいる場合、またはコンポーネントが既に存在する場合（デバッグビルド）
     *
     * @details
     * 指定したコンポーネントをエンティティに追加します。
     * コンポーネントがBehaviourを継承している場合、Tick()で自動的に更新されます。
     */
    template<class T, class...Args>
    T& Add(Entity e, Args&&...args) {
        return AddWithCause<T>(e, Cause::Unknown, std::forward<Args>(args)...);
    }

    /**
     * @brief エンティティにコンポーネントを追加 (原因付き)
     */
    template<class T, class...Args>
    T& AddWithCause(Entity e, Cause cause, Args&&...args) {
        if (!IsAlive(e)) {
            char msg[160];
            sprintf_s(msg, "死亡/無効なエンティティにコンポーネント追加を試行 (ID: %u, gen: %u)", e.id, e.gen);
            DEBUGLOG_ERROR(std::string(msg));
            throw std::runtime_error(msg);
        }

        auto& s = getStore<T>();

#ifdef _DEBUG
        // デバッグモードでは重複チェック
        if (s.map.find(e.id) != s.map.end()) {
            char msg[160];
            sprintf_s(msg, "コンポーネント %s は既にエンティティに存在します (ID: %u, gen: %u)", typeid(T).name(), e.id, e.gen);
            DEBUGLOG_ERROR(std::string(msg));
            throw std::runtime_error(msg);
        }
#endif

        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *obj;
        s.map[e.id] = std::move(obj);
        registerBehaviourWithCause<T>(e, &ref, cause);

        ECS_TRACE_LOG("コンポーネント " + std::string(typeid(T).name()) + " をエンティティ " + std::to_string(e.id) + " に追加");

        return ref;
    }

    /**
     * @brief エンティティからコンポーネントを削除
     *
     * @tparam T 削除するコンポーネントの型
     * @param[in] e 対象エンティティ
     * @return bool 削除に成功した場合true、コンポーネントが存在しなかった場合false
     */
    template<class T>
    bool Remove(Entity e) {
        if (!IsAlive(e)) {
            DEBUGLOG_WARNING("死亡/無効なエンティティからコンポーネント削除を試行 (ID: " + std::to_string(e.id) + ")");
            return false;
        }

        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return false;

        auto* s = static_cast<Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return false;

        // Behaviourの場合は登録解除
        unregisterBehaviour<T>(e, it->second.get());

        // コンポーネントを削除
        s->map.erase(it);

        ECS_TRACE_LOG("コンポーネント " + std::string(typeid(T).name()) + " をエンティティ " + std::to_string(e.id) + " から削除");

        return true;
    }

    template<class T>
    bool Has(Entity e) const {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return false;
        auto* s = static_cast<const Store<T>*>(itS->second);
        return s->map.find(e.id) != s->map.end();
    }

    template<class T>
    T* TryGet(Entity e) {
        if (!IsAlive(e)) return nullptr;
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return nullptr;
        auto* s = static_cast<Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return nullptr;
        return it->second.get();
    }

    template<class T>
    const T* TryGet(Entity e) const {
        if (!IsAlive(e)) return nullptr;
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return nullptr;
        auto* s = static_cast<const Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return nullptr;
        return it->second.get();
    }

    template<class T>
    T& Get(Entity e) {
        T* ptr = TryGet<T>(e);
        if (!ptr) {
            char msg[160];
            sprintf_s(msg, "Component %s not found on entity (ID: %u, gen: %u)", typeid(T).name(), e.id, e.gen);
            throw std::runtime_error(msg);
        }
        return *ptr;
    }

    template<class T>
    const T& Get(Entity e) const {
        const T* ptr = TryGet<T>(e);
        if (!ptr) {
            char msg[160];
            sprintf_s(msg, "Component %s not found on entity (ID: %u, gen: %u)", typeid(T).name(), e.id, e.gen);
            throw std::runtime_error(msg);
        }
        return *ptr;
    }

    template<class T, class F>
    void ForEach(F&& fn) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return;
        auto* s = static_cast<Store<T>*>(itS->second);

        // イテレーション中の削除に対応するため、IDのコピーを作成
        std::vector<uint32_t> ids;
        ids.reserve(s->map.size());
        for (const auto& pair : s->map) {
            ids.push_back(pair.first);
        }

        for (uint32_t id : ids) {
            if (alive_.count(id) == 0) continue; // 処理中にエンティティが削除されたか確認

            auto it = s->map.find(id);
            if (it != s->map.end()) {
                fn(Entity{id, generations_[id]}, *it->second);
            }
        }
    }

    template<class T1, class T2, class F>
    void ForEach(F&& fn) {
        auto itS1 = stores_.find(std::type_index(typeid(T1)));
        if (itS1 == stores_.end()) return;
        auto* s1 = static_cast<Store<T1>*>(itS1->second);

        // イテレーション中の削除に対応するため、IDのコピーを作成
        std::vector<uint32_t> ids;
        ids.reserve(s1->map.size());
        for (const auto& pair : s1->map) {
            ids.push_back(pair.first);
        }

        for (uint32_t id : ids) {
            if (alive_.count(id) == 0) continue; // 処理中にエンティティが削除されたか確認

            auto it1 = s1->map.find(id);
            if (it1 == s1->map.end()) continue;

            T2* comp2 = TryGet<T2>(Entity{ id, generations_[id] });
            if (comp2) {
                fn(Entity{ id, generations_[id] }, *it1->second, *comp2);
            }
        }
    }

    /**
     * @brief すべてのBehaviourコンポーネントを更新
     */
    void Tick(float dt) {
#ifdef _DEBUG
        // フレーム番号をログに反映
        DebugLog::GetInstance().SetFrame(frameCount_ + 1);
#endif
        if (dt < 0.0f) {
            DEBUGLOG_WARNING("World::Tickで負のdeltaTimeを検出: " + std::to_string(dt));
            dt = 0.0f;
        }

        if (dt > 1.0f) {
            DEBUGLOG_WARNING("World::Tickで非常に大きいdeltaTimeを検出: " + std::to_string(dt) + "s");
        }

        // フレーム開始時に、フレーム内カウンタをリセット
        // これにより、初期化時など前フレーム由来のカウントを持ち越さず、
        // Metrics の期待値計算が正しくなります。
        createdThisFrame_ = 0;
        destroyedThisFrame_ = 0;

        // 整合性チェック用: フレーム開始時点（スポーン反映前）の生存数を記録
        windowAliveStart_ = alive_.size();

        // この時点からフレーム内会計を有効化
        trackFrameAccounting_ = true;

        // まずスポーンをスタート・オブ・フレームで反映（契約：メインスレッド）
        FlushSpawnStartOfFrame();

        // メトリクス更新（最近Nフレーム）
        recentCount_++;
        recentDtSum_ += dt;
        if (dt < recentDtMin_) recentDtMin_ = dt;
        if (dt > recentDtMax_) recentDtMax_ = dt;

        inUpdate_ = true;

        size_t startedCount = 0;
        for (size_t i = 0; i < behaviours_.size(); ) {
            if (i >= behaviours_.size()) break;
            auto& entry = behaviours_[i];

            if (!entry.started && IsAlive(entry.e)) {
                try {
                    entry.b->OnStart(*this, entry.e);
                    entry.started = true;
                    startedCount++;
                    // 原因付きログ
                    ECS_TRACE_LOG(std::string("ビヘイビア開始: ") + typeid(*entry.b).name() +
                             " on Entity " + std::to_string(entry.e.id) +
                             " (gen " + std::to_string(entry.e.gen) + ")" +
                             " 原因=" + CauseToString(entry.cause));
                } catch (const std::exception& ex) {
                    DEBUGLOG_ERROR("エンティティ " + std::to_string(entry.e.id) + " のBehaviour::OnStartで例外発生: " + ex.what());
                }
            }

            if (i < behaviours_.size() && behaviours_[i].e == entry.e) {
                i++;
            }
        }

        if (startedCount > 0) {
            ECS_TRACE_LOG(std::to_string(startedCount) + " 個の新しいビヘイビアを開始");
        }

        // OnUpdateの実行（より安全なイテレーション）
        for (size_t i = 0; i < behaviours_.size(); ) {
            if (i >= behaviours_.size()) break;

            auto& entry = behaviours_[i];
            Entity currentEntity = entry.e;
            Behaviour* currentBehaviour = entry.b;

            if (!IsAlive(currentEntity)) {
                // 死んだエンティティはスキップ
                i++;
                continue;
            }

            try {
                entry.b->OnUpdate(*this, currentEntity, dt);
            } catch (const std::exception& ex) {
                DEBUGLOG_ERROR("エンティティ " + std::to_string(currentEntity.id) + " のBehaviour::OnUpdateで例外発生: " + ex.what());
            }

            if (i >= behaviours_.size()) {
                break;
            }

            if (i < behaviours_.size()) {
                if (behaviours_[i].e == currentEntity && behaviours_[i].b == currentBehaviour) {
                    i++;
                }
            }
        }

        inUpdate_ = false;

        // End-of-frame contract: 全System更新が終わった後に破棄を反映
        FlushDestroyEndOfFrame();

        // 無効になったBehaviour（死んだエンティティに紐づくもの）をまとめて削除
        size_t beforeCleanup = behaviours_.size();
        behaviours_.erase(
            std::remove_if(behaviours_.begin(), behaviours_.end(),
                [this](const BEntry& entry) {
                    return !IsAlive(entry.e);
                }),
            behaviours_.end()
        );

        if (behaviours_.size() != beforeCleanup) {
            ECS_TRACE_LOG(std::to_string(beforeCleanup - behaviours_.size()) + " 個の死んだビヘイビアをクリーンアップ");
        }

        // 整合性チェック（生存数 = 開始時 + 作成 - 破棄）
        size_t expectedAlive = windowAliveStart_ + createdThisFrame_ - destroyedThisFrame_;
        if (alive_.size() != expectedAlive) {
            DEBUGLOG_WARNING("メトリクス不一致: alive=" + std::to_string(alive_.size()) +
                             ", expected=" + std::to_string(expectedAlive) +
                             ", startAlive=" + std::to_string(windowAliveStart_) +
                             ", createdThisFrame=" + std::to_string(createdThisFrame_) +
                             ", destroyedThisFrame=" + std::to_string(destroyedThisFrame_));
        }

        // 同フレームで破棄されたIDは、フレーム終端で再利用可能に移動
        if (!freeIdsPending_.empty()) {
            freeIdsReady_.insert(freeIdsReady_.end(), freeIdsPending_.begin(), freeIdsPending_.end());
            freeIdsPending_.clear();
        }

        // Nフレームごとに集計ログを出す（スパム抑制）
        if (recentCount_ >= metricsWindow_) {
            float avg = (recentCount_ > 0) ? (recentDtSum_ / recentCount_) : 0.0f;
            ECS_TRACE_LOG("メトリクス: frames=" + std::to_string(metricsWindow_) +
                     ", dt(avg/min/max)=" + std::to_string(avg) + "/" + std::to_string(recentDtMin_) + "/" + std::to_string(recentDtMax_) +
                     ", created=" + std::to_string(recentCreated_) +
                     ", destroyed=" + std::to_string(recentDestroyed_) +
                     ", maxAlive=" + std::to_string(maxAlive_) +
                     ", aliveNow=" + std::to_string(alive_.size())
            );
            // リセット
            recentDtSum_ = 0.0f;
            recentDtMin_ = std::numeric_limits<float>::infinity();
            recentDtMax_ = 0.0f;
            recentCount_ = 0;
            recentCreated_ = 0;
            recentDestroyed_ = 0;
        }

        // フレーム単位のカウンタを集計に反映
        recentCreated_ += createdThisFrame_;
        recentDestroyed_ += destroyedThisFrame_;

        // フレーム内会計を終了
        trackFrameAccounting_ = false;

        frameCount_++;
    }

    /**
     * @brief メインスレッドのみで呼ぶ（契約）。この時点では全SystemのUpdateは完了していること。
     * 破棄要求キューを処理します。
     */
    void FlushDestroyEndOfFrame() {
        std::vector<std::pair<uint32_t, Cause>> toDestroy;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            if (pendingDestroy_.empty()) return;
            toDestroy.swap(pendingDestroy_);
        }

        // 重複除去（最後の原因を優先)
        std::unordered_map<uint32_t, Cause> lastCause;
        lastCause.reserve(toDestroy.size());
        for (auto& p : toDestroy) lastCause[p.first] = p.second;

        size_t destroyed = 0;
        for (auto& kv : lastCause) {
            DestroyEntityInternal(kv.first, kv.second);
            destroyed++;
        }
        if (destroyed > 0) {
            ECS_TRACE_LOG("破棄キューをフラッシュ: " + std::to_string(destroyed) + " 個のエンティティ");
        }
    }

    /**
     * @brief メインスレッドのみで呼ぶ（契約）。フレーム開始時にスポーンキューを反映。
     */
    void FlushSpawnStartOfFrame() {
        if (systemsStopped_) {
            // システム停止後はスポーンキューを処理しない
            std::lock_guard<std::mutex> lock(spawnMutex_);
            if (!pendingSpawn_.empty()) {
                DEBUGLOG_WARNING("システム停止後、" + std::to_string(pendingSpawn_.size()) + " 個の保留スポーンを破棄");
                pendingSpawn_.clear();
            }
            return;
        }

        std::vector<std::pair<Cause, std::function<void(Entity)>>> toSpawn;
        {
            std::lock_guard<std::mutex> lock(spawnMutex_);
            if (pendingSpawn_.empty()) return;
            toSpawn.swap(pendingSpawn_);
        }
        size_t spawned = 0;
        for (auto& item : toSpawn) {
            Entity e = CreateEntityWithCause(item.first);
            if (item.second) item.second(e);
            spawned++;
        }
        if (spawned > 0) {
            ECS_TRACE_LOG("スポーンキューをフラッシュ: " + std::to_string(spawned) + " 個のエンティティ");
        }
    }

    // デバッグオプション: Update中の生成/破棄禁止を有効化
    void SetEnforceNoMutateDuranteUpdate(bool en) { enforceNoMutateDuranteUpdate_ = en; }

    /**
     * @brief すべてのシステムを停止（新規Spawn無効化）
     * @details
     * シャットダウン前に呼び出し、新規エンティティの生成を無効化します。
     * この後はEnqueueSpawn()が拒否され、既存のエンティティのみ処理されます。
     */
    void StopAllSystems() {
        if (systemsStopped_) return; // 冪等性
        ECS_TRACE_CATEGORY(DebugLog::Category::ECS, "World::StopAllSystems() - すべてのシステムを停止");
        systemsStopped_ = true;

        // 保留中のスポーンキューをクリア
        {
            std::lock_guard<std::mutex> lock(spawnMutex_);
            if (!pendingSpawn_.empty()) {
                DEBUGLOG_WARNING("システム停止時、" + std::to_string(pendingSpawn_.size()) + " 個の保留スポーンをクリア");
                pendingSpawn_.clear();
            }
        }

        ECS_TRACE_CATEGORY(DebugLog::Category::ECS, "新規Spawnが無効化されました");
    }

    /**
     * @brief エンティティの事前予約
     * @param count予約するエンティティの数
     * @details
     * エンティティの事前予約を行うことで、
     * メモリの再割り当てを減らし、パフォーマンスを向上させます。
     */
    void Reserve(size_t count) {
        freeIdsReady_.reserve(count);
        generations_.reserve(count);
    }

    /**
     * @brief 指定されたコンポーネントを持つエンティティの数を取得
     * @tparam T コンポーネントの型
     * @return size_t 指定されたコンポーネントを持つエンティティの数
     */
    template<class T>
    size_t GetComponentCount() const {
        auto it = stores_.find(std::type_index(typeid(T)));
        if (it == stores_.end()) {
            return 0;
        }
        auto* store = static_cast<const Store<T>*>(it->second);
        return store->map.size();
    }

private:
    /**
     * @interface IStore
     * @brief コンポーネント格納用のインターフェース
     */
    struct IStore {
        virtual ~IStore() = default;
        virtual void Erase(Entity) = 0;
    };

    template<class T>
    struct Store : IStore {
        std::unordered_map<uint32_t, std::unique_ptr<T>> map;  ///< EntityID -> コンポーネントインスタンス
        void Erase(Entity e) override { map.erase(e.id); }
    };

    template<class T>
    Store<T>& getStore() {
        auto key = std::type_index(typeid(T));
        auto it = stores_.find(key);
        if (it == stores_.end()) {
            auto* s = new Store<T>();
            stores_[key] = s;
            erasers_.push_back([s](Entity e) { s->Erase(e); });
            return *s;
        }
        return *static_cast<Store<T>*>(it->second);
    }

    // Behaviour登録（原因付き）
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviourWithCause(Entity e, TDerived* obj, Cause cause) {
        behaviours_.push_back({ e, obj, false, cause });
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviourWithCause(Entity, TDerived*, Cause) {}

    // 後方互換（原因Unknownで登録）
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity e, TDerived* obj) {
        registerBehaviourWithCause<TDerived>(e, obj, Cause::Unknown);
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity, TDerived*) {}

    // Behaviourコンポーネントの登録を解除(C++14互換)
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity e, TDerived* obj) {
        behaviours_.erase(
            std::remove_if(behaviours_.begin(), behaviours_.end(),
                [e, obj](const BEntry& entry) { return entry.e == e && entry.b == obj; }),
            behaviours_.end());
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity, TDerived*) {}

    struct BEntry {
        Entity e;           ///< エンティティ
        Behaviour* b;       ///< Behaviourへのポインタ
        bool started = false; ///< OnStartが呼ばれたかどうか
        Cause cause = Cause::Unknown; ///< 事象の原因タグ

        bool operator==(const BEntry& other) const {
            return e == other.e && b == other.b;
        }
    };

    // 内部破棄: 世代インクリメント + フリーIDは次フレームまで保留
    void DestroyEntityInternal(uint32_t id, Cause cause = Cause::Unknown) {
        ECS_TRACE_LOG("エンティティ破棄中 (ID: " + std::to_string(id) + ", 原因=" + CauseToString(cause) + ")");

        // Behaviourリストから該当IDを除去
        size_t behaviourCount = behaviours_.size();
        behaviours_.erase(
            std::remove_if(behaviours_.begin(), behaviours_.end(),
                [id](const BEntry& entry) { return entry.e.id == id; }),
            behaviours_.end());
        size_t removedBehaviours = behaviourCount - behaviours_.size();
        if (removedBehaviours > 0) {
            ECS_TRACE_LOG("エンティティ " + std::to_string(id) + " から " + std::to_string(removedBehaviours) + " 個のビヘイビアを削除");
        }

        // 全コンポーネント削除
        for (auto& er : erasers_) { er(Entity{ id, 0 }); }

        // 生存フラグを削除
        alive_.erase(id);

        // 世代インクリメント（古いハンドル無効化）
        if (id >= generations_.size()) generations_.resize(id + 1, 1);
        generations_[id]++;

        // 再利用は次フレーム以降
        freeIdsPending_.push_back(id);

        // メトリクス
        totalDestroyed_++;
        if (trackFrameAccounting_) { destroyedThisFrame_++; }

        ECS_TRACE_LOG("エンティティ破棄成功 (ID: " + std::to_string(id) + ", 総生存数: " + std::to_string(alive_.size()) + ")");
    }

    uint32_t nextId_ = 0;
    std::vector<uint32_t> freeIdsReady_;
    std::vector<uint32_t> freeIdsPending_;

    std::unordered_set<uint32_t> alive_;
    std::unordered_map<std::type_index, IStore*> stores_;
    std::vector<std::function<void(Entity)>> erasers_;
    std::vector<BEntry> behaviours_;

    std::vector<uint32_t> generations_{1};

    // メトリクス
    uint64_t frameCount_ = 0;
    uint64_t totalCreated_ = 0;
    uint64_t totalDestroyed_ = 0;
    size_t   maxAlive_ = 0;

    // フレーム窓メトリクス
    const uint32_t metricsWindow_ = 1000;
    uint32_t recentCount_ = 0;
    float recentDtSum_ = 0.0f;
    float recentDtMin_ = std::numeric_limits<float>::infinity();
    float recentDtMax_ = 0.0f;
    uint32_t recentCreated_ = 0;
    uint32_t recentDestroyed_ = 0;
    size_t windowAliveStart_ = 0;

    // 今フレームの作成/破棄数
    uint32_t createdThisFrame_ = 0;
    uint32_t destroyedThisFrame_ = 0;

    // フレーム会計フラグ（Tick区間のみtrue）
    bool trackFrameAccounting_ = false;

    // 並行保護（最小限）
    std::mutex entityMutex_;

    // 破棄要求キュー（MPMC: ロックで保護）
    std::vector<std::pair<uint32_t, Cause>> pendingDestroy_;
    std::mutex pendingMutex_;

    // スポーン要求キュー（MPMC: ロックで保護）
    std::vector<std::pair<Cause, std::function<void(Entity)>>> pendingSpawn_;
    std::mutex spawnMutex_;

    // オプション: Update中の生成/破棄禁止
    bool inUpdate_ = false;
    bool enforceNoMutateDuranteUpdate_ = false;

    // システム停止フラグ（新規Spawn無効化）
    bool systemsStopped_ = false;

    friend class EntityBuilder;
};

/**
 * @brief EntityBuilder::With()の実装
 */
template<typename T, typename... Args>
EntityBuilder& EntityBuilder::With(Args&&... args) {
    world_->Add<T>(entity_, std::forward<Args>(args)...);
    return *this;
}

/**
 * @brief EntityBuilder::WithCause()の実装
 * @tparam T 追加するコンポーネントの型
 * @tparam CauseType 原因の型（World::CauseまたはWorld::Cause互換の整数型）
 * @tparam Args コンストラクタ引数の型(可変長)
 * @param[in] cause 事象の原因タグ（World::Causeまたはその基礎型）
 * @param[in] args コンポーネントのコンストラクタに転送する引数
 * @return EntityBuilder& メソッドチェーン用の自身への参照
 */
template<typename T, typename CauseType, typename... Args>
EntityBuilder& EntityBuilder::WithCause(CauseType cause, Args&&... args) {
    world_->AddWithCause<T>(entity_, static_cast<World::Cause>(cause), std::forward<Args>(args)...);
    return *this;
}

