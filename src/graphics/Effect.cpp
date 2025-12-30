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
#include <algorithm>

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
    predefined_ = {
        {"Goal",           "Assets/Effect/Goal/efe_goal.efkefc"},
        {"WarpIn",         "Assets/Effect/Warp/warpin_effect.efkefc"},
        {"WarpOut",        "Assets/Effect/Warp/warpout_effect.efkefc"},
        {"DashBoard",      "Assets/Effect/SpeedUp/efe_SpeedUp2.efkefc"},
        {"SpeedUp",        "Assets/Effect/SpeedUp/efe_SpeedUp.efkefc"},
        {"FireFirst",      "Assets/Effect/Fire/fire flare.efkefc"},
        {"FireFirstToSec", "Assets/Effect/Fire/fire middle.efkefc"},
        {"FireThird",      "Assets/Effect/Fire/flare 2.efkefc"},
        {"FireSecToThird", "Assets/Effect/Fire/firecore.efkefc"},
    };

    for (const auto& def : predefined_) {
        std::wstring wpath(def.path.begin(), def.path.end());
        m_effects[def.name] = Effekseer::Effect::Create(m_pManager, reinterpret_cast<const char16_t*>(wpath.c_str()));
    }
}

//=====================
//エフェクトの再生
//=====================
int  EffekseerManager::PlayEffect(const std::string& effectName,DirectX::XMFLOAT3 pos,DirectX::XMFLOAT3 scale, bool loop)
{
    auto it = m_effects.find(effectName);
    if (it == m_effects.end()) {
        // 定義済みリストにあればロードを試みる
        auto defIt = std::find_if(predefined_.begin(), predefined_.end(), [&](const EffectDef& d){ return d.name == effectName; });
        if (defIt != predefined_.end()) {
            std::wstring wpath(defIt->path.begin(), defIt->path.end());
            auto eff = Effekseer::Effect::Create(m_pManager, reinterpret_cast<const char16_t*>(wpath.c_str()));
            if (eff != nullptr) {
                 m_effects[effectName] = eff;
                 it = m_effects.find(effectName);
            }
        }
    }
    if (it == m_effects.end()) {
        return -1;
    }

    int handle = m_pManager->Play(it->second, pos.x,pos.y,pos.z);
    
    m_pManager->SetScale(handle, scale.x, scale.y, scale.z);

    if (loop)
    {
        LoopInfo info;
        info.effectName = effectName;
        info.position = pos;
        info.scale = scale;
        info.rotation = {0,0,0};
        info.handle = handle;
        m_loopEffects.push_back(info);
    }

    return handle;
}

std::optional<int> EffekseerManager::PlayEffectSafe(const std::string &effectName, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, bool loop)
{
    int h = PlayEffect(effectName, pos, scale, loop);
    if (h < 0) return std::nullopt;
    return h;
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
            if (it->handle != -1) {
                m_pManager->StopEffect(it->handle);
            }
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
            if (it->handle != -1) {
                m_pManager->StopEffect(it->handle);
            }
            it = m_loopEffects.erase(it);
        } else {
            ++it;
        }
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
        // ループエフェクトの場合はoriginalHandleからcurrentHandleを引いて更新する
        for (auto& info : m_loopEffects)
        {
            if (info.handle == handle)
            {
                info.position = pos;
                m_pManager->SetLocation(info.handle, pos.x, pos.y, pos.z);
                return;
            }
        }
        //通常の一回再生
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
    for (auto& info : m_loopEffects) {
        if (info.handle == handle) {
            info.rotation = rotation;
            break;
        }
    }
}

//=============================
//エフェクトの大きさ更新処理
//============================
void EffekseerManager::SetEffectScale(int handle, DirectX::XMFLOAT3 scale)
{
    if (m_pManager != nullptr && m_pManager->Exists(handle)) {
        m_pManager->SetScale(handle, scale.x, scale.y, scale.z);
        for (auto& info : m_loopEffects) {
            if (info.handle == handle) {
                info.scale = scale;
                break;
            }
        }
    }
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

    // 時間を更新
    time++;

    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end(); )
    {
        if (!m_pManager->Exists(it->handle))
        {
             auto effIt = m_effects.find(it->effectName);
             if (effIt != m_effects.end())
             {
                it->handle = m_pManager->Play(effIt->second, it->position.x, it->position.y, it->position.z);
                m_pManager->SetScale(it->handle, it->scale.x, it->scale.y, it->scale.z);
                float radX = it->rotation.x * (DirectX::XM_PI / 180.0f);
                float radY = it->rotation.y * (DirectX::XM_PI / 180.0f);
                float radZ = it->rotation.z * (DirectX::XM_PI / 180.0f);
                m_pManager->SetRotation(it->handle, radX, radY, radZ);
             }
        }
        ++it;
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

