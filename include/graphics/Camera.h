#pragma once
#include <DirectXMath.h>

/**
 * @file Camera.h
 * @brief 3Dカメラシステムの定義
 * @author 山内陽
 * @date 2025
 * @version 5.0
 * 
 * @details
 * このファイルは3D空間のカメラ(視点)を管理する構造体を定義します。
 */

/**
 * @struct Camera
 * @brief 3D空間のカメラ(ビュー・プロジェクション行列)を管理
 * 
 * @details
 * カメラは、「どこから」「どこを」「どの向きで」見るかを制御します。
 * また、レンダリングに必要なビュー行列とプロジェクション行列を保持します。
 * 
 * ### カメラの構成要素:
 * - **View行列**: カメラの位置と向きを表す
 * - **Projection行列**: 3D空間を2D画面に投影する方法を表す
 * 
 * ### 座標系:
 * - X軸: 右方向
 * - Y軸: 上方向
 * - Z軸: 奥方向(カメラの視線方向)
 * 
 * @par 使用例(基本)
 * @code
 * // カメラを作成(座標(0,2,-5)から原点を見る)
 * Camera camera = Camera::LookAtLH(
 *     DirectX::XM_PIDIV4,              // 視野角45度
 *     1280.0f / 720.0f,                // アスペクト比
 *     0.1f,                            // ニアクリップ
 *     1000.0f,                         // ファークリップ
 *     DirectX::XMFLOAT3{0, 2, -5},    // カメラ位置
 *     DirectX::XMFLOAT3{0, 0, 0},     // 注視点
 *     DirectX::XMFLOAT3{0, 1, 0}      // 上方向
 * );
 * @endcode
 * 
 * @par 使用例(オービット回転)
 * @code
 * // カメラを回転(マウスドラッグ等)
 * camera.Orbit(deltaYaw, deltaPitch);  // 左右・上下回転
 * camera.Zoom(-0.1f);                  // ズームイン
 * @endcode
 * 
 * @see DirectX::XMMATRIX 行列型
 * 
 * @author 山内陽
 */
struct Camera {
    DirectX::XMMATRIX View;  ///< ビュー行列(カメラの位置・向き)
    DirectX::XMMATRIX Proj;  ///< プロジェクション行列(透視方法)

    //View行列のゲッター
    const DirectX::XMMATRIX& GetViewMatrix() const
    {
        return View;
    }

    //Projの行列のゲッター
    const DirectX::XMMATRIX& GetProjectionMatrix() const
    {
        return Proj;
    }

    
    DirectX::XMFLOAT3 position;  ///< カメラの位置
    DirectX::XMFLOAT3 target;    ///< カメラが見ている点(注視点)
    DirectX::XMFLOAT3 up;        ///< カメラの上方向ベクトル
    
    float fovY;     ///< 垂直視野角(ラジアン)
    float aspect;   ///< アスペクト比(幅/高さ)
    float nearZ;    ///< ニアクリッププレーン
    float farZ;     ///< ファークリッププレーン

