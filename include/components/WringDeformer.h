#pragma once
#include <DirectXMath.h>

/**
 * @file WringDeformer.h
 * @brief メッシュに対して「雑巾絞り」風のねじり・縮みを与えるデータコンポーネント
 *
 * @details
 * 頂点シェーダ内でローカル座標に基づくねじり・半径縮小・軸圧縮を適用するパラメータ群。
 *
 * 雑巾絞りの特徴:
 * - 上下を手で握って固定（gripRange で端を保護）
 * - 中央を強く絞る（squeezeAmount で絞り量を制御）
 * - ねじると短くなる（axisCompress で軸方向圧縮）
 * - 螺旋状の表面変形（twistUV でテクスチャも連動）
 *
 * 推奨設定例:
 * WringDeformer wr;
 * wr.twistAmount = 3.0f;        // 3回転
 * wr.squeezeAmount = 0.7f;      // 70%絞り
 * wr.gripRange = 0.15f;         // 上下15%を固定
 * wr.axisCompress = 0.2f;       // 20%短くなる
 * wr.twistFalloff = 3.0f;       // 端を強く保護
 * wr.squeezeFalloff = 2.5f;     // 絞りも端を保護
 * wr.baseRadius = 0.5f;         // 半径の基準（メッシュ半径）
 * wr.lobeCount = 3.0f;          // ローブ数
 * wr.bulgeAmp = 0.25f;          // 膨らみの強さ
 *
 * @author 山内陽
 * @version 7.1 - ローブ制御パラメータを追加
 */
struct WringDeformer {
    // 基本ON/OFF
    bool enabled = true;  ///< デフォーム有効化

    // 基本パラメータ
    float twistAmount = 3.0f;     ///< ねじり量（回転数）
    float squeezeAmount = 0.6f;   ///< 絞り量（0..1）
    float gripRange = 0.15f;      ///< 上下の「握っている範囲」（0..0.5）

    // === 高度な制御 ===

    float axisCompress = 0.1f;    ///< 軸方向（Y軸）の圧縮量（0..1）
    float twistFalloff = 3.0f;    ///< ねじりの減衰カーブ（1.0以上）
    float squeezeFalloff = 2.5f;  ///< 絞りの減衰カーブ（1.0以上）

    // ローブ（膨らみ）制御
    float baseRadius = 0.5f;      ///< 半径の基準値（r/baseRadius で正規化）
    float lobeCount = 3.0f;       ///< 周方向のローブ数（2〜5程度が見た目良い）
    float bulgeAmp = 0.25f;       ///< 膨らみの基本振幅（実振幅は squeeze×ねじり勾配で変調）

    // UV連動
    bool twistUV = true;          ///< UVもねじるか
    float uvTwistScale = 1.0f;    ///< UVのねじり倍率
    DirectX::XMFLOAT2 uvCenter{0.5f, 0.5f}; ///< UV回転の中心

    // 内部用（シェーダーには送らない）
    float pivotY = 0.0f;          ///< ねじりの基準Y座標（ローカル座標）
    float halfHeight = 1.0f;      ///< 正規化範囲の半分（ローカル座標）
};
