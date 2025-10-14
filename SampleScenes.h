// ========================================================
// SampleScenes.h - �w�K�p�T���v���V�[���W
// ========================================================
// �y�ړI�z���w�҂��R���|�[�l���g�w����i�K�I�Ɋw�ׂ�T���v��
// �y�g�����z�e�֐����R�s�[���ĉ������Ă݂悤�I
// ========================================================
#pragma once

#include "World.h"
#include "Entity.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Rotator.h"
#include "Animation.h"
#include "ComponentSamples.h"
#include <DirectXMath.h>

namespace SampleScenes {

// ========================================================
// ���x��1: �ł��V���v���ȃG���e�B�e�B
// ========================================================
// �y�w�ׂ邱�Ɓz
// - �G���e�B�e�B�̍쐬���@
// - Transform�R���|�[�l���g�̐ݒ�
// - MeshRenderer�ŐF��t����
// ========================================================

inline Entity CreateSimpleCube(World& world) {
    // �G���e�B�e�B���쐬�i�r���_�[�p�^�[���j
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},  // �ʒu
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},  // ��]
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}   // �X�P�[��
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f}) // �ԐF
        .Build();
    
    return cube;
}

// ========================================================
// ���x��2: �����̂���G���e�B�e�B
// ========================================================
// �y�w�ׂ邱�Ɓz
// - Behaviour�R���|�[�l���g�iRotator�j�̎g����
// - �R���|�[�l���g�̑g�ݍ��킹
// ========================================================

inline Entity CreateRotatingCube(World& world, const DirectX::XMFLOAT3& position) {
    Entity cube = world.Create()
        .With<Transform>(
            position,
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f}) // �ΐF
        .With<Rotator>(45.0f) // ���b45�x��]
        .Build();
    
    return cube;
}

// ========================================================
// ���x��3: �J�X�^��Behaviour���g��
// ========================================================
// �y�w�ׂ邱�Ɓz
// - ComponentSamples.h�̃J�X�^��Behaviour���g��
// - �����̃R���|�[�l���g��g�ݍ��킹��
// ========================================================

inline Entity CreateBouncingCube(World& world) {
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{-3.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.8f, 0.8f, 0.8f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 1.0f, 0.0f}) // ���F
        .With<Bouncer>() // �㉺�ɓ����iComponentSamples.h�Q�Ɓj
        .Build();
    
    return cube;
}

// ========================================================
// ���x��4: ������Behaviour��g�ݍ��킹��
// ========================================================
// �y�w�ׂ邱�Ɓz
// - 1�̃G���e�B�e�B�ɕ�����Behaviour��ǉ�
// - ���ꂼ�ꂪ�Ɨ����ē��삷��
// ========================================================

inline Entity CreateComplexCube(World& world) {
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{3.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 1.0f}) // �}�[���^
        .With<Rotator>(30.0f)    // ��]����
        .With<PulseScale>()      // �傫�����ς��iComponentSamples.h�Q�Ɓj
        .Build();
    
    return cube;
}

// ========================================================
// ���x��5: �]���̕��@�ŃG���e�B�e�B���쐬
// ========================================================
// �y�w�ׂ邱�Ɓz
// - �r���_�[�p�^�[�����g��Ȃ����@
// - �ォ��R���|�[�l���g��ǉ�������@
// ========================================================

inline Entity CreateCubeOldStyle(World& world) {
    // �X�e�b�v1: �G���e�B�e�B���쐬
    Entity cube = world.CreateEntity();
    
    // �X�e�b�v2: Transform�R���|�[�l���g��ǉ�
    Transform transform;
    transform.position = DirectX::XMFLOAT3{0.0f, -2.0f, 0.0f};
    transform.rotation = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
    transform.scale = DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f};
    world.Add<Transform>(cube, transform);
    
    // �X�e�b�v3: MeshRenderer�R���|�[�l���g��ǉ�
    MeshRenderer renderer;
    renderer.color = DirectX::XMFLOAT3{0.0f, 1.0f, 1.0f}; // �V�A��
    world.Add<MeshRenderer>(cube, renderer);
    
    // �X�e�b�v4: Rotator�R���|�[�l���g��ǉ�
    Rotator rotator;
    rotator.speedDegY = 90.0f;
    world.Add<Rotator>(cube, rotator);
    
    return cube;
}

// ========================================================
// ���x��6: �R���|�[�l���g�̌ォ��̕ύX
// ========================================================
// �y�w�ׂ邱�Ɓz
// - TryGet�Ŏ擾���Ēl��ύX
// - �R���|�[�l���g�̓��I�ȑ���
// ========================================================

inline void ModifyEntityExample(World& world, Entity entity) {
    // Transform���擾���ĕύX
    auto* transform = world.TryGet<Transform>(entity);
    if (transform) {
        transform->position.y += 1.0f; // Y���W��1�グ��
        transform->scale = DirectX::XMFLOAT3{2.0f, 2.0f, 2.0f}; // 2�{�̑傫����
    }
    
    // MeshRenderer���擾���ĐF��ύX
    auto* renderer = world.TryGet<MeshRenderer>(entity);
    if (renderer) {
        renderer->color = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}; // ���ɕύX
    }
    
    // Rotator���擾���đ��x��ύX
    auto* rotator = world.TryGet<Rotator>(entity);
    if (rotator) {
        rotator->speedDegY = 180.0f; // ���x��2�{��
    }
}

