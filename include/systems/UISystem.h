/**
 * @file UISystem.h
 * @brief UI描画・更新システム
 */
#pragma once
#include "components/Component.h"
#include "components/UIComponents.h"
#include "components/UIImageComponents.h"
#include "graphics/TextSystem.h"
#include "graphics/ImageSystem.h"
#include "graphics/TextureManager.h"
#include "input/InputSystem.h"
#include "input/GamepadSystem.h"
#include "ecs/World.h"
#include "config/ConfigVar.h"
#include <DirectXMath.h>
#include "app/ServiceLocator.h"
#include <algorithm>
#include <vector>

inline ConfigVar<float> cfg_ControllerRumbleNavigateStrength{"Gamepad.Rumble.UI", "NavigateStrength", 0.180f, "UI選択時のコントローラー振動強度"};
inline ConfigVar<float> cfg_ControllerRumbleNavigateDuration{"Gamepad.Rumble.UI", "NavigateDuration", 0.070f, "UI選択時のコントローラー振動継続時間"};
inline ConfigVar<float> cfg_ControllerRumbleSubmitStrength{"Gamepad.Rumble.UI", "SubmitStrength", 0.320f, "UI決定時のコントローラー振動強度"};
inline ConfigVar<float> cfg_ControllerRumbleSubmitDuration{"Gamepad.Rumble.UI", "SubmitDuration", 0.120f, "UI決定時のコントローラー振動継続時間"};

/**
 * @struct UIRenderSystem
 * @brief UI要素を描画するシステム
 *
 * @details
 * TextSystemを使用してボタン、テキスト、パネルを描画します。
 */
struct UIRenderSystem {
    TextSystem *textSystem_ = nullptr;
    ImageSystem *imageSystem_ = nullptr;
    float screenWidth_ = 1280.0f;
    float screenHeight_ = 720.0f;
    float renderOffsetX_ = 0.0f; // トランジション用X方向オフセット

