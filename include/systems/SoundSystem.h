/*
 * @
 * 内部でCOMオブジェクトを利用するため、LoadTexture関数より後にInitSound関数呼び出すと
 * エラーになる
 */
#ifndef __SOUNDSYSTEM_H__
#define __SOUNDSYSTEM_H__

#include <xaudio2.h>
#include <vector>
#include <map>
#include <string>

#pragma comment(lib, "xaudio2.lib")

//インスタンス用アクセスマクロ
#define SOUND_SYS SoundSystem::GetInstance()

//----------
// 構造体
//----------
struct SoundData {
    WAVEFORMATEX format;  // WAVフォーマット
    BYTE *pBuffer;        // サウンドデータ
    DWORD bufSize;        // データサイズ
    XAUDIO2_BUFFER sound; // サウンドバッファ
};
struct MP3FormatInfo {
    DWORD offset;
    DWORD dataSize;
};
struct MP3FrameInfo {
    BYTE channel;
    BYTE padding;
    DWORD sampleRate;
    DWORD bitRate;
    DWORD frameSize;
};

/**
 * @brief サウンド管理システム(シングルトン)
 */
class SoundSystem {
public:
    //シングルトンインスタンスの取得
    static SoundSystem& GetInstance() {
      static SoundSystem instance;
        return instance;
  }
    //----------
    // プロトタイプ宣言
    //----------
    HRESULT Init(void);        //初期化
    void Uninit(void);         //終了

    // サウンドファイルの読み込み
    XAUDIO2_BUFFER *LoadSound(const char *file, bool loop = false);

    // サウンドの再生
#undef PlaySound // winapiのPlaySoundを無効にする
    IXAudio2SourceVoice *PlaySound(XAUDIO2_BUFFER *pSound);

    //パスを指定してSEを再生
    void PlaySE(const std::string& path, bool multiple);

    //パスを指定してBGMを再生
    void PlayBGM(const std::string &path);

    //SEの停止
    void StopSE(const std::string &path);

    //BGM停止
    void StopBGM();

    //音量を更新
    void UpdateVolume();

    //音量を更新
    void UpdateSEVolume(const std::string& path);

    // XAudio2デバイスの取得 (VideoPlayerなどが直接利用するため)
    IXAudio2* GetXAudio() const { return m_pXAudio; }

  private:
     //メンバ変数
    IXAudio2 *m_pXAudio;
    IXAudio2MasteringVoice *m_pMasterVoice;
    std::map<std::string, SoundData> m_soundMap;
    IXAudio2SourceVoice *m_pBgmVoice = nullptr;
    std::string m_currentBgmPath;

    SoundSystem() : m_pXAudio(nullptr), m_pMasterVoice(nullptr) {}
    ~SoundSystem() { Uninit(); }
    SoundSystem(const SoundSystem &) = delete;
    SoundSystem &operator=(const SoundSystem &) = delete;

    std::map<std::string, IXAudio2SourceVoice *> m_seVoices;
   

    //内部ヘルパー
    HRESULT LoadWav(const char *file, SoundData *pData);
    HRESULT LoadMP3(const char *file, SoundData *pData);
};
#endif // __SOUNDSYSTEM_H__