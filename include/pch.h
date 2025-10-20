/**
 * @file pch.h
 * @brief �v���R���p�C���ς݃w�b�_�[
 * @author �R���z
 * @date 2024
 * @version 1.0
 *
 * @details
 * �悭�g���w�b�_�[���܂Ƃ߂��v���R���p�C���ς݃w�b�_�[�ł��B
 * ���̃t�@�C�����C���N���[�h���邾���ŁA��{�I�ȃR���|�[�l���g��
 * �V�X�e�����g����悤�ɂȂ�܂��B
 */
#pragma once

// ========================================================
// �W�����C�u����
// ========================================================
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ========================================================
// DirectX
// ========================================================
#include <DirectXMath.h>

// ========================================================
// ECS �R�A
// ========================================================
#include "ecs/Entity.h"
#include "ecs/World.h"

// ========================================================
// ��{�R���|�[�l���g
// ========================================================
#include "components/Component.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"

// ========================================================
// �V�X�e��
// ========================================================
#include "input/InputSystem.h"
#include "graphics/Camera.h"
#include "graphics/GfxDevice.h"
#include "graphics/TextureManager.h"
#include "graphics/RenderSystem.h"

// ========================================================
// �V�[���Ǘ�
// ========================================================
#include "scenes/SceneManager.h"

// ========================================================
// �T���v���R���|�[�l���g�i�I�v�V�����j
// ========================================================
#include "samples/ComponentSamples.h"

// ========================================================
// �A�j���[�V�����i�I�v�V�����j
// ========================================================
#include "animation/Animation.h"
