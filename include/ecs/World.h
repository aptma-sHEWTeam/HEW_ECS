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
     * @tparam T 追加するコンポーネント型
     * @tparam Args コンストラクタ引数型
     * @param[in] args コンストラクタ引数
     * @return EntityBuilder& 自身
     */
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args);

    /**
     * @brief メソッドチェーンでコンポーネントを追加（原因付き）
     * @tparam T 追加するコンポーネント型
     * @tparam CauseType 原因タグ型
     * @tparam Args コンストラクタ引数型
     * @param[in] cause 追加起因
     * @param[in] args コンストラクタ引数
     * @return EntityBuilder& 自身
     */
    template<typename T, typename CauseType, typename... Args>
    EntityBuilder& WithCause(CauseType cause, Args&&... args);

    /**
     * @brief エンティティ確定
     * @return Entity 作成結果
     */
    Entity Build() { return entity_; }

    /**
     * @brief 暗黙変換
     * @return Entity 作成結果
     */
    operator Entity() const { return entity_; }

private:
    World* world_;    ///< Worldインスタンス
    Entity entity_;   ///< 作成エンティティ
};

/**
 * @class World
 * @brief ECSワールド管理クラス
 * @details 全エンティティ/コンポーネント/Behaviourの生成・破棄・更新を統括します。
 */
class World {
public:
    enum class Cause {
        Unknown = 0,
        Spawner,
        WaveTimer,
        Collision,
        LifetimeExpired,
        StageReset,
        SceneInit,
        SceneTeardown,
        SceneUnload,
        AppShutdown
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
     * @brief 生存エンティティ数取得
     * @return size_t 生存数
     */
    size_t GetAliveCount() const {
        return alive_.size();
    }

    /**
     * @brief 総エンティティ数取得（現在は生存集合と同義）
     * @return size_t 総数
     */
    size_t GetEntityCount() const {
        return alive_.size();
    }

    /**
     * @brief デストラクタ
     * @details 保留破棄反映後に全ストア解放
     */
    ~World() {
        ECS_TRACE_LOG("World::~World() - World破棄中");
        ECS_TRACE_LOG("アクティブエンティティ: " + std::to_string(alive_.size()));
        ECS_TRACE_LOG("アクティブビヘイビア: " + std::to_string(behaviours_.size()));
        FlushDestroyEndOfFrame();
        if (!alive_.empty()) {
            DEBUGLOG_WARNING(std::to_string(alive_.size()) + " 個の残存エンティティを強制破棄 (原因=AppShutdown)");
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
        ClearQueryCache();
    }

    /**
     * @brief エンティティ作成（原因Unknown）
     * @return Entity 新規エンティティ
     */
    Entity CreateEntity() { return CreateEntityWithCause(Cause::Unknown); }

    /**
     * @brief エンティティ作成（原因付き）
     * @param cause 作成起因
     * @return Entity 新規エンティティ
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
        ClearQueryCache(); // エンティティ作成時にキャッシュをクリア

        // メトリクス更新
        totalCreated_++;
        if (trackFrameAccounting_) { createdThisFrame_++; }
        if (alive_.size() > maxAlive_) maxAlive_ = alive_.size();

        return Entity{ id, generations_[id] };
    }

    /**
     * @brief スレッドセーフにスポーン要求をキュー
     * @param cause 起因タグ
     * @param onCreated 生成後コールバック（メインスレッド実行）
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
     * @brief ビルダーパターン開始
     * @return EntityBuilder ビルダー
     */
    EntityBuilder Create() {
        return EntityBuilder(this, CreateEntity());
    }

    /**
     * @brief 生存判定
     * @param e 対象
     * @return bool 生存ならtrue
     */
    bool IsAlive(Entity e) const {
        // IDが生存かつ世代一致
        if (alive_.count(e.id) == 0) return false;
        if (e.id >= generations_.size()) return false;
        return generations_[e.id] == e.gen;
    }

    /**
     * @brief エンティティ破棄要求（原因Unknown）
     * @param e 対象
     */
    void DestroyEntity(Entity e) { DestroyEntityWithCause(e, Cause::Unknown); }

    /**
     * @brief エンティティ破棄要求（原因付き）
     * @param e 対象エンティティ
     * @param cause 起因タグ
     */
    void DestroyEntityWithCause(Entity e, Cause cause) {
        if (!IsAlive(e)) {
            DEBUGLOG_WARNING("既に死亡/無効なエンティティの破棄を試行 (ID: " + std::to_string(e.id) + ", gen: " + std::to_string(e.gen) + ")");
            return;
        }
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingDestroy_.push_back({ e.id, cause });
        }
        ECS_TRACE_LOG(std::string("破棄をキューに追加 (ID: ") + std::to_string(e.id) + ", 原因=" + CauseToString(cause) + ")");
    }

