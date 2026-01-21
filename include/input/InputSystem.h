#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "app/DebugLog.h"
#include <cstdint>
#include <cstring>

/**
 * @file InputSystem.h
 * @brief キーボード・マウス入力管理システム
 * @author 山内陽
 * @date 2025
 * @version 5.0
 * 
 * @details
 * このファイルはキーボードとマウスの入力を管理するシステムを提供します。
 */

/**
 * @class InputSystem
 * @brief キーボード・マウス入力を管理するクラス
 * 
 * @details
 * Windows APIを使用してキーボードとマウスの入力状態を管理します。
 * ゲームループ内で毎フレームUpdate()を呼び出すことで、入力状態が更新されます。
 * 
 * @par 使用例
 * @code
 * InputSystem input;
 * input.Init();
 * 
 * while (running) {
 *     input.Update();
 *     
 *     if (input.GetKeyDown(VK_SPACE)) {
 *         // スペースキー押下時
 *     }
 * }
 * @endcode
 * 
 * @author 山内陽
 */
class InputSystem {
public:
    /**
     * @enum KeyState
     * @brief キーの状態を表す列挙型
     */
    enum class KeyState : uint8_t {
        None = 0,      ///< 何も押されていない
        Down = 1,      ///< このフレームで押された
        Pressed = 2,   ///< 押され続けている
        Up = 3         ///< このフレームで離された
    };
    
    /**
     * @enum MouseButton
     * @brief マウスボタンの識別子
     */
    enum MouseButton {
        Left = 0,      ///< 左ボタン
        Right = 1,     ///< 右ボタン
        Middle = 2     ///< 中ボタン
    };

