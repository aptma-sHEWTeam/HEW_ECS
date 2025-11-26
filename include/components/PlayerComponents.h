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
#include "components/Collision.h"
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

#include "config/ConfigVar.h"

// =========================================
// 定数定義
// =========================================
namespace PlayerConstants {
    constexpr int ANGLE_HISTORY_SIZE = 30;
    constexpr float EPSILON = 1e-5f;
}

// =========================================
// ベロシティ計算コンポーネント
// =========================================

struct PlayerVelocity : Behaviour {
    inline static ConfigVar<float> cfg_Speed{"Player", "MoveSpeed", 8.0f};
    float speed = cfg_Speed;                       ///< 移動速度(単位/秒) - 速度を上げて動きを明確に
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
    CollisionSphere *collision_ = nullptr;
    // Config Variables
    inline static ConfigVar<float> cfg_MinChargeSpeed{"Player", "MinChargeSpeedFactor", 0.4f};
    inline static ConfigVar<float> cfg_ChargeMaxTime{"Player", "ChargeMaxTime", 0.7f};
    inline static ConfigVar<float> cfg_ReleaseThreshold{"Player", "ReleaseThreshold", 0.3f};
    inline static ConfigVar<float> cfg_LimitX{"Player", "LimitX", 15.0f};
    inline static ConfigVar<float> cfg_LimitY{"Player", "LimitY", 15.0f};
    inline static ConfigVar<float> cfg_ChargeMoveAmount{"Player", "ChargeMoveAmount", 0.025};

    // チャージ挙動設定
    float minChargeSpeedFactor = cfg_MinChargeSpeed;   ///< チャージ中の最低速度係数(0.0-1.0)
    float chargeMaxTime = cfg_ChargeMaxTime;          ///< チャージ最大時間(秒)

    // 入力モード
    bool flickOnly = true;               ///< 左スティックの通常移動を無効化し、はじく移動（チャージ&リリース）のみ有効にする

    // 内部状態
    bool isCharging_ = false;            ///< 現在チャージ中か
    DirectX::XMFLOAT2 lastStickDir_ {0.0f, 0.0f}; ///< 直近の左スティック方向(正規化)
    bool wasCharging_ = false;           ///< 前フレームでチャージしていたか(ローカル検出)

      //角度履歴
    float angleHistory[PlayerConstants::ANGLE_HISTORY_SIZE] = {};
    int angleIndex = 0;
    bool angleFilled = false;
    
    // 角度平均計算用の累積値（最適化）
    float sumSin = 0.0f;
    float sumCos = 0.0f;

    int frame = 0;
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

        // Sync Config
        v->speed = PlayerVelocity::cfg_Speed;
        minChargeSpeedFactor = cfg_MinChargeSpeed;
        chargeMaxTime = cfg_ChargeMaxTime;

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
            if (mag > PlayerConstants::EPSILON) {
                lastStickDir_.x = -(gx / mag);
                lastStickDir_.y = -(gy / mag);
            }

            //角度履歴
            float ang = std::atan2f(lastStickDir_.y, lastStickDir_.x);
            
            // 古い角度の寄与を削除（リングバッファの上書き時）
            if (angleFilled) {
                sumSin -= std::sinf(angleHistory[angleIndex]);
                sumCos -= std::cosf(angleHistory[angleIndex]);
            }
            
            // 新しい角度を保存し、累積値に加算
            angleHistory[angleIndex] = ang;
            sumSin += std::sinf(ang);
            sumCos += std::cosf(ang);
            
            angleIndex = (angleIndex + 1) % 30;             //現在の保存位置
            if (angleIndex == 0)                            //30フレーム埋まったら true
            {
                angleFilled = true;
            }

            // ローカルしきい値によるチャージ/リリース検出（GamepadSystemのフォールバック）
            const float releaseThreshold = cfg_ReleaseThreshold;
            bool chargingNowLocal = (mag > releaseThreshold);

