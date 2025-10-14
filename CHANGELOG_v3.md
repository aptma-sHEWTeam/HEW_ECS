# ?? �R���|�[�l���g�w�����P - �ύX�_�܂Ƃ�

## ?? ���P�T�v

���w�҂�**�R���|�[�l���g�w����ǂ��₷��**�A**�R���|�[�l���g�̍쐬�E�ǉ����ȒP��**���邽�߂̑啝���P�����{���܂����B

---

## ? ��ȉ��P�_

### 1. **�r���_�[�p�^�[���̓���**

#### Before�i�]���j
```cpp
Entity e1 = world.CreateEntity();
world.Add<Transform>(e1, Transform{...});
world.Add<MeshRenderer>(e1, MeshRenderer{...});
world.Add<Rotator>(e1, Rotator{45.0f});
```

#### After�i�V�����j
```cpp
Entity e1 = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<Rotator>(45.0f)
    .Build();
```

**�����b�g:**
- �R���|�[�l���g�̑g�ݍ��킹����ڂŕ�����
- �G���e�B�e�BID�����x�������Ȃ��Ă���
- ���\�b�h�`�F�[���œǂ݂₷��

---

### 2. **�R���|�[�l���g��`�}�N��**

#### �f�[�^�R���|�[�l���g�p
```cpp
// Before: �S���菑��
struct Health : IComponent {
    float hp = 100.0f;
    float maxHp = 100.0f;
};

// After: �}�N����1�s
DEFINE_DATA_COMPONENT(Health,
    float hp = 100.0f;
    float maxHp = 100.0f;
);
```

#### Behaviour�p
```cpp
// Before: �S���菑��
struct Bouncer : Behaviour {
    float speed = 2.0f;
    float time = 0.0f;
    void OnUpdate(World& w, Entity self, float dt) override {
        time += dt * speed;
        auto* t = w.TryGet<Transform>(self);
        if (t) t->position.y = sinf(time);
    }
};

// After: �}�N���ŊȌ���
DEFINE_BEHAVIOUR(Bouncer,
    float speed = 2.0f;
    float time = 0.0f;
,
    time += dt * speed;
    auto* t = w.TryGet<Transform>(self);
    if (t) t->position.y = sinf(time);
);
```

**�����b�g:**
- �{�C���[�v���[�g�R�[�h���s�v
- ���w�҂��{���ɏW���ł���

---

### 3. **�ڍׂȓ��{��R�����g**

���ׂẴt�@�C���Ɉȉ���ǉ��F

#### �R�����g�̍\��
```cpp
// ========================================================
// �R���|�[�l���g�� - ��s����
// ========================================================
// �y�����z��������R���|�[�l���g��
// �y�����o�ϐ��z�e�ϐ��̈Ӗ�
// �y�g�����z��̓I�ȃR�[�h��
// �y�d�g�݁z�����łǂ�������
// ========================================================
```

#### ���P�����t�@�C��
- `Component.h` - ���N���X�ƃ}�N���̐���
- `Entity.h` - �G���e�B�e�B�̊T�O
- `World.h` - ECS���[���h�̖���
- `Transform.h` - �e�����o�ϐ��̈Ӗ�
- `MeshRenderer.h` - �����_�����O�̐ݒ�
- `Rotator.h` - Behaviour�̎���
- `Animation.h` - �A�j���[�V�����̎g����

---

### 4. **�T���v���R���|�[�l���g�W�̒ǉ�**

�V�t�@�C��: `ComponentSamples.h`

#### �܂܂��T���v��
- **Health** - �̗̓V�X�e��
- **Velocity** - ���x�R���|�[�l���g
- **PlayerTag / EnemyTag** - �^�O�R���|�[�l���g
- **Bouncer** - �㉺�ړ�
- **MoveForward** - �O�i
- **PulseScale** - �g��k��
- **DestroyOnDeath** - �̗�0�ō폜
- **RandomWalk** - �����_���ړ�
- **LifeTime** - ���Ԍo�߂ō폜

**�����b�g:**
- �����Ɏg����T���v��
- �R�s�y���ĉ����ł���
- �w�K���ނƂ��čœK

---

### 5. **App.h��CreateDemoScene()��啝���P**

#### Before
```cpp
void CreateDemoScene() {
    Entity e1 = world.CreateEntity();
    world.Add<Transform>(e1, Transform{...});
    world.Add<MeshRenderer>(e1, MeshRenderer{...});
    world.Add<Rotator>(e1, Rotator{45.0f});
    // ...
}
```

#### After
```cpp
void CreateDemoScene() {
    // ========================================================
    // �L���[�u1: �P�F�ŉ�]����L���[�u�i��ԃV���v���j
    // ========================================================
    // �y�\���zTransform + MeshRenderer + Rotator
    // �y�|�C���g�z�e�N�X�`���Ȃ��A�J���[�̂�
    
    Entity cube1 = world.Create()
        .With<Transform>(DirectX::XMFLOAT3{-2, 0, 0})
        .With<MeshRenderer>(DirectX::XMFLOAT3{0.2f, 0.7f, 1.0f})
        .With<Rotator>(45.0f)
        .Build();
    
    // �L���[�u2...
    // �L���[�u3...
    
    // �y���K���z�R�����g�A�E�g���ꂽ�ۑ肠��
}
```

**�����b�g:**
- �e�G���e�B�e�B�̖ړI�����m
- �R���|�[�l���g�̑g�ݍ��킹��
- ���K���Ŋw�K�ł���

---

### 6. **README.md�̑S�ʉ���**

#### �V�Z�N�V����
1. **�R���|�[�l���g�w���Ƃ́H** - �T�O�̐���
2. **���ȒP�I�G���e�B�e�B�̍���** - 2�p�^�[���Љ�
3. **�R���|�[�l���g�̍���** - 3��ނ̕��@
4. **�w�K�K�C�h** - �X�e�b�v�o�C�X�e�b�v
5. **�悭����g����** - ���p��
6. **���K���** - ����/����/�㋉
7. **FAQ** - �悭���鎿��

**�����b�g:**
- �v���W�F�N�g�̖ړI�����m
- �w�K�̓��؂�������
- �����Ɏn�߂���

---

## ?? ���w�҂ւ̔z��

### �R�����g�̕��j
- **�u�����v�����łȂ��u�Ȃ��v�����**
- **��̗��K���܂߂�**
- **���p��ɂ͕⑫����**
- **�i�K�I�ɗ����ł���\��**

### �R�[�h�̕��j
- **�ǐ����ŗD��**�i�����͓x�O���j
- **��ѐ��̂��閽���K��**
- **�璷�ł�������₷��**
- **�}�W�b�N�i���o�[�͋ɗ͔r��**

---

## ?? �w�K�Ȑ��̉��P

### Before
```
��Փx: ����������������������
           �}�Ȋw�K�Ȑ� ��
                       ��
���w��: �����ō��܄���������
```

### After
```
��Փx: ����������������������
       �� �Ȃ��炩�Ȋw�K�Ȑ�
       ��
���w��: ������������������������ �����ł���I
    �T���v�� �� ���K �� ���p
```

---

## ?? �Z�p�I�ȉ��P

### EntityBuilder�N���X
```cpp
class EntityBuilder {
public:
    EntityBuilder(World* world, Entity entity);
    
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args);
    
    Entity Build();
    operator Entity() const; // �Öٕϊ�
};
```

**����:**
- ���\�b�h�`�F�[���Ή�
- ���S�]���Ńp�t�H�[�}���X�ێ�
- Build()�ȗ��\

### Component.h�̃}�N��
```cpp
#define DEFINE_DATA_COMPONENT(Name, Members) \
    struct Name : IComponent { Members }

#define DEFINE_BEHAVIOUR(Name, Members, Code) \
    struct Name : Behaviour { \
        Members \
        void OnUpdate(World& w, Entity self, float dt) override { \
            Code \
        } \
    }
```

**����:**
- �ϒ������Ή�
- �^���S�����ێ�
- C++14�݊�

---

## ?? �ύX�t�@�C���ꗗ

### �V�K�쐬
- `ComponentSamples.h` - �T���v���R���|�[�l���g�W

### �啝�ύX
- `Component.h` - �}�N���ǉ��A�R�����g�[��
- `Entity.h` - �R�����g�ǉ�
- `World.h` - EntityBuilder�ǉ��A�R�����g�[��
- `Transform.h` - �R�����g�[��
- `MeshRenderer.h` - �R�����g�[��
- `Rotator.h` - �R�����g�[��
- `Animation.h` - �R�����g�[��
- `App.h` - CreateDemoScene()���P
- `README.md` - �S�ʉ���

### �݊���
- **100%����݊�** - �����R�[�h�͂��̂܂ܓ���
- �]���̕��@�����������g�p�\
- �V�����ƍ��݂��Ă�OK

---

## ?? �w�K���\�[�X

### �t�@�C����ǂޏ��ԁi�����j

1. **README.md** - �S�̑���c��
2. **Entity.h** - �G���e�B�e�B�Ƃ�
3. **Component.h** - �R���|�[�l���g�Ƃ�
4. **Transform.h** - �f�[�^�R���|�[�l���g�̎���
5. **Rotator.h** - Behaviour�̎���
6. **World.h** - World�̖���
7. **App.h** - CreateDemoScene()�őg�ݍ��킹�����w��
8. **ComponentSamples.h** - �L�x�ȃT���v��

### ���K�̐i�ߕ�

1. **�T���v���𓮂���** - �܂��r���h�����s
2. **���l��������** - Rotator�̑��x��ς��Ă݂�
3. **�F��ς���** - MeshRenderer�̐F��ς���
4. **�V�����L���[�u��ǉ�** - ���K��������
5. **�V�����R���|�[�l���g�����** - �T���v�����Q�l��
6. **�g�ݍ��킹�ėV��** - �����̃R���|�[�l���g

---

## ?? �݌v�v�z

### �R���|�[�l���g�w���̖{��

```
? �_���ȗ�: 
class Enemy : public Character {
    // �G��p�̏����������፬��
}

? �ǂ���:
Entity enemy = world.Create()
    .With<Transform>()    // �ʒu�̋@�\
    .With<Health>()       // �̗͂̋@�\
    .With<EnemyAI>()      // AI�̋@�\
    .With<MeshRenderer>() // �����ڂ̋@�\
```

### �����̗��_
- **�ė��p** - Health�͓G�ɂ��v���C���[�ɂ��g����
- **�e�X�g** - �e�R���|�[�l���g���ʂɃe�X�g
- **�g��** - �V�@�\��ǉ����Ă��R���|�[�l���g�𑝂₷����
- **����** - ���������i�Ȃ̂ŗ������₷��

---

## ?? ����̊w�K�X�e�b�v

### ������
- [ ] �T���v����S������
- [ ] �F�⑬�x��ς���
- [ ] �V�����L���[�u��ǉ�

### ������
- [ ] �V����Behaviour�����
- [ ] �R���|�[�l���g���m�ŘA�g
- [ ] �Q�[�����W�b�N������

### �㋉��
- [ ] �Փ˔���V�X�e��
- [ ] �p�[�e�B�N���V�X�e��
- [ ] �J�X�^�������_�����O

---

## ?? �Ō��

���̃v���W�F�N�g��**�{���̏��w�Ҍ����ɃR���|�[�l���g�w����������邽��**�ɍ쐬����܂����B

### ��؂ɂ�������
- ? ���p����ɗ͎g��Ȃ�
- ? �u�Ȃ��v���������
- ? ��𓮂����Ċw�ׂ�
- ? �����Ɍ��ʂ�������
- ? �i�K�I�ɓ�Փx���オ��

**�N�ł��R���|�[�l���g�w���������ł���**�悤�ɐS�����܂����B

�y����Ŋw��ł��������I??

---

**�쐬��: �R���z**
**�o�[�W����: v3.0 - Component-Oriented Learning Edition**
