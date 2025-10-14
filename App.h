#pragma once
// ========================================================
// �v���v���Z�b�T�f�B���N�e�B�u�i�R���p�C���O�̐ݒ�j
// ========================================================

// WIN32_LEAN_AND_MEAN: Windows�w�b�_�[����g��Ȃ����������O���āA�R���p�C����������
#define WIN32_LEAN_AND_MEAN

// NOMINMAX: Windows�� min/max �}�N���𖳌����iC++�W����std::min/max�Ƌ������邽�߁j
#define NOMINMAX

// ========================================================
// �K�v�ȃw�b�_�[�t�@�C���̃C���N���[�h�i�O���@�\�̓ǂݍ��݁j
// ========================================================

// Windows API �̊�{�@�\�i�E�B���h�E�쐬�A���b�Z�[�W�����Ȃǁj
#include <Windows.h>

// ����w�b�_�[�t�@�C���Q
#include "GfxDevice.h"      // DirectX11�̃f�o�C�X�Ǘ��N���X
#include "RenderSystem.h"   // �`��V�X�e���i�V�F�[�_�[�A�`�揈���j
#include "World.h"          // ECS���[���h�i�G���e�B�e�B�ƃR���|�[�l���g�̊Ǘ��j
#include "Camera.h"         // �J�����i���_�Ɠ��e�̐ݒ�j
#include "Transform.h"      // �ʒu�E��]�E�X�P�[�����
#include "MeshRenderer.h"   // ���b�V���̕`��ݒ�
#include "Rotator.h"        // ������]�̐U�镑��

// DirectX�̐��w���C�u�����i�x�N�g���A�s��v�Z�p�j
#include <DirectXMath.h>

// ���Ԍv���p��C++�W�����C�u����
#include <chrono>

// ========================================================
// App - �A�v���P�[�V�����S�̂��Ǘ�����N���X
// �i�E�B���h�E�A�O���t�B�b�N�X�A�Q�[�����[�v�𓝊��j
// ========================================================
struct App {
    // ========================================================
    // �����o�ϐ��i���̃N���X���ێ�����f�[�^�j
    // ========================================================
    
    HWND hwnd = nullptr;        // �E�B���h�E�n���h���i�쐬�����E�B���h�E�̎��ʎq�j
    GfxDevice gfx;              // DirectX11�̃f�o�C�X�Ǘ��I�u�W�F�N�g
    RenderSystem renderer;      // �`��V�X�e���i�V�F�[�_�[��`�揈���j
    World world;                // ECS���[���h�i�S�G���e�B�e�B�ƃR���|�[�l���g���Ǘ��j
    Camera cam;                 // �J�����i3D��Ԃ̎��_�ݒ�j