    /**
     * @brief コンポーネント追加（原因Unknown）
     * @tparam T 追加型
     * @tparam Args 引数型
     * @param e 対象
     * @param args コンストラクタ引数
     * @return T& 追加結果参照
     */
    template<class T, class...Args>
    T& Add(Entity e, Args&&...args) {
        return AddWithCause<T>(e, Cause::Unknown, std::forward<Args>(args)...);
    }

    /**
     * @brief コンポーネント追加（原因付き）
     * @tparam T 追加型
     * @tparam Args 引数型
     * @param e 対象エンティティ
     * @param cause 起因タグ
     * @param args コンストラクタ引数
     * @return T& 追加結果参照
     * @throws std::runtime_error 無効エンティティ / 重複（DEBUG）
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
        ClearQueryCache();
        ECS_TRACE_LOG("コンポーネント " + std::string(typeid(T).name()) + " をエンティティ " + std::to_string(e.id) + " に追加");
        return ref;
    }

    /**
     * @brief コンポーネント削除
     * @tparam T 削除型
     * @param e 対象
     * @return bool 成功ならtrue
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
        unregisterBehaviour<T>(e, it->second.get());
        s->map.erase(it);
        ClearQueryCache();
        ECS_TRACE_LOG("コンポーネント " + std::string(typeid(T).name()) + " をエンティティ " + std::to_string(e.id) + " から削除");
        return true;
    }

    /**
     * @brief コンポーネント存在判定
     * @tparam T 型
     * @param e 対象
     * @return bool 所持ならtrue
     */
    template<class T>
    bool Has(Entity e) const {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return false;
        auto* s = static_cast<const Store<T>*>(itS->second);
        return s->map.find(e.id) != s->map.end();
    }

    /**
     * @brief コンポーネント取得（存在しない場合nullptr）
     * @tparam T 型
     * @param e 対象
     * @return T* ポインタ
     */
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

    /**
     * @brief コンポーネント取得（const版 / 無ければnullptr）
     * @tparam T 型
     * @param e 対象
     * @return const T* ポインタ
     */
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

    /**
     * @brief コンポーネント参照取得（存在必須）
     * @tparam T 型
     * @param e 対象
     * @return T& 参照
     * @throws std::runtime_error 未所持
     */
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

    /**
     * @brief コンポーネント参照取得（const版 / 存在必須）
     * @tparam T 型
     * @param e 対象
     * @return const T& 参照
     * @throws std::runtime_error 未所持
     */
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

    /**
     * @brief 単一コンポーネント全走査
     * @tparam T 対象型
     * @tparam F 関数 (Entity, T&)
     * @param fn 適用関数
     */
    template<class T, class F>
    void ForEach(F&& fn) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return;
        auto* s = static_cast<Store<T>*>(itS->second);
        std::vector<uint32_t> ids;
        ids.reserve(s->map.size());
        for (const auto& pair : s->map) {
            ids.push_back(pair.first);
        }
        for (uint32_t id : ids) {
            if (alive_.count(id) == 0) continue;
            auto it = s->map.find(id);
            if (it != s->map.end()) {
                fn(Entity{id, generations_[id]}, *it->second);
            }
        }
    }

    /**
     * @brief 2コンポーネント同時走査
     * @tparam T1 主走査型
     * @tparam T2 追加条件型
     * @tparam F 関数 (Entity, T1&, T2&)
     * @param fn 適用関数
     */
    template<class T1, class T2, class F>
    void ForEach(F&& fn) {
        auto itS1 = stores_.find(std::type_index(typeid(T1)));
        if (itS1 == stores_.end()) return;
        auto* s1 = static_cast<Store<T1>*>(itS1->second);
        std::vector<uint32_t> ids;
        ids.reserve(s1->map.size());
        for (const auto& pair : s1->map) {
            ids.push_back(pair.first);
        }
        for (uint32_t id : ids) {
            if (alive_.count(id) == 0) continue;
            auto it1 = s1->map.find(id);
            if (it1 == s1->map.end()) continue;
            T2* comp2 = TryGet<T2>(Entity{ id, generations_[id] });
            if (comp2) {
                fn(Entity{ id, generations_[id] }, *it1->second, *comp2);
            }
        }
    }

    /**
     * @brief 指定コンポーネント集合を全て持つエンティティ列取得（キャッシュ）
     * @tparam Components 可変長コンポーネント型列
     * @return std::vector<Entity> 結果
     */
    template<typename... Components>
    std::vector<Entity> GetEntitiesWith() {
        std::vector<std::type_index> queryKey = {std::type_index(typeid(Components))...};
        std::sort(queryKey.begin(), queryKey.end());
        if (m_queryCache.count(queryKey)) {
            return m_queryCache[queryKey];
        }
        std::vector<Entity> resultEntities;
        if (sizeof...(Components) > 0) {
            for (uint32_t id : alive_) {
                Entity e = {id, generations_[id]};
                if (!IsAlive(e)) continue;
                bool hasAll = true;
                ([&] {
                    if (!Has<Components>(e)) {
                        hasAll = false;
                    }
                }(), ...);
                if (hasAll) {
                    resultEntities.push_back(e);
                }
            }
        }
        m_queryCache[queryKey] = resultEntities; return resultEntities;
    }

