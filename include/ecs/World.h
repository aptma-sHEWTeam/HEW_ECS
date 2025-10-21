#pragma once
#include "ecs/Entity.h"
#include "components/Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <cstdio>

#ifdef _DEBUG
#include <cassert>
#endif

/**
 * @file World.h
 * @brief ECS���[���h�Ǘ��V�X�e���ƃG���e�B�e�B�r���_�[�̒�`
 * @author �R���z
 * @date 2025
 * @version 5.0
 * 
 * @details
 * ECS�A�[�L�e�N�`���̒��j�ƂȂ�World�N���X�ƁA
 * �G���e�B�e�B��֗��ɍ쐬���邽�߂�EntityBuilder�N���X���`���܂��B
 */

class World; ///< �O���錾

/**
 * @class EntityBuilder
 * @brief �G���e�B�e�B�쐬�p�̃r���_�[�p�^�[���N���X
 * 
 * @details
 * ���\�b�h�`�F�[�����g�p���āA�����̃R���|�[�l���g�����G���e�B�e�B��
 * �����I�ɍ쐬�ł��܂��BWorld�N���X�ƘA�g���ē��삵�܂��B
 * 
 * @par �g�p��
 * @code
 * Entity player = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 *     .With<Rotator>(45.0f)
 *     .Build();
 * @endcode
 * 
 * @note Build()�͏ȗ��\�ł�(�ÖٓI��Entity�֕ϊ�����܂�)
 * @see World
 * 
 * @author �R���z
 */
class EntityBuilder {
public:
    /**
     * @brief �R���X�g���N�^
     * @param[in] world World�C���X�^���X�ւ̃|�C���^
     * @param[in] entity �쐬���ꂽ�G���e�B�e�B
     */
    EntityBuilder(World* world, Entity entity) : world_(world), entity_(entity) {}
    
    /**
     * @brief ���\�b�h�`�F�[���ŃR���|�[�l���g��ǉ�
     * 
     * @tparam T �ǉ�����R���|�[�l���g�̌^
     * @tparam Args �R���X�g���N�^�����̌^(�ϒ�)
     * @param[in] args �R���|�[�l���g�̃R���X�g���N�^�ɓ]���������
     * @return EntityBuilder& ���\�b�h�`�F�[���p�̎��g�ւ̎Q��
     * 
     * @details
     * �w�肵���R���|�[�l���g���쐬���A�G���e�B�e�B�ɒǉ����܂��B
     * ���\�b�h�`�F�[���ŕ����̃R���|�[�l���g��A�����Ēǉ��ł��܂��B
     * 
     * @par �g�p��
     * @code
     * world.Create()
     *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
     *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
     *     .Build();
     * @endcode
     */
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args);
    
    /**
     * @brief �G���e�B�e�B���m�肵�ĕԂ�
     * @return Entity �쐬���ꂽ�G���e�B�e�B
     * 
     * @details
     * �r���_�[�p�^�[�����������A�쐬���ꂽ�G���e�B�e�B��Ԃ��܂��B
     * �ȗ��\�ŁA�ÖٓI��Entity�ɕϊ�����܂��B
     */
    Entity Build() { return entity_; }
    
    /**
     * @brief Entity�ւ̈ÖٓI�^�ϊ����Z�q
     * @return Entity �쐬���ꂽ�G���e�B�e�B
     * 
     * @details
     * Build()���Ă΂Ȃ��Ă��A�����I��Entity�ɕϊ�����܂��B
     */
    operator Entity() const { return entity_; }

private:
    World* world_;    ///< World�C���X�^���X�ւ̃|�C���^
    Entity entity_;   ///< �쐬���ꂽ�G���e�B�e�B
};

/**
 * @class World
 * @brief ECS���[���h�Ǘ��N���X
 * 
 * @details
 * ECS�A�[�L�e�N�`���ɂ����邷�ׂẴG���e�B�e�B�ƃR���|�[�l���g���Ǘ����܂��B
 * 
 * ### ��ȋ@�\:
 * - �G���e�B�e�B�̍쐬/�j��
 * - �R���|�[�l���g�̒ǉ�/�폜/�擾
 * - Behaviour�R���|�[�l���g�̍X�V
 * 
 * @par ��{�I�Ȏg�p���@
 * @code
 * World world;
 * 
 * // �G���e�B�e�B���쐬���ăR���|�[�l���g��ǉ�
 * Entity player = world.CreateEntity();
 * world.Add<Transform>(player, Transform{...});
 * world.Add<MeshRenderer>(player, MeshRenderer{...});
 * 
 * // �R���|�[�l���g���擾���đ���
 * auto* transform = world.TryGet<Transform>(player);
 * if (transform) {
 *     transform->position.x += 1.0f;
 * }
 * 
 * // ���t���[���X�V
 * world.Tick(deltaTime);
 * @endcode
 * 
 * @par �r���_�[�p�^�[��(����)
 * @code
 * Entity player = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 *     .With<Rotator>(45.0f)
 *     .Build();
 * @endcode
 * 
 * @see Entity
 * @see IComponent
 * @see Behaviour
 * 
 * @author �R���z
 */