    /**
     * @brief 初期化
     * 
     * @details
     * すべての入力状態をリセットし初期状態にします。
     */
    void Init() {
#ifdef _DEBUG
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, "InputSystem::Init() - 初期化開始");
#endif
        memset(keyStates_, 0, sizeof(keyStates_));
        memset(prevKeyStates_, 0, sizeof(prevKeyStates_));
        mouseX_ = mouseY_ = 0;
        mouseDeltaX_ = mouseDeltaY_ = 0;
        mouseWheel_ = 0;
        mouseWheelAccum_ = 0;
        hwnd_ = nullptr;
#ifdef _DEBUG
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, "InputSystem::Init() - 初期化完了");
#endif
    }

    void SetWindowHandle(HWND hwnd) {
        hwnd_ = hwnd;
        if (!hwnd_) {
            return;
        }
        POINT pt{};
        if (GetCursorPos(&pt) && ScreenToClient(hwnd_, &pt)) {
            mouseX_ = pt.x;
            mouseY_ = pt.y;
        } else {
            mouseX_ = 0;
            mouseY_ = 0;
        }
    }


    /**
     * @brief シャットダウン
     * 
     * @details
     * 入力システムをシャットダウンします。
     */
    void Shutdown() {
        hwnd_ = nullptr;
#ifdef _DEBUG
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, "InputSystem::Shutdown() - シャットダウン中");
        memset(keyStates_, 0, sizeof(keyStates_));
        memset(prevKeyStates_, 0, sizeof(prevKeyStates_));
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, "InputSystem::Shutdown() - 完了");
#endif
    }

    /**
     * @brief 入力状態の更新(毎フレーム呼ぶ)
     * 
     * @details
     * 前フレームの状態を保存し、現在の入力状態を取得します。
     * キーの押下・離された瞬間の判定はこの更新処理によって行われます。
     */
    void Update() {
        memcpy(prevKeyStates_, keyStates_, sizeof(keyStates_));
        
        for (int i = 0; i < 256; ++i) {
            bool isDown = (GetAsyncKeyState(i) & 0x8000) != 0;
            KeyState prevState = static_cast<KeyState>(prevKeyStates_[i]);
            bool wasDown = (prevState == KeyState::Down || prevState == KeyState::Pressed);

            if (isDown) {
                keyStates_[i] = static_cast<uint8_t>(wasDown ? KeyState::Pressed : KeyState::Down);
            } else {
                keyStates_[i] = static_cast<uint8_t>(wasDown ? KeyState::Up : KeyState::None);
              
            }
            float moveX = 0.0f;
            float moveY = 0.0f;
        }
        
        POINT pt;
        if (GetCursorPos(&pt)) {
            POINT clientPt = pt;
            if (hwnd_ && ScreenToClient(hwnd_, &clientPt)) {
                pt = clientPt;
            }

            int newX = pt.x;
            int newY = pt.y;
            mouseDeltaX_ = newX - mouseX_;
            mouseDeltaY_ = newY - mouseY_;
            mouseX_ = newX;
            mouseY_ = newY;
        }
        
        mouseWheel_ = mouseWheelAccum_;
        mouseWheelAccum_ = 0;
    }

    /**
     * @brief キーが押されているか
     * @param[in] vkCode 仮想キーコード
     * @return true 押されている, false 押されていない
     * 
     * @details
     * キーが押され続けている間、またはこのフレームで押された瞬間に true を返します。
     * 
     * @par 使用例
     * @code
     * if (input.GetKey('W')) {
     *     // Wキーが押されている間、前進
     * }
     * @endcode
     */
    bool GetKey(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        KeyState state = static_cast<KeyState>(keyStates_[vkCode]);
        return state == KeyState::Pressed || state == KeyState::Down;
    }

    /**
     * @brief キーがこのフレームで押された瞬間か
     * @param[in] vkCode 仮想キーコード
     * @return true 押された瞬間, false それ以外
     * 
     * @details
     * キーが押された最初のフレームのみtrueを返します。
     * 押し続けても次のフレームからはfalseになります。
     * 
     * @par 使用例
     * @code
     * if (input.GetKeyDown(VK_SPACE)) {
     *     // スペースキーが押された瞬間にジャンプ
     *     Jump();
     * }
     * @endcode
     */
    bool GetKeyDown(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        return static_cast<KeyState>(keyStates_[vkCode]) == KeyState::Down;
    }

    /**
     * @brief キーがこのフレームで離された瞬間か
     * @param[in] vkCode 仮想キーコード
     * @return true 離された瞬間, false それ以外
     * 
     * @details
     * キーが離された最初のフレームのみtrueを返します。
     * 
     * @par 使用例
     * @code
     * if (input.GetKeyUp(VK_SPACE)) {
     *     // スペースキーを離した瞬間の処理
     * }
     * @endcode
     */
    bool GetKeyUp(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        return static_cast<KeyState>(keyStates_[vkCode]) == KeyState::Up;
    }

    /**
     * @brief マウスボタンが押されているか
     * @param[in] button マウスボタン
     * @return true 押されている, false 押されていない
     * 
     * @details
     * 指定したマウスボタンが押されている間trueを返します。
     * 
     * @par 使用例
     * @code
     * if (input.GetMouseButton(InputSystem::Left)) {
     *     // 左クリック中の処理
     * }
     * @endcode
     */
    bool GetMouseButton(MouseButton button) const {
        int vk = 0;
        switch (button) {
            case Left:   vk = VK_LBUTTON; break;
            case Right:  vk = VK_RBUTTON; break;
            case Middle: vk = VK_MBUTTON; break;
        }
        return GetKey(vk);
    }

    /**
     * @brief マウスボタンがこのフレームで押された瞬間か
     * @param[in] button マウスボタン
     * @return true 押された瞬間, false それ以外
     * 
     * @details
     * マウスボタンが押された最初のフレームのみtrueを返します。
     */
    bool GetMouseButtonDown(MouseButton button) const {
        int vk = 0;
        switch (button) {
            case Left:   vk = VK_LBUTTON; break;
            case Right:  vk = VK_RBUTTON; break;
            case Middle: vk = VK_MBUTTON; break;
        }
        return GetKeyDown(vk);
    }

    /**
     * @brief マウスボタンがこのフレームで離された瞬間か
     * @param[in] button マウスボタン
     * @return true 離された瞬間, false それ以外
     * 
     * @details
     * マウスボタンが離された最初のフレームのみtrueを返します。
     */
    bool GetMouseButtonUp(MouseButton button) const {
        int vk = 0;
        switch (button) {
            case Left:   vk = VK_LBUTTON; break;
            case Right:  vk = VK_RBUTTON; break;
            case Middle: vk = VK_MBUTTON; break;
        }
        return GetKeyUp(vk);
    }

    /**
     * @brief マウスX座標を取得
     * @return int X座標(スクリーン座標系)
     */
    int GetMouseX() const { return mouseX_; }
    
    /**
     * @brief マウスY座標を取得
     * @return int Y座標(スクリーン座標系)
     */
    int GetMouseY() const { return mouseY_; }

    /**
     * @brief マウス位置を取得
     * @return POINT マウス座標(クライアント座標系)
     */
    POINT GetMousePosition() const {
        POINT pt;
        pt.x = mouseX_;
        pt.y = mouseY_;
        return pt;
    }
    
    /**
     * @brief マウスの移動量(X方向)を取得
     * @return int X方向移動量
     * 
     * @details
     * 前フレームからのマウスカーソルの移動量を返します。
     */
    int GetMouseDeltaX() const { return mouseDeltaX_; }
    
    /**
     * @brief マウスの移動量(Y方向)を取得
     * @return int Y方向移動量
     * 
     * @details
     * 前フレームからのマウスカーソルの移動量を返します。
     */
    int GetMouseDeltaY() const { return mouseDeltaY_; }
    
    /**
     * @brief マウスホイールの回転量を取得
     * @return int ホイール回転量(正:上回転, 負:下回転)
     */
    int GetMouseWheel() const { return mouseWheel_; }
    
    /**
     * @brief マウスホイールイベント
     * @param[in] delta ホイール回転量
     * 
     * @details
     * ウィンドウプロシージャからホイールイベントを受け取るために使用します。
     */
    void OnMouseWheel(int delta) {
        mouseWheelAccum_ += delta / WHEEL_DELTA;
    }

private:
    HWND hwnd_ = nullptr;            ///< Tracking window handle for client-space coordinates
    uint8_t keyStates_[256];        ///< 現在のキー状態
    uint8_t prevKeyStates_[256];    ///< 前フレームのキー状態
    
    int mouseX_;        ///< マウスX座標
    int mouseY_;        ///< マウスY座標
    int mouseDeltaX_;   ///< マウスX移動量
    int mouseDeltaY_;   ///< マウスY移動量
    int mouseWheel_;    ///< マウスホイール回転量
    int mouseWheelAccum_;  ///< マウスホイール累積値
};

/**
 * @brief グローバルな入力システムインスタンスを取得
 * @return InputSystem& シングルトンインスタンス
 * 
 * @details
 * どこからでもアクセス可能な入力システムのインスタンスを返します。
 * 
 * @par 使用例
 * @code
 * if (GetInput().GetKeyDown(VK_ESCAPE)) {
 *     // ESCキーでゲーム終了
 * }
 * @endcode
 * 
 * @author 山内陽
 */
inline InputSystem& GetInput() {
    static InputSystem instance;
    return instance;
}