    void Render(World &w) {
        if (!textSystem_ || !textSystem_->IsInitialized())
            return;
        if (!imageSystem_ || !imageSystem_->IsInitialized())
            return;

        auto drawImages = [&](bool overlayFlag) {
            imageSystem_->BeginDraw();
            w.ForEach<UICanvas>([&](Entity, UICanvas &canvas) {
                if (!canvas.enabled)
                    return;
                w.ForEach<UITransform, UIImage>([&](Entity, UITransform &t, UIImage &img) {
                    if (img.overlay == overlayFlag)
                        DrawImage(t, img);
                });
            });
            imageSystem_->EndDraw();
        };

        auto drawPanels = [&](bool beforeImages) {
            textSystem_->BeginDraw();
            w.ForEach<UICanvas>([&](Entity, UICanvas &canvas) {
                if (!canvas.enabled)
                    return;

                w.ForEach<UITransform, UIPanel>([&](Entity, UITransform &t, UIPanel &p) {
                    if (p.visible && p.drawBeforeImages == beforeImages)
                        DrawPanel(t, p);
                });
            });
            textSystem_->EndDraw();
        };

        drawPanels(true);
        drawImages(false);
        drawPanels(false);
        textSystem_->BeginDraw();
        w.ForEach<UICanvas>([&](Entity, UICanvas &canvas) {
            if (!canvas.enabled)
                return;

            w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &t, UIButton &b) {
                if (!b.enabled || t.size.x <= 0.0f || t.size.y <= 0.0f)
                    return;
                DrawButton(t, b);
                if (auto *txt = w.TryGet<UIText>(e)) {
                    if (!txt->text.empty()) {
                        DrawButtonText(t, b, *txt);
                    }
                }
            });

            w.ForEach<UITransform, UIText>([&](Entity e, UITransform &t, UIText &txt) {
                if (w.Has<UIButton>(e))
                    return;
                if (t.size.x <= 0.0f || t.size.y <= 0.0f)
                    return;
                if (txt.text.empty())
                    return;
                DrawText(t, txt);
            });
        });
        textSystem_->EndDraw();
        drawImages(true);
    }

    void SetTextSystem(TextSystem *ts) {
        textSystem_ = ts;
    }
    void SetImageSystem(ImageSystem *is) {
        imageSystem_ = is;
    }
    void SetScreenSize(float w, float h) {
        screenWidth_ = w;
        screenHeight_ = h;
    }
    void SetRenderOffset(float offsetX) {
        renderOffsetX_ = offsetX;
    }

  private:
    void DrawPanel(const UITransform &transform, const UIPanel &panel) {
        DirectX::XMFLOAT2 pos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        pos.x += renderOffsetX_; // Apply transition offset
        textSystem_->FillRect(pos.x, pos.y, transform.size.x, transform.size.y, panel.color);
    }
    void DrawButton(const UITransform &transform, const UIButton &button) {
        DirectX::XMFLOAT2 pos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        pos.x += renderOffsetX_;

        float pad = 0.0f;
        float pressOffsetY = 0.0f;
        if (button.enabled) {
            if (button.state == UIButton::State::Hovered) {
                pad = 6.0f;
            } else if (button.state == UIButton::State::Pressed) {
                pad = 4.0f;
                pressOffsetY = 2.0f;
            }
        }

        const float drawX = pos.x - pad;
        const float drawY = pos.y - pad + pressOffsetY;
        const float drawW = transform.size.x + pad * 2.0f;
        const float drawH = transform.size.y + pad * 2.0f;

        if (button.enabled) {
            const DirectX::XMFLOAT4 shadow{0.0f, 0.0f, 0.0f, 0.25f};
            textSystem_->FillRect(drawX + 2.0f, drawY + 2.0f, drawW, drawH, shadow);
        }

        DirectX::XMFLOAT4 color = button.GetCurrentColor();
        textSystem_->FillRect(drawX, drawY, drawW, drawH, color);
    }

    void DrawButtonText(const UITransform &transform, const UIButton &button, const UIText &text) {
        float pressOffsetY = (button.enabled && button.state == UIButton::State::Pressed) ? 2.0f : 0.0f;

        DirectX::XMFLOAT2 pos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        pos.x += renderOffsetX_;

        TextSystem::TextParams p;
        p.text = text.text;
        p.x = pos.x;
        p.y = pos.y + pressOffsetY;
        p.width = transform.size.x;
        p.height = transform.size.y;
        p.color = text.color;
        p.outlineColor = text.outlineColor;
        p.outlineThickness = text.outlineThickness;
        p.fillTexturePath = text.fillTexturePath;
        p.formatId = text.formatId;
        p.fontSize = text.fontSize;
        textSystem_->DrawText(p);
    }
    void DrawText(const UITransform &transform, const UIText &text) {
        DirectX::XMFLOAT2 pos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        pos.x += renderOffsetX_; // Apply transition offset
        TextSystem::TextParams p;
        p.text = text.text;
        p.x = pos.x;
        p.y = pos.y;
        p.width = transform.size.x;
        p.height = transform.size.y;
        p.color = text.color;
        p.outlineColor = text.outlineColor;
        p.outlineThickness = text.outlineThickness;
        p.fillTexturePath = text.fillTexturePath;
        p.formatId = text.formatId;
        p.fontSize = text.fontSize;
        textSystem_->DrawText(p);
    }
    void DrawImage(const UITransform &transform, const UIImage &img) {
        DirectX::XMFLOAT2 pos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        pos.x += renderOffsetX_; // Apply transition offset
        const auto &uv = img.uvRect;
        if (img.textureHandle != TextureManager::INVALID_TEXTURE) {
            auto &texMgr = ServiceLocator::Get<TextureManager>();
            uint32_t tw = 0, th = 0;
            D2D1_RECT_F *srcPtr = nullptr;
            D2D1_RECT_F src;
            if (texMgr.GetSize(img.textureHandle, tw, th)) {
                float sx = uv[0] * tw;
                float sy = uv[1] * th;
                float sw = uv[2] * tw;
                float sh = uv[3] * th;
                src = D2D1::RectF(sx, sy, sx + sw, sy + sh);
                srcPtr = &src;
            }
            imageSystem_->Draw(img.textureHandle, pos.x, pos.y, transform.size.x, transform.size.y, img.opacity, img.keepAspect, srcPtr, transform.rotation, img.aspectFill);
        } else {
            ImageSystem::Params p;
            p.filePath = img.filePath;
            p.x = pos.x;
            p.y = pos.y;
            p.width = transform.size.x;
            p.height = transform.size.y;
            p.opacity = img.opacity;
            p.keepAspect = img.keepAspect;
            p.aspectFill = img.aspectFill;
            p.srcX = uv[0];
            p.srcY = uv[1];
            p.srcW = uv[2];
            p.srcH = uv[3];
            p.rotation = transform.rotation;
            imageSystem_->Draw(p);
        }
    }
};

/**
 * @struct UIInteractionSystem
 * @brief UIの入力処理システム
 *
 * @details
 * マウス入力を受け取り、ボタンの状態を更新します。
 */
struct UIInteractionSystem : Behaviour {
    InputSystem *input_ = nullptr;
    float screenWidth_ = 1280.0f;
    float screenHeight_ = 720.0f;

    int selectedIndex_ = -1;
    bool stickUpPrev_ = false;
    bool stickDownPrev_ = false;
    bool dpadUpPrev_ = false;
    bool dpadDownPrev_ = false;
    float rumbleTimer_ = 0.0f;
    bool rumbleActive_ = false;

