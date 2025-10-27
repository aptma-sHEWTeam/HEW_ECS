# �v���W�F�N�g������ - �N�C�b�N�X�^�[�g

## ���݂̖��
? Issue ���v���W�F�N�g�ɒǉ������
? **Role, Team, Priority �Ȃǂ̃t�B�[���h����̂܂�**

## �C�����e
? Issue �e���v���[�g��������������o
? �v���W�F�N�g�̃J�X�^���t�B�[���h�Ɏ����ݒ�
? �蓮���͕s�v

---

## ?? �����Ɏ��s����菇

### 1?? �t�B�[���hID���擾�i1��̂݁j

```powershell
# PowerShell �Ŏ��s
.\.github\scripts\get_project_fields.ps1 -Token "YOUR_TOKEN_HERE"
```

**�g�[�N���̎擾���@:**
1. GitHub �� Settings �� Developer settings �� Personal access tokens �� Tokens (classic)
2. "Generate new token (classic)" ���N���b�N
3. ������I��: `repo`, `project`
4. �g�[�N�����R�s�[

### 2?? ���[�N�t���[���X�V

�o�͂��ꂽ���� `.github/workflows/add_to_project.yml` �ɃR�s�[���y�[�X�g�F

```javascript
// ���̕������X�V�i61�s�ڂ�����j
const projectId = 'PVT_kwHOAKhcas4AvQ1B'; // �� ���ۂ�Project ID�ɒu������

const fieldIds = {
  role: 'PVTSSF_lAHO...', // �� ���ۂ�Field ID�ɒu������
  team: 'PVTSSF_lAHO...',
  // ...
};

const fieldValueIds = {
  role: {
    'Planner': 'xxxxx',  // �� ���ۂ�Option ID�ɒu������
    'Programmer': 'yyyy',
    // ...
  },
  // ...
};
```

### 3?? �e�X�g

1. �V���� Task Issue ���쐬
2. Role, Team, Priority ��I��
3. Submit
4. �v���W�F�N�g�{�[�h�Ŋm�F �� **�����I�Ƀt�B�[���h���ݒ肳���I**

---

## ?? �Ή��t�B�[���h

| Issue �e���v���[�g | �v���W�F�N�g�t�B�[���h |
|------------------|-------------------|
| Role (dropdown) | Role |
| Team (dropdown) | Team |
| Priority (dropdown) | Priority |
| Start date (input) | Start date |
| End date (input) | End date |
| Component (input) | Component (�C��) |

---

## ?? �g���u���V���[�e�B���O

### �G���[: "Field not updated"
�� `.github/workflows/add_to_project.yml` �� ID ���Ԉ���Ă���
�� �ēx `get_project_fields.ps1` �����s���čŐV��ID���擾

### �t�B�[���h����̂܂�
�� Actions �^�u�Ń��[�N�t���[���O���m�F
�� �g�[�N���̌������m�F�i`project` �������K�v�j

### ���t�����f����Ȃ�
�� YYYY-MM-DD �`���œ��͂���Ă��邩�m�F�i��: 2025-11-15�j

---

## ?? ���̑��̃h�L�������g

�ڍׂȐ���: [PROJECT_AUTOMATION_SETUP.md](.github/docs/PROJECT_AUTOMATION_SETUP.md)
