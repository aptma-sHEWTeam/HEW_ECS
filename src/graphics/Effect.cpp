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
#include <string>
#include <filesystem>
#include "app/DebugLog.h"

namespace { std::string WideToUtf8(const std::wstring& src); }

namespace
{
class RelativeFileInterface final : public Effekseer::FileInterface
{
public:
    explicit RelativeFileInterface(std::u16string baseDir)
        : baseDir_(std::move(baseDir))
    {
    }

    void SetBaseDir(std::u16string baseDir)
    {
        baseDir_ = std::move(baseDir);
    }

    Effekseer::FileReaderRef OpenRead(const char16_t* path) override
    {
        auto resolved = ResolvePath(path);
        std::u16string orig = path ? std::u16string(path) : std::u16string();
        std::string reqs;
        std::string rps;
        try { reqs = orig.empty() ? std::string() : WideToUtf8(std::wstring(orig.begin(), orig.end())); } catch (...) { reqs = "<conv-error>"; }
        try { rps = resolved.empty() ? std::string() : WideToUtf8(std::wstring(resolved.begin(), resolved.end())); } catch (...) { rps = "<conv-error>"; }
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer File OpenRead request: original='") + reqs + "' -> resolved='" + rps + "'");
        auto reader = underlying_.OpenRead(resolved.c_str());
        if (reader == nullptr) {
            DEBUGLOG_WARNING(std::string("Effekseer OpenRead failed for '") + rps + "'");
        } else {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer OpenRead succeeded for '") + rps + "'");
        }
        return reader;
    }

    Effekseer::FileReaderRef TryOpenRead(const char16_t* path) override
    {
        auto resolved = ResolvePath(path);
        std::string rps;
        try { rps = resolved.empty() ? std::string() : WideToUtf8(std::wstring(resolved.begin(), resolved.end())); } catch (...) { rps = "<conv-error>"; }
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer File TryOpenRead -> resolved='") + rps + "'");
        auto reader = underlying_.TryOpenRead(resolved.c_str());
        if (reader == nullptr) {
            DEBUGLOG_WARNING(std::string("Effekseer TryOpenRead failed for '") + rps + "'");
        } else {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer TryOpenRead succeeded for '") + rps + "'");
        }
        return reader;
    }

    Effekseer::FileWriterRef OpenWrite(const char16_t* path) override
    {
        auto resolved = ResolvePath(path);
        std::string rps;
        try { rps = resolved.empty() ? std::string() : WideToUtf8(std::wstring(resolved.begin(), resolved.end())); } catch (...) { rps = "<conv-error>"; }
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer File OpenWrite -> resolved='") + rps + "'");
        auto writer = underlying_.OpenWrite(resolved.c_str());
        if (writer == nullptr) {
            DEBUGLOG_WARNING(std::string("Effekseer OpenWrite failed for '") + rps + "'");
        } else {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer OpenWrite succeeded for '") + rps + "'");
        }
        return writer;
    }

private:
    std::u16string ResolvePath(const char16_t* path) const
    {
        if (path == nullptr)
        {
            return std::u16string();
        }

        std::u16string p(path);
        if (p.empty())
        {
            return p;
        }

        if (baseDir_.empty())
        {
            return p;
        }

        std::filesystem::path req(p.begin(), p.end());
        if (req.is_absolute())
        {
            return p;
        }

        auto filename = req.filename();
        if (filename.empty())
        {
            return p;
        }

        std::filesystem::path base(baseDir_.begin(), baseDir_.end());
        std::filesystem::path resolved = (base / filename).lexically_normal();
        std::u16string out;
        auto s = resolved.u16string();
        out.assign(s.begin(), s.end());
        return out;
    }

    std::u16string baseDir_;
    Effekseer::DefaultFileInterface underlying_;
};
}

