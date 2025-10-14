// ========================================================
// ComponentSamples.h - �w�K�p�R���|�[�l���g�W
// ========================================================
// �y�ړI�z�R�s�y���Ďg������p�I�ȃR���|�[�l���g�T���v��
// �y�w�ѕ��z�R�[�h��ǂ� �� �������� �� �����ō��
// ========================================================
#pragma once

#include "Component.h"
#include "World.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstdlib>

// ========================================================
// �J�e�S��1: �f�[�^�����̃R���|�[�l���g
// ========================================================
// �y�����z��Ԃ�ۑ����邾���A�����Ȃ�
// �y�g�����z����Behaviour����Q�Ƃ����
// ========================================================

// �̗̓R���|�[�l���g
struct Health : IComponent {
    float current = 100.0f;  // ���݂̗̑�
    float max = 100.0f;      // �ő�̗�
    
    // �_���[�W���󂯂�
    void TakeDamage(float damage) {
        current -= damage;
        if (current < 0.0f) current = 0.0f;
    }
    
    // �񕜂���
    void Heal(float amount) {
        current += amount;
        if (current > max) current = max;
    }
    
    // ����ł��邩
    bool IsDead() const {
        return current <= 0.0f;
    }
};

// ���x�R���|�[�l���g
struct Velocity : IComponent {
    DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    
    // ���x��������
    void AddVelocity(float x, float y, float z) {
        velocity.x += x;
        velocity.y += y;
        velocity.z += z;
    }
};

// �X�R�A�R���|�[�l���g�i�}�N���Łj
DEFINE_DATA_COMPONENT(Score,
    int points = 0;
    
    void AddPoints(int p) {
        points += p;
    }
    
    void Reset() {
        points = 0;
    }
);

// ���O�R���|�[�l���g�i�}�N���Łj
DEFINE_DATA_COMPONENT(Name,
    const char* name = "Unnamed";
);

// �^�O�R���|�[�l���g�i�f�[�^�Ȃ��j
struct PlayerTag : IComponent {};
struct EnemyTag : IComponent {};
struct BulletTag : IComponent {};

// ========================================================
// �J�e�S��2: �V���v����Behaviour�i1�̋@�\�j
// ========================================================
// �y�����z1�̖��m�ȓ��������
// �y�w�K�|�C���g�zOnUpdate�̎g����
// ========================================================

// �㉺�ɓ����i�o�E���X�j
struct Bouncer : Behaviour {
    float speed = 2.0f;      // �������x
    float amplitude = 2.0f;  // ������
    float time = 0.0f;       // �o�ߎ��ԁi�����Ǘ��j
    float startY = 0.0f;     // �J�n�ʒu�i�����Ǘ��j
    
    void OnStart(World& w, Entity self) override {
        // �ŏ���1�񂾂��Ă΂��
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            startY = t->position.y; // �J�n�ʒu���L�^
        }
    }
    
    void OnUpdate(World& w, Entity self, float dt) override {
        // ���t���[���Ă΂��
        time += dt * speed;
        
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            // sin�g�ŏ㉺�ɓ�����
            t->position.y = startY + sinf(time) * amplitude;
        }
    }
};

// �O�ɐi��
struct MoveForward : Behaviour {
    float speed = 2.0f; // �O�i���x�i�P��/�b�j
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        // Z�������i�O�j�ɐi��
        t->position.z += speed * dt;
        
        // �����ɍs������폜�i�I�v�V�����j
        if (t->position.z > 20.0f) {
            w.DestroyEntity(self);
        }
    }
};

// �g��k���i�p���X�j
struct PulseScale : Behaviour {
    float speed = 3.0f;           // �p���X���x
    float minScale = 0.5f;        // �ŏ��X�P�[��
    float maxScale = 1.5f;        // �ő�X�P�[��
    float time = 0.0f;            // �o�ߎ��ԁi�����Ǘ��j
    
    void OnUpdate(World& w, Entity self, float dt) override {
        time += dt * speed;
        
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        // sin�g�ŃX�P�[����ω�
        float scale = minScale + (maxScale - minScale) * (sinf(time) * 0.5f + 0.5f);
        t->scale = DirectX::XMFLOAT3{ scale, scale, scale };
    }
};

// �F���ς��i�T�C�N���j
struct ColorCycle : Behaviour {
    float speed = 1.0f;  // �F�ω��̑��x
    float time = 0.0f;   // �o�ߎ��ԁi�����Ǘ��j
    
    void OnUpdate(World& w, Entity self, float dt) override {
        time += dt * speed;
        
        auto* mr = w.TryGet<MeshRenderer>(self);
        if (!mr) return;
        
        // HSV���ɐF��ω��i���F�j
        float hue = fmodf(time, 1.0f);
        mr->color.x = sinf(hue * DirectX::XM_2PI) * 0.5f + 0.5f;
        mr->color.y = sinf((hue + 0.333f) * DirectX::XM_2PI) * 0.5f + 0.5f;
        mr->color.z = sinf((hue + 0.666f) * DirectX::XM_2PI) * 0.5f + 0.5f;
    }
};

// ========================================================
// �J�e�S��3: �������G��Behaviour�i�����̋@�\�j
// ========================================================
// �y�����z�����̃R���|�[�l���g��g�ݍ��킹��
// �y�w�K�|�C���g�z�R���|�[�l���g�Ԃ̘A�g
// ========================================================

