# ?? ECS�x�[�X�̃~�j�Q�[�� - �V���[�e�B���O�Q�[��

## ?? �T�v

���̃v���W�F�N�g�́A**Entity Component System (ECS)** ���g�����w�K�p�̃V���v���ȃV���[�e�B���O�Q�[���ł��B

- **�ǐ��ŏd��** - ���S�҂ł��R�[�h���ǂ߂�
- **�V�[���}�l�[�W���[����** - �Q�[����ʂ̐؂�ւ��Ǘ�
- **�V���v���ȃQ�[�����W�b�N** - �������₷������

---

## ?? �Q�[�����e

### ���[��
- **�v���C���[�i�΂̃L���[�u�j** �𑀍삵�ēG��|��
- **�G�i�Ԃ̃L���[�u�j** ���ォ��~���Ă���
- **�e�i���F�̃L���[�u�j** �𔭎˂��ēG������
- �G��|���� **�X�R�A +10�_**

### ������@
| �L�[ | ���� |
|------|------|
| **A** | ���Ɉړ� |
| **D** | �E�Ɉړ� |
| **�X�y�[�X** | �e�𔭎� |
| **ESC** | �Q�[���I�� |

---

## ?? ���s���@

### �r���h�Ǝ��s
1. Visual Studio�Ńv���W�F�N�g���J��
2. `F5`�L�[�Ńr���h�����s
3. �Q�[�����y���ށI

### �����
- **OS**: Windows 10/11
- **DirectX**: DirectX 11�Ή�GPU
- **�J����**: Visual Studio 2019�ȍ~
- **C++�W��**: C++14

---

## ?? �t�@�C���\��

### ?? �Q�[���֘A
```
MiniGame.h         - �V���[�e�B���O�Q�[���̎���
SceneManager.h     - �V�[���؂�ւ��V�X�e��
App.h              - �A�v���P�[�V�����{��
```

### ??? ECS�V�X�e��
```
World.h            - �G���e�B�e�B�ƃR���|�[�l���g�̊Ǘ�
Entity.h           - �G���e�B�e�B�̒�`
Component.h        - �R���|�[�l���g�̊��N���X
```

### ?? �O���t�B�b�N�X
```
GfxDevice.h        - DirectX11�f�o�C�X�Ǘ�
RenderSystem.h     - �����_�����O�V�X�e��
Camera.h           - �J��������
DebugDraw.h        - �f�o�b�O�`��
```

### ?? �R���|�[�l���g
```
Transform.h        - �ʒu�E��]�E�X�P�[��
MeshRenderer.h     - ���b�V���`��
```

### ?? ����
```
InputSystem.h      - �L�[�{�[�h�E�}�E�X����
```

---

## ?? �R�[�h�̓ǂݕ�

### 1. �G���g���[�|�C���g
**`main.cpp`** ����n�܂�܂��B
```cpp
int WINAPI WinMain(...) {
    App app;
    app.Init(hInstance);
    app.Run();  // �Q�[�����[�v�J�n
}
```

### 2. �A�v���P�[�V�����{��
**`App.h`** ���Q�[���S�̂��Ǘ����܂��B
```cpp
// ������
bool Init() {
    // �E�B���h�E�ADirectX�A�V�[����������
}

// ���C�����[�v
void Run() {
    while (�Q�[����) {
        ���͏���();
        �V�[���X�V();
        �`��();
    }
}
```

### 3. �Q�[�����W�b�N
**`MiniGame.h`** �ɃQ�[���̃��[��������܂��B
```cpp
class GameScene : public IScene {
    void OnUpdate() {
        �v���C���[�ړ�();
        �e�̔���();
        �G�̐���();
        �Փ˔���();
    }
}
```

### 4. �R���|�[�l���g
**`MiniGame.h`** ���Œ�`����Ă��܂��B
```cpp
// �v���C���[�̈ړ�
struct PlayerMovement : Behaviour {
    void OnUpdate(World& w, Entity self, float dt) {
        // �ړ�����
    }
};

// �e�̈ړ�
struct BulletMovement : Behaviour {
    void OnUpdate(World& w, Entity self, float dt) {
        // ��ɐi��
    }
};
```

---

## ??? �J�X�^�}�C�Y���@

### �Q�[���o�����X��ς���

#### �G�̏o�����x��ς���
**`MiniGame.h`** �� `UpdateEnemySpawning()` ��:
```cpp
// 1�b���ƂɓG�𐶐� �� 0.5�b�ɕύX
if (enemySpawnTimer_ >= 0.5f) {  // 1.0f ����ύX
```

#### �v���C���[�̈ړ����x��ς���
**`MiniGame.h`** �� `PlayerMovement`:
```cpp
float speed = 8.0f;  // ���̒l��ς���
```

#### �e�̑��x��ς���
**`MiniGame.h`** �� `BulletMovement`:
```cpp
float speed = 15.0f;  // ���̒l��ς���
```

#### �e�̘A�ˑ��x��ς���
**`MiniGame.h`** �� `UpdateShooting()` ��:
```cpp
shootCooldown_ = 0.2f;  // �N�[���_�E�����ԁi�b�j
```

### �V�����@�\��ǉ�����

#### ��: �̗̓V�X�e����ǉ�
```cpp
// 1. �R���|�[�l���g���`
struct Health : IComponent {
    int hp = 3;
};

// 2. �v���C���[�ɒǉ�
playerEntity_ = world.Create()
    .With<Transform>(...)
    .With<MeshRenderer>(...)
    .With<Player>()
    .With<Health>()  // �� �ǉ�
    .Build();

// 3. �Փˎ��Ƀ_���[�W����
auto* health = world.TryGet<Health>(playerEntity_);
if (health) {
    health->hp -= 1;
    if (health->hp <= 0) {
        // �Q�[���I�[�o�[����
    }
}
```

---

## ?? �w�K���\�[�X

### ECS�ɂ��Ċw��
- **Entity**: �Q�[���I�u�W�F�N�g��ID
- **Component**: �f�[�^�̉�i�ʒu�A�����ڂȂǁj
- **System**: �R���|�[�l���g���������郍�W�b�N

### ���̃v���W�F�N�g��ECS
```
Entity (�G���e�B�e�B)
  ���� Component (�R���|�[�l���g)
       ���� Transform (�ʒu�E��]�E�X�P�[��)
       ���� MeshRenderer (������)
       ���� Behaviour (����)
       ��   ���� PlayerMovement
       ��   ���� BulletMovement
       ��   ���� EnemyMovement
       ���� Tag (�ڈ�)
           ���� Player
           ���� Enemy
           ���� Bullet
```

---

## ?? �f�o�b�O���@

### �f�o�b�O�`��
`_DEBUG`���[�h�ł́A�O���b�h�ƍ��W�����\������܂��B

### �u���[�N�|�C���g
Visual Studio�ňȉ��̏ꏊ�Ƀu���[�N�|�C���g��ݒ肷��ƕ֗�:
- `GameScene::OnUpdate()` - ���t���[���̏���
- `CheckCollisions()` - �Փ˔���
- `UpdateShooting()` - �e�̔���

---

## ?? �g���u���V���[�e�B���O

### �r���h�G���[���o��
- DirectX SDK���C���X�g�[������Ă��邩�m�F
- Windows SDK 10���K�v

### �Q�[�����N�����Ȃ�
- DirectX 11�Ή���GPU���K�v
- �O���t�B�b�N�X�h���C�o�[���ŐV�ɍX�V

### ���삪�d��
- `_DEBUG`���[�h�ł͂Ȃ�`Release`���[�h�Ńr���h

---

## ?? ���C�Z���X

���̃v���W�F�N�g�͊w�K�p�ł��B���R�ɉ��ρE�Ĕz�z�ł��܂��B

---

## ?? �쐬��

**�R���z**  
�o�[�W����: v5.0 - �~�j�Q�[�������Łi�ǐ��ŏd���j

---

## ?? ���̃X�e�b�v

���̃v���W�F�N�g�𗝉�������A���ɒ��킵�Ă݂悤�F

1. **�p���[�A�b�v�A�C�e���̒ǉ�**
2. **�����̓G�^�C�v**
3. **�^�C�g����ʂƃQ�[���I�[�o�[��ʂ̎���**
4. **�n�C�X�R�A�̕ۑ�**
5. **���ʉ���BGM�̒ǉ�**

�撣���Ă��������I ??
