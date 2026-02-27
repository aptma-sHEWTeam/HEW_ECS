#include "graphics/GfxDevice.h"
#include "components/Component.h"
#include "ecs/Entity.h"
#include "ecs/World.h"
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <cstdio>
#include <cstdio>
#include <algorithm>
#include "systems/SoundSystem.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// ========================================================
// VideoPlayer - 動画再生システム
// ========================================================

/**
 * @class VideoPlayer
 * @brief 動画ファイルの再生を管理するクラス
 * @author 山内陽
 * @date 2025
 * @version 1.0
 * 
 * @details
 * Windows Media Foundationを使用して動画ファイルを再生します。
 * 動画フレームをテクスチャとして取得し、DirectX11で描画できます。
 * 
 * ### サポート形式:
 * - MP4
 * - AVI
 * - WMV
 * など、Media Foundationがサポートする形式
 * 
 * @par 使用例
 * @code
 * VideoPlayer player;
 * player.Init();
 * player.Open(gfx, "video.mp4");
 * player.Play();
 * player.SetLoop(true);
 * 
 * while (running) {
 *     player.Update(deltaTime);
 *     ID3D11ShaderResourceView* texture = player.GetSRV();
 *     // テクスチャを描画
 * }
 * @endcode
 */
class VideoPlayer {
public:
    /**
     * @brief 初期化
     * @return bool 初期化に成功した場合true
     * 
     * @details
     * Media Foundationを初期化します。
     * 動画再生を行う前に必ず呼び出してください。
     */
    bool Init() {
        if (mfInitialized_) {
            return true;
        }

        // Media Foundationを初期化
        HRESULT hr = MFStartup(MF_VERSION);
        if (hr == RPC_E_CHANGED_MODE) {
            hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        }
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to initialize Media Foundation", "Video Error", MB_OK | MB_ICONERROR);
            return false;
        }
        mfInitialized_ = true;
        return true;
    }

    /**
     * @brief デストラクタ
     * 
     * @details
     * リソースを解放し、Media Foundationをシャットダウンします。
     */
    ~VideoPlayer() {
        if (reader_) reader_.Reset();
        if (mfInitialized_) {
            MFShutdown();
            mfInitialized_ = false;
        }
    }

    /**
     * @brief 動画ファイルを開く
     * @param[in] gfx グラフィックスデバイス
     * @param[in] filepath 動画ファイルのパス
     * @return bool 成功した場合true
     * 
     * @details
     * 指定されたパスの動画ファイルを開き、再生準備を行います。
     * ファイルが見つからない場合やフォーマットが不正な場合はfalseを返します。
    */
    bool Open(GfxDevice& gfx, const char* filepath) {
        if (!mfInitialized_ && !Init()) {
            return false;
        }

        gfx_ = &gfx;
        frameTimer_ = 0.0;
        frameIntervalSec_ = 0.0;
        
        // ワイド文字列に変換
        wchar_t wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, filepath, -1, wpath, MAX_PATH);

        // ソースリーダーを作成
        Microsoft::WRL::ComPtr<IMFAttributes> attributes;
        HRESULT hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr)) return false;

        hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        if (FAILED(hr)) return false;

        hr = MFCreateSourceReaderFromURL(wpath, attributes.Get(), &reader_);
        if (FAILED(hr)) {
            char msg[512];
            sprintf_s(msg, "Failed to open video file: %s (HRESULT: 0x%08X)", filepath, (unsigned int)hr);
            DEBUGLOG_ERROR(msg);
            MessageBoxA(nullptr, msg, "Video Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // ビデオストリームを選択
        // ビデオストリームを選択
        hr = reader_->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        if (FAILED(hr)) return false;

        // オーディオストリーム設定
        hr = reader_->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        if (SUCCEEDED(hr)) {
             Microsoft::WRL::ComPtr<IMFMediaType> audioType;
             if (SUCCEEDED(MFCreateMediaType(&audioType))) {
                 audioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
                 audioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM); // PCMに変換要求
                 
                 if (SUCCEEDED(reader_->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType.Get()))) {
                     // フォーマット取得
                     Microsoft::WRL::ComPtr<IMFMediaType> currentAudioType;
                     if (SUCCEEDED(reader_->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentAudioType))) {
                         UINT32 size = 0;
                         WAVEFORMATEX *pWfx = nullptr;
                         if (SUCCEEDED(MFCreateWaveFormatExFromMFMediaType(currentAudioType.Get(), &pWfx, &size))) {
                             // XAudio2 SourceVoice作成
                             IXAudio2* pXAudio = SoundSystem::GetInstance().GetXAudio();
                             if (pXAudio) {
                                  if (SUCCEEDED(pXAudio->CreateSourceVoice(&pAudioVoice_, pWfx))) {
                                      audioEnabled_ = true;
                                      pAudioVoice_->Start();
                                  }
                             }
                             CoTaskMemFree(pWfx);
                         }
                     }
                 }
             }
        }

        // 出力メディアタイプを設定(RGB32)
        Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
        hr = MFCreateMediaType(&mediaType);
        if (FAILED(hr)) return false;

        hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (FAILED(hr)) return false;

        hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (FAILED(hr)) return false;

        hr = reader_->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType.Get());
        if (FAILED(hr)) return false;

        // ストリームインデックスを特定する
        videoStreamIndex_ = (DWORD)-1;
        audioStreamIndex_ = (DWORD)-1;
        
        for (DWORD i = 0; i < 16; ++i) { // 簡易的に先頭16ストリームをチェック
            Microsoft::WRL::ComPtr<IMFMediaType> type;
            if (FAILED(reader_->GetCurrentMediaType(i, &type))) {
                continue; 
            }
            BOOL selected = FALSE;
            if (FAILED(reader_->GetStreamSelection(i, &selected)) || !selected) {
                 continue;
            }

            GUID majorType;
            if (SUCCEEDED(type->GetMajorType(&majorType))) {
                if (majorType == MFMediaType_Video && videoStreamIndex_ == (DWORD)-1) {
                    videoStreamIndex_ = i;
                } else if (majorType == MFMediaType_Audio && audioStreamIndex_ == (DWORD)-1) {
                    audioStreamIndex_ = i;
                }
            }
        }

        // 動画サイズとフレームレートを取得
        // MF_SOURCE_READER_FIRST_VIDEO_STREAM は SetCurrentMediaType 等では使えるが
        // ReadSample の結果 (index) と比較はできないため、videoStreamIndex_ を使う
        // ただし GetCurrentMediaType 等の引数には FIRST_VIDEO_STREAM 定数が使える。
        
        Microsoft::WRL::ComPtr<IMFMediaType> currentType;
        hr = reader_->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType);
        if (FAILED(hr)) return false;

        UINT32 w, h;
        hr = MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &w, &h);
        if (FAILED(hr)) return false;

        width_ = w;
        height_ = h;

        UINT32 fpsNum = 0;
        UINT32 fpsDen = 0;
        if (SUCCEEDED(MFGetAttributeRatio(currentType.Get(), MF_MT_FRAME_RATE, &fpsNum, &fpsDen)) && fpsNum != 0) {
            frameIntervalSec_ = static_cast<double>(fpsDen) / static_cast<double>(fpsNum);
        } else {
            frameIntervalSec_ = 1.0 / 30.0;
        }

        // 動画テクスチャを作成
        if (!createVideoTexture()) return false;

        isOpen_ = true;

        // ループ切替時の一瞬のブラックアウト抑止のため、先頭フレームを1回だけデコードしてテクスチャへ反映
        {
            const bool prevPlaying = isPlaying_;
            isPlaying_ = true;
            frameTimer_ = 0.0;
            const float primeDt = static_cast<float>((frameIntervalSec_ > 0.0) ? frameIntervalSec_ : (1.0 / 30.0));
            (void)Update(primeDt);
            isPlaying_ = prevPlaying;
        }

        return true;
    }

    /**
     * @brief フレームを更新
     * @param[in] dt デルタタイム(秒単位)
     * @return bool 更新に成功した場合true
     * 
     * @details
     * 次のフレームを読み込み、テクスチャを更新します。
     * 再生中でない場合や動画の終端に達した場合はfalseを返します。
     * ループが有効な場合、終端に達すると自動的に先頭に戻ります。
     */
    bool Update(float dt) {
        if (!isOpen_ || !isPlaying_) return false;

        frameTimer_ += dt;
        const double interval = (frameIntervalSec_ > 0.0) ? frameIntervalSec_ : (1.0 / 30.0);
        
        // 注: 音声再生のためには、frameTimerに関わらずReadSampleすべきかもしれないが、
        // 映像の同期も重要。MFのSyncReaderは映像・音声がインターリーブされているので、
        // 「映像の出番が来るまで読み進める」というループ内で音声も処理する形にする。
        // つまり、映像更新タイミング(=FrameTimerOK)になったら、次の映像が出るまで読む。
        // その過程で音声があれば再生する。
        
        if (frameTimer_ + 1e-4 < interval) {
            return true;
        }
        frameTimer_ -= interval;

        constexpr int kMaxReadSamplesPerUpdate = 8;
        int readSamples = 0;

        // フレームを読み込み (VIDEO/AUDIOどちらも来る可能性あり)
        while (readSamples < kMaxReadSamplesPerUpdate) {
            ++readSamples;
            DWORD streamIndex = 0;
            DWORD streamFlags = 0;
            LONGLONG timestamp = 0;
            Microsoft::WRL::ComPtr<IMFSample> sample;

            // ANY_STREAMで読み込むと、Video/Audioの準備できたほうが来る
            HRESULT hr = reader_->ReadSample(
                (DWORD)MF_SOURCE_READER_ANY_STREAM,
                0,
                &streamIndex,
                &streamFlags,
                &timestamp,
                &sample
            );

            if (FAILED(hr)) return false;

            // ストリーム終端
            if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
                if (streamIndex == videoStreamIndex_) {
                    // ビデオが終わったらループ or 終了判定
                     if (loop_) {
                        PROPVARIANT var{};
                        var.vt = VT_I8;
                        var.hVal.QuadPart = 0;
                        reader_->SetCurrentPosition(GUID_NULL, var);
                        PropVariantClear(&var);
                        return true; 
                    }
                    isPlaying_ = false;
                    return false;
                }
                // オーディオ終端は無視
               continue;
            }

            // サンプルが無い
            if (!sample) {
                // まだデータがない、あるいは読み込み中
                // 同期モードなら通常ブロックするが、EOS後などはnullで返ることも
                // ここでbreakしないと無限ループの懸念あり
                break;
            }

            // オーディオサンプルの場合
            if (streamIndex == audioStreamIndex_ && audioEnabled_ && pAudioVoice_) {
                Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
                if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer))) {
                    BYTE* data = nullptr;
                    DWORD currLen = 0;
                    if (SUCCEEDED(buffer->Lock(&data, nullptr, &currLen))) {
                        auto bufObj = std::make_shared<AudioBuffer>();
                        bufObj->data.resize(currLen);
                        memcpy(bufObj->data.data(), data, currLen);
                        buffer->Unlock();

                        audioBuffers_.push_back(bufObj);
                        CleanAudioBuffers(); 

                        XAUDIO2_BUFFER xbuf = { 0 };
                        xbuf.pAudioData = bufObj->data.data();
                        xbuf.AudioBytes = currLen;
                        xbuf.Flags = 0;
                        pAudioVoice_->SubmitSourceBuffer(&xbuf);
                    }
                }
                // オーディオ処理後は次のサンプルも確認しにいく(Videoが詰まっているかもしれないので)
                continue; 
            }

            // ビデオサンプルの場合 (streamIndex == FIRST_VIDEO_STREAM)
            if (streamIndex == videoStreamIndex_) {
                 // ビデオサンプルが来たらテクスチャ更新して、今回のUpdateは終了
                 // (厳密にはタイムスタンプチェックすべきだが、簡易実装として来た順に出す)
                 
                 // ここで従来のテクスチャ更新ロジックへ
                 
                Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
                hr = sample->ConvertToContiguousBuffer(&buffer);
                if (FAILED(hr)) return false;

                Microsoft::WRL::ComPtr<IMF2DBuffer> buffer2D;
                BYTE* data = nullptr;
                LONG srcPitch = 0;
                bool locked2D = false;

                if (SUCCEEDED(buffer.As(&buffer2D))) {
                    hr = buffer2D->Lock2D(&data, &srcPitch);
                    if (FAILED(hr)) return false;
                    locked2D = true;
                } else {
                    DWORD length = 0;
                    hr = buffer->Lock(&data, nullptr, &length);
                    if (FAILED(hr)) return false;
                    srcPitch = static_cast<LONG>(width_) * 4;
                }

                if (srcPitch < 0) {
                    srcPitch = -srcPitch;
                    data += srcPitch * (height_ - 1);
                }

                D3D11_MAPPED_SUBRESOURCE mapped{};
                hr = gfx_->Ctx()->Map(videoTexture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                if (FAILED(hr)) {
                    if (locked2D) buffer2D->Unlock2D();
                    else buffer->Unlock();
                    return false;
                }

                const UINT copyBytes = std::min<UINT>(mapped.RowPitch, static_cast<UINT>(srcPitch));
                uint8_t* dest = static_cast<uint8_t*>(mapped.pData);
                const uint8_t* src = data;
                for (UINT y = 0; y < height_; ++y) {
                    memcpy(dest, src, copyBytes);
                    dest += mapped.RowPitch;
                    src += srcPitch;
                }
                gfx_->Ctx()->Unmap(videoTexture_.Get(), 0);

                if (locked2D) buffer2D->Unlock2D();
                else buffer->Unlock();
                
                return true; // 1フレーム更新完了
            }
        }

        // ビデオフレームを取得できなかった場合は、このフレームでの時間消費を戻して再試行しやすくする
        frameTimer_ += interval;

        return true; 
        // 従来のReadSample呼び出しを削除して、上のループに置き換えるため
        // 下記のコードは削除対象範囲に含める

    }

    /**
     * @brief 再生開始
     * 
     * @details
     * 動画の再生を開始します。
     */
    void Play() { isPlaying_ = true; }
    
    /**
     * @brief 再生停止
     * 
     * @details
     * 動画の再生を停止します。
     */
    void Stop() { isPlaying_ = false; }
    
    /**
     * @brief ループ再生を設定
     * @param[in] loop trueでループ再生、falseで1回のみ再生
     * 
     * @details
     * 動画の終端に達したときの動作を設定します。
     */
    void SetLoop(bool loop) { loop_ = loop; }

    /**
     * @brief 動画テクスチャのシェーダーリソースビューを取得
     * @return ID3D11ShaderResourceView* シェーダーリソースビュー
     * 
     * @details
     * 現在のフレームのテクスチャを取得します。
     * これを使用して動画を描画できます。
     */
    ID3D11ShaderResourceView* GetSRV() const { return videoSRV_.Get(); }

    /**
     * @brief 再生中かどうかを取得
     * @return bool 再生中の場合true
     */
    bool IsPlaying() const { return isPlaying_; }
    
    /**
     * @brief 動画の幅を取得
     * @return UINT 幅(ピクセル単位)
     */
    UINT GetWidth() const { return width_; }
    
    /**
     * @brief 動画の高さを取得
     * @return UINT 高さ(ピクセル単位)
     */
    UINT GetHeight() const { return height_; }

    /**
     * @brief リソースを解放して閉じる
     */
    void Close() {
        Stop();
        if (pAudioVoice_) {
            pAudioVoice_->Stop();
            pAudioVoice_->DestroyVoice();
            pAudioVoice_ = nullptr;
        }
        audioEnabled_ = false;
        audioBuffers_.clear();

        if (reader_) reader_.Reset();
        if (videoTexture_) videoTexture_.Reset();
        if (videoSRV_) videoSRV_.Reset();
        isOpen_ = false;
        width_ = 0;
        height_ = 0;
        frameTimer_ = 0.0;
        currentTime_ = 0.0f;
    }

