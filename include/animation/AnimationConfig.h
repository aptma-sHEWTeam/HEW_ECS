#pragma once
#include <string>

// アニメーション名やアセットパスを集中管理
namespace AnimationConfig {

struct Paths {
    inline static const char* PlayerModel = "Assets/Models/Player/obj_player.fbx";
    inline static const char* PlayerAnimFry = "Assets/Models/Player/anm_fry.fbx";
    inline static const char* PlayerAnimCharge = "Assets/Models/Player/anm_charge.fbx";
    inline static const char* PlayerAnimChargeIn = "Assets/Models/Player/anm_charge_in.fbx";
    inline static const char* PlayerAnimChargeOut = "Assets/Models/Player/anm_charge_out.fbx";
    inline static const char* PlayerAnimDeath = "Assets/Models/Player/anm_death.fbx";
    inline static const char* PlayerAnimTimeup = "Assets/Models/Player/anm_timeup.fbx";
    inline static const char *PlayerAnimTitle = "Assets/Models/Player/anm_title.fbx";
    inline static const char* PlayerAnimFallback = ""; // 必要なら設定
};

struct Clips {
    // クリップ名がFBX側で空の場合、ファイル名（拡張子なし）を名前として付与する
    inline static const char* PlayerIdle = "Idle";          // anm_fry.fbx 内の想定
    inline static const char* PlayerCharge = "anm_charge";  // ファイル名をデフォルト名に
    inline static const char* PlayerChargeIn = "anm_charge_in";
    inline static const char* PlayerChargeOut = "anm_charge_out";
    inline static const char* PlayerDeath = "anm_death";
    inline static const char* PlayerTimeup = "anm_timeup";
    inline static const char *PlayerTitle = "anm_title";
    inline static const char* PlayerDefault = PlayerIdle;
};

struct UI {
    // スプライトシートの基本設定例
    inline static const int FadeFrames = 18;
    inline static const int FadeCols = 18;
    inline static const float FadeFrameTime = 0.3f;
    inline static const int DeathFadeFrames = 18;
    inline static const int DeathFadeCols = 18;
    inline static const float DeathFadeFrameTime = 0.06f;
};

} // namespace AnimationConfig
