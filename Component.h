#pragma once

// ========================================================
// Component - ECS�R���|�[�l���g�V�X�e��
// ========================================================
// �y�R���|�[�l���g�Ƃ́H�z
// �Q�[���I�u�W�F�N�g�Ɏ��t����u���i�v�̂���
// ��: �u�ʒu�v�u�����ځv�u�����v�Ȃǂ�ʁX�̃R���|�[�l���g�Ƃ��ĊǗ�
// ========================================================

class World; // �O���錾
struct Entity; // �O���錾

// ========================================================
// IComponent - ���ׂẴR���|�[�l���g�̊��C���^�[�t�F�[�X
// ========================================================
// �y�����z�^����ێ����邽�߂̋��ʂ̐e�N���X
// �i���w�҂͓��Ɉӎ����Ȃ���OK�j
struct IComponent {
    virtual ~IComponent() = default;
};

// ========================================================
// Behaviour - ���t���[���X�V�����R���|�[�l���g
// ========================================================
// �y���g���H�z
// - �I�u�W�F�N�g�𓮂�������
// - ���Ԍo�߂ŉ���������������
// - �A�j���[�V������������
// 
// �y�g�����z
// struct MyBehaviour : Behaviour {
//     void OnStart(World& w, Entity self) override {
//         // �ŏ���1�񂾂��Ă΂��i�����������j
//     }
//     void OnUpdate(World& w, Entity self, float dt) override {
//         // ���t���[���Ă΂��idt = �O�t���[������̌o�ߎ��ԁj
//     }
// };
// ========================================================
struct Behaviour : IComponent {
    // �ŏ���1�񂾂��Ă΂��i�������p�j
    virtual void OnStart(World&, Entity) {}
    
    // ���t���[���Ă΂��idt = �f���^�^�C�� = �O�t���[������̌o�ߕb���j
    virtual void OnUpdate(World&, Entity, float dt) {}
};

// ========================================================
// �ȒP�R���|�[�l���g��`�}�N��
// ========================================================
// �y�g�����z�f�[�^�����̃R���|�[�l���g��1�s�Œ�`�ł���
//
// ��: �̗̓R���|�[�l���g
// DEFINE_DATA_COMPONENT(Health, float hp = 100.0f; float maxHp = 100.0f;)
//
// ��: ���x�R���|�[�l���g
// DEFINE_DATA_COMPONENT(Velocity, 
//     DirectX::XMFLOAT3 velocity{0, 0, 0};
// )
// ========================================================
#define DEFINE_DATA_COMPONENT(ComponentName, ...) \
    struct ComponentName : IComponent { \
        __VA_ARGS__ \
    }

// ========================================================
// �ȒPBehaviour��`�}�N��
// ========================================================
// �y�g�����z�����̂���R���|�[�l���g���ȒP�ɒ�`
//
// ��: �㉺�ɓ����R���|�[�l���g
// DEFINE_BEHAVIOUR(Bouncer,
//     float speed = 1.0f;
//     float time = 0.0f;
// ,
//     time += dt * speed;
//     auto* t = w.TryGet<Transform>(self);
//     if (t) t->position.y = sinf(time);
// )
// ========================================================
#define DEFINE_BEHAVIOUR(BehaviourName, DataMembers, UpdateCode) \
    struct BehaviourName : Behaviour { \
        DataMembers \
        void OnUpdate(World& w, Entity self, float dt) override { \
            UpdateCode \
        } \
    }
