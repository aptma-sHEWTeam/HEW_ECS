#pragma once
// ========================================================
// MiniGame.h - �V���v���ȃV���[�e�B���O�Q�[��
// ========================================================
// �y�Q�[�����e�z
// - �v���C���[�i�΃L���[�u�j��A/D�L�[�ō��E�Ɉړ�
// - �X�y�[�X�L�[�Œe�𔭎�
// - �G�i�ԃL���[�u�j���ォ��~���Ă���
// - �e���G�ɓ�����ƓG��������
// - �X�R�A���҂����I
// ========================================================

#include "SceneManager.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Component.h"
#include <cstdlib>
#include <ctime>
#include <vector>

// ========================================================
// �Q�[���p�R���|�[�l���g
// ========================================================

// �v���C���[�^�O
struct Player : IComponent {};

// �G�^�O
struct Enemy : IComponent {};

// �e�^�O
struct Bullet : IComponent {};

// �v���C���[�̈ړ�
struct PlayerMovement : Behaviour {
    float speed = 8.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        // �L�[�{�[�h���͂�GameScene�ŏ���
        // �����ł͈ʒu�̐����̂�
        if (t->position.x < -8.0f) t->position.x = -8.0f;
        if (t->position.x > 8.0f) t->position.x = 8.0f;
    }
};

// �e�̈ړ��i��ɐi�ށj
struct BulletMovement : Behaviour {
    float speed = 15.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        t->position.y += speed * dt;
        
        // ��ʊO�ɏo����폜
        if (t->position.y > 10.0f) {
            w.DestroyEntity(self);
        }
    }
};

// �G�̈ړ��i���ɐi�ށj
struct EnemyMovement : Behaviour {
    float speed = 3.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        t->position.y -= speed * dt;
        
        // ��ʉ��ɏo����폜
        if (t->position.y < -8.0f) {
            w.DestroyEntity(self);
        }
    }
};

// ========================================================
// �Q�[���V�[��
// ========================================================
class GameScene : public IScene {
public:
    void OnEnter(World& world) override {
        // �����_���V�[�h������
        srand(static_cast<unsigned int>(time(nullptr)));
        
        // �v���C���[���쐬
        playerEntity_ = world.Create()
            .With<Transform>(
                DirectX::XMFLOAT3{0.0f, -6.0f, 0.0f},   // ��ʉ���
                DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
                DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}
            )
            .With<MeshRenderer>(DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f}) // �ΐF
            .With<Player>()
            .With<PlayerMovement>()
            .Build();
        
        score_ = 0;
        enemySpawnTimer_ = 0.0f;
        shootCooldown_ = 0.0f;
    }
    
    void OnUpdate(World& world, InputSystem& input, float deltaTime) override {
        // �v���C���[�ړ�
        UpdatePlayerMovement(world, input, deltaTime);
        
        // �e�̔���
        UpdateShooting(world, input, deltaTime);
        
        // �G�̐���
        UpdateEnemySpawning(world, deltaTime);
        
        // �Փ˔���
        CheckCollisions(world);
        
        // �Q�[�����W�b�N�̍X�V
        world.Tick(deltaTime);
    }
    
    void OnExit(World& world) override {
        // �S�G���e�B�e�B���폜�i�C�e���[�^��������h���j
        std::vector<Entity> entitiesToDestroy;
        
        world.ForEach<Transform>([&](Entity e, Transform& t) {
            entitiesToDestroy.push_back(e);
        });
        
        for (const auto& entity : entitiesToDestroy) {
            world.DestroyEntity(entity);
        }
    }
    
    int GetScore() const { return score_; }

