#pragma once

// ========================================================
// Component - ECS�R���|�[�l���g�̊��C���^�[�t�F�[�X
// ========================================================
struct IComponent {
    virtual ~IComponent() = default;
};

// ========================================================
// Behaviour - �X�V�\�ȃR���|�[�l���g���N���X
// ========================================================
class World; // �O���錾
struct Entity; // �O���錾

struct Behaviour : IComponent {
    virtual void OnStart(World&, Entity) {}
    virtual void OnUpdate(World&, Entity, float /*dt*/) {}
};