namespace
{
bool ShouldLogPlay(const std::string& effectName)
{
    if (effectName == "StarSmall" || effectName == "StarMedium" || effectName == "StarBig")
    {
        return false;
    }
    return true;
}

std::wstring Utf8ToWide(const std::string& src)
{
    if (src.empty())
    {
        return std::wstring();
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
    if (len <= 0)
    {
        return std::wstring();
    }

    std::wstring dst(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &dst[0], len);
    return dst;
}

std::string WideToUtf8(const std::wstring& src)
{
    if (src.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return std::string();
    std::string dst(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, &dst[0], len, nullptr, nullptr);
    return dst;
}
}

//=====================
//初期化処理
//=====================
void EffekseerManager::Init(GfxDevice device, Camera camera)
{
    m_Camera = camera;

    fileInterface_ = Effekseer::MakeRefPtr<RelativeFileInterface>(std::u16string());

    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: installed RelativeFileInterface"));

    m_pManager = ::Effekseer::Manager::Create(8000);
    m_pRenderer = ::EffekseerRendererDX11::Renderer::Create(device.Dev(), device.Ctx(), 8000);

    if (m_pManager == nullptr || m_pRenderer == nullptr)
    {
        m_pManager = nullptr;
        m_pRenderer = nullptr;
        return;
    }

    m_pManager->SetSpriteRenderer(m_pRenderer->CreateSpriteRenderer());
    m_pManager->SetRibbonRenderer(m_pRenderer->CreateRibbonRenderer());
    m_pManager->SetRingRenderer(m_pRenderer->CreateRingRenderer());
    m_pManager->SetTrackRenderer(m_pRenderer->CreateTrackRenderer());
    m_pManager->SetModelRenderer(m_pRenderer->CreateModelRenderer());

    m_pManager->SetTextureLoader(EffekseerRenderer::CreateTextureLoader(m_pRenderer->GetGraphicsDevice(), fileInterface_));
    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: SetTextureLoader with custom FileInterface"));
    m_pManager->SetModelLoader(m_pRenderer->CreateModelLoader());
    m_pManager->SetMaterialLoader(EffekseerRendererDX11::CreateMaterialLoader(m_pRenderer->GetGraphicsDevice(), fileInterface_));
    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: SetMaterialLoader with custom FileInterface"));
    m_pManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

    m_pManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
}

//=====================
//終了処理
//=====================
void EffekseerManager::UnInit()
{
    if (m_pManager != nullptr)
    {
        m_pManager->StopAllEffects();
        m_loopEffects.clear();
        m_effects.clear();
    }

    if (m_pRenderer != nullptr)
    {
        m_pRenderer = nullptr;
    }

    if (m_pManager != nullptr)
    {
        m_pManager = nullptr;
    }
}

//=====================
//エフェクト読み込み
//=====================
void EffekseerManager::Load()
{
    if (m_pManager == nullptr)
    {
        return;
    }

    predefined_ = {
        {"Goal",           "Assets/Effect/Goal/efe_goal.efkefc"},
        {"WarpIn",         "Assets/Effect/Warp/warpin_effect.efkefc"},
        {"WarpOut",        "Assets/Effect/Warp/warpout_effect.efkefc"},
        {"DashBoard",      "Assets/Effect/SpeedUp/efe_SpeedUp2.efkefc"},
        {"SpeedUp",        "Assets/Effect/SpeedUp/efe_SpeedUp.efkefc"},
        {"FireFirst",      "Assets/Effect/Fire/fire1.efkefc"},
        {"FireSecond",     "Assets/Effect/Fire/fire_2.efkefc"},
        {"FireThird",      "Assets/Effect/Fire/fire_3.efkefc"},
        {"StarSmall",      "Assets/Effect/Star/Star_Effects_Small.efkefc"},
        {"StarMedium",     "Assets/Effect/Star/Star_Effects_Medium.efkefc"},
        {"StarBig",        "Assets/Effect/Star/Star_Effects_Big.efkefc"},
    };

    for (const auto& def : predefined_)
    {
        std::wstring wpath = Utf8ToWide(def.path);
        if (wpath.empty())
        {
            continue;
        }

        if (fileInterface_ != nullptr)
        {
            std::filesystem::path effectPath(wpath);
            auto baseDir16 = effectPath.parent_path().u16string();
            if (auto* rel = dynamic_cast<RelativeFileInterface*>(fileInterface_.Get()))
            {
                rel->SetBaseDir(baseDir16);
                std::string bep;
                try { bep = effectPath.parent_path().wstring().empty() ? std::string() : WideToUtf8(effectPath.parent_path().wstring()); } catch(...) { bep = "<conv-error>"; }
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer Load: set effect base dir to '") + bep + "' for effect '" + def.path + "'");
            }
        }

        std::string wpath_s = WideToUtf8(wpath);
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: creating effect from '") + wpath_s + "'");
        auto effect = Effekseer::Effect::Create(m_pManager, reinterpret_cast<const char16_t*>(wpath.c_str()));
        if (effect != nullptr)
        {
            m_effects[def.name] = effect;
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: effect loaded '") + wpath_s + "' as '" + def.name + "'");
        }
        else
        {
            DEBUGLOG_WARNING(std::string("EffekseerManager: failed to load effect '") + wpath_s + "'");
        }
    }
}

//=====================
//エフェクトの再生
//=====================
int EffekseerManager::PlayEffect(const std::string& effectName, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, bool loop)
{
    if (m_pManager == nullptr)
    {
        return -1;
    }

    if (effectName == "Goal")
    {
        scale.x *= 0.5f;
        scale.y *= 0.5f;
        scale.z *= 0.5f;
    }

    auto it = m_effects.find(effectName);
    if (it == m_effects.end())
    {
        auto defIt = std::find_if(predefined_.begin(), predefined_.end(), [&](const EffectDef& d)
                                  { return d.name == effectName; });
        if (defIt != predefined_.end())
        {
            std::wstring wpath = Utf8ToWide(defIt->path);
            if (!wpath.empty())
            {
                if (fileInterface_ != nullptr)
                {
                    std::filesystem::path effectPath(wpath);
                    auto baseDir16 = effectPath.parent_path().u16string();
                    if (auto* rel = dynamic_cast<RelativeFileInterface*>(fileInterface_.Get()))
                    {
                        rel->SetBaseDir(baseDir16);
                        std::string bep;
                        try { bep = effectPath.parent_path().wstring().empty() ? std::string() : WideToUtf8(effectPath.parent_path().wstring()); } catch(...) { bep = "<conv-error>"; }
                        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("Effekseer PlayEffect: set effect base dir to '") + bep + "' for effect '" + effectName + "'");
                    }
                }

                std::string wpath_s = WideToUtf8(wpath);
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: PlayEffect creating effect from '") + wpath_s + "' for '" + effectName + "'");
                auto eff = Effekseer::Effect::Create(m_pManager, reinterpret_cast<const char16_t*>(wpath.c_str()));
                if (eff != nullptr)
                {
                    m_effects[effectName] = eff;
                    it = m_effects.find(effectName);
                    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: PlayEffect loaded effect '") + wpath_s + "' for '" + effectName + "'");
                }
                else
                {
                    DEBUGLOG_WARNING(std::string("EffekseerManager: PlayEffect failed to load effect '") + wpath_s + "' for '" + effectName + "'");
                }
            }
        }
    }
    if (it == m_effects.end() || it->second == nullptr)
    {
        return -1;
    }

    int handle = m_pManager->Play(it->second, pos.x, pos.y, pos.z);
    if (!m_pManager->Exists(handle))
    {
        DEBUGLOG_WARNING(std::string("EffekseerManager: Play returned invalid handle for effect '") + effectName + "'");
        return -1;
    }
    else
    {
        if (ShouldLogPlay(effectName))
        {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, std::string("EffekseerManager: Play succeeded handle=") + std::to_string(handle) + " for '" + effectName + "'");
        }
    }