            // チャージ状態更新（統合）
            bool chargingSys = gamepad_->IsLeftStickCharging();
            bool effectiveCharging = chargingSys && chargingNowLocal;
            frame++;
            if (effectiveCharging)
            {
                isCharging_ = true;
                collision_->radius * 0.01f;            //< 当たり判定変更
                float charge = gamepad_->GetLeftStickChargeAmount(chargeMaxTime); // 0..1
                slowFactor = std::max(minChargeSpeedFactor, 1.0f - charge);
                
                static float moveAmount = cfg_ChargeMoveAmount; //< チャージ中Xの移動量
                
                //< 2フレーム
                if (frame % 2 == 0)
                {
                    t->position.x += moveAmount;

                    if (t->position.x >= moveAmount || t->position.x <= -moveAmount)
                    {
                        moveAmount = -moveAmount;
                    }
                }
            }

            // リリースで方向転換＋通常速度に復帰（統合: システム検出 or ローカル検出）
            bool releasedSys = gamepad_->IsLeftStickReleased();
            bool releasedLocal = (wasCharging_ && !chargingNowLocal);
          
            if (releasedSys || releasedLocal)
            {
                //三項演算子     angleFilled = trueの時 count = 30,falseの時 count = angleIndex
                int count = angleFilled ? PlayerConstants::ANGLE_HISTORY_SIZE : angleIndex;

                // 累積値を使って平均角度を計算（ループ不要）
                float avgRad = std::atan2f(sumSin / count, sumCos / count);

                // 平均角度ベクトル化
                float dirX = std::cosf(avgRad);
                float dirY = std::sinf(avgRad);

               //プレイヤーの速度に平均角度を乗算
                float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
                if (dirLen > PlayerConstants::EPSILON)
                {
                    v->velocity.x = (dirX / dirLen) * v->speed;
                    v->velocity.y = (dirY / dirLen) * v->speed;

                    float yawRad = std::atan2(v->velocity.y, v->velocity.x);
                    t->rotation.y = yawRad * (180.0f / DirectX::XM_PI);

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

        const float limitX = cfg_LimitX;
        const float limitY = cfg_LimitY;
        if (t->position.x < -limitX) t->position.x = -limitX;
        if (t->position.x > limitX)  t->position.x =  limitX;
        if (t->position.z < -limitY) t->position.z = -limitY;
        if (t->position.z > limitY)  t->position.z =  limitY;
    }
    float CalcMoveRotation()
    {
        return std::atan2f(lastStickDir_.y, lastStickDir_.x) * (180.0f / DirectX::XM_PI);
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

    // Config Variables
    inline static ConfigVar<float> cfg_GuideScaleX{"Player", "GuideScaleX", 2.5f};
    inline static ConfigVar<float> cfg_GuideScaleY{"Player", "GuideScaleY", 1.0f};
    inline static ConfigVar<float> cfg_GuideScaleZ{"Player", "GuideScaleZ", 0.1f};
    inline static ConfigVar<float> cfg_GuideOffsetDistance{"Player", "GuideOffsetDistance", 2.0f};

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
        guidTransform->rotation.y = -rad * (180.0f / DirectX::XM_PI); // deg(度)変換

        // チャージ状態の判別処理
        if (!playerMove->isCharging_)
        {
            guidTransform->scale = {0, 0, 0};   // チャージしてないときはガイドの大きさを0にする
        }
        else
        {
            guidTransform->scale = {cfg_GuideScaleX.Get(), cfg_GuideScaleY.Get(), cfg_GuideScaleZ.Get()};   // チャージ中はガイドの大きさを1にする

            // (x,y) = (cosΘ, sinΘ)
            guidTransform->position.x += std::cosf(rad) * cfg_GuideOffsetDistance.Get();
            guidTransform->position.z += std::sinf(rad) * cfg_GuideOffsetDistance.Get();
        }
    }
};
