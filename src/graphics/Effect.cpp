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
    m_effects["Goal"]           = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Goal/efe_goal.efkefc");
    m_effects["SpeedUp"]        = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/SpeedUp/efe_speedUp2.efkefc");
    m_effects["WarpIn"]         = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Warp/warpin_effect.efkefc");
    m_effects["WarpOut"]        = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Warp/warpout_effect.efkefc");
    m_effects["FireFirst"]      = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Fire/fire flare.efkefc");
    m_effects["FireFirstToSec"] = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Fire/fire middle.efkefc");
    m_effects["FireThird"]      = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Fire/flare 2.efkefc");
    m_effects["FireSecToThird"] = Effekseer::Effect::Create(m_pManager, u"Assets/Effect/Fire/firecore.efkefc");
}

//=====================
//エフェクトの再生
//=====================
int  EffekseerManager::PlayEffect(const std::string& effectName,DirectX::XMFLOAT3 pos,DirectX::XMFLOAT3 scale, bool loop)
{
    auto it = m_effects.find(effectName);

    int handle = m_pManager->Play(it->second, pos.x,pos.y,pos.z);
    
    m_pManager->SetScale(handle, scale.x, scale.y, scale.z);

    if (loop)
    {
        LoopInfo info;
        info.effectName = effectName;
        info.position = pos;
        info.scale = scale;
        info.handle = handle;
        m_loopEffects.push_back(info);
    }

    return handle;
}
//=====================
//エフェクトの停止
//=====================
void EffekseerManager::StopEffect() 
{
    m_pManager->StopAllEffects();
    m_loopEffects.clear();
}

void EffekseerManager::StopEffect(const std::string &effectName)
{
    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end(); )
    {
        if (it->effectName == effectName)
        {
            m_pManager->StopEffect(it->handle);
            it = m_loopEffects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EffekseerManager::StopEffectHandle(Effekseer::Handle handle)
{
    if (handle != -1)
    {
        m_pManager->StopEffect(handle);
    }

    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end();)
    {
        if (it->handle == handle) {
            m_pManager->StopEffect(it->handle);
            it = m_loopEffects.erase(it);
        } else {
            ++it;
        }
    }
}

void EffekseerManager::SetEffectScale(int handle, DirectX::XMFLOAT3 scale)
{
    if (m_pManager != nullptr && m_pManager->Exists(handle))
    {
        m_pManager->SetScale(handle, scale.x, scale.y, scale.z);
    }
}


//=====================
//カメラ処理
//=====================
void EffekseerManager::SetCamera(const Camera& camera)
{
    const DirectX::XMMATRIX &appViewMat = camera.GetViewMatrix();
    const DirectX::XMMATRIX &appProjMat = camera.GetProjectionMatrix();

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

//=============================
//エフェクトの座標更新処理
//============================
void EffekseerManager::SetEffectPosition(int handle, DirectX::XMFLOAT3 pos) 
{
    if (m_pManager != nullptr)
    {
        //直接ハンドルと新しいざひょうをお渡しする
        m_pManager->SetLocation(handle, pos.x, pos.y, pos.z);
    }
}

//=============================
//エフェクトの向きの更新処理
//============================
void EffekseerManager::SetEffectRotation(Effekseer::Handle handle, DirectX::XMFLOAT3 rotation)
{
    //ラジアン返還
    float radX = rotation.x * (DirectX::XM_PI / 180.0f);
    float radY = rotation.y * (DirectX::XM_PI / 180.0f);
    float radZ = rotation.z * (DirectX::XM_PI / 180.0f);

    m_pManager->SetRotation(handle, radX, radY, radZ);
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

    for (auto& info : m_loopEffects)
    {
        if (!m_pManager->Exists(info.handle))
        {
             auto it = m_effects.find(info.effectName);
             if (it != m_effects.end())
             {
                 info.handle = m_pManager->Play(it->second, info.position.x, info.position.y, info.position.z);
                 m_pManager->SetScale(info.handle, info.scale.x, info.scale.y, info.scale.z);
                 m_pManager->SetRotation(info.handle, info.rotation.x, info.rotation.y, info.rotation.z);
             }
        }
    }
}

//=====================
//描画処理
//=====================
void EffekseerManager::Draw(const Camera& camera)
{
    m_pRenderer->SetTime(time / 60.0f);
    SetCamera(camera);
    m_pRenderer->BeginRendering();

    Effekseer::Manager::DrawParameter drawParameter;
    drawParameter.ZNear = 0.0f;
    drawParameter.ZFar = 1.0f;
    drawParameter.ViewProjectionMatrix = m_pRenderer->GetCameraProjectionMatrix();
    m_pManager->Draw(drawParameter);

    m_pRenderer->EndRendering();
}