// �̗͂�0�ɂȂ�����폜
struct DestroyOnDeath : Behaviour {
    void OnUpdate(World& w, Entity self, float dt) override {
        // Health�R���|�[�l���g���m�F
        auto* health = w.TryGet<Health>(self);
        if (!health) return;
        
        // �̗͂�0�ȉ��Ȃ�폜
        if (health->IsDead()) {
            w.DestroyEntity(self);
        }
    }
};

// �����_���ɓ������
struct RandomWalk : Behaviour {
    float speed = 2.0f;             // �ړ����x
    float changeInterval = 2.0f;    // �����]���̊Ԋu�i�b�j
    float timer = 0.0f;             // �^�C�}�[�i�����Ǘ��j
    DirectX::XMFLOAT3 direction{ 1.0f, 0.0f, 0.0f }; // ���݂̕���
    
    void OnStart(World& w, Entity self) override {
        // �����_���ȏ�������
        ChooseRandomDirection();
    }
    
    void OnUpdate(World& w, Entity self, float dt) override {
        timer += dt;
        
        // ��莞�Ԃ��Ƃɕ����]��
        if (timer >= changeInterval) {
            timer = 0.0f;
            ChooseRandomDirection();
        }
        
        // �ړ�
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        t->position.x += direction.x * speed * dt;
        t->position.y += direction.y * speed * dt;
        t->position.z += direction.z * speed * dt;
        
        // �͈͊O�ɏo����߂�
        ClampPosition(t);
    }
    
private:
    void ChooseRandomDirection() {
        // -1.0 �` 1.0 �̃����_���Ȓl
        direction.x = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        direction.y = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        direction.z = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        
        // ���K���i������1�Ɂj
        float length = sqrtf(direction.x * direction.x + 
                           direction.y * direction.y + 
                           direction.z * direction.z);
        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        }
    }
    
    void ClampPosition(Transform* t) {
        const float range = 10.0f;
        if (t->position.x < -range) t->position.x = -range;
        if (t->position.x > range) t->position.x = range;
        if (t->position.y < -range) t->position.y = -range;
        if (t->position.y > range) t->position.y = range;
        if (t->position.z < -range) t->position.z = -range;
        if (t->position.z > range) t->position.z = range;
    }
};

// ���Ԍo�߂ō폜
struct LifeTime : Behaviour {
    float remainingTime = 5.0f; // �c�莞�ԁi�b�j
    
    void OnUpdate(World& w, Entity self, float dt) override {
        remainingTime -= dt;
        
        // ���Ԑ؂�ō폜
        if (remainingTime <= 0.0f) {
            w.DestroyEntity(self);
        }
    }
};

// ========================================================
// �J�e�S��4: �}�N�����g�����Ȍ��Ȓ�`
// ========================================================
// �y�����zDEFINE_BEHAVIOUR�}�N���ŒZ��������
// �y�w�K�|�C���g�z�{�C���[�v���[�g�̍팸
// ========================================================

// ��]���Ȃ���F���ς��i�}�N���Łj
DEFINE_BEHAVIOUR(SpinAndColor,
    float rotSpeed = 90.0f;
    float colorSpeed = 1.0f;
    float time = 0.0f;
,
    time += dt * colorSpeed;
    
    // ��]
    auto* t = w.TryGet<Transform>(self);
    if (t) {
        t->rotation.y += rotSpeed * dt;
    }
    
    // �F�ω�
    auto* mr = w.TryGet<MeshRenderer>(self);
    if (mr) {
        float hue = fmodf(time, 1.0f);
        mr->color.x = sinf(hue * 6.28f) * 0.5f + 0.5f;
        mr->color.y = cosf(hue * 6.28f) * 0.5f + 0.5f;
        mr->color.z = 0.5f;
    }
);

// �~�O����`���i�}�N���Łj
DEFINE_BEHAVIOUR(CircularMotion,
    float radius = 3.0f;
    float speed = 1.0f;
    float angle = 0.0f;
    float centerY = 0.0f;
,
    angle += speed * dt;
    
    auto* t = w.TryGet<Transform>(self);
    if (t) {
        t->position.x = cosf(angle) * radius;
        t->position.z = sinf(angle) * radius;
        t->position.y = centerY;
    }
);

// ========================================================
// �g�����̗�
// ========================================================
/*

// ��1: �㉺�ɓ����Ԃ��L���[�u
Entity cube = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<Bouncer>()
    .Build();

// ��2: 5�b��ɏ�����L���[�u
Entity temp = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
    .Build();

LifeTime lt;
lt.remainingTime = 5.0f;
world.Add<LifeTime>(temp, lt);

// ��3: �̗̓V�X�e���t���L���[�u
Entity enemy = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<EnemyTag>()
    .Build();

Health hp;
hp.current = 50.0f;
hp.max = 50.0f;
world.Add<Health>(enemy, hp);
world.Add<DestroyOnDeath>(enemy, DestroyOnDeath{});

// �_���[�W��^����
auto* health = world.TryGet<Health>(enemy);
if (health) {
    health->TakeDamage(10.0f);
}

*/

// ========================================================
// �쐬��: �R���z
// �o�[�W����: v4.0 - �w�K�p�R���|�[�l���g�W
// ========================================================
