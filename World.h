#pragma once
#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

// ========================================================
// EntityBuilder - �G���e�B�e�B�쐬���ȒP�ɂ���w���p�[�N���X
// ========================================================
// �y�g�����z���\�b�h�`�F�[���ŃR���|�[�l���g��A���ǉ�
//
// world.Create()
//     .With<Transform>(pos, rot, scale)
//     .With<MeshRenderer>(color)
//     .With<Rotator>(speed)
//     .Build();
//
// ���ꂾ����3�̃R���|�[�l���g�����G���e�B�e�B���ł���I
// ========================================================
class World; // �O���錾

class EntityBuilder {
public:
    EntityBuilder(World* world, Entity entity) : world_(world), entity_(entity) {}
    
    // �R���|�[�l���g��ǉ��i���\�b�h�`�F�[���\�j
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args);
    
    // �G���e�B�e�B���m�肵�ĕԂ�
    Entity Build() { return entity_; }
    
    // �ÖٓI��Entity�ɕϊ��\�iBuild()���ȗ��ł���j
    operator Entity() const { return entity_; }

private:
    World* world_;
    Entity entity_;
};

// ========================================================
// World - ECS���[���h�Ǘ��N���X
// ========================================================
// �y�����z
// - �G���e�B�e�B�̍쐬�E�폜
// - �R���|�[�l���g�̒ǉ��E�폜�E�擾
// - �S�R���|�[�l���g�̍X�V
//
// �y���w�Ҍ����K�C�h�z
// World�́u�Q�[�����E�v���̂���
// - CreateEntity() �ŃQ�[���I�u�W�F�N�g�����
// - Add<�R���|�[�l���g>() �ŋ@�\��ǉ�
// - TryGet<�R���|�[�l���g>() �ŋ@�\���擾
// - Tick() �őS�I�u�W�F�N�g���X�V
// ========================================================
class World {
public:
    // ========================================================
    // �G���e�B�e�B�̍쐬�i��{�Łj
    // ========================================================
    Entity CreateEntity() {
        Entity e{ ++nextId_ };
        alive_[e.id] = true;
        return e;
    }
    
    // ========================================================
    // �G���e�B�e�B�̍쐬�i�r���_�[�Łj- �������߁I
    // ========================================================
    // �y�g�����z
    // Entity player = world.Create()
    //     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    //     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    //     .Build();
    EntityBuilder Create() {
        return EntityBuilder(this, CreateEntity());
    }

    // �G���e�B�e�B���������Ă��邩�m�F
    bool IsAlive(Entity e) const {
        auto it = alive_.find(e.id);
        return it != alive_.end() && it->second;
    }

    // �G���e�B�e�B�̔j��
    void DestroyEntity(Entity e) {
        if (!IsAlive(e)) return;
        alive_[e.id] = false;
        for (auto& er : erasers_) er(e);
        for (size_t i = 0; i < behaviours_.size(); ++i)
            if (behaviours_[i].e.id == e.id) { behaviours_.erase(behaviours_.begin() + i); --i; }
    }

    // ========================================================
    // �R���|�[�l���g�̒ǉ�
    // ========================================================
    // �y�g�����z
    // Entity e = world.CreateEntity();
    // world.Add<Transform>(e, Transform{...});
    // world.Add<MeshRenderer>(e, MeshRenderer{...});
    template<class T, class...Args>
    T& Add(Entity e, Args&&...args) {
        assert(IsAlive(e) && "Add on dead entity");
        auto& s = getStore<T>();
        T& obj = s.map[e.id] = T{ std::forward<Args>(args)... };
        registerBehaviour<T>(e, &obj);
        return obj;
    }

