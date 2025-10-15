# �w�b�_�[�C���N���[�h�ȗ����K�C�h

## ���@1: PCH�i�v���R���p�C���ς݃w�b�_�[�j�y�����z

### ���_
- ? **�r���h������**: ��x�R���p�C�������w�b�_�[���ė��p
- ? **�R�[�h���Ȍ�**: 1�s�őS���C���N���[�h
- ? **�v���W�F�N�g�S�̂œ���**: ���ׂẴt�@�C���œ�����

### �ݒ���@
`docs/PCH_SETUP.md` ���Q��

### �g����
```cpp
#include "pch.h"

// ���ꂾ���ňȉ������ׂĎg����:
// - Entity, World
// - Transform, MeshRenderer, Rotator
// - InputSystem, SceneManager
// - ComponentSamples
// - Animation
```

## ���@2: AllComponents.h�yPCH��ցz

### ���_
- ? **�ݒ�s�v**: �����g����
- ? **�t�@�C���P�ʂőI����**: �K�v�ȃt�@�C�������Ŏg����

### �g����
```cpp
#include "AllComponents.h"

// PCH�Ɠ����悤�Ɏg����
Entity e = world.Create()
    .With<Transform>()
    .With<MeshRenderer>()
    .Build();
```

## �g������

| �� | �������� |
|------|----------|
| Visual Studio �Ńv���W�F�N�g�J�� | **PCH** |
| �P��t�@�C�������Ŏ������� | **AllComponents.h** |
| ���̃R���p�C�����g�� | **AllComponents.h** |
| �r���h���x���ŗD�� | **PCH** |

## �]���̕��@�i�ʃC���N���[�h�j

�������A�ʂɃC���N���[�h���邱�Ƃ��ł��܂��F

```cpp
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "ecs/World.h"
// �K�v�Ȃ��̂���
```

### ���g���H
- �ˑ��֌W�𖾊m�ɂ������ꍇ
- �w�b�_�[�t�@�C���i.h�j���ōŏ����̃C���N���[�h
- ���C�u�����Ƃ��Č��J����ꍇ

## �V�����R���|�[�l���g�ǉ���

### PCH�g�p��
1. �R���|�[�l���g��`�i��: `MyComponent.h`�j
2. `pch.h` �ɒǉ��i�I�v�V�����A�悭�g���ꍇ�̂݁j
3. `#include "pch.h"` �Ŏg����

### AllComponents.h�g�p��
1. �R���|�[�l���g��`�i��: `MyComponent.h`�j
2. `AllComponents.h` �ɒǉ�
3. `#include "AllComponents.h"` �Ŏg����

### �ʃC���N���[�h��
1. �R���|�[�l���g��`�i��: `MyComponent.h`�j
2. �g�������t�@�C���� `#include "MyComponent.h"`

## �T���v����r

### Before�i�ώG�j
```cpp
#include "scenes/SceneManager.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/Rotator.h"
#include "components/Component.h"
#include "ecs/World.h"
#include "ecs/Entity.h"
#include <cstdlib>
#include <ctime>
#include <vector>

struct MyComponent : IComponent {
    // ...
};
```

### After�i�Ȍ��j
```cpp
#include "pch.h"  // �܂��� AllComponents.h

struct MyComponent : IComponent {
    // ...
};
```

## �p�t�H�[�}���X��r

| ���@ | ����r���h | 2��ڈȍ~ | �R�[�h�� |
|------|-----------|----------|---------|
| �ʃC���N���[�h | �x�� | �x�� | ���� |
| AllComponents.h | �x�� | �x�� | ���Ȃ� |
| **PCH** | **���x��** | **����** | **�ŏ�** |

## �g���u���V���[�e�B���O

### Q: PCH�ݒ��A�r���h�G���[���o��
A: ���ׂĂ�.cpp�t�@�C���̐擪�� `#include "pch.h"` ��ǉ����Ă�������

### Q: ����̃t�@�C������PCH���g�������Ȃ�
A: ���̃t�@�C���̃v���p�e�B��PCH���u�g�p���Ȃ��v�ɐݒ�

### Q: pch.h ��ύX������S�t�@�C���ăr���h�����
A: ����ȓ���ł��Bpch.h �͕p�ɂɕύX���Ȃ��悤�ɂ��܂��傤
