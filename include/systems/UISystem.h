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
#include "ecs/World.h"
#include <DirectXMath.h>
#include "app/ServiceLocator.h"

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

        auto drawTextPass = [&]() {
            textSystem_->BeginDraw();
            w.ForEach<UICanvas>([&](Entity, UICanvas &canvas) {
                if (!canvas.enabled)
                    return;

                w.ForEach<UITransform, UIPanel>([&](Entity, UITransform &t, UIPanel &p) {
                    if (p.visible)
                        DrawPanel(t, p);
                });

                w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &t, UIButton &b) {
                    DrawButton(t, b);
                    if (auto *txt = w.TryGet<UIText>(e))
                        DrawButtonText(t, *txt);
                });

                w.ForEach<UITransform, UIText>([&](Entity e, UITransform &t, UIText &txt) {
                    if (!w.Has<UIButton>(e))
                        DrawText(t, txt);
                });
            });
            textSystem_->EndDraw();
        };

        drawImages(false);
        drawTextPass();
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
        pos.x += renderOffsetX_; // Apply transition offset
        DirectX::XMFLOAT4 color = button.GetCurrentColor();
        TextSystem::TextParams p;
        p.text = L"█";
        p.x = pos.x;
        p.y = pos.y;
        p.width = transform.size.x;
        p.height = transform.size.y;
        p.color = color;
        p.formatId = "panel";
        for (float y = 0; y < transform.size.y; y += 20.0f) {
            p.y = pos.y + y;
            textSystem_->DrawText(p);
        }
    }
    void DrawButtonText(const UITransform &transform, const UIText &text) {
        DrawText(transform, text);
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
            imageSystem_->Draw(img.textureHandle, pos.x, pos.y, transform.size.x, transform.size.y, img.opacity, img.keepAspect, srcPtr);
        } else {
            ImageSystem::Params p;
            p.filePath = img.filePath;
            p.x = pos.x;
            p.y = pos.y;
            p.width = transform.size.x;
            p.height = transform.size.y;
            p.opacity = img.opacity;
            p.keepAspect = img.keepAspect;
            p.srcX = uv[0];
            p.srcY = uv[1];
            p.srcW = uv[2];
            p.srcH = uv[3];
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

    void OnUpdate(World &w, Entity self, float dt) override {
        if (!input_)
            return;
        float mx = static_cast<float>(input_->GetMouseX());
        float my = static_cast<float>(input_->GetMouseY());
        bool leftClick = input_->GetMouseButtonDown(InputSystem::Left);
        bool leftHeld = input_->GetMouseButton(InputSystem::Left);
        w.ForEach<UITransform, UIButton>([&](Entity e, UITransform &t, UIButton &b) {
            if (!b.enabled) {
                b.state = UIButton::State::Disabled;
                return;
            }
            boolean hover = t.Contains(mx, my, screenWidth_, screenHeight_);
            if (hover) {
                if (leftHeld)
                    b.state = UIButton::State::Pressed;
                else {
                    b.state = UIButton::State::Hovered;
                    if (leftClick && b.onClick)
                        b.onClick();
                }
            } else
                b.state = UIButton::State::Normal;
        });
    }
    void SetInputSystem(InputSystem *input) {
        input_ = input;
    }
    void SetScreenSize(float w, float h) {
        screenWidth_ = w;
        screenHeight_ = h;
    }
};