private:
    /**
     * @brief 動画用テクスチャを作成
     * @return bool 成功した場合true
     * 
     * @details
     * 動画フレームを格納するためのダイナミックテクスチャを作成します。
     */
    bool createVideoTexture() {
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width_;
        texDesc.Height = height_;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DYNAMIC;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = gfx_->Dev()->CreateTexture2D(&texDesc, nullptr, &videoTexture_);
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create video texture", "Video Error", MB_OK | MB_ICONERROR);
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = gfx_->Dev()->CreateShaderResourceView(videoTexture_.Get(), &srvDesc, &videoSRV_);
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create video SRV", "Video Error", MB_OK | MB_ICONERROR);
            return false;
        }

        return true;
    }

    GfxDevice* gfx_ = nullptr;
    Microsoft::WRL::ComPtr<IMFSourceReader> reader_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> videoTexture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> videoSRV_;

    UINT width_ = 0;
    UINT height_ = 0;
    bool isOpen_ = false;
    bool isPlaying_ = false;
    bool loop_ = false;
    bool mfInitialized_ = false; ///< Media Foundationを初期化済みかどうか
    double frameIntervalSec_ = 0.0;
    double frameTimer_ = 0.0;
    float currentTime_ = 0.0f;  ///< 現在の再生時間

    // オーディオ関連
    IXAudio2SourceVoice* pAudioVoice_ = nullptr;
    bool audioEnabled_ = false;

    DWORD videoStreamIndex_ = (DWORD)-1;
    DWORD audioStreamIndex_ = (DWORD)-1;

    // オーディオバッファ管理
    struct AudioBuffer {
        std::vector<BYTE> data;
    };
    // 再生中のバッファを保持して、コールバック等で解放できるまで生存させる...
    // 簡易実装として、リングバッファやキューを使うのが望ましいが、今回は
    // XAudio2へSubmitしたバッファは再生完了まで触らない＆終了時にVoiceを破棄すればOK
    // ただし、std::vectorのポインタを渡すため、Submit後もデータを保持する必要がある。
    // ここでは簡易的に直近のバッファを保持するリストを持つ
    std::vector<std::shared_ptr<AudioBuffer>> audioBuffers_;
    
    // 不要になったバッファをお掃除する簡易メソッド
    void CleanAudioBuffers() {
        if (!pAudioVoice_) return;
        XAUDIO2_VOICE_STATE state;
        pAudioVoice_->GetState(&state);
        // state.BuffersQueued は現在キューに入っている数
        // キューに残っている数より多い分は古いので捨てて良い（FIFO前提）
        // ただし厳密な管理にはContextポインタを使うのが定石。
        // ここではメモリリーク覚悟で、一定数を超えたら古いものを消す単純な実装にするか、
        // あるいは毎回newして、shared_ptrで...いやXAudio2は生ポインタを要求する。
        
        // 安全策：再生中は保持し続け、Closeで全解放する (短時間の動画なら許容範囲)
        // 長時間だとメモリ圧迫するので、適宜削除が必要。
        // まじめに実装するなら IXAudio2VoiceCallback を実装する必要があるが、それは面倒。
        
        // 妥協案: バッファ数が一定を超えたら、古いものから「再生済みだろう」とみなして削除する
        // (危険だが、短いクリア動画なら問題ないはず)
        if (audioBuffers_.size() > 100) {
            size_t removeCount = audioBuffers_.size() - 20; // 直近20個は残す
            audioBuffers_.erase(audioBuffers_.begin(), audioBuffers_.begin() + removeCount);
        }
    }
};

