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
#include"components/GameStats.h"
#include "input/InputSystem.h"
#include "input/GamepadSystem.h"
#include "components/Collision.h"
// GameScene 定義への依存を避けるため、グローバル参照宣言のみを利用
#include "scenes/CollisionHandlers.h"
#include "components/StageComponents.h"

// 前方宣言と外部参照（インライン定義は CollisionHandlers.h 内）
class GameScene; extern GameScene* g_GameScene; extern bool g_respawnPending;
// GameScene 呼び出しラッパー
void GameScene_OnChargeStart(World &w);
void GameScene_OnChargeRelease(World &w, float chargeAmount01);

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <vector>

#include "config/ConfigVar.h"

// ConfigVar (重複排除)
inline static ConfigVar<float> cfg_MinChargeSpeed{"Player.Charge", "MinChargeSpeedFactor", 0.4f};
inline static ConfigVar<float> cfg_ChargeMaxTime{"Player.Charge", "ChargeMaxTime", 0.2f};
inline static ConfigVar<float> cfg_ReleaseThreshold{"Player.Charge", "ReleaseThreshold", 0.3f};
inline static ConfigVar<float> cfg_ChargeMoveAmount{"Player.Charge", "ChargeMoveAmount", 0.025f};
inline static ConfigVar<float> cfg_LimitX{"Player.Bounds", "LimitX", 15.0f};
inline static ConfigVar<float> cfg_LimitY{"Player.Bounds", "LimitY", 15.0f};
inline static ConfigVar<float> cfg_AccelerateMagnification{"Player.Movement", "AccelerateMagnification", 1.5f};

namespace PlayerConstants {
constexpr int ANGLE_HISTORY_SIZE = 30;
constexpr float EPSILON = 1e-5f;
}

struct PlayerStatus : Behaviour { bool isStartAfterWallHit = false; };

// ===============================
// PlayerVelocity（重複宣言整理）
// ===============================
struct PlayerVelocity : Behaviour {
    inline static ConfigVar<float> cfg_Speed{"Player.Movement", "MoveSpeed", 6.0f};

    float Acceleration = cfg_AccelerateMagnification;
    float MinSpeed = cfg_MinChargeSpeed;
    float speed = cfg_Speed;
    float SlowFactor = 1.0f; // 減速量(単位/秒)
    bool isBoosting = false;
    bool isDecelerating = false;
    float DebugSpeed = 0.0f;

    DirectX::XMFLOAT2 velocity = {0.0f, 0.0f};
    DirectX::XMFLOAT2 boostDir = {0.0f, 0.0f};
    float boostSpeed = 2.0f;

    void SetVelocity(DirectX::XMFLOAT2 speedIn) { velocity = speedIn; }

    // 入力適用→減速適用（dtスケール）
    void UpdateVelocity(const DirectX::XMFLOAT2 &inputDir, float dt) {
        if ((inputDir.x != 0.0f || inputDir.y != 0.0f) && !isDecelerating) {
            float len = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            if (len > 0.0f) {
                float nx = inputDir.x / len;
                float ny = inputDir.y / len;
                velocity.x = nx * speed;
                velocity.y = ny * speed;
            }
        }
        if (isDecelerating) {
            float cur = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            if (cur > 0.0f) {
                float decel = SlowFactor * std::max(0.0f, dt);
                float newSpd = std::max(MinSpeed, cur - decel);
                float nx = velocity.x / cur;
                float ny = velocity.y / cur;
                velocity.x = nx * newSpd;
                velocity.y = ny * newSpd;
                DebugSpeed = newSpd;
            }
        }
    }