class World {
public:
    /**
     * @brief �f�X�g���N�^
     * @details �m�ۂ����R���|�[�l���g�X�g�A�̃�������������܂�
     */
    ~World() {
        for (auto& pair : stores_) {
            delete pair.second;
        }
    }
    
    /**
     * @brief �V�����G���e�B�e�B���쐬
     * @return Entity ��ӂ�ID�����V�K�쐬���ꂽ�G���e�B�e�B
     * 
     * @details
     * ��ӂ�ID�����V�����G���e�B�e�B���쐬���܂��B������Ԃł̓R���|�[�l���g�͕t���Ă��܂���B
     * �폜���ꂽ�G���e�B�e�B��ID���ė��p���ă��������������コ���܂��B
     * 
     * @note ���֗��ȃG���e�B�e�B�쐬�ɂ�Create()(�r���_�[)�̎g�p�𐄏����Ă�������
     */
    Entity CreateEntity() {
        uint32_t id;
        if (!freeIds_.empty()) {
            // �ė��p�\��ID������΂�����g��
            id = freeIds_.back();
            freeIds_.pop_back();
        } else {
            // �Ȃ���ΐV�KID
            id = ++nextId_;
        }
        alive_[id] = true;
        return Entity{id};
    }
    
    /**
     * @brief �r���_�[�p�^�[���ŐV�����G���e�B�e�B���쐬
     * @return EntityBuilder ���\�b�h�`�F�[���p�̃r���_�[�I�u�W�F�N�g
     * 
     * @details
     * �����I�ȃR���|�[�l���g�ǉ����\�ɂ���EntityBuilder��Ԃ��܂��B
     * ���\�b�h�`�F�[���ŕ����̃R���|�[�l���g��A�����Ēǉ��ł��܂��B
     * 
     * @par �g�p��
     * @code
     * Entity enemy = world.Create()
     *     .With<Transform>(DirectX::XMFLOAT3{5, 0, 0})
     *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
     *     .With<Enemy>()
     *     .Build();
     * @endcode
     * 
     * @see EntityBuilder
     */
    EntityBuilder Create() {
        return EntityBuilder(this, CreateEntity());
    }

    /**
     * @brief �G���e�B�e�B���������Ă��邩�m�F
     * 
     * @param[in] e �m�F����G���e�B�e�B
     * @return true �������Ă���, false �j���ς�
     * 
     * @details
     * �G���e�B�e�B���܂��L�����ǂ������m�F���܂��B
     * �j�����ꂽ�G���e�B�e�B�ւ̃A�N�Z�X��h�����߂Ɏg�p���܂��B
     */
    bool IsAlive(Entity e) const {
        auto it = alive_.find(e.id);
        return it != alive_.end() && it->second;
    }

    /**
     * @brief �G���e�B�e�B�Ƃ��̂��ׂẴR���|�[�l���g��j��
     * 
     * @param[in] e �j������G���e�B�e�B
     * 
     * @details
     * �w�肳�ꂽ�G���e�B�e�B�Ƃ���Ɋ֘A���邷�ׂẴR���|�[�l���g���폜���܂��B
     * Behaviour�R���|�[�l���g�������I�ɓo�^��������܂��B
     * ID�͍ė��p�p�Ƀv�[������܂��B
     * 
     * @warning �j�����ꂽ�G���e�B�e�B���g�p����ƃN���b�V������\��������܂�
     */
    void DestroyEntity(Entity e) {
        if (!IsAlive(e)) return;
        alive_[e.id] = false;
        
        // �S�R���|�[�l���g���폜
        for (auto& er : erasers_) er(e);
        
        // Behaviour���X�g����폜�i���ׂĂ̊Y���G���g�����폜�j
        for (size_t i = 0; i < behaviours_.size(); ) {
            if (behaviours_[i].e.id == e.id) {
                behaviours_.erase(behaviours_.begin() + i);
                // �C���f�b�N�X��i�߂Ȃ��i�폜�ɂ�莟�̗v�f��i�Ԗڂɗ���j
            } else {
                ++i;
            }
        }
        
        // ID�ė��p�p�ɕۑ�
        freeIds_.push_back(e.id);
    }

