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

//ConfigVar
inline static ConfigVar<float> cfg_MinChargeSpeed{"Player", "MinChargeSpeedFactor", 0.4f};
inline static ConfigVar<float> cfg_ChargeMaxTime{"Player", "ChargeMaxTime", 0.7f};
inline static ConfigVar<float> cfg_ReleaseThreshold{"Player", "ReleaseThreshold", 0.3f};
inline static ConfigVar<float> cfg_ChargeMoveAmount{"Player", "ChargeMoveAmount", 0.025f};
inline static ConfigVar<float> cfg_LimitX{"Player", "LimitX", 15.0f};
inline static ConfigVar<float> cfg_LimitY{"Player", "LimitY", 15.0f};
inline static ConfigVar<float> cfg_AccelerateMagnification{"Player", "AccelerateMagnification", 1.5f};

// =========================================
// 定数定義
// =========================================
namespace PlayerConstants {
constexpr int ANGLE_HISTORY_SIZE = 30;
constexpr float EPSILON = 1e-5f;
} // namespace PlayerConstants

// =========================================
// ベロシティ計算コンポーネント
// =========================================

struct PlayerVelocity : Behaviour {
    inline static ConfigVar<float> cfg_Speed{"Player", "MoveSpeed", 8.0f};

    float Acceleration = cfg_AccelerateMagnification; ///< 加速度の倍率
    float MinSpeed = cfg_MinChargeSpeed;              ///< 減速時の最小速度
    float speed = cfg_Speed;                          ///< 移動速度(単位/秒)
    float SlowFactor = 1.0f;                          ///< 毎フレームの減速量
    bool isBoosting = false;                          ///< 加速中か判定
    bool isDecelerating = false;                      ///< 減速中か判定
    float DebugSpeed = 0.0f;                          ///< デバッグ用

    DirectX::XMFLOAT2 velocity = {0.0f, 0.0f}; ///< 現在の移動ベロシティ
    DirectX::XMFLOAT2 boostDir = {0.0f, 0.0f}; ///< ブースト方向（正規化）
    float boostSpeed = 0.0f;                   ///< ブースト速度（単位/秒）

    void SetVelocity(DirectX::XMFLOAT2 speed) {
        velocity = speed;
    }

    void UpdateVelocity(const DirectX::XMFLOAT2 &inputDir) {
        if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
            float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            if (length > 0.0f && !isDecelerating) {
                float normal_x = inputDir.x / length;
                float normal_y = inputDir.y / length;
                velocity.x = normal_x * speed;
                velocity.y = normal_y * speed;
            }
        }

    // 外部から指定方向・指定速度でブースト開始
    void StartBoost(const DirectX::XMFLOAT2 &dir, float spd) {
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0f) {
            boostDir.x = dir.x / len;
            boostDir.y = dir.y / len;
        } else {
            boostDir = {0.0f, 0.0f};
        }
        boostSpeed = spd;
        isBoosting = true;
        isDecelerating = false;
    }

    void StopBoost() {
        isBoosting = false;
        boostSpeed = 0.0f;
    }

        // 減速処理
        if (isDecelerating) {
            float currentSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            if (currentSpeed > 0.0f) {
                float newSpeed = std::max(MinSpeed, currentSpeed - SlowFactor);
                float decelerate_x = velocity.x / currentSpeed;
                float decelerate_y = velocity.y / currentSpeed;
                velocity.x = decelerate_x * newSpeed;
                velocity.y = decelerate_y * newSpeed;
                DebugSpeed = newSpeed;
            }
        }
    }

    void UpdatePosition(World &w, Entity self, float dt) {
        auto *t = w.TryGet<Transform>(self);
        auto *v = w.TryGet<PlayerVelocity>(self);
        if (!t || !v)
            return;

        t->position.x += v->velocity.x * dt;
        t->position.z += v->velocity.y * dt;

        const float limitX = cfg_LimitX;
        const float limitY = cfg_LimitY;
        if (t->position.x < -limitX)
            t->position.x = -limitX;
        if (t->position.x > limitX)
            t->position.x = limitX;
        if (t->position.z < -limitY)
            t->position.z = -limitY;
        if (t->position.z > limitY)
            t->position.z = limitY;

        // 加速中なら速度を上書き
        if (isBoosting) {
            float curSpeed = std::sqrt(v->velocity.x * v->velocity.x + v->velocity.y * v->velocity.y);
            if (curSpeed > 0.0f) {
                float boosted = curSpeed * Acceleration;
                float nx = v->velocity.x / curSpeed;
                float ny = v->velocity.y / curSpeed;
                v->velocity.x = nx * boosted;
                v->velocity.y = ny * boosted;
            } else {
                // 速度がゼロの場合、前進方向がないため、通常速度を加速倍率で適用
                v->velocity.x = speed * Acceleration;
                v->velocity.y = 0.0f;
            }
            // ブースト中は減速しない
            v->isDecelerating = false;
        }
    }

    DirectX::XMFLOAT2 GetVelocity() {
        return velocity;
    }

    float GetVelocitySqrted() {
        float l = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        return l;
    }
};

