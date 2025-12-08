// 初期化時
LightSystem::GetInstance().Initialize(device);
LightSystem::GetInstance().SetAmbientLight({ 0.1f, 0.1f, 0.15f }, 1.0f);

// ライト付きエンティティ作成
Entity lightEntity = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{ 5.0f, 3.0f, 0.0f })
    .With<PointLight>(DirectX::XMFLOAT3{ 1.0f, 0.8f, 0.6f }, 2.0f, 15.0f)
    .Build();

// 更新ループ内
LightSystem::GetInstance().Update(world, cameraPosition);

// 描画時
LightSystem::GetInstance().Bind(context, 1);

// シャットダウン時
LightSystem::GetInstance().Shutdown();
