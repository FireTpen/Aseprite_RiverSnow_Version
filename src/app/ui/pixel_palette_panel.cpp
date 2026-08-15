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
  std::array<app::Color, 8> colors;
};

const std::array<Preset, 5> kPresets = {{
  { "灌木", "先用主体铺形，再用中阴影刻体积；极高光只点少量叶尖。", {
      Color::fromRgb(172, 221, 143), Color::fromRgb(132, 196, 116),
      Color::fromRgb(99, 172, 80),   Color::fromRgb(72, 148, 57),
      Color::fromRgb(54, 126, 43),   Color::fromRgb(43, 100, 37),
      Color::fromRgb(35, 75, 33),    Color::fromRgb(25, 52, 28) } },
  { "树冠", "高光留给树冠上缘；深阴影用于叶团之间的缝隙。", {
      Color::fromRgb(205, 225, 132), Color::fromRgb(158, 192, 88),
      Color::fromRgb(115, 159, 59),  Color::fromRgb(81, 130, 43),
      Color::fromRgb(61, 107, 37),   Color::fromRgb(45, 82, 33),
      Color::fromRgb(32, 61, 29),    Color::fromRgb(21, 43, 25) } },
  { "水体", "主体铺水面，受光和亮部画波纹；轮廓只留给深水边界。", {
      Color::fromRgb(176, 237, 239), Color::fromRgb(112, 210, 220),
      Color::fromRgb(68, 181, 196),  Color::fromRgb(37, 155, 174),
      Color::fromRgb(25, 129, 151),  Color::fromRgb(20, 101, 125),
      Color::fromRgb(17, 73, 96),    Color::fromRgb(10, 46, 70) } },
  { "草地", "用主体铺底，再以亮部和中阴影做不规则小块，避免棋盘格。", {
      Color::fromRgb(220, 229, 142), Color::fromRgb(177, 199, 92),
      Color::fromRgb(134, 169, 62),  Color::fromRgb(99, 143, 43),
      Color::fromRgb(76, 119, 36),   Color::fromRgb(58, 94, 31),
      Color::fromRgb(41, 69, 27),    Color::fromRgb(27, 49, 23) } },
  { "土地", "先看明度层级；受光色画颗粒，轮廓用于石块和地块分界。", {
      Color::fromRgb(243, 210, 154), Color::fromRgb(216, 169, 108),
      Color::fromRgb(188, 132, 78),  Color::fromRgb(158, 104, 58),
      Color::fromRgb(132, 82, 46),   Color::fromRgb(104, 61, 37),
      Color::fromRgb(77, 44, 29),    Color::fromRgb(52, 30, 23) } },
}};

Shade make_shade(const Preset& preset)
{
  return Shade(preset.colors.begin(), preset.colors.end());
}

} // namespace

PixelPalettePanel::PixelPalettePanel()
  : Box(VERTICAL)
  , m_title("Pixel Palette Lab · 像素资产配色")
  , m_presets(HORIZONTAL)
  , m_bush("灌木")
  , m_tree("树冠")
  , m_water("水体")
  , m_grass("草地")
  , m_earth("土地")
  , m_shades(make_shade(kPresets[0]), ColorShades::ClickEntries)
  , m_roleHint("左键：前景色 · 右键：背景色 · 极高光 → 轮廓")
  , m_usageHint(kPresets[0].usage)
{
  setBorder(gfx::Border(4 * ui::guiscale()));
  addChild(&m_title);
  addChild(&m_presets);
  addChild(&m_shades);
  addChild(&m_roleHint);
  addChild(&m_usageHint);

  m_presets.addChild(&m_bush);
  m_presets.addChild(&m_tree);
  m_presets.addChild(&m_water);
  m_presets.addChild(&m_grass);
  m_presets.addChild(&m_earth);

  m_shades.setExpansive(true);
  m_bush.Click.connect([this] { setPreset(0); });
  m_tree.Click.connect([this] { setPreset(1); });
  m_water.Click.connect([this] { setPreset(2); });
  m_grass.Click.connect([this] { setPreset(3); });
  m_earth.Click.connect([this] { setPreset(4); });
  m_shades.Click.connect(&PixelPalettePanel::onShadeClick, this);

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
  m_shades.setShade(make_shade(preset));
  m_usageHint.setText(preset.usage);
  layout();
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

} // namespace app
