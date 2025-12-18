#include "systems/SoundSystem.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <windows.h>
#include <xaudio2.h>
#include <debugapi.h>
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

//======================================================================
// Windows / XAudio2 実装
//======================================================================
namespace GameSound {
// ---- グローバル状態 ----
static std::atomic_bool g_enabled{true};
static Config g_cfg{};
static VirtualBeepHandler g_visual;

// ---- 簡易 WAV（16bit PCM）----
struct PcmData {
    WAVEFORMATEX fmt{};
    std::vector<uint8_t> data;
};

static std::wstring ResolveAssetPath(const wchar_t *rel) {
    wchar_t exePath[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring base = (n > 0) ? std::wstring(exePath, exePath + n) : L"";
    size_t pos = base.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        base.resize(pos);
    // Prefer executable directory; if rel is absolute, return as-is
    if (rel && ((rel[0] == L'/' || rel[0] == L'\\') || (wcslen(rel) > 1 && rel[1] == L':'))) {
        return std::wstring(rel);
    }
    std::wstring full = base;
    full += L"\\";
    full += rel ? rel : L"";
    return full;
}

static bool LoadPcmWav(const wchar_t *path, PcmData &out) {
    out = PcmData{};
    std::wstring full = ResolveAssetPath(path);
    FILE *fp = nullptr;
    _wfopen_s(&fp, full.c_str(), L"rb");
    if (!fp)
        return false;

    auto rd = [&](void *p, size_t n) -> bool { return fread(p, 1, n, fp) == n; };

    struct RIFF {
        char id[4];
        uint32_t size;
        char wave[4];
    } riff{};
    if (!rd(&riff, sizeof(riff)) || std::memcmp(riff.id, "RIFF", 4) || std::memcmp(riff.wave, "WAVE", 4)) {
        fclose(fp);
        return false;
    }

    bool gotFmt = false, gotData = false;
    std::vector<uint8_t> databuf;
    WAVEFORMATEX fmt{};
    while (!gotFmt || !gotData) {
        struct CH {
            char id[4];
            uint32_t size;
        } ch{};
        if (!rd(&ch, sizeof(ch)))
            break;

        if (!std::memcmp(ch.id, "fmt ", 4)) {
            if (ch.size < sizeof(WAVEFORMATEX)) {
                std::vector<uint8_t> tmp(ch.size);
                if (!rd(tmp.data(), ch.size)) {
                    fclose(fp);
                    return false;
                }
                std::memset(&fmt, 0, sizeof(fmt));
                std::memcpy(&fmt, tmp.data(), std::min<size_t>(tmp.size(), sizeof(WAVEFORMATEX)));
            } else {
                if (!rd(&fmt, sizeof(fmt))) {
                    fclose(fp);
                    return false;
                }
                if (ch.size > sizeof(fmt))
                    fseek(fp, (long) (ch.size - sizeof(fmt)), SEEK_CUR);
            }
            gotFmt = true;
        } else if (!std::memcmp(ch.id, "data", 4)) {
            databuf.resize(ch.size);
            if (!rd(databuf.data(), ch.size)) {
                fclose(fp);
                return false;
            }
            gotData = true;
        } else {
            fseek(fp, ch.size, SEEK_CUR);
        }
    }
    fclose(fp);

    if (!gotFmt || !gotData)
        return false;
    if (fmt.wFormatTag != WAVE_FORMAT_PCM || fmt.wBitsPerSample != 16)
        return false;

    out.fmt = fmt;
    out.data = std::move(databuf);
    return true;
}

// ---- アセットパス（rom/sounds下）----
static const wchar_t *kSePath(SeId id) {
    switch (id) {
        case SeId::Ok:
            return L"rom/sounds/ok.wav";
        case SeId::Ng:
            return L"rom/sounds/ng.wav";
        case SeId::Build:
            return L"rom/sounds/build.wav";
        case SeId::Refund:
            return L"rom/sounds/ref.wav";
        case SeId::Tip:
            return L"rom/sounds/Tip.wav";
        case SeId::Reward:
            return L"rom/sounds/rew.wav";
        case SeId::Warn:
            return L"rom/sounds/wa.wav";
        case SeId::LifeDown:
            return L"rom/sounds/down.wav";
        case SeId::Alert:
            return L"rom/sounds/Alert.wav";
        case SeId::Enter:
            return L"rom/sounds/enter.wav";
        case SeId::Exit:
            return L"rom/sounds/exit.wav";
        default:
            return nullptr;
    }
}

static const wchar_t *kBgmPath(BgmId id) {
    switch (id) {
        case BgmId::MainTheme:
            return L"rom/sounds/main.wav";
        case BgmId::Calm:
            return L"rom/sounds/Calm.wav";
        case BgmId::Tense:
            return L"rom/sounds/tense.wav";
        default:
            return nullptr;
    }
}

// ---- XAudio2 状態 ----
struct VoiceHandle {
    IXAudio2SourceVoice *v = nullptr;
    uint64_t ticket = 0;
};

static IXAudio2 *g_xa = nullptr;
static IXAudio2MasteringVoice *g_master = nullptr;

static std::unordered_map<SeId, PcmData> g_seCache;
static std::vector<VoiceHandle> g_seVoices;
static const size_t kMaxSeVoices = 16;
static uint64_t g_ticketCtr = 0;

static BgmId g_bgmNow = BgmId::None;
static PcmData g_bgmPcm;
static IXAudio2SourceVoice *g_bgmVoice = nullptr;

static std::mutex g_mutex;

// ---- 内部ユーティリティ ----
static bool IsStopped(IXAudio2SourceVoice *v) {
    XAUDIO2_VOICE_STATE st{};
    v->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return (st.BuffersQueued == 0);
}

static float FinalBgmVolume() {
    const float mv = std::clamp(g_cfg.masterVolume, 0.0f, 1.0f);
    const float bv = std::clamp(g_cfg.bgmVolume, 0.0f, 1.0f);
    return std::clamp(mv * bv, 0.0f, 1.0f);
}

// ---- ライフサイクル ----
bool Init() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_xa)
        return true;

    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
        // 既に初期化済みでも続行
    }
    if (FAILED(XAudio2Create(&g_xa, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        return false;
    }
    if (FAILED(g_xa->CreateMasteringVoice(&g_master))) {
        g_xa->Release();
        g_xa = nullptr;
        return false;
    }
    g_seCache.clear();
    g_seVoices.clear();
    g_bgmNow = BgmId::None;
    g_bgmPcm = PcmData{};
    g_bgmVoice = nullptr;
    return true;
}

void Uninit() {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_bgmVoice) {
        g_bgmVoice->Stop(0);
        g_bgmVoice->DestroyVoice();
        g_bgmVoice = nullptr;
    }
    g_bgmPcm = PcmData{};
    g_bgmNow = BgmId::None;

    for (auto &vh : g_seVoices) {
        if (vh.v) {
            vh.v->Stop(0);
            vh.v->DestroyVoice();
        }
    }
    g_seVoices.clear();
    g_seCache.clear();

    if (g_master) {
        g_master->DestroyVoice();
        g_master = nullptr;
    }
    if (g_xa) {
        g_xa->Release();
        g_xa = nullptr;
    }
    CoUninitialize();
}

