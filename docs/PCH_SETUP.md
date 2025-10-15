# PCH�i�v���R���p�C���ς݃w�b�_�[�j�ݒ�K�C�h

## Visual Studio �ł̐ݒ�菇

### 1. pch.cpp �̐ݒ�
1. �\�����[�V�����G�N�X�v���[���[�� `src/pch.cpp` ���E�N���b�N
2. **�v���p�e�B** ��I��
3. **C/C++** �� **�v���R���p�C���ς݃w�b�_�[** ���J��
4. **�v���R���p�C���ς݃w�b�_�[** �� **�쐬 (/Yc)** �ɐݒ�
5. **�v���R���p�C���ς݃w�b�_�[ �t�@�C��** �� **pch.h** �ɐݒ�
6. **OK** ���N���b�N

### 2. �v���W�F�N�g�S�̂̐ݒ�
1. �v���W�F�N�g���E�N���b�N �� **�v���p�e�B**
2. **C/C++** �� **�v���R���p�C���ς݃w�b�_�[** ���J��
3. **�v���R���p�C���ς݃w�b�_�[** �� **�g�p (/Yu)** �ɐݒ�
4. **�v���R���p�C���ς݃w�b�_�[ �t�@�C��** �� **pch.h** �ɐݒ�
5. **OK** ���N���b�N

### 3. �����t�@�C���̍X�V�i�C�Ӂj
������ .cpp �t�@�C���̐擪�Ɉȉ���ǉ��F
```cpp
#include "pch.h"
```

���ꂾ���ŁA�ȉ��̃w�b�_�[�������I�ɃC���N���[�h����܂��F
- ���ׂĂ�ECS�R�A�iEntity, World�j
- ��{�R���|�[�l���g�iTransform, MeshRenderer, Rotator�j
- ���ׂẴV�X�e���iInput, Graphics, Scene�j
- �T���v���R���|�[�l���g

## �g�p��

### Before�i�]���j
```cpp
#include "scenes/SceneManager.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/Rotator.h"
#include "components/Component.h"
#include <cstdlib>
#include <ctime>
#include <vector>

// �R���|�[�l���g��`...
```

### After�iPCH�g�p�j
```cpp
#include "pch.h"

// �R���|�[�l���g��`...
```

## �����b�g

1. **�R�[�h���Ȍ���**: 1�s�̃C���N���[�h�ŕK�v�Ȃ��̂����ׂĎg����
2. **�r���h������**: �p�ɂɎg���w�b�_�[�����O�R���p�C�������
3. **�����e�i���X������**: �w�b�_�[�̊Ǘ�����ӏ��ɏW��

## �g���u���V���[�e�B���O

### �G���[: "�v���R���p�C���ς݃w�b�_�[��������܂���"
�� `pch.cpp` ���u�쐬 (/Yc)�v�ɐݒ肳��Ă��邩�m�F

### �G���[: "pch.h ��������܂���"
�� �C���N���[�h�f�B���N�g���� `include` ���ǉ�����Ă��邩�m�F

### �r���h���x��
�� PCH�t�@�C���ɕs�v�ȃw�b�_�[���܂܂�Ă��Ȃ����m�F
�� �p�ɂɕύX����w�b�_�[��PCH�Ɋ܂߂Ȃ�

## ���ӎ���

- **pch.h �͕p�ɂɕύX���Ȃ�**: �ύX����ƑS�t�@�C�����ăR���p�C�������
- **�傫�ȃw�b�_�[�̂ݒǉ�**: �����ȃw�b�_�[�͒��ڃC���N���[�h���������ǂ�
- **�v���W�F�N�g�ŗL�̃w�b�_�[�͐T�d��**: �悭�ύX������͔̂�����
