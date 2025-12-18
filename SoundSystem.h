#pragma once
#include "ecs/World.h"

class SoundSystem {
  private:

	//論理ID
    enum BgmId : uint32_t {None = 0,MainTheme,Goal};
    enum SeId  : uint32_t {None = 0,Start,CountDown,Charge,Release,Impact,Goal};

  public:
    SoundSystem();
    ~SoundSystem();
    void Update(float dt = 0.0f);

    //コンフィグからデータを取得する
    void SetConfig();




};