    /**
     * @brief �G���e�B�e�B�ɃR���|�[�l���g��ǉ�
     * 
     * @tparam T �ǉ�����R���|�[�l���g�̌^
     * @tparam Args �R���X�g���N�^�����̌^(�ϒ�)
     * @param[in] e �ΏۃG���e�B�e�B
     * @param[in] args �R���|�[�l���g�̃R���X�g���N�^����
     * @return T& �ǉ����ꂽ�R���|�[�l���g�ւ̎Q��
     * @throws std::runtime_error �G���e�B�e�B������ł���ꍇ�A�܂��̓R���|�[�l���g�����ɑ��݂���ꍇ�i�f�o�b�O�r���h�j
     * 
     * @details
     * �w�肵���R���|�[�l���g���G���e�B�e�B�ɒǉ����܂��B
     * �R���|�[�l���g��Behaviour���p�����Ă���ꍇ�ATick()�Ŏ����I�ɍX�V����܂��B
     * 
     * @par �g�p��
     * @code
     * Entity player = world.CreateEntity();
     * world.Add<Transform>(player, Transform{
     *     DirectX::XMFLOAT3{0, 0, 0},  // �ʒu
     *     DirectX::XMFLOAT3{0, 0, 0},  // ��]
     *     DirectX::XMFLOAT3{1, 1, 1}   // �X�P�[��
     * });
     * @endcode
     * 
     * @note �G���e�B�e�B�͐������Ă���K�v������܂�
     * @warning �f�o�b�O�r���h�ł͊����R���|�[�l���g�ւ̒ǉ����ɗ�O���X���[���܂�
     */
    template<class T, class...Args>
    T& Add(Entity e, Args&&...args) {
        if (!IsAlive(e)) {
            throw std::runtime_error("Attempting to add component to dead entity");
        }
        
        auto& s = getStore<T>();
        
#ifdef _DEBUG
        // �f�o�b�O���[�h�ł͏d���`�F�b�N
        if (s.map.find(e.id) != s.map.end()) {
            throw std::runtime_error("Component already exists on entity");
        }
#endif
        
        T& obj = s.map[e.id] = T{ std::forward<Args>(args)... };
        registerBehaviour<T>(e, &obj);
        return obj;
    }

    /**
     * @brief �G���e�B�e�B���w�肵���R���|�[�l���g�������Ă��邩�m�F
     * 
     * @tparam T �m�F����R���|�[�l���g�̌^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return true �����Ă���, false �����Ă��Ȃ�
     * 
     * @details
     * �R���|�[�l���g�̑��݊m�F�𖾎��I�ɍs���܂��B
     * TryGet()��nullptr�`�F�b�N���Ӑ}�����m�ɂȂ�܂��B
     * 
     * @par �g�p��
     * @code
     * if (world.Has<Transform>(entity)) {
     *     // Transform�������Ă���ꍇ�̏���
     *     auto* transform = world.TryGet<Transform>(entity);
     *     transform->position.x += 1.0f;
     * }
     * 
     * // ���Ȍ��ȏ�����
     * if (world.Has<Health>(enemy) && world.Has<Transform>(enemy)) {
     *     // ���������Ă���ꍇ�̏���
     * }
     * @endcode
     */
    template<class T>
    bool Has(Entity e) const {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return false;
        auto* s = static_cast<const Store<T>*>(itS->second);
        return s->map.find(e.id) != s->map.end();
    }