    m_pManager->SetScale(handle, scale.x, scale.y, scale.z);

    if (loop)
    {
        LoopInfo info;
        info.effectName = effectName;
        info.position = pos;
        info.scale = scale;
        info.rotation = {0, 0, 0};
        info.handle = handle;
        m_loopEffects.push_back(info);
    }

    return handle;
}

std::optional<int> EffekseerManager::PlayEffectSafe(const std::string& effectName, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, bool loop)
{
    int handle = PlayEffect(effectName, pos, scale, loop);
    if (handle < 0)
    {
        return std::optional<int>();
    }
    return handle;
}

//=====================
//エフェクトの停止
//=====================
void EffekseerManager::StopEffect()
{
    if (m_pManager == nullptr)
    {
        return;
    }

    m_pManager->StopAllEffects();
    m_loopEffects.clear();
}

void EffekseerManager::StopEffect(const std::string& effectName)
{
    if (m_pManager == nullptr)
    {
        return;
    }

    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end();)
    {
        if (it->effectName == effectName)
        {
            if (it->handle != -1 && m_pManager->Exists(it->handle))
            {
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
    if (m_pManager == nullptr)
    {
        return;
    }

    if (handle != -1 && m_pManager->Exists(handle))
    {
        m_pManager->StopEffect(handle);
    }

    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end();)
    {
        if (it->handle == handle)
        {
            if (it->handle != -1 && m_pManager->Exists(it->handle))
            {
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

//=====================
//カメラ処理
//=====================
void EffekseerManager::SetCamera(const Camera& camera)
{
    if (m_pRenderer == nullptr)
    {
        return;
    }

    const DirectX::XMMATRIX& appViewMat = camera.GetViewMatrix();
    const DirectX::XMMATRIX& appProjMat = camera.GetProjectionMatrix();

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
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
    if (m_pManager == nullptr || !m_pManager->Exists(handle))
    {
        return;
    }

    for (auto& info : m_loopEffects)
    {
        if (info.handle == handle)
        {
            info.position = pos;
            m_pManager->SetLocation(info.handle, pos.x, pos.y, pos.z);
            return;
        }
    }

    m_pManager->SetLocation(handle, pos.x, pos.y, pos.z);
}

//=============================
//エフェクトの向きの更新処理
//============================
void EffekseerManager::SetEffectRotation(Effekseer::Handle handle, DirectX::XMFLOAT3 rotation)
{
    if (m_pManager == nullptr || !m_pManager->Exists(handle))
    {
        return;
    }

    float radX = rotation.x * (DirectX::XM_PI / 180.0f);
    float radY = rotation.y * (DirectX::XM_PI / 180.0f);
    float radZ = rotation.z * (DirectX::XM_PI / 180.0f);

    m_pManager->SetRotation(handle, radX, radY, radZ);
    for (auto& info : m_loopEffects)
    {
        if (info.handle == handle)
        {
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
    if (m_pManager == nullptr || !m_pManager->Exists(handle))
    {
        return;
    }

    m_pManager->SetScale(handle, scale.x, scale.y, scale.z);
    for (auto& info : m_loopEffects)
    {
        if (info.handle == handle)
        {
            info.scale = scale;
            break;
        }
    }
}

//=====================
//更新処理
//=====================
void EffekseerManager::Update()
{
    if (m_pManager == nullptr)
    {
        return;
    }

    Effekseer::Manager::LayerParameter efkLayerParm;
    Effekseer::Matrix44 invViewMat;
    Effekseer::Matrix44::Inverse(invViewMat, m_efkViewMat);
    efkLayerParm.ViewerPosition = Effekseer::Vector3D(invViewMat.Values[3][0], invViewMat.Values[3][1], invViewMat.Values[3][2]);
    m_pManager->SetLayerParameter(0, efkLayerParm);

    m_pManager->Update();

    time++;

    for (auto it = m_loopEffects.begin(); it != m_loopEffects.end(); ++it)
    {
        if (!m_pManager->Exists(it->handle))
        {
            auto effIt = m_effects.find(it->effectName);
            if (effIt != m_effects.end() && effIt->second != nullptr)
            {
                it->handle = m_pManager->Play(effIt->second, it->position.x, it->position.y, it->position.z);
                if (m_pManager->Exists(it->handle))
                {
                    m_pManager->SetScale(it->handle, it->scale.x, it->scale.y, it->scale.z);
                    float radX = it->rotation.x * (DirectX::XM_PI / 180.0f);
                    float radY = it->rotation.y * (DirectX::XM_PI / 180.0f);
                    float radZ = it->rotation.z * (DirectX::XM_PI / 180.0f);
                    m_pManager->SetRotation(it->handle, radX, radY, radZ);
                }
            }
        }
    }
}

//=====================
//描画処理
//=====================
void EffekseerManager::Draw(const Camera& camera)
{
    if (m_pRenderer == nullptr || m_pManager == nullptr)
    {
        return;
    }

    m_pRenderer->SetTime(time / 60.0f);
    SetCamera(camera);
    m_pRenderer->BeginRendering();

    Effekseer::Manager::DrawParameter drawParameter;
    drawParameter.ZNear = camera.nearZ;
    drawParameter.ZFar = camera.farZ;
    drawParameter.ViewProjectionMatrix = m_pRenderer->GetCameraProjectionMatrix();
    m_pManager->Draw(drawParameter);

    m_pRenderer->EndRendering();
}

