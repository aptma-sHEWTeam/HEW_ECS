#pragma once

#include <cstdint>
#include <functional>

namespace GameSound {

//＝＝＝＝＝この辺はコンフィグ＝＝＝＝＝＝＝
// -------------------------
// 論理 ID
// -------------------------
enum class BgmId : uint32_t {
    None = 0,
    MainTheme,
    Calm,
    Tense
};
enum class SeId : uint32_t {
    None = 0,
    Ok,
    Ng,
    Build,
    Refund,
    Tip,
    Reward,
    Warn,
    LifeDown,
    Alert,
    Enter,
    Exit
};

// -------------------------
// 設定
// -------------------------
struct Config {
    bool enabled = true;         // マスタ ON/OFF
    float masterVolume = 1.0f;   // 0..1（bgm/se に乗算）
    float seVolume = 1.0f;       // 0..1
    float bgmVolume = 1.0f;      // 0..1
    bool visualFallback = false; // 視覚代替（デバッグ用）
};
//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

using VirtualBeepHandler = std::function<void(const wchar_t *text)>;

// -------------------------
// ライフサイクル
// -------------------------
bool Init();
void Update(float dt = 0.0f);
void Uninit();

// -------------------------
// 設定 / 有効無効
// -------------------------

//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
//コンフィグから取ったデータを基にする
void SetConfig(const Config &cfg);
Config GetConfig();
//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
void SetEnabled(bool enabled);
bool GetEnabled();

void SetVirtualBeepHandler(VirtualBeepHandler cb);

// -------------------------
// 再生 API
// -------------------------

//この辺はBehaviorで作成
bool PlayBGM(BgmId id, bool loop = true);
void StopBGM();

//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
//コンフィグのデータを変更
void SetBGMVolume(float v);
bool PlaySE(SeId id);
//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

} // namespace GameSound
