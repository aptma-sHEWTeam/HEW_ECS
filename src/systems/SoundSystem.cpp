//#include "systems/SoundSystem.h"
//#include "scenes/StageConfig.h"
//
//#include <algorithm>
//#include <atomic>
//#include <mutex>
//#include <string>
//#include <unordered_map>
//#include <vector>
//
//#include <windows.h>
//#include <xaudio2.h>
//#include <debugapi.h>
//
//#include <mfapi.h>
//#include <mfidl.h>
//#include <mfreadwrite.h>
//#include <wrl/client.h>
//
//#pragma comment(lib, "mfplat.lib")
//#pragma comment(lib, "mfreadwrite.lib")
//#pragma comment(lib, "mfuuid.lib")
//
//#pragma comment(lib, "xaudio2.lib")
//#pragma comment(lib, "ole32.lib")
//
////======================================================================
//// Windows / XAudio2 実装
////======================================================================
//namespace GameSound {
//// ---- グローバル状態 ----
//static std::atomic_bool g_enabled{true};
//static Config g_cfg{};
//static VirtualBeepHandler g_visual;
//
//// ---- 簡易 WAV（16bit PCM）----
//struct PcmData {
//    WAVEFORMATEX fmt{};
//    std::vector<uint8_t> data;
//};
//
//static std::wstring ToWString(const std::string& str) {
//    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),-1, nullptr, 0);
//    std::wstring wstr(size, L'/0');
//    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
//    return wstr.c_str();
//}
//
//
//
//static std::wstring ResolveAssetPath(const wchar_t *rel) {
//    wchar_t exePath[MAX_PATH];
//    DWORD n = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
//    std::wstring base = (n > 0) ? std::wstring(exePath, exePath + n) : L"";
//    size_t pos = base.find_last_of(L"\\/");
//    if (pos != std::wstring::npos)
//        base.resize(pos);
//    // Prefer executable directory; if rel is absolute, return as-is
//    if (rel && ((rel[0] == L'/' || rel[0] == L'\\') || (wcslen(rel) > 1 && rel[1] == L':'))) {
//        return std::wstring(rel);
//    }
//    std::wstring full = base;
//    full += L"\\";
//    full += rel ? rel : L"";
//    return full;
//}
//
//static bool LoadPcmWav(const wchar_t *path, PcmData &out) {
//    out = PcmData{};
//    std::wstring full = ResolveAssetPath(path);
//    FILE *fp = nullptr;
//    _wfopen_s(&fp, full.c_str(), L"rb");
//    if (!fp)
//        return false;
//
//    auto rd = [&](void *p, size_t n) -> bool { return fread(p, 1, n, fp) == n; };
//
//    struct RIFF {
//        char id[4];
//        uint32_t size;
//        char wave[4];
//    } riff{};
//    if (!rd(&riff, sizeof(riff)) || std::memcmp(riff.id, "RIFF", 4) || std::memcmp(riff.wave, "WAVE", 4)) {
//        fclose(fp);
//        return false;
//    }
//
//    bool gotFmt = false, gotData = false;
//    std::vector<uint8_t> databuf;
//    WAVEFORMATEX fmt{};
//    while (!gotFmt || !gotData) {
//        struct CH {
//            char id[4];
//            uint32_t size;
//        } ch{};
//        if (!rd(&ch, sizeof(ch)))
//            break;
//
//        if (!std::memcmp(ch.id, "fmt ", 4)) {
//            if (ch.size < sizeof(WAVEFORMATEX)) {
//                std::vector<uint8_t> tmp(ch.size);
//                if (!rd(tmp.data(), ch.size)) {
//                    fclose(fp);
//                    return false;
//                }
//                std::memset(&fmt, 0, sizeof(fmt));
//                std::memcpy(&fmt, tmp.data(), std::min<size_t>(tmp.size(), sizeof(WAVEFORMATEX)));
//            } else {
//                if (!rd(&fmt, sizeof(fmt))) {
//                    fclose(fp);
//                    return false;
//                }
//                if (ch.size > sizeof(fmt))
//                    fseek(fp, (long) (ch.size - sizeof(fmt)), SEEK_CUR);
//            }
//            gotFmt = true;
//        } else if (!std::memcmp(ch.id, "data", 4)) {
//            databuf.resize(ch.size);
//            if (!rd(databuf.data(), ch.size)) {
//                fclose(fp);
//                return false;
//            }
//            gotData = true;
//        } else {
//            fseek(fp, ch.size, SEEK_CUR);
//        }
//    }
//    fclose(fp);
//
//    if (!gotFmt || !gotData)
//        return false;
//    if (fmt.wFormatTag != WAVE_FORMAT_PCM || fmt.wBitsPerSample != 16)
//        return false;
//
//    out.fmt = fmt;
//    out.data = std::move(databuf);
//    return true;
//}
//
//bool LoadAudioFileWithMF(const wchar_t* path, PcmData& out) {
//    Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
//    HRESULT hr = MFCreateSourceReaderFromURL(path, nullptr, &pReader);
//    if (FAILED(hr)) return false;
//
//
//    //出力形式を16bitPCMに設定
//    Microsoft::WRL::ComPtr<IMFMediaType> pTargetType;
//    MFCreateMediaType(&pTargetType);
//    pTargetType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
//    pTargetType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
//    pTargetType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
//    hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,nullptr,pTargetType.Get());
//    if (FAILED(hr)) return false;
//
//    //読み込んだファイルの種類を判別している
//    Microsoft::WRL::ComPtr<IMFMediaType> pCurrentType;
//    pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pCurrentType);
//
//    WAVEFORMATEX *pWfx = nullptr;
//    UINT32 wfxSize = 0;
//    MFCreateWaveFormatExFromMFMediaType(pCurrentType.Get(),&pWfx, &wfxSize);
//    out.fmt = *pWfx;
//    CoTaskMemFree(pWfx);
//
//    //データの読み込み
//    out.data.clear();
//    while (true) {
//        DWORD flags = 0;
//        Microsoft::WRL::ComPtr<IMFSample> pSample;
//        hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &pSample);
//        //読み込みに失敗したら処理を止める
//        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) break;
//
//        //サンプルをデコードしてベクターの末尾に追加
//        if (pSample) {
//            Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
//            pSample->ConvertToContiguousBuffer(&pBuffer);
//
//            BYTE *pAudioData = nullptr;
//            DWORD cbBuffer = 0;
//            pBuffer->Lock(&pAudioData, nullptr, &cbBuffer);
//            
//            size_t currentSize = out.data.size();
//            out.data.resize(currentSize + cbBuffer);
//            memcpy(&out.data[currentSize], pAudioData, cbBuffer);
//
//            pBuffer->Unlock();
//        }
//    }
//
//    return !out.data.empty();
//}
//
//// ---- アセットパス（rom/sounds下）----
//
//static std::wstring kBgmPath(BgmId id) {
//    switch (id) {
//        case BgmId::MainTheme:
//            //BGMが来てから
//            return L"Assets";
//        default:
//            return L"";
//    }
//}
//
//static std::wstring kSePath(SeId id) {
//    switch (id) {
//        case SeId::Cold:        return ToWString(cfg_ColdMP3Pass.Get());
//        case SeId::Collide:     return ToWString(cfg_CollideMP3Pass.Get());
//        case SeId::Death:       return ToWString(cfg_DeathMP3Pass.Get());
//        case SeId::Drift:       return ToWString(cfg_DriftMP3Pass.Get());
//        case SeId::Fire1:       return ToWString(cfg_Fire1MP3Pass.Get());
//        case SeId::Fire2:       return ToWString(cfg_Fire2MP3Pass.Get());
//        case SeId::Fire3:       return ToWString(cfg_Fire3MP3Pass.Get());
//        case SeId::Siren:       return ToWString(cfg_SirenMP3Pass.Get());
//        case SeId::Start:       return ToWString(cfg_StartMP3Pass.Get());
//        case SeId::WarpDown:    return ToWString(cfg_WarpDownMP3Pass.Get());
//        case SeId::WarpUp:      return ToWString(cfg_WarpUpMP3Pass.Get());
//    }
//}
//
//bool PlayPcm(IXAudio2SourceVoice* pSourceVoice, const PcmData& pcm, bool loop) {
//    if (!pSourceVoice) return false;
//
//    //再生中なら止める
//    pSourceVoice->Stop(0);
//    pSourceVoice->FlushSourceBuffers();
//
//    //バッファの設定
//    XAUDIO2_BUFFER buf = {0};
//    buf.pAudioData = pcm.data.data();
//    buf.AudioBytes = static_cast<UINT32>(pcm.data.size());
//    buf.Flags = XAUDIO2_END_OF_STREAM;
//
//    //ループ設定
//    buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
//
//    //ボイスにバッファを送信
//    HRESULT hr = pSourceVoice->SubmitSourceBuffer(&buf);
//    if (FAILED(hr)) {
//        OutputDebugStringW(L"SubmitSourceBuffer 失敗/n");
//        return false;
//    }
//
//    //再生開始
//    hr = pSourceVoice->Start(0);
//    return SUCCEEDED(hr);
//}
//
//bool PlaySeFromConfig(SeId id) {
//    std::wstring path = kSePath(id);
//    if (path.empty())
//        return false;
//
//    PcmData tempPcm;
//    if (!LoadAudioFileWithMF(ResolveAssetPath(path.c_str()).c_str(), tempPcm)) {
//        return false;
//    }
//
//
//}
//
//// ---- XAudio2 状態 ----
//struct VoiceHandle {
//    IXAudio2SourceVoice *v = nullptr;
//    uint64_t ticket = 0;
//};
//
//static IXAudio2 *g_xa = nullptr;
//static IXAudio2MasteringVoice *g_master = nullptr;
//
//static std::unordered_map<SeId, PcmData> g_seCache;
//static std::vector<VoiceHandle> g_seVoices;
//static const size_t kMaxSeVoices = 16;
//static uint64_t g_ticketCtr = 0;
//
//static BgmId g_bgmNow = BgmId::None;
//static PcmData g_bgmPcm;
//static IXAudio2SourceVoice *g_bgmVoice = nullptr;
//
//static std::mutex g_mutex;
//
//// ---- 内部ユーティリティ ----
//static bool IsStopped(IXAudio2SourceVoice *v) {
//    XAUDIO2_VOICE_STATE st{};
//    v->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
//    return (st.BuffersQueued == 0);
//}
//
//static float FinalBgmVolume() {
//    const float mv = std::clamp(g_cfg.masterVolume, 0.0f, 1.0f);
//    const float bv = std::clamp(g_cfg.bgmVolume, 0.0f, 1.0f);
//    return std::clamp(mv * bv, 0.0f, 1.0f);
//}
//
//// ---- ライフサイクル ----
//bool Init() {
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (g_xa)
//        return true;
//
//    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
//        // 既に初期化済みでも続行
//    }
//    if (FAILED(XAudio2Create(&g_xa, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
//        return false;
//    }
//    if (FAILED(g_xa->CreateMasteringVoice(&g_master))) {
//        g_xa->Release();
//        g_xa = nullptr;
//        return false;
//    }
//    g_seCache.clear();
//    g_seVoices.clear();
//    g_bgmNow = BgmId::None;
//    g_bgmPcm = PcmData{};
//    g_bgmVoice = nullptr;
//
//    HRESULT hr = MFStartup(MF_VERSION);
//
//    return SUCCEEDED(hr);
//}
//
//void Uninit() {
//    std::lock_guard<std::mutex> lock(g_mutex);
//
//    if (g_bgmVoice) {
//        g_bgmVoice->Stop(0);
//        g_bgmVoice->DestroyVoice();
//        g_bgmVoice = nullptr;
//    }
//    g_bgmPcm = PcmData{};
//    g_bgmNow = BgmId::None;
//
//    for (auto &vh : g_seVoices) {
//        if (vh.v) {
//            vh.v->Stop(0);
//            vh.v->DestroyVoice();
//        }
//    }
//    g_seVoices.clear();
//    g_seCache.clear();
//
//    if (g_master) {
//        g_master->DestroyVoice();
//        g_master = nullptr;
//    }
//    if (g_xa) {
//        g_xa->Release();
//        g_xa = nullptr;
//    }
//    CoUninitialize();
//    MFShutdown();
//}
//
//void Update(float /*dt*/) {
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (!g_xa)
//        return;
//
//    for (auto &vh : g_seVoices) {
//        if (vh.v && IsStopped(vh.v)) {
//            vh.v->DestroyVoice();
//            vh.v = nullptr;
//        }
//    }
//    g_seVoices.erase(
//        std::remove_if(g_seVoices.begin(), g_seVoices.end(),
//                       [](const VoiceHandle &h) { return h.v == nullptr; }),
//        g_seVoices.end());
//}
//
//// ---- 設定 ----
//void SetConfig(const Config &cfg) {
//    g_cfg = cfg;
//    g_enabled.store(g_cfg.enabled);
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (g_bgmVoice) {
//        g_bgmVoice->SetVolume(FinalBgmVolume());
//    }
//}
//
//void SetEnabled(bool enabled) {
//    g_enabled.store(enabled);
//    g_cfg.enabled = enabled;
//}
//
//bool GetEnabled() {
//    return g_enabled.load();
//}
//
//Config GetConfig() {
//    return g_cfg;
//}
//
//void SetVirtualBeepHandler(VirtualBeepHandler cb) {
//    g_visual = std::move(cb);
//}
//
//void SetBGMVolume(float v) {
//    g_cfg.bgmVolume = std::clamp(v, 0.0f, 1.0f);
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (g_bgmVoice)
//        g_bgmVoice->SetVolume(FinalBgmVolume());
//}
//
//// ---- 再生（SE）----
//static bool EnsureSeLoaded(SeId id) {
//    if (id == SeId::None)
//        return false;
//    if (g_seCache.find(id) != g_seCache.end())
//        return true;
//
//    const wchar_t path = kSePath(id);
//    if (!path)
//        return false;
//
//    PcmData pcm{};
//    if (!LoadPcmWav(path, pcm))
//        return false;
//    g_seCache[id] = std::move(pcm);
//    return true;
//}
//
//bool PlaySE(SeId id) {
//    if (!g_enabled.load())
//        return true;
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (!g_xa || id == SeId::None)
//        return false;
//    if (!EnsureSeLoaded(id))
//        return false;
//
//    const PcmData &pcm = g_seCache[id];
//
//    // 最大数を超える場合は最古を破棄
//    if (g_seVoices.size() >= kMaxSeVoices) {
//        auto it = std::min_element(g_seVoices.begin(), g_seVoices.end(),
//                                   [](const VoiceHandle &a, const VoiceHandle &b) { return a.ticket < b.ticket; });
//        if (it != g_seVoices.end() && it->v) {
//            it->v->Stop(0);
//            it->v->DestroyVoice();
//        }
//        if (it != g_seVoices.end())
//            g_seVoices.erase(it);
//    }
//
//    IXAudio2SourceVoice *v = nullptr;
//    if (FAILED(g_xa->CreateSourceVoice(&v, &pcm.fmt)))
//        return false;
//
//    XAUDIO2_BUFFER buf{};
//    buf.AudioBytes = (UINT32) pcm.data.size();
//    buf.pAudioData = pcm.data.data();
//    buf.Flags = XAUDIO2_END_OF_STREAM;
//
//    if (FAILED(v->SubmitSourceBuffer(&buf))) {
//        v->DestroyVoice();
//        return false;
//    }
//    // SE は seVolume * master を適用
//    const float mv = std::clamp(g_cfg.masterVolume, 0.0f, 1.0f);
//    const float sv = std::clamp(g_cfg.seVolume, 0.0f, 1.0f);
//    v->SetVolume(std::clamp(mv * sv, 0.0f, 1.0f));
//    if (FAILED(v->Start(0))) {
//        v->DestroyVoice();
//        return false;
//    }
//
//    g_seVoices.push_back(VoiceHandle{v, ++g_ticketCtr});
//    return true;
//}
//
//// ---- 再生（BGM）----
//bool PlayBGM(BgmId id, bool loop) {
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (!g_xa) {
//        OutputDebugStringW(L"[GameSound] ERROR: PlayBGM called but XAudio2 not initialized.\n");
//        return false;
//    }
//
//    if (id == g_bgmNow) {
//        return true; // 同一曲なら何もしない
//    }
//
//    if (g_bgmVoice) {
//        g_bgmVoice->Stop(0);
//        g_bgmVoice->DestroyVoice();
//        g_bgmVoice = nullptr;
//    }
//
//    const wchar_t *path = kBgmPath(id);
//    if (!path) {
//        g_bgmNow = BgmId::None;
//        g_bgmPcm = PcmData{};
//        OutputDebugStringW(L"[GameSound] INFO: Stopping BGM.\n");
//        return true;
//    }
//
//    wchar_t msg[512];
//    swprintf_s(msg, L"[GameSound] INFO: Loading BGM '%s'\n", path);
//    OutputDebugStringW(msg);
//
//    PcmData pcm{};
//    if (!LoadPcmWav(path, pcm)) {
//        swprintf_s(msg, L"[GameSound] CRITICAL: LoadPcmWav FAILED for '%s' (must be 16-bit PCM).\n", path);
//        OutputDebugStringW(msg);
//        g_bgmNow = BgmId::None;
//        g_bgmPcm = PcmData{};
//        return false;
//    }
//
//    IXAudio2SourceVoice *v = nullptr;
//    if (FAILED(g_xa->CreateSourceVoice(&v, &pcm.fmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr))) {
//        OutputDebugStringW(L"[GameSound] ERROR: CreateSourceVoice failed.\n");
//        return false;
//    }
//
//    XAUDIO2_BUFFER buf{};
//    buf.AudioBytes = (UINT32) pcm.data.size();
//    buf.pAudioData = pcm.data.data();
//    if (loop) {
//        buf.LoopBegin = 0;
//        buf.LoopLength = 0;
//        buf.LoopCount = XAUDIO2_LOOP_INFINITE;
//        buf.Flags = 0;
//    } else {
//        buf.Flags = XAUDIO2_END_OF_STREAM;
//    }
//
//    if (FAILED(v->SubmitSourceBuffer(&buf))) {
//        OutputDebugStringW(L"[GameSound] ERROR: SubmitSourceBuffer failed.\n");
//        v->DestroyVoice();
//        return false;
//    }
//
//    const float finalVol = FinalBgmVolume();
//    swprintf_s(msg, L"[GameSound] INFO: BGM volume set to %.2f (master*bgm)\n", finalVol);
//    OutputDebugStringW(msg);
//    v->SetVolume(finalVol);
//
//    if (FAILED(v->Start(0))) {
//        OutputDebugStringW(L"[GameSound] ERROR: Voice Start failed.\n");
//        v->DestroyVoice();
//        return false;
//    }
//
//    OutputDebugStringW(L"[GameSound] SUCCESS: PlayBGM OK.\n");
//
//    g_bgmNow = id;
//    g_bgmPcm = std::move(pcm);
//    g_bgmVoice = v;
//    return true;
//}
//
//void StopBGM() {
//    std::lock_guard<std::mutex> lock(g_mutex);
//    if (g_bgmVoice) {
//        g_bgmVoice->Stop(0);
//        g_bgmVoice->DestroyVoice();
//        g_bgmVoice = nullptr;
//    }
//    g_bgmPcm = PcmData{};
//    g_bgmNow = BgmId::None;
//}
//
//} // namespace GameSound
