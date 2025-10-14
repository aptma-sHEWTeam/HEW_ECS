#pragma once
// ========================================================
// SceneManager.h - �V�[���Ǘ��V�X�e��
// ========================================================
// �y�����z�Q�[���̉�ʁi�V�[���j��؂�ւ���
// �y��z�^�C�g����� �� �Q�[����� �� ���U���g���
// ========================================================

#include "World.h"
#include "InputSystem.h"
#include <unordered_map>
#include <string>

// ========================================================
// IScene - �V�[���̊��N���X
// ========================================================
// �y�g�����z
// �e�V�[���͂��̃N���X���p�����āA�ȉ�����������F
// - OnEnter(): �V�[�����n�܂�Ƃ���1�񂾂��Ă΂��
// - OnUpdate(): ���t���[���Ă΂��i�Q�[�����W�b�N�j
// - OnExit(): �V�[�����I���Ƃ���1�񂾂��Ă΂��
// ========================================================
class IScene {
public:
    virtual ~IScene() = default;

    // �V�[���J�n���̏����i�G���e�B�e�B�̐����Ȃǁj
    virtual void OnEnter(World& world) = 0;

    // ���t���[���̍X�V�����i���́A�ړ��A�Փ˔���Ȃǁj
    virtual void OnUpdate(World& world, InputSystem& input, float deltaTime) = 0;

    // �V�[���I�����̏����i�N���[���A�b�v�j
    virtual void OnExit(World& world) = 0;

    // ���̃V�[���֑J�ڂ��邩�H
    virtual bool ShouldChangeScene() const { return false; }

    // ���̃V�[������Ԃ�
    virtual const char* GetNextScene() const { return nullptr; }
};

// ========================================================
// SceneManager - �V�[���؂�ւ��Ǘ�
// ========================================================
class SceneManager {
public:
    // �������i�ŏ��̃V�[����ݒ�j
    void Init(IScene* startScene, World& world) {
        currentScene_ = startScene;
        if (currentScene_) {
            currentScene_->OnEnter(world);
        }
    }

    // �V�[����o�^�i���O�ŃV�[����؂�ւ�����悤�ɂ���j
    void RegisterScene(const char* name, IScene* scene) {
        scenes_[name] = scene;
    }

    // ���t���[���̍X�V
    void Update(World& world, InputSystem& input, float deltaTime) {
        if (!currentScene_) return;

        // ���݂̃V�[�����X�V
        currentScene_->OnUpdate(world, input, deltaTime);

        // �V�[���J�ڃ`�F�b�N
        if (currentScene_->ShouldChangeScene()) {
            const char* nextSceneName = currentScene_->GetNextScene();
            ChangeScene(nextSceneName, world);
        }
    }

    // �V�[����؂�ւ�
    void ChangeScene(const char* sceneName, World& world) {
        if (!sceneName) return;

        auto it = scenes_.find(sceneName);
        if (it == scenes_.end()) return;

        // ���݂̃V�[�����I��
        if (currentScene_) {
            currentScene_->OnExit(world);
        }

        // �V�����V�[�����J�n
        currentScene_ = it->second;
        if (currentScene_) {
            currentScene_->OnEnter(world);
        }
    }

    ~SceneManager() {
        // �e�V�[�����폜
        for (auto& pair : scenes_) {
            delete pair.second;
        }
    }

private:
    IScene* currentScene_ = nullptr;
    std::unordered_map<std::string, IScene*> scenes_;
};