    /**
     * @brief �G���e�B�e�B����R���|�[�l���g���擾
     * 
     * @tparam T �擾����R���|�[�l���g�̌^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return T* �R���|�[�l���g�ւ̃|�C���^�A������Ȃ��ꍇ��nullptr
     * 
     * @details
     * �w�肵���G���e�B�e�B����w�肵���R���|�[�l���g���擾���܂��B
     * �R���|�[�l���g�����݂��Ȃ��ꍇ��nullptr��Ԃ��܂��B
     * 
     * @par �g�p��
     * @code
     * auto* transform = world.TryGet<Transform>(player);
     * if (transform) {
     *     transform->position.x += 1.0f;
     * }
     * @endcode
     * 
     * @warning �g�p�O�ɕK��nullptr�`�F�b�N���s���Ă�������
     */
    template<class T>
    T* TryGet(Entity e) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return nullptr;
        auto* s = static_cast<Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return nullptr;
        return &it->second;
    }

    /**
     * @brief �G���e�B�e�B����R���|�[�l���g���擾�iconst�Łj
     * 
     * @tparam T �擾����R���|�[�l���g�̌^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return const T* �R���|�[�l���g�ւ̃|�C���^�A������Ȃ��ꍇ��nullptr
     * 
     * @details
     * const�ł�TryGet�B�ǂݎ���p�A�N�Z�X�p�B
     */
    template<class T>
    const T* TryGet(Entity e) const {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return nullptr;
        auto* s = static_cast<const Store<T>*>(itS->second);
        auto it = s->map.find(e.id);
        if (it == s->map.end()) return nullptr;
        return &it->second;
    }

    /**
     * @brief �G���e�B�e�B����R���|�[�l���g���擾�i��O�Łj
     * 
     * @tparam T �擾����R���|�[�l���g�̌^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return T& �R���|�[�l���g�ւ̎Q��
     * @throws std::runtime_error �R���|�[�l���g�����݂��Ȃ��ꍇ
     * 
     * @details
     * �K�����݂���͂��̃R���|�[�l���g���擾����ۂɎg�p���܂��B
     * nullptr�`�F�b�N���s�v�ɂȂ�R�[�h���Ȍ��ɂȂ�܂��B
     * 
     * @par �g�p��
     * @code
     * // �K��Transform�����ƕ������Ă���ꍇ
     * Transform& transform = world.Get<Transform>(player);
     * transform.position.x += 1.0f;
     * 
     * // try-catch�ŗ�O������
     * try {
     *     MeshRenderer& renderer = world.Get<MeshRenderer>(entity);
     *     renderer.color = DirectX::XMFLOAT3{1, 0, 0};
     * } catch (const std::runtime_error& e) {
     *     // �R���|�[�l���g�����݂��Ȃ��ꍇ�̏���
     *     printf("Error: %s\n", e.what());
     * }
     * @endcode
     * 
     * @warning �R���|�[�l���g�����݂��Ȃ��ꍇ�͗�O���X���[����܂�
     */
    template<class T>
    T& Get(Entity e) {
        T* ptr = TryGet<T>(e);
        if (!ptr) {
            throw std::runtime_error("Component not found on entity");
        }
        return *ptr;
    }

    /**
     * @brief �G���e�B�e�B����R���|�[�l���g���擾�iconst��O�Łj
     * 
     * @tparam T �擾����R���|�[�l���g�̌^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return const T& �R���|�[�l���g�ւ�const�Q��
     * @throws std::runtime_error �R���|�[�l���g�����݂��Ȃ��ꍇ
     */
    template<class T>
    const T& Get(Entity e) const {
        const T* ptr = TryGet<T>(e);
        if (!ptr) {
            throw std::runtime_error("Component not found on entity");
        }
        return *ptr;
    }

    /**
     * @brief �w�肳�ꂽ�R���|�[�l���g�������ׂẴG���e�B�e�B�ɑ΂��Ċ֐������s
     * 
     * @tparam T �N�G���Ώۂ̃R���|�[�l���g�^
     * @tparam F �֐��̌^
     * @param[in] fn ���s����֐�(Entity��T&���󂯎��)
     * 
     * @details
     * �w�肵���R���|�[�l���g�������ׂẴG���e�B�e�B�ɑ΂��āA
     * �񋟂��ꂽ�֐������s���܂��B
     * �C�e���[�V�������̃G���e�B�e�B�폜�ɑΉ����Ă��܂��B
     * 
     * @par �g�p��
     * @code
     * // ���ׂĂ�Transform�����G���e�B�e�B����Ɉړ�
     * world.ForEach<Transform>([](Entity e, Transform& t) {
     *     t.position.y += 0.1f;
     * });
     * 
     * // ���ׂĂ̓G��HP���m�F�i�폜�����S)
     * world.ForEach<Enemy>([&](Entity e, Enemy& enemy) {
     *     if (enemy.health <= 0) {
     *         world.DestroyEntity(e);  // ? ���S�ɍ폜�\
     *     }
     * });
     * @endcode
     */
    template<class T, class F>
    void ForEach(F&& fn) {
        auto itS = stores_.find(std::type_index(typeid(T)));
        if (itS == stores_.end()) return;
        auto* s = static_cast<Store<T>*>(itS->second);
        
        // ID�̃��X�g���ɍ쐬�i�C�e���[�V�������̍폜�ɑΉ�)
        std::vector<uint32_t> ids;
        ids.reserve(s->map.size());
        for (auto& pair : s->map) {
            ids.push_back(pair.first);
        }
        
        // ���S�ɃC�e���[�g
        for (uint32_t id : ids) {
            Entity e{id};
            if (!IsAlive(e)) continue;
            auto it = s->map.find(id);
            if (it == s->map.end()) continue;
            fn(e, it->second);
        }
    }

    /**
     * @brief 2�̃R���|�[�l���g�����G���e�B�e�B�ɑ΂��ď���
     * 
     * @tparam T1 1�ڂ̃R���|�[�l���g�^
     * @tparam T2 2�ڂ̃R���|�[�l���g�^
     * @tparam F �֐��̌^
     * @param[in] fn ���s����֐�(Entity, T1&, T2&���󂯎��)
     * 
     * @details
     * �w�肵��2�̃R���|�[�l���g�𗼕����G���e�B�e�B�ɑ΂��āA
     * �񋟂��ꂽ�֐������s���܂��B
     * �C�e���[�V�������̃G���e�B�e�B�폜�ɑΉ����Ă��܂��B
     * 
     * @par �g�p��
     * @code
     * // Transform��MeshRenderer�𗼕����G���e�B�e�B������
     * world.ForEach<Transform, MeshRenderer>(
     *     [](Entity e, Transform& t, MeshRenderer& r) {
     *         // �����̃R���|�[�l���g�ɃA�N�Z�X�\
     *         r.color.x = t.position.x / 10.0f;
     *     }
     * );
     * 
     * // �������Z�̗�
     * world.ForEach<Transform, Velocity>(
     *     [](Entity e, Transform& t, Velocity& v) {
     *         t.position.x += v.velocity.x * dt;
     *         t.position.y += v.velocity.y * dt;
     *         t.position.z += v.velocity.z * dt;
     *     }
     * );
     * 
     * // �G�̗̑̓`�F�b�N
     * world.ForEach<Enemy, Health>([&](Entity e, Enemy& enemy, Health& hp) {
     *     if (hp.IsDead()) {
     *         world.DestroyEntity(e);  // ? ���S�ɍ폜�\
     *     }
     * });
     * @endcode
     */
    template<class T1, class T2, class F>
    void ForEach(F&& fn) {
        auto itS1 = stores_.find(std::type_index(typeid(T1)));
        if (itS1 == stores_.end()) return;
        auto* s1 = static_cast<Store<T1>*>(itS1->second);
        
        // ID�̃��X�g���ɍ쐬�i�C�e���[�V�������̍폜�ɑΉ��j
        std::vector<uint32_t> ids;
        ids.reserve(s1->map.size());
        for (auto& pair : s1->map) {
            ids.push_back(pair.first);
        }
        
        // ���S�ɃC�e���[�g
        for (uint32_t id : ids) {
            Entity e{id};
            if (!IsAlive(e)) continue;
            
            auto it1 = s1->map.find(id);
            if (it1 == s1->map.end()) continue;
            
            T2* comp2 = TryGet<T2>(e);
            if (!comp2) continue;
            
            fn(e, it1->second, *comp2);
        }
    }

    /**
     * @brief ���ׂĂ�Behaviour�R���|�[�l���g���X�V
     * 
     * @param[in] dt �f���^�^�C��(�O�t���[������̌o�ߎ���)
     * 
     * @details
     * ���ׂĂ�Behaviour�R���|�[�l���g��OnUpdate()���Ăяo���܂��B���t���[���Ăяo���K�v������܂��B
     * ����Ăяo�����ɂ�OnStart()�����s����܂��B
     * OnUpdate���ł̃G���e�B�e�B�폜�ɑΉ����Ă��܂��B
     * 
     * @par �g�p��
     * @code
     * // �Q�[�����[�v
     * while (running) {
     *     float deltaTime = CalculateDeltaTime();
     *     
     *     // ���ׂĂ�Behaviour���X�V
     *     world.Tick(deltaTime);
     *     
     *     // �`�揈��...
     * }
     * @endcode
     */
    void Tick(float dt) {
        // �C�e���[�V�������̍폜�ɑΉ����邽�߃C���f�b�N�X�x�[�X�̃��[�v���g�p
        for (size_t i = 0; i < behaviours_.size(); ) {
            auto& entry = behaviours_[i];
            
            // ���񂾃G���e�B�e�B��Behaviour���폜
            if (!IsAlive(entry.e)) {
                behaviours_.erase(behaviours_.begin() + i);
                continue;  // �C���f�b�N�X��i�߂Ȃ�
            }
            
            // OnStart��OnUpdate�����s
            if (!entry.started) {
                entry.b->OnStart(*this, entry.e);
                entry.started = true;
            }
            entry.b->OnUpdate(*this, entry.e, dt);
            
            // �ēx�����m�F�iOnUpdate���ō폜���ꂽ��������Ȃ��j
            if (IsAlive(entry.e)) {
                ++i;  // �������Ă���΃C���f�b�N�X��i�߂�
            }
            // �폜����Ă����玩���I�Ɏ��̗v�f��i�Ԗڂɗ���̂ŃC���f�b�N�X��i�߂Ȃ�
        }
    }

