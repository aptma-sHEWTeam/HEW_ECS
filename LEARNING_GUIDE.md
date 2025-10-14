# ?? ECS�w�K�K�C�h - �X�e�b�v�o�C�X�e�b�v

���̃K�C�h�́A**�����킩��Ȃ����w��**��**�����ŃR���|�[�l���g������悤�ɂȂ�**�܂ł̓��̂�ł��B

---

## ?? �w�K�̃S�[��

���̃K�C�h���I����ƁA�ȉ����ł���悤�ɂȂ�܂��F

- ? �G���e�B�e�B�ƃR���|�[�l���g�̊T�O�𗝉�����
- ? �����ŃR���|�[�l���g���쐬�ł���
- ? �R���|�[�l���g��g�ݍ��킹�ăQ�[���I�u�W�F�N�g������
- ? �f�o�b�O���@���킩��

---

## ?? ���b�X��1: �G���e�B�e�B�𗝉�����

### �G���e�B�e�B�Ƃ́H

**�G���e�B�e�B = �Q�[�����E�ɑ��݂���u���v��\��ID�ԍ�**

```cpp
Entity player;   // �����ID�ԍ��i��: 1�j
Entity enemy;    // �����ID�ԍ��i��: 2�j
Entity bullet;   // �����ID�ԍ��i��: 3�j
```

### �d�v�ȃ|�C���g

- ? �G���e�B�e�B���̂ɂ͋@�\���Ȃ��i�����̔ԍ��j
- ? �R���|�[�l���g��t���邱�Ƃŋ@�\������
- ? �����G���e�B�e�B�ɕ����̃R���|�[�l���g��t������

### ���K: �G���e�B�e�B������Ă݂�

```cpp
// App.h��CreateDemoScene()�ɒǉ�

void CreateDemoScene() {
    // �G���e�B�e�B��1�쐬
    Entity myFirstEntity = world_.CreateEntity();
    
    // �܂������@�\���Ȃ��iID�ԍ������j
    // ���̃��b�X���ŃR���|�[�l���g��t���܂�
}
```

**�����Ă݂悤:**
1. ��L�̃R�[�h��`App.h`�ɒǉ�
2. �r���h�����s�iF5�j
3. �����\������Ȃ� �� ����I�i�܂������ڂ��Ȃ�����j

---

## ?? ���b�X��2: �R���|�[�l���g�𗝉�����

### �R���|�[�l���g�Ƃ́H

**�R���|�[�l���g = �G���e�B�e�B�ɕt����u���i�v**

��: ���S�u���b�N�̂悤�ɁA���i��g�ݍ��킹�ĕ������

```
�v���C���[ = �G���e�B�e�B
           + Transform�i�ʒu�j
           + MeshRenderer�i�����ځj
           + PlayerController�i����j
           + Health�i�̗́j
```

### �R���|�[�l���g�̎��

#### 1. �f�[�^�R���|�[�l���g�i��Ԃ�ۑ����邾���j

```cpp
struct Transform : IComponent {
    DirectX::XMFLOAT3 position; // �ʒu
    DirectX::XMFLOAT3 rotation; // ��]
    DirectX::XMFLOAT3 scale;    // �傫��
};
```

#### 2. Behaviour�R���|�[�l���g�i�������`�j

```cpp
struct Rotator : Behaviour {
    float speedDegY = 45.0f; // ��]���x
    
    // ���t���[���Ă΂��
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            t->rotation.y += speedDegY * dt; // ��]������
        }
    }
};
```

### ���K: �R���|�[�l���g��ǉ����Ă݂�

```cpp
void CreateDemoScene() {
    // �G���e�B�e�B���쐬
    Entity myFirstEntity = world_.CreateEntity();
    
    // Transform�i�ʒu���j��ǉ�
    Transform transform;
    transform.position = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f}; // ���_
    transform.rotation = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
    transform.scale = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};
    world_.Add<Transform>(myFirstEntity, transform);
    
    // MeshRenderer�i�����ځj��ǉ�
    MeshRenderer renderer;
    renderer.color = DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f}; // �ԐF
    world_.Add<MeshRenderer>(myFirstEntity, renderer);
}
```

**�����Ă݂悤:**
1. ��L�̃R�[�h��`App.h`�ɒǉ�
2. �r���h�����s
3. �Ԃ��L���[�u���\�������I ??

---

## ?? ���b�X��3: �r���_�[�p�^�[�����g��

### �����ƊȒP�ɏ������@

���b�X��2�̃R�[�h�͒����ł��ˁB**�r���_�[�p�^�[��**���g���ƒZ�������܂��B

#### Before�i�����j
```cpp
Entity myFirstEntity = world_.CreateEntity();
Transform transform;
transform.position = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
// ... �ȗ� ...
world_.Add<Transform>(myFirstEntity, transform);
```

#### After�i�Z���I�j
```cpp
Entity myFirstEntity = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})  // �ʒu�����w��
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0}) // �ԐF
    .Build();
```

### ���K: �r���_�[�p�^�[���ŏ�������

```cpp
void CreateDemoScene() {
    // �Ԃ��L���[�u���쐬
    Entity redCube = world_.Create()
        .With<Transform>(
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},  // �ʒu
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},  // ��]
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}   // �X�P�[��
        )
        .With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f}) // ��
        .Build();
}
```

**�ۑ�:**
1. �ΐF�̃L���[�u������Ă݂�i�ʒu: X=2�j
2. �F�̃L���[�u������Ă݂�i�ʒu: X=-2�j

<details>
<summary>�𓚗�</summary>

```cpp
// �ΐF�̃L���[�u
Entity greenCube = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{2.0f, 0.0f, 0.0f})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f})
    .Build();

// �F�̃L���[�u
Entity blueCube = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{-2.0f, 0.0f, 0.0f})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f})
    .Build();
```

</details>

---

## ?? ���b�X��4: ������t����iBehaviour�j

### Rotator��ǉ�����

��]������ɂ́A`Rotator`�R���|�[�l���g��ǉ����܂��B

```cpp
Entity rotatingCube = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<Rotator>(45.0f) // �� ���b45�x��]
    .Build();
```

### ���K: ��]���x��ς��Ă݂�

```cpp
// ��������]
.With<Rotator>(10.0f)   // ���b10�x

// ������]
.With<Rotator>(180.0f)  // ���b180�x

// �t��]
.With<Rotator>(-45.0f)  // �t����
```

**�ۑ�:**
1. ������]����΂̃L���[�u�����
2. �t��]����̃L���[�u�����

---

## ?? ���b�X��5: �R���|�[�l���g��g�ݍ��킹��

### ������Behaviour��ǉ�

1�̃G���e�B�e�B�ɁA������Behaviour��t�����܂��I

```cpp
Entity cube = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 1, 0})
    .With<Rotator>(45.0f)      // ��]����
    .With<Bouncer>()           // �㉺�ɓ����iComponentSamples.h�j
    .Build();
```

���̃L���[�u�́F
- ? ��]���Ȃ���
- ? �㉺�ɓ���

### ���K: ComponentSamples.h���g��

`ComponentSamples.h`�ɂ́A�֗��ȃR���|�[�l���g���������񂠂�܂��B

```cpp
#include "ComponentSamples.h" // �� App.h�̍ŏ��ɒǉ�

void CreateDemoScene() {
    // �㉺�ɓ����L���[�u
    Entity bouncing = world_.Create()
        .With<Transform>(DirectX::XMFLOAT3{-3, 0, 0})
        .With<MeshRenderer>(DirectX::XMFLOAT3{1, 1, 0})
        .With<Bouncer>() // �� ComponentSamples.h����
        .Build();
    
    // �傫�����ς��L���[�u
    Entity pulse = world_.Create()
        .With<Transform>(DirectX::XMFLOAT3{3, 0, 0})
        .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 1})
        .With<PulseScale>() // �� ComponentSamples.h����
        .Build();
}
```

**�ۑ�:**
1. `Bouncer`��`Rotator`�𗼕��t���Ă݂�
2. `PulseScale`��`ColorCycle`�𗼕��t���Ă݂�

---

## ?? ���b�X��6: �����ŃR���|�[�l���g�����

### �ŏ��̃J�X�^��Behaviour

**�ڕW:** �O�ɐi�ރR���|�[�l���g�����

#### �X�e�b�v1: �\���̂��`

```cpp
// App.h��private:�̑O�ɒǉ�

struct MoveForward : Behaviour {
    float speed = 2.0f; // ���x
    
    void OnUpdate(World& w, Entity self, float dt) override {
        // Transform���擾
        auto* t = w.TryGet<Transform>(self);
        if (!t) return; // Transform���Ȃ���ΏI��
        
        // Z�������ɐi��
        t->position.z += speed * dt;
    }
};
```

#### �X�e�b�v2: �g���Ă݂�

```cpp
Entity movingCube = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, -5})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 1})
    .With<MoveForward>() // �� ����R���|�[�l���g
    .Build();
```

**����:** �L���[�u�����Ɍ������Đi��ł����I

### ���K: �������Ă݂�

```cpp
// ��ɐi��
t->position.y += speed * dt;

// ���ɐi��
t->position.x += speed * dt;

// �΂߂ɐi��
t->position.x += speed * dt;
t->position.y += speed * dt;
```

**�ۑ�:**
1. ��ɐi�ރR���|�[�l���g`MoveUp`�����
2. �~��`���R���|�[�l���g`CircleMotion`�����i����j

<details>
<summary>CircleMotion�̉𓚗�</summary>

```cpp
struct CircleMotion : Behaviour {
    float radius = 3.0f;
    float speed = 1.0f;
    float angle = 0.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        angle += speed * dt;
        
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            t->position.x = cosf(angle) * radius;
            t->position.z = sinf(angle) * radius;
        }
    }
};
```

</details>

---

## ?? ���b�X��7: �R���|�[�l���g�Ԃ̘A�g

### ���̃R���|�[�l���g���Q�Ƃ���

**��:** �̗͂�0�ɂȂ�����폜����

#### �X�e�b�v1: Health�R���|�[�l���g��ǉ�

```cpp
Entity enemy = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .Build();

// �̗͂�ǉ�
Health hp;
hp.current = 100.0f;
hp.max = 100.0f;
world_.Add<Health>(enemy, hp);
```

#### �X�e�b�v2: DestroyOnDeath��ǉ�

```cpp
world_.Add<DestroyOnDeath>(enemy, DestroyOnDeath{});
```

#### �X�e�b�v3: �_���[�W��^����

```cpp
// �ǂ����Łi��: �L�[���͎��j
auto* health = world_.TryGet<Health>(enemy);
if (health) {
    health->TakeDamage(10.0f); // 10�_���[�W
}
```

**����:** �̗͂�0�ɂȂ�����G���e�B�e�B��������I

---

## ?? ���b�X��8: �f�o�b�O���@

### �R���|�[�l���g�̒l���m�F

```cpp
// Transform�̒l���m�F
auto* t = world_.TryGet<Transform>(entity);
if (t) {
    // �u���[�N�|�C���g��ݒ肵�Č���
    float x = t->position.x;
    float y = t->position.y;
    float z = t->position.z;
}
```

### OutputDebugString���g��

```cpp
#include <sstream>

void OnUpdate(World& w, Entity self, float dt) override {
    auto* t = w.TryGet<Transform>(self);
    if (t) {
        std::wostringstream oss;
        oss << L"Position: " << t->position.x << L", " 
            << t->position.y << L", " << t->position.z << L"\n";
        OutputDebugString(oss.str().c_str());
    }
}
```

Visual Studio�́u�o�́v�E�B���h�E�ɕ\������܂��B

---

## ?? ���b�X��9: ���H�v���W�F�N�g

### �v���W�F�N�g1: �V���v���ȃQ�[��

**�ڕW:** �v���C���[������ł���L���[�u�����

```cpp
// 1. Player�^�O���`
struct PlayerTag : IComponent {};

// 2. �v���C���[����R���|�[�l���g
struct PlayerController : Behaviour {
    float speed = 5.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (!t) return;
        
        // TODO: InputSystem���g����WASD�ňړ�
    }
};

// 3. �v���C���[�G���e�B�e�B�쐬
Entity player = world_.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
    .With<PlayerTag>()
    .With<PlayerController>()
    .Build();
```

### �v���W�F�N�g2: �G�̐����V�X�e��

**�ڕW:** ��莞�Ԃ��ƂɓG�𐶐�

```cpp
struct EnemySpawner : Behaviour {
    float spawnInterval = 2.0f; // 2�b����
    float timer = 0.0f;
    
    void OnUpdate(World& w, Entity self, float dt) override {
        timer += dt;
        
        if (timer >= spawnInterval) {
            timer = 0.0f;
            SpawnEnemy(w);
        }
    }
    
    void SpawnEnemy(World& w) {
        // �����_���Ȉʒu
        float x = (rand() % 10) - 5.0f;
        
        w.Create()
            .With<Transform>(DirectX::XMFLOAT3{x, 0, 10})
            .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
            .With<MoveForward>()
            .Build();
    }
};
```

---

## ?? ���Ǝ���

�ȉ����ł�����AECS�}�X�^�[�I

### �K�{�ۑ�
- [ ] ������Behaviour��3���
- [ ] ������g�ݍ��킹���G���e�B�e�B�����
- [ ] �R���|�[�l���g�ԂŘA�g������

### ���W�ۑ�
- [ ] �Փ˔���V�X�e�������
- [ ] �p�[�e�B�N���V�X�e�������
- [ ] UI�\���V�X�e�������

---

## ?? �Q�l����

### �v���W�F�N�g��
- `ComponentSamples.h` - �R���|�[�l���g�̗�
- `SampleScenes.h` - �V�[���쐬�̗�
- `README.md` - �S�̃K�C�h

### �O������
- DirectX11 �����h�L�������g
- ECS �A�[�L�e�N�`�����

---

**�撣���Ă��������I ??**

����������Ȃ����Ƃ�����΁A�R�[�h�̃R�����g��ǂ�ł݂Ă��������B  
���ׂẴR�[�h�ɐ����������Ă���܂��B
