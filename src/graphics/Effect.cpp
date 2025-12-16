/*****************************************************************/ /**
 * \file   Effect.cpp
 * \brief  Effekseer実装システム
 * 
 * \author 飯島英菜
 * \date   12/11
 *********************************************************************/

//=====================
//インクルード部
//=====================
#include "graphics/Effect.h"
#include<stdio.h>
#include<string.h>
#include<Windows.h>

//=====================
//初期化処理
//=====================
void EffekseerManager::Init(GfxDevice device, Camera camera)
{
    m_Camera = camera;

    //エフェクトのマネージャー作成
   m_pManager = ::Effekseer::Manager::Create(8000);

    //DirectXレンダラーの作成
   m_pRenderer = ::EffekseerRendererDX11::Renderer::Create(device.Dev(), device.Ctx(), 8000);

   //描画モジュールの設定
   m_pManager->SetSpriteRenderer(m_pRenderer->CreateSpriteRenderer());          //スプライト描画機能
   m_pManager->SetRibbonRenderer(m_pRenderer->CreateRibbonRenderer());          //メッシュ  描画機能
   m_pManager->SetRingRenderer(m_pRenderer->CreateRingRenderer());              //リング    描画機能
   m_pManager->SetTrackRenderer(m_pRenderer->CreateTrackRenderer());            //軌跡      描画機能
   m_pManager->SetModelRenderer(m_pRenderer->CreateModelRenderer());            //モデル    描画機能

   //テクスチャ、モデル、カーブ、音の読み込み
   m_pManager->SetTextureLoader(m_pRenderer->CreateTextureLoader());            
   m_pManager->SetModelLoader(m_pRenderer->CreateModelLoader());
   m_pManager->SetMaterialLoader(m_pRenderer->CreateMaterialLoader());
   m_pManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

   //座標系の設定
   m_pManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
  
}

//=====================
//終了処理
//=====================
void EffekseerManager::UnInit() 
{
    if (m_pManager != nullptr)
    {
        m_pManager = nullptr;
    }
    if (m_pRenderer != nullptr)
    {
        m_pRenderer = nullptr;
    }
}

//=====================
//エフェクト読み込み
//=====================
void EffekseerManager::Load() 
{
    m_effects["Goal"]      = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Goal/efe_goal.efkefc");
    m_effects["SpeedUp"]   = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/SpeedUp/efe_speedup.efkefc");
    m_effects["WarpIn"]    = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Warp/warpin_effect.efkefc");
    m_effects["WarpOut"]   = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Warp/warpout_effect.efkefc");
}

//=====================
//エフェクトの再生
//=====================
int  EffekseerManager::PlayEffect(const std::string& effectName,DirectX::XMFLOAT3 pos) 
{
    auto it = m_effects.find(effectName);

    int handle = m_pManager->Play(it->second, pos.x,pos.y,pos.z);

    return handle;
}

//=====================
//エフェクトの停止
//=====================
void EffekseerManager::StopEffect() 
{
    m_pManager->StopAllEffects();
}

//=====================
//カメラ処理
//=====================
void EffekseerManager::SetCamera()
{
    const DirectX::XMMATRIX &appViewMat = m_Camera.GetViewMatrix();
    const DirectX::XMMATRIX &appProjMat = m_Camera.GetProjectionMatrix();

    for (int i = 0;i < 4;i++)
    {
        for (int j = 0;j < 4;j++)
        {
            m_efkViewMat.Values[i][j] = appViewMat.r[i].m128_f32[j];

            m_efkProjMat.Values[i][j] = appProjMat.r[i].m128_f32[j];
        }
    }

    m_pRenderer->SetCameraMatrix(m_efkViewMat);
    m_pRenderer->SetProjectionMatrix(m_efkProjMat);
}

//=====================
//更新処理
//=====================
void EffekseerManager::Update() 
{
    Effekseer::Manager::LayerParameter efkLayerParm;
    Effekseer::Matrix44 invViewMat;
    Effekseer::Matrix44::Inverse(invViewMat, m_efkViewMat);
    efkLayerParm.ViewerPosition = Effekseer::Vector3D(invViewMat.Values[3][0], invViewMat.Values[3][1], invViewMat.Values[3][2]);
    m_pManager->SetLayerParameter(0, efkLayerParm);

    m_pManager->Update();   

}

//=====================
//描画処理
//=====================
void EffekseerManager::Draw()
{
    m_pRenderer->SetTime(time / 60.0f);
    SetCamera();
    m_pRenderer->BeginRendering();

    Effekseer::Manager::DrawParameter drawParameter;
    drawParameter.ZNear = 0.0f;
    drawParameter.ZFar = 1.0f;
    drawParameter.ViewProjectionMatrix = m_pRenderer->GetCameraProjectionMatrix();
    m_pManager->Draw(drawParameter);

    m_pRenderer->EndRendering();
}

