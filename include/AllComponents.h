/**
 * @file AllComponents.h
 * @brief ���ׂẴR���|�[�l���g���ꊇ�C���N���[�h�iPCH��ցj
 * @author �R���z
 * @date 2025
 * @version 1.0
 * 
 * @details
 * PCH���g���Ȃ�����A����̃t�@�C�������Ŏg�������ꍇ�̑�փw�b�_�[�ł��B
 * 
 * ### �g����:
 * @code
 * #include "AllComponents.h"  // ���ꂾ����OK
 * 
 * // ���ׂẴR���|�[�l���g���g����
 * Entity e = world.Create()
 *     .With<Transform>()
 *     .With<MeshRenderer>()
 *     .With<Rotator>()
 *     .Build();
 * @endcode
 */
#pragma once

// ECS �R�A
#include "ecs/Entity.h"
#include "ecs/World.h"

// ��{�R���|�[�l���g
#include "components/Component.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/Rotator.h"

// �T���v���R���|�[�l���g
#include "samples/ComponentSamples.h"

// �A�j���[�V����
#include "animation/Animation.h"

// �V�X�e��
#include "input/InputSystem.h"
#include "scenes/SceneManager.h"

// �W�����C�u�����i�悭�g�����́j
#include <vector>
#include <memory>
#include <cmath>
