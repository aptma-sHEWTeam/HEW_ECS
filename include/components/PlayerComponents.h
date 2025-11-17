/**
 * @file PlayerComponents.h
 * @brief プレイヤー専用コンポーネント集
 * @author 山内陽
 * @date 2025
 * @version 1.0
 *
 * @details
 * このファイルはプレイヤーキャラクターに関連するコンポーネントを定義します。
 * 移動、ステータス管理などのプレイヤー専用機能を提供します。
 */
#pragma once

#include "components/Component.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "input/InputSystem.h"
#include "input/GamepadSystem.h"
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

// =========================================
// ベロシティ計算コンポーネント
// =========================================

struct PlayerVelocity : Behaviour {
    float speed = 10.0f;                       ///< 移動速度(単位/秒) - 速度を上げて動きを明確に
    DirectX::XMFLOAT2 velocity = {0.0f, 0.0f}; ///< 現在の移動ベロシティ

    void SetVelocity(DirectX::XMFLOAT2 speed)
    {
        velocity = speed;
    }

    void UpdateVelocity(const DirectX::XMFLOAT2 &inputDir) {
        if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
            float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            if (length > 0.0f) {
                float normal_x = inputDir.x / length;
                float normal_y = inputDir.y / length;
                velocity.x = normal_x * speed;
                velocity.y = normal_y * speed;
            }
        }
    }

    DirectX::XMFLOAT2 GetVelocity() { return velocity; }

    float GetVelocitySqrted() {
        float l = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        return l;
    }
};

// ========================================================
// プレイヤー移動コンポーネント
// ========================================================

/**
 * @struct PlayerMovement
 * @brief プレイヤーの移動を管理するBehaviour
 *
 * @details
 * スティック入力に基づいてプレイヤーの移動を制御します。
 * スティックを倒している間はその方向に移動し、ニュートラルに戻った際には最後に入力された方向に基づいて慣性（ベロシティ）を与えます。
 * また、画面外に出ないように自動的に境界制限を行います。
 *
 * @par 使用例
 * @code
 * Entity player = world.Create()
 * .With<Transform>()
 * .With<MeshRenderer>()
 * .With<PlayerTag>()
 * .Build();
 *
 * auto& movement = world.Add<PlayerMovement>(player);
 * movement.input_ = &GetInput();
 * movement.speed =8.0f;
 * @endcode
 *
 * @note InputSystemへの参照を設定する必要があります
 * @see InputSystem
 */
struct PlayerMovement : Behaviour {
    InputSystem *input_ = nullptr;     ///< 入力システムへのポインタ
    GamepadSystem *gamepad_ = nullptr; ///< ゲームパッドシステムへのポインタ
    // チャージ挙動設定
    float minChargeSpeedFactor = 0.2f;   ///< チャージ中の最低速度係数(0.0-1.0)
    float chargeMaxTime = 0.7f;          ///< チャージ最大時間(秒)

    // 入力モード
    bool flickOnly = true;               ///< 左スティックの通常移動を無効化し、はじく移動（チャージ&リリース）のみ有効にする

    // 内部状態
    bool isCharging_ = false;            ///< 現在チャージ中か
    DirectX::XMFLOAT2 lastStickDir_ {0.0f, 0.0f}; ///< 直近の左スティック方向(正規化)
    bool wasCharging_ = false;           ///< 前フレームでチャージしていたか(ローカル検出)

    /**
     * @brief 毎フレーム更新処理
     * @param[in,out] w ワールド参照
     * @param[in] self このコンポーネントが付いているエンティティ
     * @param[in] dt デルタタイム(前フレームからの経過時間)
     *
     * @details
     * スティック入力を読み取り、プレイヤーの位置とベロシティを更新します。
     * キーボード入力とすべての接続されているゲームパッド（XInput + DirectInput）の入力を統合します。
     * 入力がない場合は最後のベロシティに基づいて移動を続けます。
     */
    void OnUpdate(World &w, Entity self, float dt) override {
        auto *t = w.TryGet<Transform>(self);
        auto *v = w.TryGet<PlayerVelocity>(self);

        MeshRenderer renderer;
        renderer.meshType = MeshType::Cone;

        if (!t || !v || (!input_ && !gamepad_))
            return;

        DirectX::XMFLOAT2 inputDir = {0.0f, 0.0f};

        // キーボード入力の処理
        if (input_) {
            if (input_->GetKey('W') || input_->GetKey(VK_UP)) {
                inputDir.y += 1.0f;
            }
            if (input_->GetKey('S') || input_->GetKey(VK_DOWN)) {
                inputDir.y -= 1.0f;
            }
            if (input_->GetKey('A') || input_->GetKey(VK_LEFT)) {
                inputDir.x -= 1.0f;
            }
            if (input_->GetKey('D') || input_->GetKey(VK_RIGHT)) {
                inputDir.x += 1.0f;
            }
        }

        // すべての接続されているゲームパッドの入力を統合（XInput + DirectInput）
        float slowFactor = 1.0f; // このフレームの速度係数（チャージ中は低下)
        if (gamepad_)
        {
            float gx = gamepad_->GetLeftStickX();
            float gy = gamepad_->GetLeftStickY();
            float mag = std::sqrt(gx * gx + gy * gy);

            // 方向キャッシュ（常時）
            if (mag > 1e-5f) {
                lastStickDir_.x = -(gx / mag);
                lastStickDir_.y = -(gy / mag);
            }

            // ローカルしきい値によるチャージ/リリース検出（GamepadSystemのフォールバック）
            const float releaseThreshold = 0.5f;
            bool chargingNowLocal = (mag > releaseThreshold);

            // チャージ状態更新（統合）
            bool chargingSys = gamepad_->IsLeftStickCharging();
            bool effectiveCharging = chargingSys && chargingNowLocal;
            if (effectiveCharging)
            {
                isCharging_ = true;

                float charge = gamepad_->GetLeftStickChargeAmount(chargeMaxTime); // 0..1
                slowFactor = std::max(minChargeSpeedFactor, 1.0f - charge);
            }

            // リリースで方向転換＋通常速度に復帰（統合: システム検出 or ローカル検出）
            bool releasedSys = gamepad_->IsLeftStickReleased();
            bool releasedLocal = (wasCharging_ && !chargingNowLocal);

            if (releasedSys || releasedLocal)
            {
                float dirLen = std::sqrt(lastStickDir_.x * lastStickDir_.x + lastStickDir_.y * lastStickDir_.y);
                if (dirLen > 1e-5f)
                {
                    v->velocity.x = (lastStickDir_.x / dirLen) * v->speed;
                    v->velocity.y = (lastStickDir_.y / dirLen) * v->speed;
                    float yawRad = std::atan2(v->velocity.y, v->velocity.x);
                    t->rotation.y = yawRad * (180.0f / 3.1415926535f);

                isCharging_ = false;
                slowFactor = 1.0f;
                }
            }

            // 次フレーム用にローカル状態を保持
            wasCharging_ = chargingNowLocal;

            if (!flickOnly)
            {
                inputDir.x += -gx;
                inputDir.y += -gy;
            }
        }

        if (inputDir.x != 0.0f || inputDir.y != 0.0f)
        {
            v->UpdateVelocity(inputDir);
        }

        t->position.x += v->velocity.x * dt * slowFactor;
        t->position.z += v->velocity.y * dt * slowFactor;

        const float limitX = 9.0f;
        const float limitY = 9.5f;
        if (t->position.x < -limitX) t->position.x = -limitX;
        if (t->position.x > limitX)  t->position.x =  limitX;
        if (t->position.z < -limitY) t->position.z = -limitY;
        if (t->position.z > limitY)  t->position.z =  limitY;
    }

    float CalcMoveRotation()
    {
        return std::atan2f(lastStickDir_.y, lastStickDir_.x) * (180.0f / 3.1415926535f);
    }
};

