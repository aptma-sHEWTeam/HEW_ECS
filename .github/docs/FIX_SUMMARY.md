# ?? �C������ - �v���W�F�N�g������

## ���

Issue ���쐬���ăv���W�F�N�g�ɒǉ�����Ă��A**Role�ATeam�APriority �Ȃǂ̃J�X�^���t�B�[���h����̂܂�**�������B

---

## �������e

### ? �C�����ꂽ�t�@�C��

1. **`.github/workflows/add_to_project.yml`**
   - Issue �e���v���[�g����t�B�[���h���������o
   - GraphQL API �Ńv���W�F�N�g�̃t�B�[���hID�𓮓I�Ɏ擾
   - �e�t�B�[���h�������ݒ�iRole, ProgramTeam, Priority, Component, Size, Estimate, Start/End date�j
   - �ڍׂȃf�o�b�O���O�o��

2. **`.github/ISSUE_TEMPLATE/01_task.yml`**
   - �v���W�F�N�g�̃t�B�[���h���ɍ��킹�čX�V
   - `Team` �� `ProgramTeam`
   - `Component` ���h���b�v�_�E���ɕύX
   - `Size` �� `Estimate` �t�B�[���h��ǉ�

3. **`.github/ISSUE_TEMPLATE/02_bug.yml`**
   - Bug �ɂ��v���W�F�N�g�t�B�[���h��ǉ�

4. **�h�L�������g**
   - `QUICKSTART_PROJECT_AUTOMATION.md` - �����Ɏg����ȈՃK�C�h
   - `.github/docs/PROJECT_AUTOMATION_SETUP.md` - �ڍ׃Z�b�g�A�b�v�K�C�h
   - `.github/scripts/get_project_fields.ps1` - �t�B�[���hID�擾�X�N���v�g

---

## ?? ���̃X�e�b�v

### 1. �R�~�b�g & �v�b�V��

�ύX���R�~�b�g���ăv�b�V�����Ă�������:

```bash
git add .
git commit -m "fix: �v���W�F�N�g�������̃t�B�[���h�����ݒ������"
git push
```

### 2. �e�X�g Issue ���쐬

1. GitHub �ŐV���� Issue ���쐬
2. **Task** �e���v���[�g��I��
3. �ȉ������:
   ```
   �T�v: �e�X�g�p�^�X�N
   Role: Programmer
   ProgramTeam: A
   Priority: High
   Component: Core
   Size: M
   Estimate: 2
   Start date: 2025-01-15
   End date: 2025-01-20
   ```
4. **Submit new issue**

### 3. �m�F

#### A. Actions �^�u�Ŋm�F
1. Repository �� **Actions**
2. "Auto Add Issue to Project" ���[�N�t���[
3. ���O���m�F:
   ```
   === Extracted Fields ===
   Role: Programmer
   Team: A
   Priority: High
   ...
   
   ? Set Role: Programmer
   ? Set ProgramTeam: A
   ? Set Priority: High
   ? Set Component: Core
   ? Set Size: M
   ? Set Estimate: 2
   ? Set Start date: 2025-01-15
   ? Set End date: 2025-01-20
   ? All fields processed
   ```

#### B. �v���W�F�N�g�{�[�h�Ŋm�F
1. Projects �� HEW project
2. �쐬���� Issue ���m�F
3. �ȉ��̃t�B�[���h�������ݒ肳��Ă���͂�:
   - ? Status: (�f�t�H���g�l)
   - ? Role: Programmer
   - ? ProgramTeam: A
   - ? Priority: High
   - ? Component: Core
   - ? Size: M
   - ? Estimate: 2
   - ? Start date: 2025-01-15
   - ? End date: 2025-01-20

---

## ?? �Z�p�I�ȕύX�_

### �]���̕��@�i���s���Ă������R�j

```yaml
# actions/add-to-project@v0.5.0 �� outputs.itemId ���擾�ł��Ă��Ȃ�����
- name: Set Project Fields
  if: steps.add-project.outputs.itemId  # �� ���ꂪ�󂾂���
```

### �V�������@�i�C���Łj

```yaml
# GraphQL API �Œ��ڃv���W�F�N�g�ƃA�C�e��ID���擾
- name: Get Project Data
  script: |
    const item = project.items.nodes.find(
      node => node.content && node.content.number === issue.number
    );
    core.setOutput('item_id', item.id);
```

**���_:**
- ? `itemId` ���m���Ɏ擾
- ? �v���W�F�N�g�̃t�B�[���h���������Ɏ擾
- ? �t�B�[���hID��I�v�V����ID���蓮�ݒ肷��K�v���Ȃ�
- ? �v���W�F�N�g�̍\�����ς���Ă������I�ɒǏ]

---

## ?? �Ή��t�B�[���h�ꗗ

| Issue �t�B�[���h | �v���W�F�N�g�t�B�[���h | �^�C�v | �K�{ |
|----------------|-------------------|--------|-----|
| Role | Role | Single Select | ? |
| ProgramTeam | ProgramTeam | Single Select | - |
| Priority | Priority | Single Select | ? |
| Component | Component | Single Select | - |
| Size | Size | Single Select | - |
| Estimate | Estimate | Number | - |
| Start date | Start date | Date | - |
| End date | End date | Date | - |

---

## ?? ���ӎ���

### �v���W�F�N�g�̃t�B�[���h���Ɗ��S��v���K�v

���[�N�t���[�͈ȉ��̃t�B�[���h����z�肵�Ă��܂�:

```javascript
await setSingleSelectField('Role', process.env.ROLE);
await setSingleSelectField('ProgramTeam', process.env.TEAM);
await setSingleSelectField('Priority', process.env.PRIORITY);
await setSingleSelectField('Component', process.env.COMPONENT);
await setSingleSelectField('Size', process.env.SIZE);
await setDateField('Start date', process.env.START_DATE);
await setDateField('End date', process.env.DUE_DATE);
await setNumberField('Estimate', process.env.ESTIMATE);
```

**�v���W�F�N�g�̃t�B�[���h�����قȂ�ꍇ�́A���[�N�t���[��ҏW���Ă��������B**

### �I�v�V�����l�����S��v���K�v

Issue �e���v���[�g�̃h���b�v�_�E���̒l�ƁA�v���W�F�N�g�̃I�v�V�����l����v���Ă���K�v������܂��B

��:
- Issue: `Programmer` �� ?
- Project: `Programmer` �� ? ��v

�s��v�̏ꍇ:
- Issue: `�v���O���}�[` �� ?
- Project: `Programmer` �� ? �ݒ肳��Ȃ�

**���O�Ŋm�F:**
```
Warning: Option "�v���O���}�[" not found for field "Role"
Available options: ["Programmer", "Designer", "Planner", "QA"]
```

---

## ?? �g���u���V���[�e�B���O

### �t�B�[���h���ݒ肳��Ȃ�

**�`�F�b�N���X�g:**
1. [ ] Actions �Ń��[�N�t���[���������Ă���i�΃`�F�b�N�j
2. [ ] "Debug - Show extracted fields" �Ńt�B�[���h�����o����Ă���
3. [ ] "Set Project Fields" �� `? Set Role:` �Ȃǂ̃��O���o�Ă���
4. [ ] �v���W�F�N�g�̃t�B�[���h������v���Ă���
5. [ ] �I�v�V�����̒l����v���Ă���
6. [ ] �g�[�N�� `ORG_PROJECTS_TOKEN` �� `project` ����������

### ���[�N�t���[�����s����

**�悭����G���[:**

1. **"Could not find issue in project"**
   - Issue ���܂��v���W�F�N�g�ɒǉ�����Ă��Ȃ�
   - `actions/add-to-project` �����s���Ă���\��
   - �g�[�N���̌������m�F

2. **"Field not found in project"**
   - �v���W�F�N�g�ɊY������t�B�[���h�����݂��Ȃ�
   - �t�B�[���h���̃X�y���~�X���m�F

3. **"Option not found for field"**
   - Issue �őI�������l���v���W�F�N�g�̃I�v�V�����ɑ��݂��Ȃ�
   - ���O�� "Available options" ���m�F���ďC��

---

## ?? ����̊g��

### �V�����t�B�[���h��ǉ�����ꍇ

1. **�v���W�F�N�g�Ƀt�B�[���h��ǉ�**
2. **Issue �e���v���[�g�ɒǉ�**
   ```yaml
   - type: dropdown
     id: new_field
     attributes:
       label: NewField
       options: [Option1, Option2, Option3]
   ```
3. **���[�N�t���[�Œ��o**
   ```javascript
   const newFieldMatch = issueBody.match(/### NewField\s*\n+\s*(.+?)(?:\n|$)/i);
   core.setOutput('new_field', newFieldMatch ? newFieldMatch[1].trim() : '');
   ```
4. **���[�N�t���[�Őݒ�**
   ```javascript
   await setSingleSelectField('NewField', process.env.NEW_FIELD);
   ```

---

## ? �����I

���ׂĂ̕ύX���������܂����B�V���� Issue ���쐬���ăe�X�g���Ă��������I

��肪����� `QUICKSTART_PROJECT_AUTOMATION.md` �̃g���u���V���[�e�B���O�Z�N�V�������Q�Ƃ��Ă��������B