    void OnUpdate(World &w, Entity self, float dt) override {
        UpdateRumble(dt);
        if (!input_)
            return;

        w.ForEach<UIButton>([&](Entity, UIButton &b) {
            if (!b.enabled) {
                b.state = UIButton::State::Disabled;
            }
        });

        struct ButtonEntry {
            Entity e;
            float screenY;
        };

        std::vector<ButtonEntry> buttons;
        buttons.reserve(32);
        w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &t, UIButton &b) {
            if (!b.enabled)
                return;
            DirectX::XMFLOAT2 pos = t.GetScreenPosition(screenWidth_, screenHeight_);
            buttons.push_back({e, pos.y});
        });

        if (buttons.empty()) {
            selectedIndex_ = -1;
            stickUpPrev_ = stickDownPrev_ = false;
            dpadUpPrev_ = dpadDownPrev_ = false;
            return;
        }

        std::sort(buttons.begin(), buttons.end(), [](const ButtonEntry &a, const ButtonEntry &b) {
            return a.screenY < b.screenY;
        });

        bool submitDown = false;
        bool submitHeld = false;

        bool up = false;
        bool down = false;

        auto *pad = ServiceLocator::TryGet<GamepadSystem>();
        if (pad) {
            const float threshold = 0.8f;
            const float ay = pad->GetLeftStickY();

            const bool stickUpNow = ay > threshold;
            const bool stickDownNow = ay < -threshold;
            const bool dpadUpNow = pad->GetButton(GamepadSystem::Button_DPad_Up);
            const bool dpadDownNow = pad->GetButton(GamepadSystem::Button_DPad_Down);

            if (stickUpNow && !stickUpPrev_)
                up = true;
            if (stickDownNow && !stickDownPrev_)
                down = true;
            if (dpadUpNow && !dpadUpPrev_)
                up = true;
            if (dpadDownNow && !dpadDownPrev_)
                down = true;

            stickUpPrev_ = stickUpNow;
            stickDownPrev_ = stickDownNow;
            dpadUpPrev_ = dpadUpNow;
            dpadDownPrev_ = dpadDownNow;

            submitDown = pad->GetButtonDown(GamepadSystem::Button_A);
            submitHeld = pad->GetButton(GamepadSystem::Button_A);
        } else {
            stickUpPrev_ = stickDownPrev_ = false;
            dpadUpPrev_ = dpadDownPrev_ = false;
        }

        const int buttonCount = static_cast<int>(buttons.size());
        if (selectedIndex_ < -1 || selectedIndex_ >= buttonCount) {
            selectedIndex_ = -1;
        }
        const int previousSelectedIndex = selectedIndex_;

        if (up) {
            if (selectedIndex_ < 0) {
                selectedIndex_ = buttonCount - 1;
            } else {
                selectedIndex_ = (selectedIndex_ - 1 + buttonCount) % buttonCount;
            }
        }
        if (down) {
            if (selectedIndex_ < 0) {
                selectedIndex_ = 0;
            } else {
                selectedIndex_ = (selectedIndex_ + 1) % buttonCount;
            }
        }

        if ((submitDown || submitHeld) && selectedIndex_ < 0) {
            selectedIndex_ = 0;
        }
        if ((up || down) && selectedIndex_ != previousSelectedIndex && selectedIndex_ >= 0) {
            TriggerRumblePulse(std::clamp(cfg_ControllerRumbleNavigateStrength.Get(), 0.0f, 1.0f),
                               std::max(0.0f, cfg_ControllerRumbleNavigateDuration.Get()));
        }


        for (size_t i = 0; i < buttons.size(); ++i) {
            auto *b = w.TryGet<UIButton>(buttons[i].e);
            if (!b || !b->enabled)
                continue;

            if (static_cast<int>(i) == selectedIndex_) {
                b->state = submitHeld ? UIButton::State::Pressed : UIButton::State::Hovered;
            } else {
                b->state = UIButton::State::Normal;
            }
        }

        if (submitDown && selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(buttons.size())) {
            auto *b = w.TryGet<UIButton>(buttons[static_cast<size_t>(selectedIndex_)].e);
            if (b && b->enabled && b->onClick) {
                TriggerRumblePulse(std::clamp(cfg_ControllerRumbleSubmitStrength.Get(), 0.0f, 1.0f),
                                   std::max(0.0f, cfg_ControllerRumbleSubmitDuration.Get()));
                b->onClick();
            }
        }
    }

    void SetInputSystem(InputSystem *input) {
        input_ = input;
    }
    void SetScreenSize(float w, float h) {
        screenWidth_ = w;
        screenHeight_ = h;
    }

  private:
    void TriggerRumblePulse(float strength, float duration) {
        if (strength <= 0.0f || duration <= 0.0f) {
            return;
        }
        auto *pad = ServiceLocator::TryGet<GamepadSystem>();
        if (!pad) {
            return;
        }
        pad->SetVibration(strength, strength);
        rumbleTimer_ = std::max(rumbleTimer_, duration);
        rumbleActive_ = true;
    }

    void UpdateRumble(float dt) {
        if (!rumbleActive_) {
            return;
        }
        rumbleTimer_ = std::max(0.0f, rumbleTimer_ - std::max(0.0f, dt));
        if (rumbleTimer_ > 0.0f) {
            return;
        }
        auto *pad = ServiceLocator::TryGet<GamepadSystem>();
        if (pad) {
            pad->SetVibration(0.0f, 0.0f);
        }
        rumbleActive_ = false;
    }
};