// ========================================================
// プレイヤーガイド表示コンポーネント
// ========================================================

/**
 * @struct PlayerGuide
 * @brief プレイヤーガイド表示をするBehaviour
 *
 * @details
 * 新たにガイド用のエンティティを追加します。
 * 追加したエンティティはPlayerMoveコンポーネントで取得できる、チャージ状態の有無で表示可否を行います。
 * チャージ中は進行予測方向にガイドが表示されるようにしています。 *
 *
 * @par 使用例
 * @code
 * Entity player = world.Create()
 * .With<Transform>()
 * .With<MeshRenderer>()
 * .With<PlayerTag>()
 * .Build();
 *
 * auto& guide = world.Add<PlayerGuide>(player);
 * @endcode
 */
struct PlayerGuide : Behaviour
{
    // コンポーネント保存用変数
    PlayerMovement *playerMove{};
    Transform *selfTransform{};
    Transform *guidTransform{};
    Entity guidEntity{};

    /**
    * @brief ガイドオブジェクト作成
    * @param[in,out] world ワールド参照
    * @param[in] position 生成する座標
    *
    * @details
    * 指定した座標に新たなエンティティを追加し、オブジェクトを描画します。
    */
    void Create(World &world, const DirectX::XMFLOAT3 &position)
    {
        // 座標初期化
        Transform t{position, {0, 0, 0}, {0, 0, 0}};

        // レンダラー初期化
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{1, 0, 0};

        // コンポーネント追加
        guidEntity = world.Create()
                         .With<Transform>(t)
                         .With<MeshRenderer>(renderer)
                         .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                         .Build();
    };

    /**
    * @brief 初期化処理
    * @param[in,out] w ワールド参照
    * @param[in] self このコンポーネントが付いているエンティティ
    *
    * @details
    * selfエンティティからTransformコンポーネントを取得し、その座標にガイドを作成しています。
    */
    void OnStart(World &w, Entity self) override
    {
        // コンポーネントの取得
        selfTransform = w.TryGet<Transform>(self);  // エンティティ(プレイヤー)の移動情報

        // ガイド作成
        Create(w, selfTransform->position);
    }

    /**
     * @brief 毎フレーム更新処理
     * @param[in,out] w ワールド参照
     * @param[in] self このコンポーネントが付いているエンティティ
     * @param[in] dt デルタタイム(前フレームからの経過時間)
     *
     * @details
     * 各コンポーネントを取得し、ガイドの方向、場所を指定します。
     * また、チャージ状態でない場合は、スケールを0にして非表示にしています。
     */
    void OnUpdate(World &w, Entity self, float dt) override
    {
        // 各コンポーネントの取得
        playerMove    = w.TryGet<PlayerMovement>(self);         // エンティティ(プレイヤー)の移動情報
        selfTransform = w.TryGet<Transform>(self);              // エンティティ(プレイヤー)の座標情報
        guidTransform = w.TryGet<Transform>(guidEntity);        // エンティティ(ガイド)の座標情報

        // ガイドの位置をプレイヤーの位置と同期(代入)
        guidTransform->position = selfTransform->position;

        // プレイヤーと同じように進行方向に回転させる
        float rad = std::atan2f(playerMove->lastStickDir_.y, playerMove->lastStickDir_.x);
        guidTransform->rotation.y = -rad * (180.0f / 3.1415926535f); // deg(度)変換

        // チャージ状態の判別処理
        if (!playerMove->isCharging_)
        {
            guidTransform->scale = {0, 0, 0};   // チャージしてないときはガイドの大きさを0にする
        }
        else
        {
            guidTransform->scale = {2, 1, 0.1};   // チャージ中はガイドの大きさを1にする

            // (x,y) = (cosΘ, sinΘ)
            guidTransform->position.x += std::cosf(rad) * 3;
            guidTransform->position.z += std::sinf(rad) * 3;
        }
    }
};