    // ========================================================
    // �R���|�[�l���g�̎擾�inullptr�̉\������j
    // ========================================================
    // �y�g�����z
    // auto* transform = world.TryGet<Transform>(entity);
    // if (transform) {
    //     transform->position.x += 1.0f;
    // }
    template<class T>
    T* TryGet(Entity e) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return nullptr;
        auto* s = static_cast<Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return nullptr;
        return &it->second;
    }

    // ========================================================
    // �R���|�[�l���g�̍폜
    // ========================================================
    template<class T>
    bool Remove(Entity e) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return false;
        auto* s = static_cast<Store<T>*>(itS->second);
        unregisterBehaviour<T>(e);
        return s->map.erase(e.id) > 0;
    }

    // ========================================================
    // �S�R���|�[�l���g�ɑ΂��ď��������s
    // ========================================================
    // �y�g�����z
    // world.ForEach<Transform>([](Entity e, Transform& t) {
    //     t->position.y += 0.01f; // �S�I�u�W�F�N�g��������Ə��
    // });
    template<class T, class F>
    void ForEach(F&& fn) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return;
        auto* s = static_cast<Store<T>*>(itS->second);
        for (auto it = s->map.begin(); it != s->map.end(); ++it) {
            Entity e{ it->first };
            if (!IsAlive(e)) continue;
            fn(e, it->second);
        }
    }

    // ========================================================
    // �SBehaviour�̍X�V�i���t���[���Ăԁj
    // ========================================================
    void Tick(float dt) {
        // �C�e���[�V�������̍폜�ɑΉ����邽�߁A�C���f�b�N�X�x�[�X�̃��[�v���g�p
        for (size_t i = 0; i < behaviours_.size(); ++i) {
            auto& it = behaviours_[i];
            if (!IsAlive(it.e)) continue;
            if (!it.started) { it.b->OnStart(*this, it.e); it.started = true; }
            it.b->OnUpdate(*this, it.e, dt);
        }
    }

private:
    // ========================================================
    // �ȉ��͓��������i���w�҂͓ǂݔ�΂���OK�j
    // ========================================================
    
    // �R���|�[�l���g�i�[�p�X�g�A���C���^�[�t�F�[�X
    struct IStore {
        virtual ~IStore() = default;
        virtual void Erase(Entity) = 0;
    };

    // �^���Ƃ̃R���|�[�l���g�X�g�A
    template<class T>
    struct Store : IStore {
        std::unordered_map<uint32_t, T> map;
        void Erase(Entity e) override { map.erase(e.id); }
    };

    // �X�g�A�̎擾�i���݂��Ȃ���΍쐬�j
    template<class T>
    Store<T>& getStore() {
        auto key = std::type_index(typeid(T));
        auto it = stores_.find(key);
        if (it == stores_.end()) {
            auto* s = new Store<T>();
            stores_[key] = s;
            erasers_.push_back([s](Entity e) { s->Erase(e); });
            return *s;
        }
        return *static_cast<Store<T>*>(it->second);
    }

    // Behaviour�̓o�^�iC++14�݊� - if constexpr �̑���j
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity e, TDerived* obj) {
        behaviours_.push_back({ e, obj, false });
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity, TDerived*) {}

    // Behaviour�̓o�^�����iC++14�݊��j
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity e) {
        for (size_t i = 0; i < behaviours_.size(); ++i) {
            if (behaviours_[i].e.id == e.id) { behaviours_.erase(behaviours_.begin() + i); break; }
        }
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity) {}

    // Behaviour�Ǘ��p�G���g��
    struct BEntry {
        Entity e;
        Behaviour* b;
        bool started = false;
    };

    // �����o�ϐ�
    uint32_t nextId_ = 0;
    std::unordered_map<uint32_t, bool> alive_;
    std::unordered_map<std::type_index, IStore*> stores_;
    std::vector<std::function<void(Entity)>> erasers_;
    std::vector<BEntry> behaviours_;
    
    // EntityBuilder��private�����o�ɃA�N�Z�X�ł���悤�ɂ���
    friend class EntityBuilder;
};

// ========================================================
// EntityBuilder �̎���
// ========================================================
template<typename T, typename... Args>
EntityBuilder& EntityBuilder::With(Args&&... args) {
    world_->Add<T>(entity_, std::forward<Args>(args)...);
    return *this;
}
