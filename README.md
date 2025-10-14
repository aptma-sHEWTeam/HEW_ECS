# Simple ECS + DirectX11 Example

## �t�@�C���\��

���̃v���W�F�N�g�́A�ǐ����ŏd�����čׂ����t�@�C����������Ă��܂��B

### ?? �f�B���N�g���\��

```
ECS_BACE/
������ main.cpp              # �G���g���[�|�C���g
������ ECS/                  # ECS�R�A�V�X�e��
��   ������ Entity.h          # �G���e�B�e�BID��`
��   ������ Component.h       # �R���|�[�l���g���N���X
��   ������ World.h           # ECS���[���h�Ǘ�
������ Components/           # �Q�[���R���|�[�l���g
��   ������ Transform.h       # �ʒu�E��]�E�X�P�[��
��   ������ MeshRenderer.h    # ���b�V�������_�����O
��   ������ Rotator.h         # ������]Behaviour
������ Graphics/             # �O���t�B�b�N�X�V�X�e��
��   ������ GfxDevice.h       # DirectX11�f�o�C�X�Ǘ�
��   ������ Camera.h          # �J�����i�r���[�E�v���W�F�N�V�����j
��   ������ RenderSystem.h    # �����_�����O�p�C�v���C��
������ Application/          # �A�v���P�[�V�����w
    ������ App.h             # �A�v���P�[�V�������C���N���X
```

### ?? �e�t�@�C���̖���

#### **ECS�R�A�V�X�e��**
- **Entity.h** - �G���e�B�e�B��ID�\���̂��`
- **Component.h** - `IComponent`���C���^�[�t�F�[�X��`Behaviour`���N���X
- **World.h** - ECS���[���h�Ǘ��i�G���e�B�e�B�쐬�A�R���|�[�l���g�ǉ�/�폜/�擾�ABehaviour�X�V�j

#### **�R���|�[�l���g**
- **Transform.h** - �ʒu�A��]�A�X�P�[����ێ�
- **MeshRenderer.h** - ���b�V�������_�����O�p�̐F���
- **Rotator.h** - ������]���s��Behaviour�iOnUpdate�����j

#### **�O���t�B�b�N�X**
- **GfxDevice.h** - DirectX11�f�o�C�X�A�X���b�v�`�F�[���A�����_�[�^�[�Q�b�g�Ǘ�
- **Camera.h** - �J�����̃r���[�E�v���W�F�N�V�����s��
- **RenderSystem.h** - �V�F�[�_�[�A�p�C�v���C���A�W�I���g���A�`�惋�[�v

#### **�A�v���P�[�V����**
- **App.h** - �E�B���h�E�쐬�A�������A���C�����[�v
- **main.cpp** - WinMain�G���g���[�|�C���g

### ?? �Z�p�d�l

- **����**: C++14�݊�
- **API**: DirectX 11
- **�r���h�c�[��**: Visual Studio 2022
- **�A�[�L�e�N�`��**: Entity Component System (ECS)

### ?? �݌v���j

1. **�P��ӔC�̌���** - �e�t�@�C����1�̖��m�Ȗ���������
2. **�ˑ��֌W�̖��m��** - �C���N���[�h�֌W���ǂ��₷���\��
3. **�J�e�S������** - �@�\���ƂɃt�H���_�Ő���
4. **�R�����g�̏[��** - �e�Z�N�V�����ɓ��{��R�����g�t��

### ?? �r���h���@

1. Visual Studio 2022�Ń\�����[�V�������J��
2. �r���h�\����I���iDebug/Release, x64�����j
3. �r���h���s

### ?? �g�����@

�V�����R���|�[�l���g��ǉ�����ꍇ�F
1. `Components/`�ɐV����`.h`�t�@�C�����쐬
2. `IComponent`�܂���`Behaviour`���p��
3. �K�v�ɉ�����`World::Add<>()`�Œǉ�

�V�����V�X�e����ǉ�����ꍇ�F
1. �K�؂ȃt�H���_��`.h`�t�@�C�����쐬
2. `App.h`����Q�Ƃ��ď�����


���̗L�@�i���@���̎p�j
