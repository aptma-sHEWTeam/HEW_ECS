/**
 * @file UISystem.h
 * @brief UI描画・更新システム
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once
#include "components/Component.h"
#include "components/UIComponents.h"
#include "graphics/TextSystem.h"
#include "input/InputSystem.h"
#include "ecs/World.h"
#include <DirectXMath.h>

/**
 * @struct UIRenderSystem
 * @brief UI要素を描画するシステム
 *
 * @details
 * TextSystemを使用してボタン、テキスト、パネルを描画します。
 */
struct UIRenderSystem {
    TextSystem *textSystem_ = nullptr;
    float screenWidth_ = 1280.0f;
    float screenHeight_ = 720.0f;

    void Render(World &w) {
        if (!textSystem_ || !textSystem_->IsInitialized()) {
            return;
        }

        textSystem_->BeginDraw();

        w.ForEach<UICanvas>([&](Entity canvasEntity, UICanvas &canvas) {
            if (!canvas.enabled)
                return;

            w.ForEach<UITransform, UIPanel>([&](Entity e, UITransform &transform, UIPanel &panel) {
                if (panel.visible) {
                    DrawPanel(transform, panel);
                }
            });

            // ボタンとテキストを描画（ボタンを持つエンティティ）
            w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &transform, UIButton &button) {
                DrawButton(transform, button);

                // ボタンのテキストを描画
                auto *text = w.TryGet<UIText>(e);
                if (text) {
                    DrawButtonText(transform, *text);
                }
            });

            // テキストのみを持つエンティティを描画
            w.ForEach<UITransform, UIText>([&](Entity e, UITransform &transform, UIText &text) {
                if (!w.Has<UIButton>(e)) {
                    DrawText(transform, text);
                }
            });
        });

        textSystem_->EndDraw();
    }

    void SetTextSystem(TextSystem *ts) {
        textSystem_ = ts;
    }
    void SetScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
    }

  private:
    void DrawPanel(const UITransform &transform, const UIPanel &panel) {
        DirectX::XMFLOAT2 screenPos = transform.GetScreenPosition(screenWidth_, screenHeight_);

        TextSystem::TextParams params;
        params.text = L"█";
        params.x = screenPos.x;
        params.y = screenPos.y;
        params.width = transform.size.x;
        params.height = transform.size.y;
        params.color = panel.color;
        params.formatId = "panel";

        for (float y = 0; y < transform.size.y; y += 20.0f) {
            params.y = screenPos.y + y;
            textSystem_->DrawText(params);
        }
    }

    void DrawButton(const UITransform &transform, const UIButton &button) {
        DirectX::XMFLOAT2 screenPos = transform.GetScreenPosition(screenWidth_, screenHeight_);
        DirectX::XMFLOAT4 color = button.GetCurrentColor();

        TextSystem::TextParams params;
        params.text = L"█";
        params.x = screenPos.x;
        params.y = screenPos.y;
        params.width = transform.size.x;
        params.height = transform.size.y;
        params.color = color;
        params.formatId = "panel";

        for (float y = 0; y < transform.size.y; y += 20.0f) {
            params.y = screenPos.y + y;
            textSystem_->DrawText(params);
        }
    }

    void DrawButtonText(const UITransform &transform, const UIText &text) {
        DrawText(transform, text);
    }

    void DrawText(const UITransform &transform, const UIText &text) {
        DirectX::XMFLOAT2 screenPos = transform.GetScreenPosition(screenWidth_, screenHeight_);

        TextSystem::TextParams params;
        params.text = text.text;
        params.x = screenPos.x;
        params.y = screenPos.y;
        params.width = transform.size.x;
        params.height = transform.size.y;
        params.color = text.color;
        params.formatId = text.formatId;
        params.fontSize = text.fontSize;

        textSystem_->DrawText(params);
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

    void OnUpdate(World &w, Entity self, float dt) override {
        if (!input_)
            return;

        // マウス座標を取得
        float mouseX = static_cast<float>(input_->GetMouseX());
        float mouseY = static_cast<float>(input_->GetMouseY());
        bool leftClick = input_->GetMouseButtonDown(InputSystem::Left);
        bool leftHeld = input_->GetMouseButton(InputSystem::Left);

        w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &transform, UIButton &button) {
            if (!button.enabled) {
                button.state = UIButton::State::Disabled;
                return;
            }

            bool isHovered = transform.Contains(
                mouseX,
                mouseY,
                screenWidth_,
                screenHeight_);

            if (isHovered) {
                if (leftHeld) {
                    button.state = UIButton::State::Pressed;
                } else {
                    button.state = UIButton::State::Hovered;
                    if (leftClick && button.onClick) {
                        button.onClick();
                    }
                }
            } else {
                button.state = UIButton::State::Normal;
            }
        });
    }

    void SetInputSystem(InputSystem *input) {
        input_ = input;
    }
    void SetScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
    }
};