    void StartBoost(const DirectX::XMFLOAT2 &dir, float spd) {
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0f) { boostDir.x = dir.x / len; boostDir.y = dir.y / len; } else { boostDir = {0.0f, 0.0f}; }
        boostSpeed = spd; isBoosting = true; isDecelerating = false;
    }
    void StopBoost() { isBoosting = false; boostSpeed = 0.0f; }

    void UpdatePosition(World &w, Entity self, float dt) {
        auto *t = w.TryGet<Transform>(self);
        auto *v = w.TryGet<PlayerVelocity>(self);
        if (!t || !v) return;
        if (isBoosting) { v->velocity.x = boostDir.x * boostSpeed; v->velocity.y = boostDir.y * boostSpeed; v->isDecelerating = false; }
        const float moveX = v->velocity.x * dt;
        const float moveZ = v->velocity.y * dt;
        t->position.x += moveX;
        t->position.z += moveZ;
        const float moveLenSq = moveX * moveX + moveZ * moveZ;
        if (moveLenSq > PlayerConstants::EPSILON) {
            w.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
                if (stats.waitingForPlayerMove) {
                    stats.waitingForPlayerMove = false;
                    stats.timerRunning = true;
                }
            });
        }
        const float limitX = cfg_LimitX; const float limitY = cfg_LimitY;
        if (t->position.x < -limitX) t->position.x = -limitX;
        if (t->position.x > limitX)  t->position.x =  limitX;
        if (t->position.z < -limitY) t->position.z = -limitY;
        if (t->position.z > limitY)  t->position.z =  limitY;
    }

    DirectX::XMFLOAT2 GetVelocity() { return velocity; }
    float GetVelocitySqrted() { return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y); }
};

// ===============================
// PlayerMovement（重複宣言整理）
// ===============================
struct PlayerMovement : Behaviour {
    InputSystem *input_ = nullptr;
    GamepadSystem *gamepad_ = nullptr;
    CollisionSphere *collision_ = nullptr;
    float collisionRadiusBackup_ = 0.0f;
    bool hasCollisionBackup_ = false;

    float minChargeSpeedFactor = cfg_MinChargeSpeed;
    float chargeMaxTime = cfg_ChargeMaxTime;
    bool flickOnly = true;

    bool isCharging_ = false;
    DirectX::XMFLOAT2 lastStickDir_{0.0f, 0.0f};
    bool wasCharging_ = false;
    bool wasChargingPrev_ = false;

    float angleHistory[PlayerConstants::ANGLE_HISTORY_SIZE] = {};
    int angleIndex = 0; bool angleFilled = false;
    float sumSin = 0.0f; float sumCos = 0.0f;
    int frame = 0; bool historyLocked_ = false;

    void ResetAngleHistory() {
        for (int i = 0; i < PlayerConstants::ANGLE_HISTORY_SIZE; ++i) angleHistory[i] = 0.0f;
        angleIndex = 0; angleFilled = false; sumSin = 0.0f; sumCos = 0.0f; historyLocked_ = false;
    }