// ========================================================
// プレイヤー移動コンポーネント
// ========================================================

struct PlayerMovement : Behaviour {
    InputSystem *input_ = nullptr;     ///< 入力システムへのポインタ
    GamepadSystem *gamepad_ = nullptr; ///< ゲームパッドシステムへのポインタ
    CollisionSphere *collision_ = nullptr;

    // チャージ挙動設定
    float minChargeSpeedFactor = cfg_MinChargeSpeed; ///< チャージ中の最低速度係数(0.0-1.0)
    float chargeMaxTime = cfg_ChargeMaxTime;         ///< チャージ最大時間(秒)

    // 入力モード
    bool flickOnly = true; ///< 左スティックの通常移動を無効化

    // 内部状態
    bool isCharging_ = false;                    ///< 現在チャージ中か
    DirectX::XMFLOAT2 lastStickDir_{0.0f, 0.0f}; ///< 直近の左スティック方向(正規化)
    bool wasCharging_ = false;                   ///< 前フレームでチャージしていたか(ローカル検出)
    bool wasChargingPrev_ = false;               ///< チャージ開始検出用

    // 角度履歴
    float angleHistory[PlayerConstants::ANGLE_HISTORY_SIZE] = {};
    int angleIndex = 0;
    bool angleFilled = false;

    // 角度平均計算用の累積値（最適化）
    float sumSin = 0.0f;
    float sumCos = 0.0f;

    int frame = 0;
    bool historyLocked_ = false; ///< 履歴記録が完了したらロック

    void ResetAngleHistory() {
        for (int i = 0; i < PlayerConstants::ANGLE_HISTORY_SIZE; ++i) {
            angleHistory[i] = 0.0f;
        }
        angleIndex = 0;
        angleFilled = false;
        sumSin = 0.0f;
        sumCos = 0.0f;
        historyLocked_ = false;
    }

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
        if (!t || !v || (!input_ && !gamepad_))
            return;

        v->speed = PlayerVelocity::cfg_Speed;
        minChargeSpeedFactor = cfg_MinChargeSpeed;
        chargeMaxTime = cfg_ChargeMaxTime;

        DirectX::XMFLOAT2 inputDir = {0.0f, 0.0f};
        v->UpdateVelocity(inputDir);

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