    /**
     * @brief すべてのBehaviour更新
     * @param dt 経過秒
     */
    void Tick(float dt) {
#ifdef _DEBUG
        DebugLog::GetInstance().SetFrame(frameCount_ + 1);
#endif
        if (dt < 0.0f) {
            DEBUGLOG_WARNING("World::Tickで負のdeltaTimeを検出: " + std::to_string(dt));
            dt = 0.0f;
        }
        if (dt > 1.0f) {
            DEBUGLOG_WARNING("World::Tickで非常に大きいdeltaTimeを検出: " + std::to_string(dt) + "s");
        }
        createdThisFrame_ = 0;
        destroyedThisFrame_ = 0;
        windowAliveStart_ = alive_.size();
        trackFrameAccounting_ = true;
        FlushSpawnStartOfFrame();
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
        for (size_t i = 0; i < behaviours_.size(); ) {
            if (i >= behaviours_.size()) break;
            auto& entry = behaviours_[i];
            Entity currentEntity = entry.e;
            Behaviour* currentBehaviour = entry.b;
            if (!IsAlive(currentEntity)) {
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
        FlushDestroyEndOfFrame();
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
        size_t expectedAlive = windowAliveStart_ + createdThisFrame_ - destroyedThisFrame_;
        if (alive_.size() != expectedAlive) {
            DEBUGLOG_WARNING("メトリクス不一致: alive=" + std::to_string(alive_.size()) +
                             ", expected=" + std::to_string(expectedAlive) +
                             ", startAlive=" + std::to_string(windowAliveStart_) +
                             ", createdThisFrame=" + std::to_string(createdThisFrame_) +
                             ", destroyedThisFrame=" + std::to_string(destroyedThisFrame_));
        }
        if (!freeIdsPending_.empty()) {
            freeIdsReady_.insert(freeIdsReady_.end(), freeIdsPending_.begin(), freeIdsPending_.end());
            freeIdsPending_.clear();
        }
        if (recentCount_ >= metricsWindow_) {
            float avg = (recentCount_ > 0) ? (recentDtSum_ / recentCount_) : 0.0f;
            ECS_TRACE_LOG("メトリクス: frames=" + std::to_string(metricsWindow_) +
                     ", dt(avg/min/max)=" + std::to_string(avg) + "/" + std::to_string(recentDtMin_) + "/" + std::to_string(recentDtMax_) +
                     ", created=" + std::to_string(recentCreated_) +
                     ", destroyed=" + std::to_string(recentDestroyed_) +
                     ", maxAlive=" + std::to_string(maxAlive_) +
                     ", aliveNow=" + std::to_string(alive_.size())
            );
            recentDtSum_ = 0.0f;
            recentDtMin_ = std::numeric_limits<float>::infinity();
            recentDtMax_ = 0.0f;
            recentCount_ = 0;
            recentCreated_ = 0;
            recentDestroyed_ = 0;
        }
        recentCreated_ += createdThisFrame_;
        recentDestroyed_ += destroyedThisFrame_;
        trackFrameAccounting_ = false;
        frameCount_++;
    }

    /**
     * @brief 破棄要求キュー反映（フレーム終端）
     */
    void FlushDestroyEndOfFrame() {
        std::vector<std::pair<uint32_t, Cause>> toDestroy;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            if (pendingDestroy_.empty()) return;
            toDestroy.swap(pendingDestroy_);
        }
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
     * @brief スポーン要求キュー反映（フレーム開始）
     */
    void FlushSpawnStartOfFrame() {
        if (systemsStopped_) {
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

    /**
     * @brief Update中の生成/破棄禁止フラグ設定
     * @param en 有効/無効
     */
    void SetEnforceNoMutateDuranteUpdate(bool en) { enforceNoMutateDuranteUpdate_ = en; }

    /**
     * @brief 全システム停止（新規スポーン無効）
     */
    void StopAllSystems() {
        if (systemsStopped_) return;
        ECS_TRACE_CATEGORY(DebugLog::Category::ECS, "World::StopAllSystems() - すべてのシステムを停止");
        systemsStopped_ = true;
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
     * @brief エンティティID予約
     * @param count 予約数
     */
    void Reserve(size_t count) {
        freeIdsReady_.reserve(count);
        generations_.reserve(count);
    }

    /**
     * @brief 指定コンポーネント数取得
     * @tparam T 型
     * @return size_t 保持数
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

    /**
     * @brief 登録Behaviour数取得
     * @return size_t 数
     */
    size_t GetBehaviourCount() const { return behaviours_.size(); }

private:
    struct IStore {
        virtual ~IStore() = default;
        virtual void Erase(Entity) = 0;
    };

    template<class T>
    struct Store : IStore {
        std::unordered_map<uint32_t, std::unique_ptr<T>> map;
        void Erase(Entity e) override { map.erase(e.id); }
    };

    /**
     * @brief コンポーネントストア取得（必要なら生成）
     * @tparam T 型
     * @return Store<T>& ストア参照
     */
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

    /**
     * @brief Behaviour登録（原因付き）
     * @tparam TDerived Behaviour派生型
     * @param e 対象エンティティ
     * @param obj インスタンス
     * @param cause 起因タグ
     */
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviourWithCause(Entity e, TDerived* obj, Cause cause) {
        behaviours_.push_back({ e, obj, false, cause });
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviourWithCause(Entity, TDerived*, Cause) {}

    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity e, TDerived* obj) {
        registerBehaviourWithCause<TDerived>(e, obj, Cause::Unknown);
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity, TDerived*) {}

    /**
     * @brief Behaviour登録解除
     * @tparam TDerived Behaviour派生型
     * @param e 対象
     * @param obj インスタンス
     */
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
        Entity e;
        Behaviour* b;
        bool started = false;
        Cause cause = Cause::Unknown;
        bool operator==(const BEntry& other) const {
            return e == other.e && b == other.b;
        }
    };

    /**
     * @brief 内部破棄処理（即時コンポーネント消去 + 世代更新）
     * @param id エンティティID
     * @param cause 起因
     */
    void DestroyEntityInternal(uint32_t id, Cause cause = Cause::Unknown) {
        ECS_TRACE_LOG("エンティティ破棄中 (ID: " + std::to_string(id) + ", 原因=" + CauseToString(cause) + ")");
        size_t behaviourCount = behaviours_.size();
        behaviours_.erase(
            std::remove_if(behaviours_.begin(), behaviours_.end(),
                [id](const BEntry& entry) { return entry.e.id == id; }),
            behaviours_.end());
        size_t removedBehaviours = behaviourCount - behaviours_.size();
        if (removedBehaviours > 0) {
            ECS_TRACE_LOG("エンティティ " + std::to_string(id) + " から " + std::to_string(removedBehaviours) + " 個のビヘイビアを削除");
        }
        for (auto& er : erasers_) { er(Entity{ id, 0 }); }
        alive_.erase(id);
        if (id >= generations_.size()) generations_.resize(id + 1, 1);
        generations_[id]++;
        freeIdsPending_.push_back(id);
        totalDestroyed_++;
        if (trackFrameAccounting_) { destroyedThisFrame_++; }
        ClearQueryCache();
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
    uint64_t frameCount_ = 0;
    uint64_t totalCreated_ = 0;
    uint64_t totalDestroyed_ = 0;
    size_t   maxAlive_ = 0;
    const uint32_t metricsWindow_ = 1000;
    uint32_t recentCount_ = 0;
    float recentDtSum_ = 0.0f;
    float recentDtMin_ = std::numeric_limits<float>::infinity();
    float recentDtMax_ = 0.0f;
    uint32_t recentCreated_ = 0;
    uint32_t recentDestroyed_ = 0;
    size_t windowAliveStart_ = 0;
    uint32_t createdThisFrame_ = 0;
    uint32_t destroyedThisFrame_ = 0;
    bool trackFrameAccounting_ = false;
    std::mutex entityMutex_;
    std::vector<std::pair<uint32_t, Cause>> pendingDestroy_;
    std::mutex pendingMutex_;
    std::vector<std::pair<Cause, std::function<void(Entity)>>> pendingSpawn_;
    std::mutex spawnMutex_;
    bool inUpdate_ = false;
    bool enforceNoMutateDuranteUpdate_ = false;
    bool systemsStopped_ = false;

    /**
     * @brief クエリキャッシュ全消去
     */
    void ClearQueryCache() {
        m_queryCache.clear();
        ECS_TRACE_LOG("クエリキャッシュをクリアしました。");
    }

    struct QueryKeyHash {
        size_t operator()(const std::vector<std::type_index>& key) const {
            size_t seed = 0;
            for (const auto& ti : key) {
                seed ^= ti.hash_code() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    std::unordered_map<std::vector<std::type_index>, std::vector<Entity>, QueryKeyHash> m_queryCache;

    friend class EntityBuilder;
};

/**
 * @brief EntityBuilder::With()の実装
 * @tparam T コンポーネント型
 * @tparam Args 引数型
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

// Model用の特殊化宣言（実装はWorld.cppにある）
template<>
EntityBuilder& EntityBuilder::With<Model>(std::string&& filePath);