void Update(float /*dt*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_xa)
        return;

    for (auto &vh : g_seVoices) {
        if (vh.v && IsStopped(vh.v)) {
            vh.v->DestroyVoice();
            vh.v = nullptr;
        }
    }
    g_seVoices.erase(
        std::remove_if(g_seVoices.begin(), g_seVoices.end(),
                       [](const VoiceHandle &h) { return h.v == nullptr; }),
        g_seVoices.end());
}

// ---- 設定 ----
void SetConfig(const Config &cfg) {
    g_cfg = cfg;
    g_enabled.store(g_cfg.enabled);
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_bgmVoice) {
        g_bgmVoice->SetVolume(FinalBgmVolume());
    }
}

void SetEnabled(bool enabled) {
    g_enabled.store(enabled);
    g_cfg.enabled = enabled;
}

bool GetEnabled() {
    return g_enabled.load();
}

Config GetConfig() {
    return g_cfg;
}

void SetVirtualBeepHandler(VirtualBeepHandler cb) {
    g_visual = std::move(cb);
}

void SetBGMVolume(float v) {
    g_cfg.bgmVolume = std::clamp(v, 0.0f, 1.0f);
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_bgmVoice)
        g_bgmVoice->SetVolume(FinalBgmVolume());
}

// ---- 再生（SE）----
static bool EnsureSeLoaded(SeId id) {
    if (id == SeId::None)
        return false;
    if (g_seCache.find(id) != g_seCache.end())
        return true;

    const wchar_t *path = kSePath(id);
    if (!path)
        return false;

    PcmData pcm{};
    if (!LoadPcmWav(path, pcm))
        return false;
    g_seCache[id] = std::move(pcm);
    return true;
}

bool PlaySE(SeId id) {
    if (!g_enabled.load())
        return true;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_xa || id == SeId::None)
        return false;
    if (!EnsureSeLoaded(id))
        return false;

    const PcmData &pcm = g_seCache[id];

    // 最大数を超える場合は最古を破棄
    if (g_seVoices.size() >= kMaxSeVoices) {
        auto it = std::min_element(g_seVoices.begin(), g_seVoices.end(),
                                   [](const VoiceHandle &a, const VoiceHandle &b) { return a.ticket < b.ticket; });
        if (it != g_seVoices.end() && it->v) {
            it->v->Stop(0);
            it->v->DestroyVoice();
        }
        if (it != g_seVoices.end())
            g_seVoices.erase(it);
    }

    IXAudio2SourceVoice *v = nullptr;
    if (FAILED(g_xa->CreateSourceVoice(&v, &pcm.fmt)))
        return false;

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = (UINT32) pcm.data.size();
    buf.pAudioData = pcm.data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(v->SubmitSourceBuffer(&buf))) {
        v->DestroyVoice();
        return false;
    }
    // SE は seVolume * master を適用
    const float mv = std::clamp(g_cfg.masterVolume, 0.0f, 1.0f);
    const float sv = std::clamp(g_cfg.seVolume, 0.0f, 1.0f);
    v->SetVolume(std::clamp(mv * sv, 0.0f, 1.0f));
    if (FAILED(v->Start(0))) {
        v->DestroyVoice();
        return false;
    }

    g_seVoices.push_back(VoiceHandle{v, ++g_ticketCtr});
    return true;
}

// ---- 再生（BGM）----
bool PlayBGM(BgmId id, bool loop) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_xa) {
        OutputDebugStringW(L"[GameSound] ERROR: PlayBGM called but XAudio2 not initialized.\n");
        return false;
    }

    if (id == g_bgmNow) {
        return true; // 同一曲なら何もしない
    }

    if (g_bgmVoice) {
        g_bgmVoice->Stop(0);
        g_bgmVoice->DestroyVoice();
        g_bgmVoice = nullptr;
    }

    const wchar_t *path = kBgmPath(id);
    if (!path) {
        g_bgmNow = BgmId::None;
        g_bgmPcm = PcmData{};
        OutputDebugStringW(L"[GameSound] INFO: Stopping BGM.\n");
        return true;
    }

    wchar_t msg[512];
    swprintf_s(msg, L"[GameSound] INFO: Loading BGM '%s'\n", path);
    OutputDebugStringW(msg);

    PcmData pcm{};
    if (!LoadPcmWav(path, pcm)) {
        swprintf_s(msg, L"[GameSound] CRITICAL: LoadPcmWav FAILED for '%s' (must be 16-bit PCM).\n", path);
        OutputDebugStringW(msg);
        g_bgmNow = BgmId::None;
        g_bgmPcm = PcmData{};
        return false;
    }

    IXAudio2SourceVoice *v = nullptr;
    if (FAILED(g_xa->CreateSourceVoice(&v, &pcm.fmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr))) {
        OutputDebugStringW(L"[GameSound] ERROR: CreateSourceVoice failed.\n");
        return false;
    }

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = (UINT32) pcm.data.size();
    buf.pAudioData = pcm.data.data();
    if (loop) {
        buf.LoopBegin = 0;
        buf.LoopLength = 0;
        buf.LoopCount = XAUDIO2_LOOP_INFINITE;
        buf.Flags = 0;
    } else {
        buf.Flags = XAUDIO2_END_OF_STREAM;
    }

    if (FAILED(v->SubmitSourceBuffer(&buf))) {
        OutputDebugStringW(L"[GameSound] ERROR: SubmitSourceBuffer failed.\n");
        v->DestroyVoice();
        return false;
    }

    const float finalVol = FinalBgmVolume();
    swprintf_s(msg, L"[GameSound] INFO: BGM volume set to %.2f (master*bgm)\n", finalVol);
    OutputDebugStringW(msg);
    v->SetVolume(finalVol);

    if (FAILED(v->Start(0))) {
        OutputDebugStringW(L"[GameSound] ERROR: Voice Start failed.\n");
        v->DestroyVoice();
        return false;
    }

    OutputDebugStringW(L"[GameSound] SUCCESS: PlayBGM OK.\n");

    g_bgmNow = id;
    g_bgmPcm = std::move(pcm);
    g_bgmVoice = v;
    return true;
}

void StopBGM() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_bgmVoice) {
        g_bgmVoice->Stop(0);
        g_bgmVoice->DestroyVoice();
        g_bgmVoice = nullptr;
    }
    g_bgmPcm = PcmData{};
    g_bgmNow = BgmId::None;
}

} // namespace GameSound