        if (gamepad_) {
            float gx = gamepad_->GetLeftStickX();
            float gy = gamepad_->GetLeftStickY();
            float mag = std::sqrt(gx * gx + gy * gy);

            if (mag > PlayerConstants::EPSILON) {
                lastStickDir_.x = -(gx / mag);
                lastStickDir_.y = -(gy / mag);
            }

            const float releaseThreshold = cfg_ReleaseThreshold;
            bool chargingNowLocal = (mag > releaseThreshold);

            bool chargingSys = gamepad_->IsLeftStickCharging();
            bool effectiveCharging = chargingSys && chargingNowLocal;
            frame++;

            if (effectiveCharging && !wasChargingPrev_) {
                ResetAngleHistory();
            }

            if (effectiveCharging) {
                isCharging_ = true;
                v->isDecelerating = true;

                if (collision_) {
                    collision_->radius *= 0.01f;
                }
                float charge = gamepad_->GetLeftStickChargeAmount(chargeMaxTime);
                (void) charge;

                static float moveAmount = cfg_ChargeMoveAmount;

                if (frame % 2 == 0) {
                    t->position.x += moveAmount;
                    if (t->position.x >= moveAmount || t->position.x <= -moveAmount) {
                        moveAmount = -moveAmount;
                    }
                }

                if (!historyLocked_ && mag > PlayerConstants::EPSILON) {
                    float ang = std::atan2f(lastStickDir_.y, lastStickDir_.x);

                    if (angleFilled) {
                        sumSin -= std::sinf(angleHistory[angleIndex]);
                        sumCos -= std::cosf(angleHistory[angleIndex]);
                    }

                    angleHistory[angleIndex] = ang;
                    sumSin += std::sinf(ang);
                    sumCos += std::cosf(ang);

                    angleIndex = (angleIndex + 1) % PlayerConstants::ANGLE_HISTORY_SIZE;
                    if (angleIndex == 0) {
                        angleFilled = true;
                        historyLocked_ = true;
                    }
                }
            }

            bool releasedSys = gamepad_->IsLeftStickReleased();
            bool releasedLocal = (wasCharging_ && !chargingNowLocal);

            if (releasedSys || releasedLocal) {
                int count = angleFilled ? PlayerConstants::ANGLE_HISTORY_SIZE : angleIndex;

                if (count > 0) {
                    float avgRad = std::atan2f(sumSin / count, sumCos / count);

                    float dirX = std::cosf(avgRad);
                    float dirY = std::sinf(avgRad);

                    float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
                    if (dirLen > PlayerConstants::EPSILON) {
                        v->velocity.x = (dirX / dirLen) * v->speed;
                        v->velocity.y = (dirY / dirLen) * v->speed;

                        float yawRad = std::atan2(v->velocity.y, v->velocity.x);
                        t->rotation.y = yawRad * (180.0f / DirectX::XM_PI);

                        isCharging_ = false;
                        v->isDecelerating = false;
                    }
                }

                ResetAngleHistory();
            }

            wasCharging_ = chargingNowLocal;
            wasChargingPrev_ = effectiveCharging;

            if (!flickOnly) {
                inputDir.x += -gx;
                inputDir.y += -gy;
            }

            v->UpdatePosition(w, self, dt);
        }

        if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
            v->UpdateVelocity(inputDir);
        }
    }

    float CalcMoveRotation() {
        return std::atan2f(lastStickDir_.y, lastStickDir_.x) * (180.0f / DirectX::XM_PI);
    }
};

// ========================================================
// プレイヤーガイド表示コンポーネント
// ========================================================

struct PlayerGuide : Behaviour {
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

    void Create(World &world, const DirectX::XMFLOAT3 &position) {
        Transform t{position, {0, 0, 0}, {0, 0, 0}};

        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{1, 0, 0};

        guidEntity = world.Create()
                         .With<Transform>(t)
                         .With<MeshRenderer>(renderer)
                         .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                         .Build();
    };

    void OnStart(World &w, Entity self) override {
        selfTransform = w.TryGet<Transform>(self);
        if (!selfTransform)
            return;
        Create(w, selfTransform->position);
    }

    void OnUpdate(World &w, Entity self, float dt) override {
        playerMove = w.TryGet<PlayerMovement>(self);
        selfTransform = w.TryGet<Transform>(self);
        guidTransform = w.TryGet<Transform>(guidEntity);
        if (!playerMove || !selfTransform || !guidTransform)
            return;

        guidTransform->position = selfTransform->position;

        float rad = std::atan2f(playerMove->lastStickDir_.y, playerMove->lastStickDir_.x);
        guidTransform->rotation.y = -rad * (180.0f / DirectX::XM_PI);

        if (!playerMove->isCharging_) {
            guidTransform->scale = {0, 0, 0};
        } else {
            guidTransform->scale = {cfg_GuideScaleX.Get(), cfg_GuideScaleY.Get(), cfg_GuideScaleZ.Get()};
            guidTransform->position.x += std::cosf(rad) * cfg_GuideOffsetDistance.Get();
            guidTransform->position.z += std::sinf(rad) * cfg_GuideOffsetDistance.Get();
        }
    }
};
