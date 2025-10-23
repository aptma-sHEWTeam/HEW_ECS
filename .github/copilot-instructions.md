# GitHub Copilot �J�X�^���w�� - HEW_ECS �v���W�F�N�g

���̃t�@�C���́AGitHub Copilot�����̃v���W�F�N�g�iHEW_ECS�j�̃R�[�h�𐶐��E�C������ۂɏ]���ׂ����[���ƋK����`���܂��B

---

## ?? �v���W�F�N�g�T�v

**�v���W�F�N�g��**: HEW_ECS (ECS_BACE)  
**�ړI**: Entity Component System (ECS) �����p�����`�[���Q�[���J��  
**����**: C++14�i����j  
**�v���b�g�t�H�[��**: Windows (DirectX 11)  
**�A�[�L�e�N�`��**: Entity Component System (ECS)  
**�J���X�^�C��**: �`�[���J���iGit/GitHub�g�p�j

---

## ?? �d�v�Ȑ��񎖍�

### 1. C++�W���ƃo�[�W����

#### ? �g�p�\�iC++14�j
```cpp
// auto�^���_
auto entity = world.CreateEntity();

// �����_��
world.ForEach<Transform>([](Entity e, Transform& t) {
    t.position.x += 1.0f;
});

// �X�}�[�g�|�C���^
std::unique_ptr<Component> component;
std::shared_ptr<Resource> resource;

// �͈�for
for (const auto& entity : entities) { }

// ���������X�g
DirectX::XMFLOAT3 position{0.0f, 0.0f, 0.0f};
```

#### ? �g�p�֎~�iC++17�ȍ~�j
```cpp
// std::optional�iC++17�j
std::optional<Transform> GetTransform(Entity e);  // NG

// if constexpr�iC++17�j
if constexpr (std::is_same_v<T, Transform>) { }  // NG

// std::filesystem�iC++17�j
std::filesystem::path filePath;  // NG

// �\���������iC++17�j
auto [x, y, z] = GetPosition();  // NG

// �C�����C���ϐ��iC++17�j
inline constexpr int MAX_ENTITIES = 1000;  // NG
```

**�Ώ��@**: C++14�݊��̑�֎�i���g�p
```cpp
// �|�C���^�ő�p
Transform* GetTransform(Entity e) {
    return world.TryGet<Transform>(e);
}

// �e���v���[�g���ꉻ�ő�p
template<typename T>
void Process(T& component);

// �]���̃t�@�C���V�X�e��API
#include <windows.h>
// �܂��� boost::filesystem�i���p�\�ȏꍇ�j
```

---

## ??? ECS�A�[�L�e�N�`���̌���

### Entity�i�G���e�B�e�B�j

**����**: ��ӂ�ID�i���ʎq�j�݂̂�����

```cpp
struct Entity {
    uint32_t id;   // �G���e�B�e�BID
    uint32_t gen;  // ����ԍ��i�폜�Ǘ��p�j
};
```

**�֎~����**:
```cpp
// ? NG: Entity�Ƀ��\�b�h��f�[�^��ǉ����Ȃ�
struct Entity {
    uint32_t id;
    void Move(float x, float y);  // NG
    float health;                 // NG
};
```

### Component�i�R���|�[�l���g�j

**2��ނ̃R���|�[�l���g**:

#### 1. �f�[�^�R���|�[�l���g�iIComponent�p���j
```cpp
// ? ������: �f�[�^�̂ݕێ�
struct Health : IComponent {
    float current = 100.0f;
    float max = 100.0f;
    
    // �w���p�[�֐���OK
    void TakeDamage(float dmg) {
        current -= dmg;
        if (current < 0.0f) current = 0.0f;
    }
    
    bool IsDead() const {
        return current <= 0.0f;
    }
};

// ? �ԈႢ: IComponent���p�����Ă��Ȃ�
struct Health {  // NG
    float current = 100.0f;
};
```

#### 2. Behaviour�R���|�[�l���g�iBehaviour�p���j
```cpp
// ? ������: ���t���[���X�V����郍�W�b�N
struct Rotator : Behaviour {
    float speedDegY = 45.0f;
    
    void OnStart(World& w, Entity self) override {
        // ����N������1�x�������s�i�I�v�V�����j
    }
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            t->rotation.y += speedDegY * dt;
        }
    }
};

// ? �ԈႢ: Behaviour���p�����Ă��Ȃ�
struct Rotator {  // NG
    void Update() { }
};
```

### System�i�V�X�e���j

**2�̎����p�^�[��**:

#### �p�^�[��1: Behaviour�p�^�[���i�����j
```cpp
struct PlayerController : Behaviour {
    InputSystem* input_ = nullptr;
    float speed = 5.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t || !input_) return;
        
        if (input_->GetKey('W')) t->position.z += speed * dt;
        if (input_->GetKey('S')) t->position.z -= speed * dt;
    }
};
```

#### �p�^�[��2: ForEach�p�^�[��
```cpp
void UpdateMovementSystem(World& world, float dt) {
    world.ForEach<Transform, Velocity>([dt](Entity e, Transform& t, Velocity& v) {
        t.position.x += v.velocity.x * dt;
        t.position.y += v.velocity.y * dt;
        t.position.z += v.velocity.z * dt;
    });
}
```

---

## ?? �R�[�f�B���O�K��

### �����K��

| �v�f | �K�� | �� |
|------|------|-----|
| **�N���X��** | PascalCase | `Transform`, `MeshRenderer`, `World` |
| **�\���̖�** | PascalCase | `Entity`, `Health`, `Velocity` |
| **�֐���** | PascalCase | `CreateEntity()`, `TryGet()`, `OnUpdate()` |
| **�ϐ���** | camelCase | `deltaTime`, `entityId`, `speed` |
| **�����o�ϐ�** | camelCase + `_` �ڔ��� | `world_`, `nextId_`, `alive_` |
| **�萔** | UPPER_SNAKE_CASE | `MAX_ENTITIES`, `DEFAULT_SPEED` |
| **�v���C�x�[�g�����o** | �A���_�[�X�R�A�ڔ��� | `stores_`, `behaviours_` |

### �R�[�h�X�^�C����

```cpp
// ? ��������
class PlayerController : public Behaviour {
public:
    float speed = 5.0f;  // public�����o�icamelCase�j
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* transform = w.TryGet<Transform>(self);
        if (transform) {
            transform->position.x += speed * dt;
        }
    }
    
private:
    InputSystem* input_;  // private�����o�i�A���_�[�X�R�A�ڔ����j
    float acceleration_;
};

// ? �Ԉ������
class player_controller {  // NG: PascalCase���g�p
public:
    float Speed;           // NG: camelCase���g�p
    
    void on_update() { }   // NG: PascalCase���g�p
    
private:
    int m_health;          // NG: �A���_�[�X�R�A�ڔ������g�p
};
```

---

## ?? World�N���X�̎g�p���@

### �G���e�B�e�B�̍쐬

#### ���@1: �r���_�[�p�^�[���i�����j
```cpp
// ? ����: ���\�b�h�`�F�[���Œ����I
Entity player = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
    .With<Rotator>(45.0f)
    .With<PlayerTag>()
    .Build();  // Build()�͏ȗ��\

// ? Build()�ȗ���
Entity enemy = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{5, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0});
```

#### ���@2: �]���̕��@
```cpp
// ���e����邪�A�r���_�[�p�^�[���𐄏�
Entity enemy = world.CreateEntity();
world.Add<Transform>(enemy, Transform{});
world.Add<MeshRenderer>(enemy, MeshRenderer{});
```

#### ���@3: �x���X�|�[���i���񏈗��Ή��j
```cpp
// �X���b�h�Z�[�t�ȃX�|�[��
world.EnqueueSpawn(World::Cause::Spawner, [](Entity e) {
    // ������̏������i���C���X���b�h�Ŏ��s�j
});
```

### �R���|�[�l���g�̑���

#### ���S�Ȏ擾�iTryGet�����j
```cpp
// ? ����: TryGet��null�`�F�b�N
auto* transform = world.TryGet<Transform>(entity);
if (transform) {
    transform->position.x += 1.0f;
}

// ?? ���e: Get�͗�O�𓊂���\������
try {
    auto& transform = world.Get<Transform>(entity);
    transform.position.x += 1.0f;
} catch (const std::runtime_error& e) {
    // �G���[����
}

// ? �ԈႢ: null�`�F�b�N�Ȃ�
auto* transform = world.TryGet<Transform>(entity);
transform->position.x += 1.0f;  // NG: �N���b�V���̉\��
```

#### �R���|�[�l���g�̒ǉ��ƍ폜
```cpp
// ? �ǉ�
world.Add<Health>(entity, Health{100.0f, 100.0f});

// ? ���݊m�F
if (world.Has<Transform>(entity)) {
    // Transform�����݂���
}

// ? �폜
world.Remove<Health>(entity);
```

#### �G���e�B�e�B�̍폜
```cpp
// ? �ʏ�̍폜�i�t���[���I�����Ɏ��ۂɍ폜�j
world.DestroyEntity(entity);

// ? �����t���폜�i�f�o�b�O���O�ɋL�^�j
world.DestroyEntityWithCause(entity, World::Cause::Collision);
world.DestroyEntityWithCause(entity, World::Cause::LifetimeExpired);
```

### ForEach�̎g�p

```cpp
// �P��R���|�[�l���g
world.ForEach<Transform>([](Entity e, Transform& t) {
    t.position.y += 0.1f;
});

// �����R���|�[�l���g
world.ForEach<Transform, Velocity>([dt](Entity e, Transform& t, Velocity& v) {
    t.position.x += v.velocity.x * dt;
    t.position.y += v.velocity.y * dt;
    t.position.z += v.velocity.z * dt;
});

// �^�O�Ńt�B���^�����O
world.ForEach<PlayerTag, Transform>([](Entity e, PlayerTag& tag, Transform& t) {
    // �v���C���[�̂ݏ���
});
```

---

## ?? DirectXMath�̎g�p

### �f�[�^�ێ���XMFLOAT3/XMFLOAT4

```cpp
// ? ������: �R���|�[�l���g���ł�XMFLOAT3���g�p
struct Transform : IComponent {
    DirectX::XMFLOAT3 position{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
};
```

### �v�Z����XMVECTOR���g�p

```cpp
// ? ����: SIMD�œK���̂���XMVECTOR�Ōv�Z
void MoveTowards(Transform& t, const DirectX::XMFLOAT3& target, float speed) {
    using namespace DirectX;
    
    // XMFLOAT3 �� XMVECTOR �ɕϊ�
    XMVECTOR pos = XMLoadFloat3(&t.position);
    XMVECTOR tgt = XMLoadFloat3(&target);
    
    // �x�N�g���v�Z�iSIMD�œK���j
    XMVECTOR dir = XMVectorSubtract(tgt, pos);
    dir = XMVector3Normalize(dir);
    XMVECTOR move = XMVectorScale(dir, speed);
    XMVECTOR newPos = XMVectorAdd(pos, move);
    
    // XMVECTOR �� XMFLOAT3 �ɖ߂�
    XMStoreFloat3(&t.position, newPos);
}

// ? �񐄏�: ���ڌv�Z�͔�����i���e�͂����j
void MoveTowards(Transform& t, const DirectX::XMFLOAT3& target, float speed) {
    t.position.x += (target.x - t.position.x) * speed;
    t.position.y += (target.y - t.position.y) * speed;
    t.position.z += (target.z - t.position.z) * speed;
}
```

---

## ?? �h�L�������e�[�V�����K��

### Doxygen�X�^�C���̃R�����g

```cpp
/**
 * @file MyComponent.h
 * @brief �R���|�[�l���g�̊Ȍ��Ȑ���
 * @author [���Ȃ��̖��O]
 * @date 2025
 * @version 6.0
 *
 * @details
 * �ڍׂȐ����������ɋL�q���܂��B
 */

/**
 * @struct MyComponent
 * @brief �R���|�[�l���g�̊Ȍ��Ȑ���
 *
 * @details
 * ���ڂ��������B���̃R���|�[�l���g�̖�����g�������L�q���܂��B
 *
 * @par �g�p��
 * @code
 * Entity e = world.Create()
 *     .With<MyComponent>(param1, param2)
 *     .Build();
 * @endcode
 *
 * @author [���Ȃ��̖��O]
 */
struct MyComponent : IComponent {
    float value = 0.0f;  ///< �l�̐���
};

/**
 * @brief �֐��̊Ȍ��Ȑ���
 * 
 * @param[in] input ���̓p�����[�^
 * @param[out] output �o�̓p�����[�^
 * @param[in,out] inout ���o�̓p�����[�^
 * @return �߂�l�̐���
 * 
 * @details
 * ���ڂ�������̐����B
 * 
 * @note �⑫���
 * @warning �x������
 * @author [���Ȃ��̖��O]
 */
ReturnType FunctionName(Type input, Type& output, Type& inout);
```

### �C�����C���R�����g

```cpp
// ? �ǂ��R�����g: �u�Ȃ��v�����
// ����t���[���ł�ID�ė��p��h�����߁A����ԍ����C���N�������g
generations_[id]++;

// sin�֐��Ŋ��炩�ȏ㉺�^���������i�͈�: [-amplitude, +amplitude]�j
t->position.y = startY + sinf(time * speed) * amplitude;

// ? �����R�����g: �R�[�h�̌J��Ԃ�
// id��1���₷
id++;  // NG: �R�[�h��ǂ߂Ε�����

// transform���擾
auto* t = world.TryGet<Transform>(entity);  // NG
```

---

## ?? �֎~����

### �A�[�L�e�N�`���̔j��

```cpp
// ? NG: Entity�Ƀ��W�b�N��ǉ�
struct Entity {
    void Update();  // NG
    void Render();  // NG
    float health;   // NG
};

// ? NG: �O���[�o���ϐ��ŃG���e�B�e�B�Ǘ�
Entity g_player;           // NG: World�ŊǗ����ׂ�
World* g_globalWorld;      // NG

// ? NG: �R���|�[�l���g�����̃R���|�[�l���g�𒼐ڎQ��
struct MyComponent : IComponent {
    Transform* transform_;  // NG: World�o�R�Ŏ擾���ׂ�
    Entity target_;         // NG: �����������\��
};

// ? ������: World�o�R�Ŏ擾
struct MyBehaviour : Behaviour {
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* transform = w.TryGet<Transform>(self);
        if (transform) {
            // �g�p
        }
    }
};
```

### �񐄏��ȃR�[�f�B���O�p�^�[��

```cpp
// ? NG: Update���ł̓����I�G���e�B�e�B�쐬
void OnUpdate(World& w, Entity self, float dt) override {
    Entity newEnemy = w.CreateEntity();  // NG: �C�e���[�^�������̉\��
}

// ? ������: �L���[�C���O
void OnUpdate(World& w, Entity self, float dt) override {
    w.EnqueueSpawn(World::Cause::Spawner, [](Entity e) {
        // ������̏�����
    });
}

// ? NG: ���t���[���������m��
void OnUpdate(World& w, Entity self, float dt) override {
    std::vector<Entity> enemies;  // NG: ���t���[���m��
    // ...
}

// ? ���P: �����o�ϐ��Ƃ��ĕێ�
struct MySystem : Behaviour {
    std::vector<Entity> enemies_;  // �����o�ϐ��ōė��p
    
    void OnUpdate(World& w, Entity self, float dt) override {
        enemies_.clear();  // �N���A���čė��p
        // ...
    }
};
```

---

## ?? �f�o�b�O�ƃ��O

### DebugLog�̎g�p

```cpp
#include "app/DebugLog.h"

// �ʏ�̃��O
DEBUGLOG("Entity created: ID=" + std::to_string(entity.id));

// �x��
DEBUGLOG_WARNING("Transform not found on entity " + std::to_string(entity.id));

// �G���[
DEBUGLOG_ERROR("Failed to load resource: " + resourceName);

// �����t�����O
#ifdef _DEBUG
    DEBUGLOG("Debug mode: Delta time = " + std::to_string(deltaTime));
#endif
```

---

## ?? �}�N���̊��p

### DEFINE_DATA_COMPONENT

```cpp
// ? �V���v���ȃf�[�^�R���|�[�l���g�p
DEFINE_DATA_COMPONENT(Score,
    int points = 0;
    
    void AddPoints(int p) {
        points += p;
    }
    
    void Reset() {
        points = 0;
    }
);
```

### DEFINE_BEHAVIOUR

```cpp
// ? �V���v����Behaviour�p
DEFINE_BEHAVIOUR(CircularMotion,
    // �����o�ϐ�
    float radius = 3.0f;
    float speed = 1.0f;
    float angle = 0.0f;
,
    // OnUpdate���̏���
    angle += speed * dt;
    
    auto* t = w.TryGet<Transform>(self);
    if (t) {
        t->position.x = cosf(angle) * radius;
        t->position.z = sinf(angle) * radius;
    }
);
```

---

## ?? �`�[���J�����[��

### �t�@�C���ҏW�̗D�揇��

#### ?? �R�A�V�X�e���i�G��Ȃ��j
�ȉ��̃t�@�C����**�ύX����ꍇ�̓`�[���S�̂ő��k**:
- `include/ecs/World.h`
- `include/ecs/Entity.h`
- `include/components/Component.h`
- `include/components/Transform.h`
- `include/components/MeshRenderer.h`

#### ? ���R�ɕҏW�\
- `include/scenes/` - �Q�[���V�[���̎���
- `include/components/Custom*.h` - �J�X�^���R���|�[�l���g
- `src/` - �����t�@�C��

#### ?? �v���k
- `include/graphics/` - �O���t�B�b�N�X�V�X�e��
- `include/input/` - ���̓V�X�e��
- `include/app/` - �A�v���P�[�V�������

### Git�R�~�b�g���b�Z�[�W

```bash
# ? �ǂ���: �v���t�B�b�N�X���g�p
git commit -m "? Add player shooting system"
git commit -m "?? Fix collision detection bug"
git commit -m "?? Update README with component guide"
git commit -m "? Optimize render loop performance"
git commit -m "?? Refactor component structure"

# ? ������: ���e���s��
git commit -m "update"
git commit -m "fix bug"
git commit -m "modified files"
```

---

## ?? �Q�l�t�@�C��

�V�����R���|�[�l���g���쐬����ۂ̎Q�l:

- `include/samples/ComponentSamples.h` - �R���|�[�l���g�̎�����
- `include/samples/SampleScenes.h` - �V�[���̎�����
- `include/scenes/MiniGame.h` - ���H�I�ȃQ�[������
- `include/ecs/World.h` - World�N���X�̎g�p���@

---

## ? �R�[�h�������̃`�F�b�N���X�g

GitHub Copilot���R�[�h�𐶐�����ۂ́A�ȉ����m�F���Ă��������F

- [ ] C++14�W���ɏ������Ă���iC++17�ȍ~�̋@�\�͕s�j
- [ ] ECS�O��v�f�iEntity, Component, System�j�𐳂�������
- [ ] �R���|�[�l���g�� `IComponent` �܂��� `Behaviour` ���p��
- [ ] �G���e�B�e�B�쐬�Ƀr���_�[�p�^�[�����g�p�i�����j
- [ ] �|�C���^�擾���� `TryGet` ���g�p���Anull�`�F�b�N
- [ ] �����K��ɏ]���Ă���iPascalCase/camelCase�j
- [ ] Doxygen�X�^�C���̃R�����g���L�q
- [ ] DirectXMath�̌^�iXMFLOAT3�Ȃǁj�𐳂����g�p
- [ ] �O���[�o���ϐ����g�p���Ă��Ȃ�
- [ ] ���������[�N�̉\�����Ȃ��iWorld�������Ǘ��j

---

**�쐬��**: �R���z  
**�ŏI�X�V**: 2025  
**�o�[�W����**: v6.0 - �`�[���Q�[���J���t���[�����[�N

---

## ?? �w�K���\�[�X

- **���S��**: `include/samples/SampleScenes.h` �̃��x��1�`3
- **������**: `include/samples/ComponentSamples.h` ��Behaviour��
- **�㋉��**: `include/scenes/MiniGame.h` �̎���

---

���̃t�@�C���ɏ]�����ƂŁA��ѐ��̂��鍂�i���ȃR�[�h����������܂��B