private:
    /**
     * @interface IStore
     * @brief �R���|�[�l���g�i�[�p�̃C���^�[�t�F�[�X
     */
    struct IStore {
        virtual ~IStore() = default;
        virtual void Erase(Entity) = 0;
    };

    /**
     * @struct Store
     * @brief �^�ŗL�̃R���|�[�l���g�i�[�\��
     * @tparam T �R���|�[�l���g�̌^
     */
    template<class T>
    struct Store : IStore {
        std::unordered_map<uint32_t, T> map;  ///< EntityID -> �R���|�[�l���g�̃}�b�v
        void Erase(Entity e) override { map.erase(e.id); }
    };

    /// �R���|�[�l���g�^T�̃X�g�A���擾�܂��͍쐬
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

    /// �����X�V�̂��߂�Behaviour�R���|�[�l���g��o�^(C++14�݊�)
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity e, TDerived* obj) {
        behaviours_.push_back({ e, obj, false });
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        registerBehaviour(Entity, TDerived*) {}

    /// Behaviour�R���|�[�l���g�̓o�^������(C++14�݊�)
    template<class TDerived>
    typename std::enable_if<std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity e) {
        // �����G���e�B�e�B�̕���Behaviour�ɑΉ�
        for (size_t i = 0; i < behaviours_.size(); ) {
            if (behaviours_[i].e.id == e.id) {
                behaviours_.erase(behaviours_.begin() + i);
                // �C���f�b�N�X��i�߂Ȃ��i�폜�ɂ�莟�̗v�f��i�Ԗڂɗ���j
            } else {
                ++i;
            }
        }
    }
    template<class TDerived>
    typename std::enable_if<!std::is_base_of<Behaviour, TDerived>::value>::type
        unregisterBehaviour(Entity) {}

    /**
     * @struct BEntry
     * @brief Behaviour�Ǘ��p�G���g��
     */
    struct BEntry {
        Entity e;           ///< �G���e�B�e�B
        Behaviour* b;       ///< Behaviour�ւ̃|�C���^
        bool started = false; ///< OnStart���Ă΂ꂽ���ǂ���
    };

    uint32_t nextId_ = 0;  ///< ���̃G���e�B�e�BID
    std::vector<uint32_t> freeIds_;  ///< �ė��p�\��ID
    std::unordered_map<uint32_t, bool> alive_;  ///< �G���e�B�e�B�̐������
    std::unordered_map<std::type_index, IStore*> stores_;  ///< �R���|�[�l���g�X�g�A
    std::vector<std::function<void(Entity)>> erasers_;  ///< �폜�p�֐�
    std::vector<BEntry> behaviours_;  ///< Behaviour���X�g
    
    friend class EntityBuilder;  ///< EntityBuilder��private�����o�ɃA�N�Z�X�ł���悤�ɂ���
};

/**
 * @brief EntityBuilder::With()�̎���
 * @tparam T �ǉ�����R���|�[�l���g�̌^
 * @tparam Args �R���X�g���N�^�����̌^
 * @param[in] args �R���X�g���N�^����
 * @return EntityBuilder& ���\�b�h�`�F�[���p�̎��g�ւ̎Q��
 */
template<typename T, typename... Args>
EntityBuilder& EntityBuilder::With(Args&&... args) {
    world_->Add<T>(entity_, std::forward<Args>(args)...);
    return *this;
}
