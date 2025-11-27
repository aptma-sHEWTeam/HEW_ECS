EmissiveRenderSystem::GetInstance().Initialize(device);

Entity startObject = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f })
    .With<MeshRenderer>()
    .With<EmissiveMaterial>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.5f }, 1.5f)
    .With<EmissivePulse>(0.8f, 2.5f, 3.0f)
    .Build();

Entity goalObject = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{ 100.0f, 0.0f, 0.0f })
    .With<MeshRenderer>()
    .With<EmissiveMaterial>(DirectX::XMFLOAT3{ 1.0f, 0.8f, 0.0f }, 2.0f)
    .With<EmissivePulse>(1.0f, 3.0f, 2.0f)
    .Build();

// 描画ループ内
EmissiveRenderSystem::GetInstance().UpdateMaterial(context, entity, world);
EmissiveRenderSystem::GetInstance().Bind(context, 2);

EmissiveRenderSystem::GetInstance().Shutdown();