    void OnUpdate(World &w, Entity self, float dt) override {
        auto *t = w.TryGet<Transform>(self);
        auto *v = w.TryGet<PlayerVelocity>(self);
        auto *playerStatus = w.TryGet<PlayerStatus>(self);
        if (!t || !v || (!input_ && !gamepad_)) return;
        if (!collision_) collision_ = w.TryGet<CollisionSphere>(self);

        auto restoreCollisionRadius = [&]() {
            if (collision_ && hasCollisionBackup_) {
                collision_->radius = collisionRadiusBackup_;
                hasCollisionBackup_ = false;
            }
        };

        if (g_respawnPending) {
            v->velocity = {0.0f, 0.0f}; v->isBoosting = false; v->isDecelerating = false; v->boostSpeed = 0.0f;
            restoreCollisionRadius();
            ResetAngleHistory(); isCharging_ = false; wasCharging_ = false; wasChargingPrev_ = false; return;
        }

        //タイマーが0になるまで動かない
        bool startBlocked = false;
        w.ForEach<GameStatus>([&](Entity e, GameStatus &stats) {
            if (stats.StartChack == false) {
                v->velocity = {0.0f, 0.0f};
                v->isBoosting = false;
                v->isDecelerating = false;
                v->boostSpeed = 0.0f;
                ResetAngleHistory();
                isCharging_ = false;
                wasCharging_ = false;
                wasChargingPrev_ = false;
                startBlocked = true;
            }
        });
        if (startBlocked) { restoreCollisionRadius(); return; }

        v->speed = PlayerVelocity::cfg_Speed;
        minChargeSpeedFactor = cfg_MinChargeSpeed;
        chargeMaxTime = cfg_ChargeMaxTime;
        v->MinSpeed = v->speed * minChargeSpeedFactor;

        DirectX::XMFLOAT2 inputDir = {0.0f, 0.0f};
        if (input_) {
            if (input_->GetKey('W') || input_->GetKey(VK_UP))    inputDir.y += 1.0f;
            if (input_->GetKey('S') || input_->GetKey(VK_DOWN))  inputDir.y -= 1.0f;
            if (input_->GetKey('A') || input_->GetKey(VK_LEFT))  inputDir.x -= 1.0f;
            if (input_->GetKey('D') || input_->GetKey(VK_RIGHT)) inputDir.x += 1.0f;
        }

        if (gamepad_ && !playerStatus->isStartAfterWallHit) {
            float gx = gamepad_->GetLeftStickX();
            float gy = gamepad_->GetLeftStickY();
            float mag = std::sqrt(gx * gx + gy * gy);
            if (mag > PlayerConstants::EPSILON) { lastStickDir_.x = -(gx / mag); lastStickDir_.y = -(gy / mag); }

            const float releaseThreshold = cfg_ReleaseThreshold;
            bool chargingNowLocal = (mag > releaseThreshold);
            bool chargingSys = gamepad_->IsLeftStickCharging();
            bool effectiveCharging = chargingSys && chargingNowLocal;
            frame++;

            if (effectiveCharging && !wasChargingPrev_) ResetAngleHistory();
            if (effectiveCharging && !wasCharging_) {
                GameScene_OnChargeStart(w);
            }

            if (effectiveCharging) {
                if (v->isBoosting) { v->StopBoost(); }
                if (collision_) {
                    if (!hasCollisionBackup_) { collisionRadiusBackup_ = collision_->radius; hasCollisionBackup_ = true; }
                    collision_->radius = collisionRadiusBackup_ * 0.01f;
                }
                isCharging_ = true;
                v->isDecelerating = true; // チャージ中は減速
                // チャージ最大時間でMinSpeedまで落とすための減速量を算出（毎秒）
                v->SlowFactor = (std::max(0.0f, v->speed - v->MinSpeed)) / std::max(0.0001f, chargeMaxTime);
                float charge = gamepad_->GetLeftStickChargeAmount(chargeMaxTime); (void)charge;

                if (!historyLocked_ && mag > PlayerConstants::EPSILON) {
                    float ang = std::atan2f(lastStickDir_.y, lastStickDir_.x);
                    if (angleFilled) { sumSin -= std::sinf(angleHistory[angleIndex]); sumCos -= std::cosf(angleHistory[angleIndex]); }
                    angleHistory[angleIndex] = ang; sumSin += std::sinf(ang); sumCos += std::cosf(ang);
                    angleIndex = (angleIndex + 1) % PlayerConstants::ANGLE_HISTORY_SIZE;
                    if (angleIndex == 0) { angleFilled = true; historyLocked_ = true; }
                }
            }

            bool releasedSys = gamepad_->IsLeftStickReleased();
            bool releasedLocal = (wasCharging_ && !chargingNowLocal);
            if (releasedSys || releasedLocal) {
                float chargeAmount = gamepad_->GetLeftStickChargeAmount(chargeMaxTime);
                int count = angleFilled ? PlayerConstants::ANGLE_HISTORY_SIZE : angleIndex;
                if (count > 0) {
                    float avgRad = std::atan2f(sumSin / count, sumCos / count);
                    float dirX = std::cosf(avgRad); float dirY = std::sinf(avgRad);
                    float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
                    if (dirLen > PlayerConstants::EPSILON) {
                        DirectX::XMFLOAT2 boostDir{dirX / dirLen, dirY / dirLen};
                        float maxBoost = v->speed * v->Acceleration;
                        v->StartBoost(boostDir, maxBoost);
                        isCharging_ = false; v->isDecelerating = false;
                    }
                }
                GameScene_OnChargeRelease(w, chargeAmount);
                restoreCollisionRadius();
                ResetAngleHistory();
            }

            wasCharging_ = chargingNowLocal; wasChargingPrev_ = effectiveCharging;
            if (!flickOnly) { inputDir.x += -gx; inputDir.y += -gy; }

            if (!effectiveCharging && !chargingNowLocal) {
                restoreCollisionRadius();
                isCharging_ = false;
            }

            // 減速をまず反映してから移動
            v->UpdateVelocity(inputDir, dt);
            v->UpdatePosition(w, self, dt);
        } else {
            v->UpdateVelocity(inputDir, dt);
        }
    }

