# ?? ECS + DirectX11 �v���W�F�N�g - �N�C�b�N���t�@�����X

**���w�Ҍ����R���|�[�l���g�w���v���O���~���O���� v4.0**

---

## ?? 5���Ŏn�߂�

### 1. �r���h�����s
```
Visual Studio 2022�ŊJ�� �� F5�L�[ �� ���s�I
```

### 2. �ŏ��ɓǂރt�@�C��
1. **README.md** - �v���W�F�N�g�S�̂̐���
2. **LEARNING_GUIDE.md** - �X�e�b�v�o�C�X�e�b�v�w�K
3. **App.h** - ���C���R�[�h

---

## ?? �t�@�C���ꗗ�i�d�v�x���j

### ??? �K�ǃt�@�C��
| �t�@�C�� | ���� | �s�� |
|---------|------|------|
| `README.md` | �v���W�F�N�g�S�̃K�C�h | 500�s |
| `LEARNING_GUIDE.md` | �i�K�I�w�K�K�C�h | 800�s |
| `App.h` | ���C���A�v���P�[�V���� | 450�s |

### ?? �d�v�t�@�C��
| �t�@�C�� | ���� | �s�� |
|---------|------|------|
| `Entity.h` | �G���e�B�e�B�̒�` | 50�s |
| `Component.h` | �R���|�[�l���g��� | 100�s |
| `World.h` | ECS���[���h�Ǘ� | 200�s |
| `Transform.h` | �ʒu�E��]�E�X�P�[�� | 30�s |
| `MeshRenderer.h` | �`��ݒ� | 40�s |
| `Rotator.h` | ������] | 50�s |

### ? �w�K�p�T���v��
| �t�@�C�� | ���� | �s�� |
|---------|------|------|
| `ComponentSamples.h` | �R���|�[�l���g�W | 500�s |
| `SampleScenes.h` | �V�[���쐬�� | 400�s |

### �V�X�e���t�@�C���i�ǂ܂Ȃ���OK�j
- `GfxDevice.h` - DirectX11�Ǘ�
- `RenderSystem.h` - �`��V�X�e��
- `InputSystem.h` - ���̓V�X�e��
- `TextureManager.h` - �e�N�X�`���Ǘ�
- `Camera.h` - �J����
- `DebugDraw.h` - �f�o�b�O�`��
- `Animation.h` - �A�j���[�V����
- `VideoPlayer.h` - ����Đ�

---

## ?? �d�v�ȊT�O�i3�����o����j

### 1. Entity�i�G���e�B�e�B�j
```cpp
Entity player = world.CreateEntity();
```
- �Q�[�����E�́u���v��\��ID�ԍ�
- ���ꎩ�̂ɂ͋@�\���Ȃ�

### 2. Component�i�R���|�[�l���g�j
```cpp
struct Transform : IComponent {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
};
```
- �G���e�B�e�B�ɕt����u���i�v
- 2���: �f�[�^�R���|�[�l���g / Behaviour�R���|�[�l���g

### 3. World�i���[���h�j
```cpp
World world;
world.Add<Transform>(entity, Transform{...});
auto* t = world.TryGet<Transform>(entity);
```
- �S�G���e�B�e�B�ƃR���|�[�l���g���Ǘ�
- �ǉ��E�폜�E�擾���s��

---

## ?? �悭����R�[�h�p�^�[��

### �p�^�[��1: �G���e�B�e�B�쐬
```cpp
// �r���_�[�p�^�[���i�����j
Entity cube = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<Rotator>(45.0f)
    .Build();
```

### �p�^�[��2: �R���|�[�l���g�擾���ύX
```cpp
auto* transform = world.TryGet<Transform>(entity);
if (transform) {
    transform->position.y += 1.0f;
}
```

### �p�^�[��3: �S�G���e�B�e�B�ɏ���
```cpp
world.ForEach<Transform>([](Entity e, Transform& t) {
    t.position.y += 0.01f;
});
```

### �p�^�[��4: �J�X�^��Behaviour�쐬
```cpp
struct MyBehaviour : Behaviour {
    void OnUpdate(World& w, Entity self, float dt) override {
        auto* t = w.TryGet<Transform>(self);
        if (t) {
            t->rotation.y += 90.0f * dt;
        }
    }
};
```

---

## ?? �悭���鑀��

### �L���[�u�̐F��ς���
```cpp
// App.h �� CreateSimpleRotatingCube() ��
.With<MeshRenderer>(DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f}) // ��
//                                      ��    ��    ��
//                                      R    G    B (0.0 �` 1.0)
```

### �L���[�u�̈ʒu��ς���
```cpp
.With<Transform>(
    DirectX::XMFLOAT3{-2.0f, 0.0f, 0.0f},  // X, Y, Z
    //                 ��     ��     ��
    //                ���E   �㉺  �O��
```

### ��]���x��ς���
```cpp
.With<Rotator>(45.0f)  // ���b45�x
//             ��
//          ���̐��l��ς���
```

---

## ?? �w�K���[�h�}�b�v

### ���x��1: ��{�����i1���ԁj
- [ ] README.md ��ǂ�
- [ ] LEARNING_GUIDE.md �̃��b�X��1-3
- [ ] App.h �̃R�[�h�𒭂߂�

### ���x��2: ��𓮂����i2���ԁj
- [ ] �F��ς��Ă݂�
- [ ] �ʒu��ς��Ă݂�
- [ ] ��]���x��ς��Ă݂�
- [ ] �V�����L���[�u��ǉ�

### ���x��3: �T���v�����p�i3���ԁj
- [ ] SampleScenes.h ���g��
- [ ] ComponentSamples.h �̃R���|�[�l���g������
- [ ] �T���v������������

### ���x��4: ����J�n�i5���ԁj
- [ ] ������Behaviour�����
- [ ] �����̃R���|�[�l���g��g�ݍ��킹��
- [ ] �I���W�i���̃Q�[���v�f�����

### ���x��5: ���p�i10���Ԉȏ�j
- [ ] �Փ˔���V�X�e��
- [ ] �p�[�e�B�N���V�X�e��
- [ ] UI�V�X�e��
- [ ] �~�j�Q�[��������������

---

## ?? �g���u���V���[�e�B���O

### Q: �r���h�G���[���o��
**A:** DirectX11 SDK���C���X�g�[������Ă��邩�m�F

### Q: �����\������Ȃ�
**A:** MeshRenderer�R���|�[�l���g���t���Ă��邩�m�F

### Q: TryGet��nullptr��Ԃ�
**A:** ���̃G���e�B�e�B�ɂ��̃R���|�[�l���g���t���Ă��Ȃ�

### Q: �R���|�[�l���g�������Ȃ�
**A:** Behaviour�̏ꍇ�Aworld.Tick(dt)���Ă΂�Ă��邩�m�F

---

## ?? �v���W�F�N�g���v

| ���� | ���l |
|------|------|
| ���R�[�h�s�� | ��3000�s |
| �w�b�_�[�t�@�C���� | 20�� |
| �T���v���R���|�[�l���g�� | 15�� |
| �T���v���V�[���� | 9�� |
| ���b�X���� | 9�� |
| �h�L�������g�s�� | ��2000�s |

---

## ?? �J���[�R�[�h�Q�l

```cpp
// ��{�F
DirectX::XMFLOAT3{1, 0, 0}  // ��
DirectX::XMFLOAT3{0, 1, 0}  // ��
DirectX::XMFLOAT3{0, 0, 1}  // ��
DirectX::XMFLOAT3{1, 1, 0}  // ��
DirectX::XMFLOAT3{1, 0, 1}  // �}�[���^
DirectX::XMFLOAT3{0, 1, 1}  // �V�A��
DirectX::XMFLOAT3{1, 1, 1}  // ��
DirectX::XMFLOAT3{0, 0, 0}  // ��

// �p�X�e���J���[
DirectX::XMFLOAT3{1.0f, 0.7f, 0.7f}  // �s���N
DirectX::XMFLOAT3{0.7f, 1.0f, 0.7f}  // ���C�g�O���[��
DirectX::XMFLOAT3{0.7f, 0.7f, 1.0f}  // ���C�g�u���[
```

---

## ?? �h�L�������g�ꗗ

| �t�@�C�� | ���e |
|---------|------|
| `README.md` | �v���W�F�N�g�S�̃K�C�h |
| `LEARNING_GUIDE.md` | �X�e�b�v�o�C�X�e�b�v�w�K |
| `CHANGELOG_v4.md` | v4.0�̕ύX�_ |
| `CHANGELOG_v3.md` | v3.0�̕ύX�_�i���Łj |
| `QUICK_REFERENCE.md` | ���̃t�@�C�� |

---

## ?? ���ɂ�邱�Ɓi�D��x���j

### �������ł���
1. F5�L�[�Ńr���h�����s
2. App.h �ŃL���[�u�̐F��ς���
3. �V�����L���[�u��1�ǉ�

### �������ɂ��
1. LEARNING_GUIDE.md �̃��b�X��1-3
2. SampleScenes.h ������
3. ������Behaviour��1���

### ���T���ɂ��
1. ComponentSamples.h �̑S�T���v��������
2. �I���W�i���̃R���|�[�l���g��5���
3. �~�j�Q�[���̃v���g�^�C�v�����

---

## ?? �L�[�{�[�h�V���[�g�J�b�g

| �L�[ | ���� |
|------|------|
| **F5** | �r���h�����s |
| **Ctrl + F5** | �f�o�b�O�Ȃ��Ŏ��s |
| **F9** | �u���[�N�|�C���g�ݒ� |
| **F10** | �X�e�b�v�I�[�o�[ |
| **F11** | �X�e�b�v�C�� |
| **ESC** | �A�v���I���i���s���j |
| **WASD** | �J������]�i���s���j |
| **Space** | ����ؑցi���s���j |

---

## ?? �Ō��

���̃v���W�F�N�g�́A**���w�҂����܂����ɃR���|�[�l���g�w�����w�ׂ�**���Ƃ��ŗD��ɍ���Ă��܂��B

### �w�K�̃R�c
- ? �������n�߂�i1�̃L���[�u����j
- ? �����Ɏ����i�R�[�h����������F5�j
- ? ���s������Ȃ��i���Ă����v�j
- ? �T���v�����R�s�y���ĉ���
- ? �킩��Ȃ���΃R�����g��ǂ�

### �T�|�[�g
- �R�[�h���̃R�����g��ǂ�
- README.md ��FAQ������
- LEARNING_GUIDE.md �ŕ��K

---

**�y����Ŋw��ł��������I ??**

---

**�쐬��: �R���z**  
**�o�[�W����: v4.0 - �ǐ�������**  
**�ŏI�X�V: 2024�N**