    // ========================================================
    // ���������\�b�h - �A�v���P�[�V�����̋N������
    // ========================================================
    // ����:
    //   hInst: �A�v���P�[�V�����̃C���X�^���X�n���h���iWindows�������I�ɓn���j
    //   width: �E�B���h�E�̉����i�s�N�Z���P�ʁj�f�t�H���g��1280
    //   height: �E�B���h�E�̏c���i�s�N�Z���P�ʁj�f�t�H���g��720
    // �߂�l:
    //   true: ����������
    //   false: ���������s�i�ǂ����ŃG���[�������j
    bool Init(HINSTANCE hInst, int width = 1280, int height = 720) {
        
        // ========================================================
        // �X�e�b�v1: �E�B���h�E�N���X�̓o�^
        // �iWindows�ɃE�B���h�E�̐݌v�}��������j
        // ========================================================
        
        // WNDCLASSEX�\���̂��������i�T�C�Y���������ݒ�j
        WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
        
        // �E�B���h�E�X�^�C��: �����E�c�����ς������S�̂��ĕ`��
        wc.style = CS_HREDRAW | CS_VREDRAW;
        
        // �E�B���h�E�v���V�[�W���i�C�x���g�����֐��j��o�^
        // WndProcStatic: �E�B���h�E�ɉ����N�����Ƃ��i�N���b�N�A���铙�j�ɌĂ΂��֐�
        wc.lpfnWndProc = WndProcStatic;
        
        // ���̃A�v���P�[�V�����̃C���X�^���X�n���h����ݒ�
        wc.hInstance = hInst;
        
        // �}�E�X�J�[�\���̌`���ݒ�iIDC_ARROW = �ʏ�̖��J�[�\���j
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        
        // �E�B���h�E�N���X�̖��O�i���ʗp�̕�����AL""�̓��C�h������j
        wc.lpszClassName = L"SimpleECS_DX11_Class";
        
        // Windows�ɃE�B���h�E�N���X��o�^�i���s������ false ��Ԃ��j
        if (!RegisterClassEx(&wc)) return false;

        // ========================================================
        // �X�e�b�v2: �E�B���h�E�T�C�Y�̒���
        // �i�N���C�A���g�̈���w��T�C�Y�ɂ��邽�߁A�g�̕����v�Z�j
        // ========================================================
        
        // RECT�\����: ��`�̈��\���ileft, top, right, bottom�j
        RECT rc{ 0, 0, width, height };
        
        // AdjustWindowRect: �^�C�g���o�[��g���܂߂����ۂ̃E�B���h�E�T�C�Y���v�Z
        // WS_OVERLAPPEDWINDOW: �W���I�ȃE�B���h�E�i�^�C�g���o�[�A�ŏ���/�ő剻�{�^���t���j
        // FALSE: ���j���[�o�[�Ȃ�
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        
        // ========================================================
        // �X�e�b�v3: �E�B���h�E�̍쐬
        // ========================================================
        
        hwnd = CreateWindowW(
            wc.lpszClassName,           // �o�^�����E�B���h�E�N���X�̖��O
            L"Simple ECS + DX11",       // �E�B���h�E�̃^�C�g���o�[�ɕ\������镶����
            WS_OVERLAPPEDWINDOW,        // �E�B���h�E�X�^�C���i�ʏ�̃E�B���h�E�j
            CW_USEDEFAULT,              // X���W�i�f�t�H���g�ʒu�j
            CW_USEDEFAULT,              // Y���W�i�f�t�H���g�ʒu�j
            rc.right - rc.left,         // �E�B���h�E�̕��i�g���݁j
            rc.bottom - rc.top,         // �E�B���h�E�̍����i�g���݁j
            nullptr,                    // �e�E�B���h�E�Ȃ�
            nullptr,                    // ���j���[�Ȃ�
            hInst,                      // �A�v���P�[�V�����C���X�^���X
            this                        // WM_NCCREATE���b�Z�[�W�ł��̃|�C���^���n�����
        );
        
        // �E�B���h�E�쐬�Ɏ��s�����ꍇ
        if (!hwnd) return false;
        
        // �E�B���h�E����ʂɕ\���iSW_SHOW = �ʏ�\���j
        ShowWindow(hwnd, SW_SHOW);

        // ========================================================
        // �X�e�b�v4: �O���t�B�b�N�X������
        // ========================================================
        
        // DirectX11�f�o�C�X�̏������i���s������ false ��Ԃ��j
        if (!gfx.Init(hwnd, width, height)) return false;
        
        // �����_�����O�V�X�e���̏������i�V�F�[�_�[��`��p�C�v���C���̏����j
        if (!renderer.Init(gfx)) return false;

        // ========================================================
        // �X�e�b�v5: �J�����̐ݒ�
        // �i3D��Ԃ��ǂ̎��_���猩�邩������j
        // ========================================================
        
        // �A�X�y�N�g��i�������c���j���v�Z
        float aspect = float(width) / float(height);
        
        // �J�������쐬�iLookAtLH = Left-Handed���W�n�ŃJ������z�u�j
        cam = Camera::LookAtLH(
            DirectX::XM_PIDIV4,              // ����p�ifovY�j= ��/4 ? 45�x
            aspect,                          // �A�X�y�N�g��i�����c�j
            0.1f,                            // �j�A�N���b�v�ʁi������߂����͕`�悳��Ȃ��j
            100.0f,                          // �t�@�[�N���b�v�ʁi�����艓�����͕`�悳��Ȃ��j
            DirectX::XMFLOAT3{ 0, 2, -6 },   // �J�����̈ʒu�i�ڂ̈ʒu: X=0, Y=2, Z=-6�j
            DirectX::XMFLOAT3{ 0, 0, 0 },    // �����_�i�J���������Ă���ꏊ: ���_�j
            DirectX::XMFLOAT3{ 0, 1, 0 }     // ������x�N�g���iY������j
        );

        // ========================================================
        // �X�e�b�v6: �V�[���̍쐬�i�Q�[���I�u�W�F�N�g�̔z�u�j
        // ========================================================
        
        // �V�����G���e�B�e�B�i�Q�[���I�u�W�F�N�g�j���쐬
        Entity e = world.CreateEntity();
        
        // Transform�R���|�[�l���g��ǉ��i�ʒu�E��]�E�X�P�[�����j
        // �ʒu: (0, 0, 0) ���_
        // ��]: (0, 0, 0) ��]�Ȃ�
        // �X�P�[��: (1, 1, 1) ���{�T�C�Y
        world.Add<Transform>(e, Transform{ 
            DirectX::XMFLOAT3{0, 0, 0},     // position�i�ʒu�j
            DirectX::XMFLOAT3{0, 0, 0},     // rotation�i��]�A�x���@�j
            DirectX::XMFLOAT3{1, 1, 1}      // scale�i�X�P�[���j
        });
        
        // MeshRenderer�R���|�[�l���g��ǉ��i�`��ݒ�j
        // �F: RGB(0.2, 0.7, 1.0) = ���F
        world.Add<MeshRenderer>(e, MeshRenderer{ 
            DirectX::XMFLOAT3{0.2f, 0.7f, 1.0f}  // color�i�ԁE�΁E�̒l�A0.0?1.0�j
        });
        
        // Rotator�R���|�[�l���g��ǉ��i������]�̐U�镑���j
        // ���b45�x�̑��x��Y������ɉ�]
        world.Add<Rotator>(e, Rotator{ 45.0f });

        // ����������
        return true;
    }

