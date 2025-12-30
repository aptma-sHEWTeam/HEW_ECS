/*****************************************************************//**
 * \file   Effect.h
 * \brief  
 * 
 * \author 飯島英菜
 * \date   12/11
 *********************************************************************/
#pragma once

//=====================
//インクルード部
//=====================
#include "../libs/Effekseer/Effekseer.h"
#include "../libs/Effekseer/Effekseer.Modules.h"
#include "../libs/Effekseer/Effekseer.SIMD.h"
#include "../libs/Effekseer/EffekseerRendererDX11.h"
#include "graphics/GfxDevice.h"
#include "graphics/Camera.h"
#include "DirectXMath.h"
#include<map>
#include<list>

struct LoopInfo
{
    std::string effectName;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT3 rotation;
    int handle;
    int currentHandle;
    int originalHandle;
};

class EffekseerManager
{
 public:
	 // インスタンス取得
    static EffekseerManager& GetInstance()
	{
       static EffekseerManager instance;
        return instance;
	}

    void Init(GfxDevice device, Camera camera); ///<初期化処理
    void UnInit();			///<終了処理

   void Load();				///<読み込み処理

   int PlayEffect(const std::string &effectName, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, bool loop = false); ///<エフェクト再生
   void StopEffect();		///<エフェクト停止
   void StopEffect(const std::string &effectName); ///<指定した名前のエフェクトを停止
   void StopEffectHandle(Effekseer::Handle handle);///<確保したハンドルをわたして停止

   void SetCamera(const Camera& camera);		///<カメラ処理

   void SetEffectPosition(int handle, DirectX::XMFLOAT3 pos);
   void SetEffectRotation(Effekseer::Handle handle, DirectX::XMFLOAT3 rotation);
   void SetEffectScale(int handle, DirectX::XMFLOAT3 scale);
   void Update();			///<更新処理
   void Draw(const Camera& camera);				///<描画処理
   
 private:
   EffekseerManager() = default;
   ~EffekseerManager() = default;

   Effekseer::ManagerRef m_pManager;
   EffekseerRendererDX11::RendererRef m_pRenderer;

   Camera m_Camera;

	Effekseer::Vector3D m_viewerPosition;
    Effekseer::Handle efkHandle = 0;

	Effekseer::Matrix44 m_efkViewMat;
    Effekseer::Matrix44 m_efkProjMat;

	int32_t time = 0;

	//エフェクトを名前で管理するためのマップ
    std::map<std::string, Effekseer::EffectRef> m_effects;
    std::map < int, std::string>m_playingEffects;

	std::vector<int> m_playingHandles;

    std::list<LoopInfo> m_loopEffects;
};