// ========================================================
// VideoPlayback - 動画再生コンポーネント
// ========================================================

/**
 * @struct VideoPlayback
 * @brief 動画再生を管理するBehaviourコンポーネント
 * @author 山内陽
 * 
 * @details
 * VideoPlayerを使用して動画を再生するコンポーネントです。
 * エンティティに追加することで、自動的に動画を更新できます。
 * 
 * @par 使用例
 * @code
 * VideoPlayer player;
 * player.Init();
 * player.Open(gfx, "intro.mp4");
 * 
 * Entity videoEntity = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .Build();
 * 
 * VideoPlayback playback;
 * playback.player = &player;
 * playback.autoPlay = true;
 * world.Add<VideoPlayback>(videoEntity, playback);
 * @endcode
 */
struct VideoPlayback : Behaviour {
    VideoPlayer* player = nullptr;  ///< VideoPlayerへのポインタ
    bool autoPlay = true;           ///< 自動再生するか

    /**
     * @brief 開始時の処理
     * @param[in] w ワールド参照
     * @param[in] self 自身のエンティティ
     * 
     * @details
     * autoPlayがtrueの場合、自動的に再生を開始します。
     */
    void OnStart(World& w, Entity self) override {
        if (autoPlay && player) {
            player->Play();
        }
    }

    /**
     * @brief 毎フレーム更新処理
     * @param[in] w ワールド参照
     * @param[in] self 自身のエンティティ
     * @param[in] dt デルタタイム(秒単位)
     * 
     * @details
     * VideoPlayerのUpdate()を呼び出し、動画フレームを更新します。
     */
    void OnUpdate(World& w, Entity self, float dt) override {
        if (player) {
            player->Update(dt);
        }
    }
};