    /**
     * @brief LookAtLHカメラの作成(左手座標系)
     * 
     * @param[in] fovY 垂直視野角(ラジアン、通常 DirectX::XM_PIDIV4 = 45度)
     * @param[in] aspect アスペクト比(幅/高さ、例: 16/9 = 1.777...)
     * @param[in] znear ニアクリップ距離(この距離より手前は描画されない)
     * @param[in] zfar ファークリップ距離(この距離より奥は描画されない)
     * @param[in] eye カメラの位置
     * @param[in] at カメラが見る点(注視点)
     * @param[in] upVec カメラの上方向(通常 {0,1,0})
     * @return Camera 設定されたカメラ
     * 
     * @details
     * カメラを作成し、ビュー行列とプロジェクション行列を計算します。
     * 
     * @par 視野角について:
     * - XM_PIDIV4 (π/4) = 45度: 標準的な視野角
     * - XM_PIDIV3 (π/3) = 60度: 広角
     * - XM_PIDIV6 (π/6) = 30度: 望遠
     * 
     * @par 使用例
     * @code
     * Camera camera = Camera::LookAtLH(
     *     DirectX::XM_PIDIV4,              // 45度
     *     1920.0f / 1080.0f,               // Full HD
     *     0.1f,                            // 0.1m手前まで
     *     1000.0f,                         // 1000m奥まで
     *     DirectX::XMFLOAT3{0, 5, -10},   // 少し後ろから
     *     DirectX::XMFLOAT3{0, 0, 0},     // 原点を見る
     *     DirectX::XMFLOAT3{0, 1, 0}      // Y軸が上
     * );
     * @endcode
     */
    static Camera LookAtLH(
        float fovY, float aspect, float znear, float zfar,
        DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 upVec)
    {
        Camera c;
        c.position = eye;
        c.target = at;
        c.up = upVec;
        c.fovY = fovY;
        c.aspect = aspect;
        c.nearZ = znear;
        c.farZ = zfar;
        
        c.View = DirectX::XMMatrixLookAtLH(
            DirectX::XMLoadFloat3(&eye),
            DirectX::XMLoadFloat3(&at),
            DirectX::XMLoadFloat3(&upVec));
        c.Proj = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, znear, zfar);
        return c;
    }
    
    /**
     * @brief カメラの更新(位置や注視点を変更した後に呼ぶ)
     * 
     * @details
     * position、target、upを変更した後、この関数を呼ぶことで
     * ビュー行列が再計算されます。
     * 
     * @par 使用例
     * @code
     * camera.position.y += 1.0f;  // カメラを上に移動
     * camera.Update();            // 行列を再計算
     * @endcode
     */
    void Update() {
        View = DirectX::XMMatrixLookAtLH(
            DirectX::XMLoadFloat3(&position),
            DirectX::XMLoadFloat3(&target),
            DirectX::XMLoadFloat3(&up));
    }
    
    /**
     * @brief オービットカメラ(ターゲットの周りを回転)
     * 
     * @param[in] deltaYaw 左右回転量(ラジアン、正で右回転)
     * @param[in] deltaPitch 上下回転量(ラジアン、正で上回転)
     * 
     * @details
     * 注視点(target)を中心に、カメラを回転させます。
     * マウスドラッグでカメラを回すような操作に使用します。
     * 
     * @note 内部でUpdate()を呼ぶため、手動でUpdate()を呼ぶ必要はありません
     * 
     * @par 使用例
     * @code
     * // マウスの移動量をカメラ回転に変換
     * float yaw = mouseDeltaX * 0.01f;    // 左右
     * float pitch = mouseDeltaY * 0.01f;  // 上下
     * camera.Orbit(yaw, pitch);
     * @endcode
     */
    void Orbit(float deltaYaw, float deltaPitch) {
        // 現在の位置からターゲットへのベクトル
        DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&position);
        DirectX::XMVECTOR targetVec = DirectX::XMLoadFloat3(&target);
        DirectX::XMVECTOR toTarget = DirectX::XMVectorSubtract(targetVec, posVec);
        
        float radius = DirectX::XMVectorGetX(DirectX::XMVector3Length(toTarget));
        
        // 球面座標系で回転
        DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(deltaYaw);
        DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationAxis(
            DirectX::XMVector3Cross(toTarget, DirectX::XMLoadFloat3(&up)), 
            deltaPitch);
        
        DirectX::XMVECTOR newDir = DirectX::XMVector3TransformNormal(toTarget, rotY);
        newDir = DirectX::XMVector3TransformNormal(newDir, rotX);
        newDir = DirectX::XMVector3Normalize(newDir);
        
        DirectX::XMVECTOR newPos = DirectX::XMVectorAdd(
            targetVec, 
            DirectX::XMVectorScale(newDir, -radius));
        
        DirectX::XMStoreFloat3(&position, newPos);
        Update();
    }
    
    /**
     * @brief ズーム(視野角の変更)
     * 
     * @param[in] delta 視野角の変更量(ラジアン、正で広角、負で望遠)
     * 
     * @details
     * 視野角を変更することでズーム効果を実現します。
     * 視野角は22.5度～90度の範囲に制限されます。
     * 
     * @par 使用例
     * @code
     * camera.Zoom(-0.1f);  // ズームイン(視野角を狭める)
     * camera.Zoom(0.1f);   // ズームアウト(視野角を広げる)
     * @endcode
     */
    void Zoom(float delta) {
        fovY += delta;
        // 視野角を制限(22.5度～90度)
        if (fovY < DirectX::XM_PIDIV4 * 0.5f) fovY = DirectX::XM_PIDIV4 * 0.5f;
        if (fovY > DirectX::XM_PIDIV2) fovY = DirectX::XM_PIDIV2;
        
        Proj = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    }
};