// ========================================================
// ���x��7: �S�G���e�B�e�B�ɑ΂��鏈��
// ========================================================
// �y�w�ׂ邱�Ɓz
// - ForEach�őS�G���e�B�e�B������
// - �����_���̎g����
// ========================================================

inline void ProcessAllTransforms(World& world) {
    // �S�Ă�Transform�����G���e�B�e�B�ɑ΂��ď���
    world.ForEach<Transform>([](Entity entity, Transform& transform) {
        // �S�ẴG���e�B�e�B��������Ƃ���Ɉړ�
        transform.position.y += 0.01f;
    });
}

inline void ChangeAllColors(World& world) {
    // �S�Ă�MeshRenderer�̐F��ύX
    world.ForEach<MeshRenderer>([](Entity entity, MeshRenderer& renderer) {
        // �S�ẴG���e�B�e�B��Ԃ��ۂ�����
        renderer.color.x = 1.0f; // R�������ő��
    });
}

// ========================================================
// ���x��8: �f���V�[���쐬
// ========================================================
// �y�w�ׂ邱�Ɓz
// - �����̃G���e�B�e�B��z�u���ăV�[�����\��
// - �ʒu���v�Z���ăO���b�h��ɔz�u
// ========================================================

inline void CreateGridOfCubes(World& world, int rows = 3, int cols = 3) {
    const float spacing = 2.5f; // �L���[�u�Ԃ̋���
    
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // �ʒu���v�Z�i���S�����_�Ɂj
            float x = (col - cols / 2.0f) * spacing;
            float z = (row - rows / 2.0f) * spacing;
            
            // �F���v�Z�i�ʒu�ɂ���ĕς���j
            float r = static_cast<float>(col) / static_cast<float>(cols - 1);
            float b = static_cast<float>(row) / static_cast<float>(rows - 1);
            
            // �L���[�u���쐬
            world.Create()
                .With<Transform>(
                    DirectX::XMFLOAT3{x, 0.0f, z},
                    DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
                    DirectX::XMFLOAT3{0.8f, 0.8f, 0.8f}
                )
                .With<MeshRenderer>(DirectX::XMFLOAT3{r, 0.5f, b})
                .With<Rotator>(45.0f + static_cast<float>(row * 10 + col * 5))
                .Build();
        }
    }
}

// ========================================================
// ���x��9: ���K���̉𓚗�
// ========================================================

// ���K1: ���F�ɉ�]����L���[�u�����
inline Entity CreateRainbowCube(World& world) {
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{0.0f, 3.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f})
        .With<Rotator>(120.0f) // ������]
        .With<ColorCycle>()    // �F���ς��iComponentSamples.h�j
        .Build();
    
    return cube;
}

// ���K2: �����_���ɓ������L���[�u
inline Entity CreateWanderingCube(World& world) {
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.6f, 0.6f, 0.6f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{0.8f, 0.3f, 0.9f})
        .With<RandomWalk>() // �����_���ړ��iComponentSamples.h�j
        .Build();
    
    return cube;
}

// ���K3: ���Ԍo�߂ŏ�����L���[�u
inline Entity CreateTemporaryCube(World& world, float lifeTime = 5.0f) {
    Entity cube = world.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{0.0f, 5.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.5f, 0.0f})
        .With<Rotator>(200.0f)
        .Build();
    
    // �����R���|�[�l���g��ǉ�
    LifeTime lt;
    lt.remainingTime = lifeTime;
    world.Add<LifeTime>(cube, lt);
    
    return cube;
}

} // namespace SampleScenes

// ========================================================
// �g�����̗�
// ========================================================
/*

// App.h��CreateDemoScene()�Ŏg���ꍇ:

void CreateDemoScene() {
    // �V���v���ȃL���[�u
    SampleScenes::CreateSimpleCube(world_);
    
    // ��]����L���[�u�i�ʒu���w��j
    SampleScenes::CreateRotatingCube(world_, DirectX::XMFLOAT3{-3, 0, 0});
    
    // �㉺�ɓ����L���[�u
    SampleScenes::CreateBouncingCube(world_);
    
    // ���G�ȓ����̃L���[�u
    SampleScenes::CreateComplexCube(world_);
    
    // 3x3�̃O���b�h
    SampleScenes::CreateGridOfCubes(world_, 3, 3);
    
    // ���K���̉𓚗�
    SampleScenes::CreateRainbowCube(world_);
    SampleScenes::CreateWanderingCube(world_);
    SampleScenes::CreateTemporaryCube(world_, 10.0f);
}

*/

// ========================================================
// �쐬��: �R���z
// �o�[�W����: v4.0 - �i�K�I�w�K�p�T���v���W
// ========================================================
