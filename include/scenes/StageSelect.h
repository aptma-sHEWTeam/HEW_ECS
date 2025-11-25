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
    void OnEnter(World &world) override {
        
        
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
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
};
