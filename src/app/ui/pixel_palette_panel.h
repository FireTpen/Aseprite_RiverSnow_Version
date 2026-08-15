// Aseprite
// Copyright (C) 2026 Palette Lab contributors
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PIXEL_PALETTE_PANEL_H_INCLUDED
#define APP_UI_PIXEL_PALETTE_PANEL_H_INCLUDED
#pragma once

#include "app/ui/color_shades.h"
#include "app/ui/dockable.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/label.h"

namespace app {

// A compact, dockable role-first palette assistant for pixel-art assets.
// It deliberately lives beside the native ColorBar so its role names remain
// visible while the artist paints.
class PixelPalettePanel final : public ui::Box,
                                public Dockable {
public:
  PixelPalettePanel();

  int dockableAt() const override { return ui::LEFT | ui::RIGHT | ui::EXPANSIVE; }

private:
  void setPreset(int presetIndex);
  void onShadeClick(ColorShades::ClickEvent& ev);

  ui::Label m_title;
  ui::Box m_presets;
  ui::Button m_bush;
  ui::Button m_tree;
  ui::Button m_water;
  ui::Button m_grass;
  ui::Button m_earth;
  ColorShades m_shades;
  ui::Label m_roleHint;
  ui::Label m_usageHint;
  int m_presetIndex = 0;
};

} // namespace app

#endif
