#pragma once
#include "Component.h"
#include "Entity.h"
#include "World.h"
#include "TextureManager.h"
#include <vector>
#include <cmath>

// ========================================================
// SpriteAnimation - �X�v���C�g�A�j���[�V�����R���|�[�l���g
// ========================================================
// �y�����z
// �����̃e�N�X�`�������Ԃɐ؂�ւ��ăp���p���A�j���[�V����������
//
// �y�g�����z
// SpriteAnimation anim;
// anim.frames = { tex1, tex2, tex3 }; // �A�j���[�V�����t���[��
// anim.frameTime = 0.1f;              // 1�t���[��0.1�b�i10fps�j
// anim.loop = true;                   // ���[�v�Đ�
// world.Add<SpriteAnimation>(entity, anim);
//
// �y�d�g�݁z
// ���Ԍo�߂�frames�����Ԃɐ؂�ւ��Ă���
// ========================================================
struct SpriteAnimation : Behaviour {
    std::vector<TextureManager::TextureHandle> frames; // �A�j���[�V�����t���[��
    float frameTime = 0.1f;   // 1�t���[���̕\�����ԁi�b�j
    bool loop = true;         // ���[�v�Đ����邩
    bool playing = true;      // �Đ�����
    
    // ������ԁi�����Ǘ������̂ŐG��Ȃ���OK�j
    float currentTime = 0.0f;
    size_t currentFrame = 0;
    bool finished = false;

    void OnUpdate(World& w, Entity self, float dt) override {
        if (!playing || frames.empty()) return;

        currentTime += dt;
        
        // �t���[���؂�ւ�
        if (currentTime >= frameTime) {
            currentTime -= frameTime;
            currentFrame++;
            
            if (currentFrame >= frames.size()) {
                if (loop) {
                    currentFrame = 0;
                } else {
                    currentFrame = frames.size() - 1;
                    playing = false;
                    finished = true;
                }
            }
        }
    }

    // ���݂̃e�N�X�`�����擾
    TextureManager::TextureHandle GetCurrentTexture() const {
        if (frames.empty()) return TextureManager::INVALID_TEXTURE;
        return frames[currentFrame];
    }

    // �A�j���[�V�������Đ�
    void Play() {
        playing = true;
        finished = false;
    }

    // �A�j���[�V�������~
    void Stop() {
        playing = false;
    }

    // �A�j���[�V���������Z�b�g
    void Reset() {
        currentFrame = 0;
        currentTime = 0.0f;
        finished = false;
    }
};

// ========================================================
// UVAnimation - UV�X�N���[���A�j���[�V����
// ========================================================
// �y�����z
// �e�N�X�`�����X�N���[�������ē����Ă���悤�Ɍ�����
// �i��: ����鐅�A�����_�A�x���g�R���x�A�j
//
// �y�g�����z
// UVAnimation uv;
// uv.scrollSpeed = DirectX::XMFLOAT2{0.5f, 0.0f}; // �������ɗ����
// world.Add<UVAnimation>(entity, uv);
//
// �܂��͊Ȍ���:
// world.Add<UVAnimation>(entity, UVAnimation{0.5f, 0.0f});
// ========================================================
struct UVAnimation : Behaviour {
    DirectX::XMFLOAT2 scrollSpeed{ 0.0f, 0.0f }; // UV���W�̃X�N���[�����x�i�P��/�b�j
    DirectX::XMFLOAT2 currentOffset{ 0.0f, 0.0f }; // ���݂̃I�t�Z�b�g�i�����X�V�j

    // �f�t�H���g�R���X�g���N�^
    UVAnimation() = default;
    
    // �X�N���[�����x���w�肷��R���X�g���N�^
    explicit UVAnimation(const DirectX::XMFLOAT2& speed) : scrollSpeed(speed) {}
    
    // �ʂ�U,V���w�肷��R���X�g���N�^
    UVAnimation(float u, float v) : scrollSpeed{u, v} {}

    void OnUpdate(World& w, Entity self, float dt) override {
        // �X�N���[���ʂ����Z
        currentOffset.x += scrollSpeed.x * dt;
        currentOffset.y += scrollSpeed.y * dt;
        
        // 0-1�͈̔͂ɐ��K���i�J��Ԃ��j
        currentOffset.x = fmodf(currentOffset.x, 1.0f);
        currentOffset.y = fmodf(currentOffset.y, 1.0f);
        
        if (currentOffset.x < 0.0f) currentOffset.x += 1.0f;
        if (currentOffset.y < 0.0f) currentOffset.y += 1.0f;
    }
};
