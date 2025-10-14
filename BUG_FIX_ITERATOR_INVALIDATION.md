# Iterator Invalidation Bug Fix

## ���̊T�v (Problem Summary)

Debug Assertion Failed �G���[���������Ă��܂����F
```
File: C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\include\list
Line: 160
Expression: cannot increment value-initialized list iterator
```

## ���{���� (Root Cause)

`MiniGame.h` �� `CheckCollisions()` ���\�b�h�ŁA**�C�e���[�^�������iIterator Invalidation�j**���������Ă��܂����B

### ���̂������R�[�h�F
```cpp
void CheckCollisions(World& world) {
    world.ForEach<Bullet>([&](Entity bulletEntity, Bullet& bullet) {
        world.ForEach<Enemy>([&](Entity enemyEntity, Enemy& enemy) {
            if (distance < 1.0f) {
                world.DestroyEntity(bulletEntity);  // �� �����ŃC�e���[�^���������I
                world.DestroyEntity(enemyEntity);
                score_ += 10;
            }
        });
    });
}
```

### �Ȃ��G���[���N�������F

1. `ForEach<Bullet>` �� `std::unordered_map` ���C�e���[�g��
2. `DestroyEntity(bulletEntity)` ���Ă΂��
3. ����ɂ�� `Store<Bullet>::map` ����v�f���폜�����
4. **�C�e���[�^�������������**
5. �O���̃��[�v�����̗v�f�ɐi�����Ƃ��� **�N���b�V��**

## �C�����e (Fix Applied)

### 1. MiniGame.h �� CheckCollisions() ���C��

**�C���O�F**
```cpp
void CheckCollisions(World& world) {
    world.ForEach<Bullet>([&](Entity bulletEntity, Bullet& bullet) {
        world.ForEach<Enemy>([&](Entity enemyEntity, Enemy& enemy) {
            if (distance < 1.0f) {
                world.DestroyEntity(bulletEntity);
                world.DestroyEntity(enemyEntity);
                score_ += 10;
            }
        });
    });
}
```

**�C����F**
```cpp
void CheckCollisions(World& world) {
    // �폜����G���e�B�e�B�����W�i�C�e���[�^��������h���j
    std::vector<Entity> entitiesToDestroy;
    
    world.ForEach<Bullet>([&](Entity bulletEntity, Bullet& bullet) {
        // ���̒e�����łɍ폜�\��Ȃ珈�����X�L�b�v
        for (const auto& e : entitiesToDestroy) {
            if (e.id == bulletEntity.id) return;
        }
        
        world.ForEach<Enemy>([&](Entity enemyEntity, Enemy& enemy) {
            // ���̓G�����łɍ폜�\��Ȃ珈�����X�L�b�v
            for (const auto& e : entitiesToDestroy) {
                if (e.id == enemyEntity.id) return;
            }
            
            if (distance < 1.0f) {
                entitiesToDestroy.push_back(bulletEntity);
                entitiesToDestroy.push_back(enemyEntity);
                score_ += 10;
            }
        });
    });
    
    // �C�e���[�V����������Ɉꊇ�폜
    for (const auto& entity : entitiesToDestroy) {
        world.DestroyEntity(entity);
    }
}
```

### 2. MiniGame.h �� OnExit() ���C��

���l�̖�肪���������ߏC���F

```cpp
void OnExit(World& world) override {
    // �S�G���e�B�e�B���폜�i�C�e���[�^��������h���j
    std::vector<Entity> entitiesToDestroy;
    
    world.ForEach<Transform>([&](Entity e, Transform& t) {
        entitiesToDestroy.push_back(e);
    });
    
    for (const auto& entity : entitiesToDestroy) {
        world.DestroyEntity(entity);
    }
}
```

### 3. World.h �� Tick() �����P

�����S�ȃC���f�b�N�X�x�[�X�̃��[�v�ɕύX�F

**�C���O�F**
```cpp
void Tick(float dt) {
    for (auto& it : behaviours_) {
        if (!IsAlive(it.e)) continue;
        it.b->OnUpdate(*this, it.e, dt);
    }
}
```

**�C����F**
```cpp
void Tick(float dt) {
    // �C�e���[�V�������̍폜�ɑΉ����邽�߁A�C���f�b�N�X�x�[�X�̃��[�v���g�p
    for (size_t i = 0; i < behaviours_.size(); ++i) {
        auto& it = behaviours_[i];
        if (!IsAlive(it.e)) continue;
        if (!it.started) { it.b->OnStart(*this, it.e); it.started = true; }
        it.b->OnUpdate(*this, it.e, dt);
    }
}
```

## �w�񂾋��P (Lessons Learned)

### �C�e���[�^��������h���x�X�g�v���N�e�B�X�F

1. **�C�e���[�g���ɃR���e�i��ύX���Ȃ�**
   - `std::vector::erase()`, `std::unordered_map::erase()` �Ȃ�
   
2. **�폜����v�f���Ɏ��W����**
   - `std::vector<Entity> toDelete;` ���g��
   - �C�e���[�V����������ɍ폜
   
3. **�C���f�b�N�X�x�[�X�̃��[�v���g��**
   - Range-based for �������S�ȏꍇ������
   - �T�C�Y���ς���Ă��Ή����₷��

4. **�f�o�b�O�r���h�Ńe�X�g����**
   - �C�e���[�^�`�F�b�N���L��
   - ���𑁊������ł���

## �Q�l�F�悭����C�e���[�^�������̃p�^�[��

```cpp
// ? �_���ȗ�
for (auto& item : container) {
    if (shouldDelete) {
        container.erase(item);  // �C�e���[�^�������������I
    }
}

// ? �ǂ��� 1: �폜���X�g�g�p
std::vector<Key> toDelete;
for (auto& item : container) {
    if (shouldDelete) {
        toDelete.push_back(item.key);
    }
}
for (auto& key : toDelete) {
    container.erase(key);
}

// ? �ǂ��� 2: erase-remove �C�f�B�I���ivector �̏ꍇ�j
container.erase(
    std::remove_if(container.begin(), container.end(),
        [](auto& item) { return shouldDelete(item); }),
    container.end()
);
```

## ����m�F

- ? �r���h����
- ? �G���[���b�Z�[�W�Ȃ�
- ? �Q�[��������ɓ��삷��͂�
