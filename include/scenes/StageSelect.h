/**
 * @file StageSelect.h
 * @brief セレクトシーン
 * @author 立山悠朔
 * @date 2025
 * @version 1.0
 */
#pragma once

/**
 * @class StageSelectScene
 * @brief 3DゲームとUIを統合したシーン
 */
class StageSlectScene : public IScene {
  public:
    inline static ConfigVar<float> cfg_UICountPosX{"UI", "CountPosX", 20.0f};
    inline static ConfigVar<float> cfg_UICountPosY{"UI", "CountPosY", 170.0f};
    inline static ConfigVar<float> cfg_UICountW{"UI", "CountWidth", 200.0f};
    inline static ConfigVar<float> cfg_UICountH{"UI", "CountHeight", 40.0f};
    inline static ConfigVar<float> cfg_UICountR{"UI", "CountColorR", 0.0f};
    inline static ConfigVar<float> cfg_UICountG{"UI", "CountColorG", 1.0f};
    inline static ConfigVar<float> cfg_UICountB{"UI", "CountColorB", 1.0f};

    int StageCount = 0;

    void OnEnter(World &world) override {

    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        CreateStageSelectUI(world);
        //enterを押したらシーン移動
        if (input.GetKeyDown(VK_RETURN)) {
            DEBUGLOG("Enter pressed!");
            auto *maneger = ServiceLocator::TryGet<SceneManager>();
            maneger->ChangeScene("Game", world);
        }
    }

    void OnExit(World &worlld) override {
      
    }

  private:
      void CreateStageSelectUI(World&world)
      {
        UITransform CountTransform;
         CountTransform.position = {cfg_UICountPosX, cfg_UICountPosY};
        CountTransform.size = {cfg_UICountW, cfg_UICountH};
        CountTransform.anchor = {0.0f, 0.0f};
        CountTransform.pivot = {0.0f, 0.0f};

         UIText CountText{L"0"};
        CountText.color = {cfg_UICountR, cfg_UICountG, cfg_UICountB, 1.0f};
        CountText.formatId = "hud";

        Entity e = world.Create()
                       .With<UITransform>(CountTransform)
                       .With<UIText>(CountText)
                       .Build();



        StageSelectEntity_ = e;  
      }
    Entity StageSelectEntity_{};
};