private:
    // �v���C���[�̈ړ�����
    void UpdatePlayerMovement(World& world, InputSystem& input, float deltaTime) {
        auto* playerTransform = world.TryGet<Transform>(playerEntity_);
        if (!playerTransform) return;
        
        const float moveSpeed = 8.0f;
        
        // A�L�[�ō��ړ�
        if (input.GetKey('A')) {
            playerTransform->position.x -= moveSpeed * deltaTime;
        }
        
        // D�L�[�ŉE�ړ�
        if (input.GetKey('D')) {
            playerTransform->position.x += moveSpeed * deltaTime;
        }
    }
    
    // �e�̔��ˏ���
    void UpdateShooting(World& world, InputSystem& input, float deltaTime) {
        shootCooldown_ -= deltaTime;
        
        // �X�y�[�X�L�[�Œe�𔭎ˁi�N�[���_�E�����͔��˂ł��Ȃ��j
        if (input.GetKey(VK_SPACE) && shootCooldown_ <= 0.0f) {
            auto* playerTransform = world.TryGet<Transform>(playerEntity_);
            if (playerTransform) {
                // �v���C���[�̈ʒu����e�𔭎�
                world.Create()
                    .With<Transform>(
                        DirectX::XMFLOAT3{playerTransform->position.x, playerTransform->position.y + 1.0f, 0.0f},
                        DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
                        DirectX::XMFLOAT3{0.3f, 0.5f, 0.3f}  // ������
                    )
                    .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 1.0f, 0.0f}) // ���F
                    .With<Bullet>()
                    .With<BulletMovement>()
                    .Build();
                
                shootCooldown_ = 0.2f; // 0.2�b�̃N�[���_�E��
            }
        }
    }
    
    // �G�̐�������
    void UpdateEnemySpawning(World& world, float deltaTime) {
        enemySpawnTimer_ += deltaTime;
        
        // 1�b���ƂɓG�𐶐�
        if (enemySpawnTimer_ >= 1.0f) {
            enemySpawnTimer_ = 0.0f;
            
            // �����_���Ȉʒu�ɓG��z�u
            float randomX = (rand() % 1600 - 800) / 100.0f; // -8.0 ~ 8.0
            
            world.Create()
                .With<Transform>(
                    DirectX::XMFLOAT3{randomX, 8.0f, 0.0f},  // ��ʏ㕔
                    DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
                    DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}
                )
                .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f}) // �ԐF
                .With<Enemy>()
                .With<EnemyMovement>()
                .Build();
        }
    }
    
    // �Փ˔���
    void CheckCollisions(World& world) {
        // �폜����G���e�B�e�B�����W�i�C�e���[�^��������h���j
        std::vector<Entity> entitiesToDestroy;
        
        // �e�ƓG�̏Փ˂��`�F�b�N
        world.ForEach<Bullet>([&](Entity bulletEntity, Bullet& bullet) {
            auto* bulletTransform = world.TryGet<Transform>(bulletEntity);
            if (!bulletTransform) return;
            
            // ���̒e�����łɍ폜�\��Ȃ珈�����X�L�b�v
            for (const auto& e : entitiesToDestroy) {
                if (e.id == bulletEntity.id) return;
            }
            
            world.ForEach<Enemy>([&](Entity enemyEntity, Enemy& enemy) {
                auto* enemyTransform = world.TryGet<Transform>(enemyEntity);
                if (!enemyTransform) return;
                
                // ���̓G�����łɍ폜�\��Ȃ珈�����X�L�b�v
                for (const auto& e : entitiesToDestroy) {
                    if (e.id == enemyEntity.id) return;
                }
                
                // �ȈՓI�ȋ�������i���̏Փˁj
                float dx = bulletTransform->position.x - enemyTransform->position.x;
                float dy = bulletTransform->position.y - enemyTransform->position.y;
                float distance = sqrtf(dx * dx + dy * dy);
                
                // �Փ˂�����폜���X�g�ɒǉ����ăX�R�A���Z
                if (distance < 1.0f) {
                    entitiesToDestroy.push_back(bulletEntity);
                    entitiesToDestroy.push_back(enemyEntity);
                    score_ += 10;
                }
            });
        });
        
        // �C�e���[�V����������Ɉꊇ�폜
        for (const auto& entity : entitiesToDestroy) {
            world.DestroyEntity(entity);
        }
    }

    Entity playerEntity_;
    int score_;
    float enemySpawnTimer_;
    float shootCooldown_;
};
