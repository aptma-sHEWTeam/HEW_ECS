#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <cstring>

// ========================================================
// InputSystem - �L�[�{�[�h�E�}�E�X���͊Ǘ�
// ========================================================
class InputSystem {
public:
    // �L�[�̏��
    enum class KeyState : uint8_t {
        None = 0,
        Down = 1,      // ���t���[�������ꂽ
        Pressed = 2,   // �����ꑱ���Ă���
        Up = 3         // ���t���[�������ꂽ
    };
    
    // �}�E�X�{�^��
    enum MouseButton {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    // ������
    void Init() {
        memset(keyStates_, 0, sizeof(keyStates_));
        memset(prevKeyStates_, 0, sizeof(prevKeyStates_));
        memset(mouseStates_, 0, sizeof(mouseStates_));
        memset(prevMouseStates_, 0, sizeof(prevMouseStates_));
        mouseX_ = mouseY_ = 0;
        mouseDeltaX_ = mouseDeltaY_ = 0;
        mouseWheel_ = 0;
    }

    // �X�V�i���C�����[�v�̍ŏ��ŌĂԁj
    void Update() {
        // �O�t���[���̏�Ԃ�ۑ�
        memcpy(prevKeyStates_, keyStates_, sizeof(keyStates_));
        memcpy(prevMouseStates_, mouseStates_, sizeof(mouseStates_));
        
        // ���݂̃L�[��Ԃ��擾
        for (int i = 0; i < 256; ++i) {
            bool current = (GetAsyncKeyState(i) & 0x8000) != 0;
            bool prev = prevKeyStates_[i];
            
            if (current && !prev) {
                keyStates_[i] = static_cast<uint8_t>(KeyState::Down);
            } else if (current && prev) {
                keyStates_[i] = static_cast<uint8_t>(KeyState::Pressed);
            } else if (!current && prev) {
                keyStates_[i] = static_cast<uint8_t>(KeyState::Up);
            } else {
                keyStates_[i] = static_cast<uint8_t>(KeyState::None);
            }
        }
        
        // �}�E�X�ʒu�̍X�V
        POINT pt;
        if (GetCursorPos(&pt)) {
            int newX = pt.x;
            int newY = pt.y;
            mouseDeltaX_ = newX - mouseX_;
            mouseDeltaY_ = newY - mouseY_;
            mouseX_ = newX;
            mouseY_ = newY;
        }
        
        // �}�E�X�z�C�[���̓��Z�b�g
        mouseWheel_ = 0;
    }

    // �L�[��������Ă��邩
    bool GetKey(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        KeyState state = static_cast<KeyState>(keyStates_[vkCode]);
        return state == KeyState::Pressed || state == KeyState::Down;
    }

    // �L�[�����t���[�������ꂽ��
    bool GetKeyDown(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        return static_cast<KeyState>(keyStates_[vkCode]) == KeyState::Down;
    }

    // �L�[�����t���[�������ꂽ��
    bool GetKeyUp(int vkCode) const {
        if (vkCode < 0 || vkCode >= 256) return false;
        return static_cast<KeyState>(keyStates_[vkCode]) == KeyState::Up;
    }

    // �}�E�X�{�^����������Ă��邩
    bool GetMouseButton(MouseButton button) const {
        int vk = VK_LBUTTON;
        if (button == Right) vk = VK_RBUTTON;
        else if (button == Middle) vk = VK_MBUTTON;
        return GetKey(vk);
    }

    // �}�E�X�{�^�������t���[�������ꂽ��
    bool GetMouseButtonDown(MouseButton button) const {
        int vk = VK_LBUTTON;
        if (button == Right) vk = VK_RBUTTON;
        else if (button == Middle) vk = VK_MBUTTON;
        return GetKeyDown(vk);
    }

    // �}�E�X�{�^�������t���[�������ꂽ��
    bool GetMouseButtonUp(MouseButton button) const {
        int vk = VK_LBUTTON;
        if (button == Right) vk = VK_RBUTTON;
        else if (button == Middle) vk = VK_MBUTTON;
        return GetKeyUp(vk);
    }

    // �}�E�X���W�擾
    int GetMouseX() const { return mouseX_; }
    int GetMouseY() const { return mouseY_; }
    
    // �}�E�X�̈ړ��ʎ擾
    int GetMouseDeltaX() const { return mouseDeltaX_; }
    int GetMouseDeltaY() const { return mouseDeltaY_; }
    
    // �}�E�X�z�C�[���擾
    int GetMouseWheel() const { return mouseWheel_; }
    
    // �}�E�X�z�C�[���C�x���g�iWM_MOUSEWHEEL����Ăԁj
    void OnMouseWheel(int delta) {
        mouseWheel_ = delta / 120; // 120�P�ʂŐ��K��
    }

private:
    uint8_t keyStates_[256];
    uint8_t prevKeyStates_[256];
    uint8_t mouseStates_[3];
    uint8_t prevMouseStates_[3];
    
    int mouseX_;
    int mouseY_;
    int mouseDeltaX_;
    int mouseDeltaY_;
    int mouseWheel_;
};

// �O���[�o���ȓ��̓V�X�e���C���X�^���X
inline InputSystem& GetInput() {
    static InputSystem instance;
    return instance;
}
