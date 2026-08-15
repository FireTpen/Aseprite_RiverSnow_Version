// Aseprite
// Copyright (C) 2026 Palette Lab contributors
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/pixel_palette_panel.h"

#include "app/ui/color_bar.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/mouse_button.h"
#include "ui/size_hint_event.h"

#include <algorithm>
#include <array>
#include <string>

namespace app {

namespace {

constexpr std::array<const char*, 8> kRoles = {
  "极高光", "高光", "亮部", "受光", "主体", "中阴影", "深阴影", "轮廓",
};

struct Preset {
  const char* name;
  const char* usage;
  app::Color base;
};

const std::array<Preset, 5> kPresets = {{
  { "灌木", "先铺主体，再用中阴影塑形；极高光只点叶尖。", Color::fromRgb(54, 126, 43) },
  { "树冠", "高光留给上缘，深阴影用于叶团缝隙。", Color::fromRgb(61, 107, 37) },
  { "水体", "受光与亮部画波纹，轮廓仅用于深水边界。", Color::fromRgb(25, 129, 151) },
  { "草地", "亮部和中阴影做碎块，避免规则棋盘格。", Color::fromRgb(76, 119, 36) },
  { "土地", "优先控制明度差，再用受光色画颗粒。", Color::fromRgb(132, 82, 46) },
}};

Shade make_roles(const app::Color& base)
{
  constexpr std::array<double, 8> kLightness = { 0.34, 0.24, 0.14, 0.06, 0.0, -0.11, -0.22, -0.32 };
  constexpr std::array<double, 8> kSaturation = { -0.22, -0.12, -0.05, 0.02, 0.0, 0.05, 0.0, -0.16 };
  Shade shade;
  const auto rgb = base.toRgb();
  const auto hue = rgb.getHslHue();
  const auto sat = rgb.getHslSaturation();
  const auto light = rgb.getHslLightness();
  for (size_t i = 0; i < kRoles.size(); ++i)
    shade.push_back(Color::fromHsl(hue,
                                   std::clamp(sat + kSaturation[i], 0.0, 1.0),
                                   std::clamp(light + kLightness[i], 0.0, 1.0)));
  return shade;
}

} // namespace

PixelPalettePanel::PixelPalettePanel()
  : Box(ui::VERTICAL)
  , m_title("Palette Lab · 灌木")
  , m_presets(ui::HORIZONTAL)
  , m_bush("灌")
  , m_tree("树")
  , m_water("水")
  , m_grass("草")
  , m_earth("土")
  , m_shades(make_roles(kPresets[0].base), ColorShades::ClickEntries)
  , m_roleHint("色阶：极高光 → 高光 → 亮部 → 受光 → 主体 → 阴影 → 轮廓")
  , m_usageHint(kPresets[0].usage)
  , m_baseColor(kPresets[0].base)
{
  setBorder(gfx::Border(4 * ui::guiscale()));
  addChild(&m_title);
  addChild(&m_presets);
  addChild(&m_wheel);
  addChild(&m_shades);
  addChild(&m_roleHint);
  addChild(&m_usageHint);

  m_presets.addChild(&m_bush);
  m_presets.addChild(&m_tree);
  m_presets.addChild(&m_water);
  m_presets.addChild(&m_grass);
  m_presets.addChild(&m_earth);

  m_wheel.setExpansive(true);
  m_wheel.selectColor(m_baseColor);
  m_bush.Click.connect([this] { setPreset(0); });
  m_tree.Click.connect([this] { setPreset(1); });
  m_water.Click.connect([this] { setPreset(2); });
  m_grass.Click.connect([this] { setPreset(3); });
  m_earth.Click.connect([this] { setPreset(4); });
  m_shades.Click.connect(&PixelPalettePanel::onShadeClick, this);
  m_wheel.ColorChange.connect(&PixelPalettePanel::onWheelColor, this);

  InitTheme.connect([this] {
    auto* theme = skin::SkinTheme::get(this);
    setStyle(theme->styles.workspaceView());
  });
  initTheme();
}

void PixelPalettePanel::setPreset(int presetIndex)
{
  m_presetIndex = presetIndex;
  const auto& preset = kPresets[m_presetIndex];
  m_baseColor = preset.base;
  m_title.setText(std::string("Palette Lab · ") + preset.name);
  m_wheel.selectColor(m_baseColor);
  regenerateRoles(m_baseColor);
  m_usageHint.setText(preset.usage);
  layout();
}

void PixelPalettePanel::onWheelColor(const app::Color& color, ui::MouseButton button)
{
  m_baseColor = color.toRgb();
  regenerateRoles(m_baseColor);
  if (auto* colorBar = ColorBar::instance()) {
    if (button == ui::kButtonRight)
      colorBar->setBgColor(m_baseColor);
    else
      colorBar->setFgColor(m_baseColor);
  }
  m_title.setText("Palette Lab · 自定义色相");
}

void PixelPalettePanel::regenerateRoles(const app::Color& baseColor)
{
  m_shades.setShade(make_roles(baseColor));
}

void PixelPalettePanel::onShadeClick(ColorShades::ClickEvent& ev)
{
  const int index = m_shades.getHotEntry();
  if (index < 0 || index >= int(kRoles.size()))
    return;

  const auto color = m_shades.getShade()[index];
  if (auto* colorBar = ColorBar::instance()) {
    if (ev.button() == ui::kButtonRight)
      colorBar->setBgColor(color);
    else
      colorBar->setFgColor(color);
  }

  m_roleHint.setText(std::string(kRoles[index]) + " · " + color.toString() +
                     (ev.button() == ui::kButtonRight ? " → 背景色" : " → 前景色"));
}

void PixelPalettePanel::onSizeHint(ui::SizeHintEvent& ev)
{
  Box::onSizeHint(ev);
  auto size = ev.sizeHint();
  size.w = std::max(size.w, 260 * ui::guiscale());
  size.h = std::max(size.h, 360 * ui::guiscale());
  ev.setSizeHint(size);
}

} // namespace app
