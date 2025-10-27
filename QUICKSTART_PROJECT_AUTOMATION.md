# �v���W�F�N�g������ - �N�C�b�N�X�^�[�g

## ���݂̖��
? Issue ���v���W�F�N�g�ɒǉ������
? **Role, Team, Priority �Ȃǂ̃t�B�[���h����̂܂�**

## �C�����e
? Issue �e���v���[�g��������������o
? �v���W�F�N�g�̃J�X�^���t�B�[���h�Ɏ����ݒ�
? **�t�B�[���hID�������擾**�i�蓮�ݒ�s�v�j
? �蓮���͕s�v

---

## ?? �����Ɏ��s����菇

### ? �X�e�b�v1: ���[�N�t���[�t�@�C���̍X�V�m�F

�ŐV�� `.github/workflows/add_to_project.yml` �͈ȉ��̋@�\���܂�ł��܂��F

- ? Issue ����t�B�[���h���������o
- ? �v���W�F�N�g�̃t�B�[���hID�𓮓I�Ɏ擾
- ? �f�o�b�O���O�o��
- ? �G���[�n���h�����O

**�ǉ��̐ݒ�͕s�v�ł��I** �����Ƀe�X�g�ł��܂��B

### ? �X�e�b�v2: �e�X�g

1. �V���� Task Issue ���쐬
2. **�K���ȉ��̃t�B�[���h�����**:
   - Role: `Programmer` / `Designer` / `Planner` / `QA` ����I��
   - ProgramTeam: `A` / `B` / `C` ����I��
   - Priority: `High` / `Medium` / `Low` ����I��
   - Start date: `2025-01-15` �`���œ���
   - End date: `2025-01-20` �`���œ���
3. Submit
4. **Actions �^�u�Ŋm�F**:
   - "Auto Add Issue to Project" ���[�N�t���[�̎��s���O���m�F
   - `? Set Role: Programmer` �Ȃǂ̃��O���\�������͂�
5. **�v���W�F�N�g�{�[�h�Ŋm�F**: �t�B�[���h�������I�ɐݒ肳���I

---

## ?? �Ή��t�B�[���h�i�����}�b�s���O�j

| Issue �e���v���[�g | �v���W�F�N�g�t�B�[���h | �^�C�v |
|------------------|-------------------|--------|
| Role (dropdown) | Role | Single Select |
| Team (dropdown) | ProgramTeam | Single Select |
| Priority (dropdown) | Priority | Single Select |
| Component (dropdown) | Component | Single Select |
| Size (dropdown) | Size | Single Select |
| Start date (input) | Start date | Date |
| End date (input) | End date | Date |
| Estimate (input) | Estimate | Number |

**����**: �v���W�F�N�g�̃t�B�[���h���Ɗ��S�Ɉ�v����K�v������܂��B

---

## ?? �g���u���V���[�e�B���O

### 1. �t�B�[���h����̂܂�

**�m�F����:**

1. **Actions �^�u�Ń��O���m�F**
   ```
   Repository �� Actions �� "Auto Add Issue to Project" �� �ŐV�̎��s
   ```

2. **"Debug - Show extracted fields" �X�e�b�v���m�F**
   ```
   Role: Programmer
   Team: A
   Priority: High
   ```
   
   �l�����o����Ă��Ȃ��ꍇ:
   - Issue �e���v���[�g�ŁuChoose an option�v�̂܂� Submit ���Ă��Ȃ����m�F
   - �K�{�t�B�[���h�iRole, Priority�j��K������

3. **"Set Project Fields" �X�e�b�v���m�F**
   ```
   ? Set Role: Programmer
   ? Set ProgramTeam: A
   ? Set Priority: High
   ```
   
   �G���[���o�Ă���ꍇ:
   - �v���W�F�N�g�̃t�B�[���h������v���Ă��邩�m�F
   - �g�[�N���̌������m�F�i���L�Q�Ɓj

### 2. "Warning: Field not found in project"

�v���W�F�N�g�̃t�B�[���h���� Issue �e���v���[�g����v���Ă��܂���B

**�C�����@:**
1. �v���W�F�N�g�̃t�B�[���h�����m�F
   - GitHub Projects �� Settings �� Fields
2. `.github/workflows/add_to_project.yml` �� `setSingleSelectField()` �Ăяo�����C��
   ```javascript
   // ��: "ProgramTeam" �� "Team" �ɕύX
   await setSingleSelectField('Team', process.env.TEAM);
   ```

### 3. "Warning: Option not found for field"

Issue �őI�������l���v���W�F�N�g�̃I�v�V�����ɑ��݂��܂���B

**�C�����@:**
1. ���O�ŗ��p�\�ȃI�v�V�������m�F
   ```
   Available options: ["Programmer", "Designer", "Planner", "QA"]
   ```
2. Issue �e���v���[�g�̃I�v�V��������v������
   ```yaml
   # .github/ISSUE_TEMPLATE/01_task.yml
   - type: dropdown
     id: role
     attributes:
       label: Role
       options: [Programmer, Designer, Planner, QA]  # ���S��v������
   ```

### 4. �g�[�N�������G���[

**�K�v�Ȍ���:**
- ? `project` (read/write)
- ? `repo` (read)
- ? `org:read` (�g�D�v���W�F�N�g�̏ꍇ)

**�g�[�N���̍Đ���:**
1. GitHub �� Settings �� Developer settings �� Personal access tokens �� Tokens (classic)
2. "Generate new token (classic)"
3. ��L�̌�����I��
4. Repository Settings �� Secrets �� `ORG_PROJECTS_TOKEN` ���X�V

### 5. ���t�����f����Ȃ�

- �`���� `YYYY-MM-DD` �ɂȂ��Ă��邩�m�F�i��: `2025-01-15`�j
- `2025/01/15` �� `15-01-2025` �͕s��
- "No date" ��󗓂̏ꍇ�͐ݒ肳��܂���

---

## ?? �f�o�b�O���[�h

�ڍׂȃ��O���m�F������@:

1. Actions �^�u �� �Y�����[�N�t���[ �� "Set Project Fields" �X�e�b�v
2. �ȉ��̂悤�ȃ��O���\������܂�:
   ```
   Setting fields for item: PVTI_lAHO...
   ? Set Role: Programmer
   ? Set ProgramTeam: A
   ? Set Priority: High
   ? Set Start date: 2025-01-15
   Skipping End date: no value
   ? All fields processed
   ```

**�G���[��:**
```
? Failed to set Role: Field value validation failed
```
�� �I�������l���v���W�F�N�g�̃I�v�V�����ɑ��݂��Ȃ�

---

## ?? ����m�F�`�F�b�N���X�g

- [ ] Issue ���쐬
- [ ] Actions �Ń��[�N�t���[�������i�΃`�F�b�N�j
- [ ] ���O�� "? Set Role:" �Ȃǂ��\�������
- [ ] �v���W�F�N�g�{�[�h�Ńt�B�[���h���ݒ肳��Ă���
- [ ] Start date / End date �����f����Ă���

���ׂ� ? �Ȃ琬���ł��I

---

## ?? �悭���鎿��

### Q: ������ Issue �ɂ͔��f����܂����H
**A:** �������B���[�N�t���[�� `on: issues: types: [opened]` �Ńg���K�[����邽�߁A�V�����쐬���ꂽ Issue �݂̂��Ώۂł��B

������ Issue �ɓK�p����ꍇ:
1. Issue ����Ă���ēx�J���i`reopened` �g���K�[��ǉ�����K�v������܂��j
2. �܂��͎蓮�Ńv���W�F�N�g�̃t�B�[���h��ݒ�

### Q: �t�B�[���h����ύX�ł��܂����H
**A:** �͂��B`.github/workflows/add_to_project.yml` �̊Y���ӏ���ҏW���Ă��������B

��: "ProgramTeam" �� "�J���`�[��"
```javascript
await setSingleSelectField('�J���`�[��', process.env.TEAM);
```

### Q: �V�����t�B�[���h��ǉ��ł��܂����H
**A:** �͂��B�ȉ��̎菇�Œǉ��ł��܂��F

1. Issue �e���v���[�g�Ƀt�B�[���h��ǉ�
2. ���[�N�t���[�� `Extract Issue Fields` �X�e�b�v�Œ��o���W�b�N��ǉ�
3. `Set Project Fields` �X�e�b�v�Őݒ胍�W�b�N��ǉ�

�ڍׂ� `.github/docs/PROJECT_AUTOMATION_SETUP.md` ���Q�ƁB

---

## ?? ���̑��̃h�L�������g

�ڍׂȐ���: [PROJECT_AUTOMATION_SETUP.md](.github/docs/PROJECT_AUTOMATION_SETUP.md)

**��肪�������Ȃ��ꍇ�́AActions �̃��O�S�̂��R�s�[���� Issue ���쐬���Ă��������I**
