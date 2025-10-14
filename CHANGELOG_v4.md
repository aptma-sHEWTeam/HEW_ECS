# ?? �ύX���� - v4.0 �ǐ�������

**�����[�X��:** 2024�N  
**�e�[�}:** ���w�҂����܂��Ȃ��R�[�h

---

## ?? v4.0�̖ڕW

> **�u�N���ǂ�ł����������ĂŌ��₷���R�[�h�v**

- ? ���������ǐ����ŗD��
- ? �R�����g�͊Ȍ��ɁA�R�[�h�Ō��
- ? �����t���[����ڂł킩��
- ? ���w�҂�����Ȃ��\��

---

## ?? ��ȕύX�_

### 1. **App.h�̊��S���t�@�N�^�����O**

#### Before�iv3.0�j
```cpp
void Run() {
    MSG msg{};
    auto prev = std::chrono::high_resolution_clock::now();
    
    while (msg.message != WM_QUIT) {
        // ���b�Z�[�W����
        if (PeekMessage(...)) { ... }
        
        // ���Ԍv�Z
        auto now = ...
        
        // ����
        input.Update();
        
        // �X�V
        world.Tick(dt);
        
        // �`��
        gfx.BeginFrame();
        renderer.Render(...);
        gfx.EndFrame();
    }
}
```

#### After�iv4.0�j
```cpp
void Run() {
    MSG msg{};
    auto previousTime = std::chrono::high_resolution_clock::now();
    
    while (msg.message != WM_QUIT) {
        // �y1�zWindows���b�Z�[�W����
        if (ProcessWindowsMessages(msg)) continue;
        
        // �y2�z���Ԃ̌v�Z
        float deltaTime = CalculateDeltaTime(previousTime);
        
        // �y3�z���͂̍X�V
        ProcessInput(deltaTime);
        
        // �y4�z�Q�[�����W�b�N�̍X�V
        UpdateGameLogic(deltaTime);
        
        // �y5�z��ʂ̕`��
        RenderFrame();
    }
}

private:
    bool ProcessWindowsMessages(MSG& msg) { ... }
    float CalculateDeltaTime(...) { ... }
    void ProcessInput(float deltaTime) { ... }
    void UpdateGameLogic(float deltaTime) { ... }
    void RenderFrame() { ... }
```

**���P�_:**
- ���C�����[�v��5�s�ɋÏk
- �e�t�F�[�Y���Ɨ������֐�
- �����̗��ꂪ�ԍ��t���Ŗ��m
- �֐����ŉ������邩��ڗđR

---

### 2. **�ϐ����̓���**

#### Before�iv3.0�j
```cpp
struct App {
    HWND hwnd = nullptr;
    GfxDevice gfx;
    RenderSystem renderer;
    TextureManager texManager;
    World world;
    Camera cam;
    InputSystem input;
    VideoPlayer* videoPlayer = nullptr;
};
```

#### After�iv4.0�j
```cpp
struct App {
    HWND hwnd_ = nullptr;
    GfxDevice gfx_;
    RenderSystem renderer_;
    TextureManager texManager_;
    World world_;
    Camera camera_;
    InputSystem input_;
    VideoPlayer* videoPlayer_ = nullptr;
};
```

**����K��:**
- �����o�ϐ�: `�ϐ���_`�i�A���_�[�X�R�A�t���j
- ���[�J���ϐ�: `�ϐ���`�i�A���_�[�X�R�A�Ȃ��j
- �萔: `CONSTANT_NAME`�i�S���啶���j

---

### 3. **CreateDemoScene()�̕���**

#### Before�iv3.0�j
```cpp
void CreateDemoScene() {
    // 700�s�̋���֐�
    Entity cube1 = ...
    // �e�N�X�`���ǂݍ���
    // ����v���C���[�ݒ�
    // ...
}
```

#### After�iv4.0�j
```cpp
void CreateDemoScene() {
    CreateSimpleRotatingCube();
    CreateTexturedCube();
    CreateVideoCube();
}

private:
    void CreateSimpleRotatingCube() { ... }
    void CreateTexturedCube() { ... }
    void CreateVideoCube() { ... }
```

**���P�_:**
- �e�L���[�u�쐬���Ɨ������֐�
- �֐����ŖړI�����m
- �ǂ݂₷���A�������₷��

---

### 4. **�R�����g�̉��P**

#### Before�iv3.0�j - �璷
```cpp
// ========================================================
// �X�e�b�v1: �E�B���h�E�N���X�̓o�^
// �iWindows�ɃE�B���h�E�̐݌v�}��������j
// ========================================================

// WNDCLASSEX�\���̂��������i�T�C�Y���������ݒ�j
WNDCLASSEX wc{ sizeof(WNDCLASSEX) };

// �E�B���h�E�X�^�C��: �����E�c�����ς������S�̂��ĕ`��
wc.style = CS_HREDRAW | CS_VREDRAW;
```

#### After�iv4.0�j - �Ȍ�
```cpp
// �E�B���h�E�N���X�̓o�^
WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
wc.style = CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc = WndProcStatic;
wc.hInstance = hInst;
wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
wc.lpszClassName = L"SimpleECS_DX11_Class";

if (!RegisterClassEx(&wc)) {
    return false;
}
```

**���j:**
- �������邩��1�s�Ő���
- �Ȃ��������邩�͕K�v�Ȏ�����
- �R�[�h���̂��ǂ݂₷����΃R�����g�s�v

---

### 5. **�V�K�t�@�C���ǉ�**

#### `SampleScenes.h`
**�ړI:** �i�K�I�Ɋw�ׂ�T���v���W

```cpp
namespace SampleScenes {
    // ���x��1: �ł��V���v��
    Entity CreateSimpleCube(World& world);
    
    // ���x��2: ������ǉ�
    Entity CreateRotatingCube(World& world, const DirectX::XMFLOAT3& pos);
    
    // ���x��3: �J�X�^��Behaviour
    Entity CreateBouncingCube(World& world);
    
    // ... ���x��9�܂�
}
```

**����:**
- ���x��1����9�܂Œi�K�I
- �e���x���ŐV�����T�O��1�w��
- �R�s�y���ĉ����ł���

#### `ComponentSamples.h`�i���P�j
**���P�_:**
- �J�e�S���ʂɐ���
- �g�p����R�����g�Ŗ��L
- �}�N���łƒʏ�ł̗������

```cpp
// �J�e�S��1: �f�[�^�R���|�[�l���g
struct Health : IComponent { ... };
struct Velocity : IComponent { ... };

// �J�e�S��2: �V���v����Behaviour
struct Bouncer : Behaviour { ... };
struct MoveForward : Behaviour { ... };

// �J�e�S��3: ���G��Behaviour
struct DestroyOnDeath : Behaviour { ... };
struct RandomWalk : Behaviour { ... };

// �J�e�S��4: �}�N����
DEFINE_BEHAVIOUR(SpinAndColor, ...);
```

#### `LEARNING_GUIDE.md`
**�ړI:** �����m��Ȃ����w�Ҍ����X�e�b�v�o�C�X�e�b�v�K�C�h

**�\��:**
- ���b�X��1: �G���e�B�e�B�̗���
- ���b�X��2: �R���|�[�l���g�̗���
- ���b�X��3: �r���_�[�p�^�[��
- ���b�X��4: Behaviour�̎g����
- ���b�X��5: �R���|�[�l���g�̑g�ݍ��킹
- ���b�X��6: ����R���|�[�l���g
- ���b�X��7: �R���|�[�l���g�ԘA�g
- ���b�X��8: �f�o�b�O���@
- ���b�X��9: ���H�v���W�F�N�g

---

### 6. **README.md�̊��S���j���[�A��**

#### �V�Z�N�V����
1. **�N�C�b�N�X�^�[�g** - 3�X�e�b�v�œ�������
2. **�w�K�̐i�ߕ�** - ���������𖾋L
3. **�d�v�ȊT�O** - Entity, Component, World�̐���
4. **�G���e�B�e�B�̍���** - 2�̕��@���r
5. **�悭����g����** - �R�[�h�X�j�y�b�g�W
6. **�t�@�C���\��** - �c���[�\��
7. **���K���** - ����/����/�㋉
8. **FAQ** - �悭���鎿��Ɖ�
9. **���̃X�e�b�v** - ���W�I�Ȋw�K

#### ���P�_
- �G�����Ŏ��o�I�ɕ�����₷��
- �R�[�h�u���b�N�ɃR�����g�t��
- ���s���ʂ𖾋L
- �i�K�I�ɓ�Փx���グ��

---

## ?? �R�[�h�i���̉��P

### ���g���N�X

| ���� | v3.0 | v4.0 | ���P |
|------|------|------|------|
| App.h�̍s�� | 700+ | 450 | ?? 35% |
| �Œ��֐��̍s�� | 150 | 30 | ?? 80% |
| �R�����g���x | ���� | �K�� | ? |
| �֐��̐� | 5 | 15 | ?? ���m�� |
| �z�I���G�x | �� | �� | ? |

### �ǐ��X�R�A

- **�֐����̖��m��:** ?? ����
- **�����t���[�̗���:** ?? �e��
- **�ϐ����̈�ѐ�:** ?? ����
- **�R�����g�̓K�؂�:** ?? �Ȍ�
- **���w�҂ւ̗D����:** ?? �ō�

---

## ?? �w�K�̌��̉��P

### Before�iv3.0�j

```
���w�҂̊w�K�t���[:
1. App.h���J��
2. 700�s�̃R�[�h�Ɉ��|�����
3. �ǂ�����ǂ߂�... ?����
```

### After�iv4.0�j

```
���w�҂̊w�K�t���[:
1. README.md��ǂ�
2. LEARNING_GUIDE.md�ŃX�e�b�v�o�C�X�e�b�v
3. SampleScenes.h�Œi�K�I�Ɋw��
4. ComponentSamples.h���R�s�y���ĉ���
5. �����ŃR���|�[�l���g������I ?����
```

---

## ??? �Z�p�I���P

### ���\�b�h�����̌���

```cpp
// Before: 1�̋��僁�\�b�h
void Run() {
    // 150�s�̃R�[�h
    // ���b�Z�[�W����
    // ���Ԍv�Z
    // ���͏���
    // �Q�[���X�V
    // �`�揈��
}

// After: ���m�ɕ������ꂽ5�̃��\�b�h
void Run() {
    while (...) {
        ProcessWindowsMessages(...);
        CalculateDeltaTime(...);
        ProcessInput(...);
        UpdateGameLogic(...);
        RenderFrame();
    }
}
```

**�����b�g:**
- �P��ӔC�̌����iSRP�j�ɏ���
- �e�X�g���₷��
- �ė��p���₷��
- �������₷��

---

## ?? �V�K�E�ύX�t�@�C���ꗗ

### �V�K�쐬
- ? `SampleScenes.h` - �i�K�I�w�K�p�T���v���W
- ? `LEARNING_GUIDE.md` - �X�e�b�v�o�C�X�e�b�v�K�C�h
- ? `CHANGELOG_v4.md` - ���̃t�@�C��

### �啝�ύX
- ?? `App.h` - ���S���t�@�N�^�����O
- ?? `ComponentSamples.h` - �J�e�S���ʐ����A�R�����g�[��
- ?? `README.md` - ���S���j���[�A��

### �y���ȕύX
- ?? ���̑��̃w�b�_�[�t�@�C�� - �R�����g�̒���

---

## ?? �݌v���j�̕ύX

### v3.0�̕��j
> �ڍׂȃR�����g�Ő�������

### v4.0�̕��j
> �R�[�h���̂��ǂ݂₷���A�R�����g�͍ŏ�����

**���R:**
- �R�����g�͌Â��Ȃ�₷��
- �R�[�h�����Ȑ����I�ł���ׂ�
- �֐����E�ϐ����ňӐ}��`����

---

## ?? ����̗\��

### v4.1�i�\��j
- [ ] InputSystem�̊ȈՃ��b�p�[
- [ ] �Փ˔���V�X�e���̃T���v��
- [ ] UI�V�X�e���̃T���v��

### v4.2�i�\��j
- [ ] �p�[�e�B�N���V�X�e��
- [ ] �T�E���h�V�X�e��
- [ ] �Z�[�u/���[�h�V�X�e��

---

## ?? �t�B�[�h�o�b�N

���̃o�[�W�����Ŋw�K���₷���Ȃ�܂������H

- �킩��₷�������_
- �킩��ɂ��������_
- ���P���Ăق����_

�t�B�[�h�o�b�N�����҂����Ă��܂��I

---

## ?? �ӎ�

���w�҂̎��_��Y�ꂸ�A�N�����y�����v���O���~���O���w�ׂ鋳�ނ�ڎw���܂����B

**�R���|�[�l���g�w���͓���Ȃ��I**

���̃v���W�F�N�g��ʂ��āA�����̐l��ECS�̊y������m���Ă��ꂽ��������ł��B

---

**�쐬��: �R���z**  
**�o�[�W����: v4.0 - �ǐ�������**  
**�����[�X��: 2024�N**

---

## ?? ���v���

### �R�[�h��
- ���s��: ��3000�s�i�R�����g�܂ށj
- C++�R�[�h: ��2000�s
- �R�����g: ��1000�s
- �h�L�������g: ��2000�s

### �t�@�C����
- �w�b�_�[�t�@�C��: 20��
- �h�L�������g: 5�iREADME, LEARNING_GUIDE, CHANGELOG���j

### �w�K�R���e���c
- �T���v���R���|�[�l���g: 15��
- �T���v���V�[��: 9��
- ���b�X����: 9��
- ���K���: 15�ȏ�

---

**�y����Ŋw��ł��������I ??**