    // ========================================================
    // ���C�����[�v���s - �A�v���P�[�V�����̐S����
    // �i�Q�[���������Ă���ԁA���̊֐��������Ɠ���������j
    // ========================================================
    void Run() {
        // MSG�\����: Windows����̃��b�Z�[�W�i�C�x���g�j���i�[
        MSG msg{};
        
        // �O��̃t���[���������L�^�i�����x�^�C�}�[���g�p�j
        auto prev = std::chrono::high_resolution_clock::now();
        
        // ���C�����[�v�iWM_QUIT ���b�Z�[�W���󂯎��܂ŉi���ɌJ��Ԃ��j
        while (msg.message != WM_QUIT) {
            
            // ========================================================
            // �t�F�[�Y1: Windows���b�Z�[�W�̏���
            // �i�}�E�X�A�L�[�{�[�h�A�E�B���h�E�C�x���g���j
            // ========================================================
            
            // PeekMessage: ���b�Z�[�W�L���[���m�F�i�ҋ@�����ɑ����ɖ߂�j
            // PM_REMOVE: ���b�Z�[�W���擾������L���[����폜
            if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                // TranslateMessage: �L�[�{�[�h���b�Z�[�W�𕶎��ɕϊ�
                TranslateMessage(&msg);
                
                // DispatchMessage: �E�B���h�E�v���V�[�W���Ƀ��b�Z�[�W�𑗂�
                DispatchMessage(&msg);
                
                // ���b�Z�[�W�������I������玟�̃��[�v��
                continue;
            }
            
            // ========================================================
            // �t�F�[�Y2: �f���^�^�C���i�O�t���[������̌o�ߎ��ԁj�̌v�Z
            // ========================================================
            
            // ���ݎ������擾
            auto now = std::chrono::high_resolution_clock::now();
            
            // �f���^�^�C���idt�j= ���ݎ��� - �O�񎞍�
            // duration<float> �ŕb�P�ʂ̕��������_���ɕϊ�
            std::chrono::duration<float> dt = now - prev;
            
            // ����̌v�Z�̂��߁A���ݎ�����ۑ�
            prev = now;

            // ========================================================
            // �t�F�[�Y3: �Q�[�����W�b�N�̍X�V
            // ========================================================
            
            // �SBehaviour�R���|�[�l���g���X�V�iRotator�Ȃǂ���]���������s�j
            world.Tick(dt.count());
            
            // ========================================================
            // �t�F�[�Y4: �`�揈��
            // ========================================================
            
            // �t���[���J�n�i��ʂ��N���A���ĕ`�揀���j
            gfx.BeginFrame();
            
            // �S�I�u�W�F�N�g��`��i�J�������_��3D��Ԃ������_�����O�j
            renderer.Render(gfx, world, cam);
            
            // �t���[���I���i�o�b�N�o�b�t�@����ʂɕ\���j
            gfx.EndFrame();
        }
    }

    // ========================================================
    // �E�B���h�E�v���V�[�W���i�ÓI���\�b�h�j
    // Windows���璼�ڌĂяo�����A�C�x���g�����̓���
    // ========================================================
    // �y�d�v�Ȏd�g�݁z
    // Windows�́u�N���X�̃����o�֐��v�𒼐ڃR�[���o�b�N�Ƃ��ēo�^�ł��Ȃ����߁A
    // static���\�b�h�ithis�|�C���^�������Ȃ��j���o�R���āA
    // ���ۂ̃����o�֐� WndProc ���Ăяo���g���b�N���g���Ă���
    // ========================================================
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = nullptr;  // ���̃E�B���h�E�Ɋ֘A�t����ꂽApp�I�u�W�F�N�g�ւ̃|�C���^
        
        // WM_NCCREATE: �E�B���h�E���쐬����钼�O�ɑ����郁�b�Z�[�W
        if (msg == WM_NCCREATE) {
            // lp�ɓn���ꂽCREATESTRUCT�\���̂��擾
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
            
            // CreateWindowW�̍Ō�̈����ithis�j��lpCreateParams�Ɋi�[����Ă���
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            
            // SetWindowLongPtr: �E�B���h�E�Ƀ��[�U�[�f�[�^�iApp�|�C���^�j���֘A�t����
            // GWLP_USERDATA: ���[�U�[��`�f�[�^�p�̃X���b�g
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        }
        else {
            // WM_NCCREATE�ȊO�̃��b�Z�[�W�ł́A�ۑ����ꂽ�|�C���^���擾
            self = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }
        
        // App�I�u�W�F�N�g�̃|�C���^���L���Ȃ�A�����o�֐� WndProc ���Ă�
        if (self) return self->WndProc(hWnd, msg, wp, lp);
        
        // App�I�u�W�F�N�g���܂��֘A�t�����Ă��Ȃ��ꍇ�́A�f�t�H���g����
        return DefWindowProc(hWnd, msg, wp, lp);
    }

    // ========================================================
    // �E�B���h�E�v���V�[�W���i�����o�֐��j
    // ���ۂ̃C�x���g�������s��
    // ========================================================
    // ����:
    //   hWnd: �C�x���g�����������E�B���h�E�̃n���h��
    //   msg: ���b�Z�[�W�̎�ށiWM_DESTROY�Ȃǁj
    //   wp: ���b�Z�[�W�ŗL�̃p�����[�^1
    //   lp: ���b�Z�[�W�ŗL�̃p�����[�^2
    // �߂�l:
    //   ���b�Z�[�W�����̌��ʁi�ʏ��0��Ԃ��j
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        
        // WM_DESTROY: �E�B���h�E���j�������Ƃ��ɑ����郁�b�Z�[�W
        case WM_DESTROY:
            // PostQuitMessage: �A�v���P�[�V������WM_QUIT���b�Z�[�W�𑗂�
            // ����ɂ�胁�C�����[�v���I�����A�v���O�������I���
            PostQuitMessage(0);
            return 0;
            
        // ���̑��̃��b�Z�[�W�i�������Ȃ��j
        default:
            break;
        }
        
        // �������Ȃ����b�Z�[�W��Windows�̃f�t�H���g�����ɔC����
        return DefWindowProc(hWnd, msg, wp, lp);
    }
};

// ========================================================
// �R�[�h����
// �쐬��: �R���z
// ========================================================