    float CalcMoveRotation() { return std::atan2f(lastStickDir_.y, lastStickDir_.x) * (180.0f / DirectX::XM_PI); }
};

// ===============================
// PlayerGuide（そのまま）
// ===============================
struct PlayerGuide : Behaviour {
    inline static ConfigVar<float> cfg_GuideScaleX{"Player.Guide", "GuideScaleX", 0.4f};
    inline static ConfigVar<float> cfg_GuideScaleY{"Player.Guide", "GuideScaleY", 0.4f};
    inline static ConfigVar<float> cfg_GuideScaleZ{"Player.Guide", "GuideScaleZ", 0.4f};
    inline static ConfigVar<float> cfg_GuideOffsetDistance{"Player.Guide", "GuideOffsetDistance", 0.5f};
    inline static ConfigVar<int> cfg_GuideQuantity{"Player.Guide", "GuideQuantity", 3};

    PlayerMovement *playerMove{}; Transform *selfTransform{}; std::vector<Transform*> guidTransforms; std::vector<Entity> guidEntities;

    void Create(World &world, const DirectX::XMFLOAT3 &position) {
        MeshRenderer renderer; renderer.meshType = MeshType::Sphere; renderer.color = DirectX::XMFLOAT3{1, 0, 0};
        int guideQuantity = cfg_GuideQuantity.Get(); guidEntities.clear(); guidTransforms.clear();
        guidEntities.reserve(static_cast<size_t>(guideQuantity)); guidTransforms.reserve(static_cast<size_t>(guideQuantity));
        for (int i = 0; i < guideQuantity; i++) {
            Transform t{position, {0, 0, 0}, {0, 0, 0}};
            Entity e = world.Create().With<Transform>(t).With<MeshRenderer>(renderer).Build();
            guidEntities.push_back(e); guidTransforms.push_back(nullptr);
        }
    }

    void OnStart(World &w, Entity self) override { selfTransform = w.TryGet<Transform>(self); if (!selfTransform) return; Create(w, selfTransform->position); }

    void OnUpdate(World &w, Entity self, float dt) override {
        if (g_respawnPending) { for (auto e : guidEntities) { if (auto *gt = w.TryGet<Transform>(e)) { gt->scale = {0, 0, 0}; } } return; }
        playerMove = w.TryGet<PlayerMovement>(self); selfTransform = w.TryGet<Transform>(self);
        bool allValid = true; int guideQuantity = cfg_GuideQuantity.Get();
        if (static_cast<int>(guidEntities.size()) != guideQuantity) { Create(w, selfTransform ? selfTransform->position : DirectX::XMFLOAT3{0, 0, 0}); }
        for (int i = 0; i < guideQuantity; i++) { guidTransforms[i] = w.TryGet<Transform>(guidEntities[i]); if (!guidTransforms[i]) { allValid = false; } }
        if (!playerMove || !selfTransform || !allValid) return;
        float rad = std::atan2f(playerMove->lastStickDir_.y, playerMove->lastStickDir_.x);
        for (int i = 0; i < guideQuantity; i++) {
            Transform *currentGuide = guidTransforms[i];
            if (!playerMove->isCharging_) { currentGuide->scale = {0, 0, 0}; }
            else {
                currentGuide->scale = {cfg_GuideScaleX.Get(), cfg_GuideScaleY.Get(), cfg_GuideScaleZ.Get()};
                currentGuide->position = selfTransform->position; currentGuide->rotation.y = -rad * (180.0f / DirectX::XM_PI);
                float offsetDistance = cfg_GuideOffsetDistance.Get() * (i + 1);
                currentGuide->position.x += std::cosf(rad) * offsetDistance;
                currentGuide->position.z += std::sinf(rad) * offsetDistance;
            }
        }
    }
};





























































