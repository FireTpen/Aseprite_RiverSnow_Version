-- Pixel Palette Lab for Aseprite
-- A role-first palette helper for 2D pixel art.

local ROLE_STEPS = {
  { name = "极高光", light = 34, sat = -22 },
  { name = "高光",   light = 24, sat = -12 },
  { name = "亮部",   light = 14, sat = -5 },
  { name = "受光",   light = 6,  sat = 2 },
  { name = "主体",   light = 0,  sat = 0 },
  { name = "中阴影", light = -11, sat = 5 },
  { name = "深阴影", light = -22, sat = 0 },
  { name = "轮廓",   light = -32, sat = -16 },
}

local ASSETS = {
  { id = "灌木", hue = 108, sat = 61, light = 45, note = "叶片从受光到轮廓；先用主体和中阴影铺体积。" },
  { id = "树冠", hue = 96,  sat = 56, light = 43, note = "树冠用 5-7 色；将极高光留给零星叶片反光。" },
  { id = "水体", hue = 203, sat = 66, light = 47, note = "水体以主体、受光和高光制造波纹，轮廓少量使用。" },
  { id = "草地", hue = 86,  sat = 53, light = 46, note = "草地可用主体铺底，亮部和深阴影组成碎块纹理。" },
  { id = "土地", hue = 31,  sat = 52, light = 45, note = "土地优先控制明度差，再用受光色画颗粒与边缘。" },
}

local state = { asset = ASSETS[1], hue = ASSETS[1].hue, sat = ASSETS[1].sat, light = ASSETS[1].light, rows = {} }
local dlg = nil
local dragging = false

local function clamp(value, low, high)
  return math.max(low, math.min(high, value))
end

local function round(value)
  return math.floor(value + 0.5)
end

local function color_from_hsl(h, s, l)
  h = (h % 360) / 360
  s = clamp(s, 0, 100) / 100
  l = clamp(l, 0, 100) / 100
  local r, g, b
  if s == 0 then
    r, g, b = l, l, l
  else
    local function hue_to_rgb(p, q, t)
      if t < 0 then t = t + 1 end
      if t > 1 then t = t - 1 end
      if t < 1/6 then return p + (q - p) * 6 * t end
      if t < 1/2 then return q end
      if t < 2/3 then return p + (q - p) * (2/3 - t) * 6 end
      return p
    end
    local q = l < 0.5 and l * (1 + s) or l + s - l * s
    local p = 2 * l - q
    r = hue_to_rgb(p, q, h + 1/3)
    g = hue_to_rgb(p, q, h)
    b = hue_to_rgb(p, q, h - 1/3)
  end
  return Color { r = round(r * 255), g = round(g * 255), b = round(b * 255), a = 255 }
end

local function hex(color)
  return string.format("#%02X%02X%02X", color.red, color.green, color.blue)
end

local function regenerate()
  state.rows = {}
  for _, step in ipairs(ROLE_STEPS) do
    table.insert(state.rows, {
      name = step.name,
      color = color_from_hsl(state.hue, state.sat + step.sat, state.light + step.light),
    })
  end
end

local function sync_colors_from_dialog()
  if not dlg then return end
  for index, row in ipairs(state.rows) do
    local color = dlg.data["role_" .. index]
    if color then row.color = color end
  end
end

local function role_colors()
  local colors = {}
  for _, row in ipairs(state.rows) do table.insert(colors, row.color) end
  return colors
end

local function apply_to_sprite()
  sync_colors_from_dialog()
  if not app.sprite then
    app.alert("请先打开或新建一个精灵，再应用调色板。")
    return
  end
  app.transaction("应用 Pixel Palette Lab 调色板", function()
    local palette = Palette(#state.rows)
    for index, row in ipairs(state.rows) do
      palette:setColor(index - 1, row.color)
    end
    app.sprite:setPalette(palette)
  end)
  app.alert("已应用 " .. #state.rows .. " 个颜色。颜色角色保留在插件列表与导出文件中。")
end

local function gpl_text()
  sync_colors_from_dialog()
  local lines = {
    "GIMP Palette",
    "Name: Pixel Palette Lab - " .. state.asset.id,
    "Columns: 4",
    "# 角色命名色板：可与 JSON 一起保存，便于在 Aseprite 外查阅用途。",
  }
  for _, row in ipairs(state.rows) do
    table.insert(lines, string.format("%3d %3d %3d %s", row.color.red, row.color.green, row.color.blue, row.name))
  end
  return table.concat(lines, "\n") .. "\n"
end

local function json_text()
  sync_colors_from_dialog()
  local colors = {}
  for _, row in ipairs(state.rows) do
    table.insert(colors, { role = row.name, hex = hex(row.color), r = row.color.red, g = row.color.green, b = row.color.blue })
  end
  return json.encode({
    tool = "Pixel Palette Lab",
    asset = state.asset.id,
    hue = state.hue,
    saturation = state.sat,
    lightness = state.light,
    colors = colors,
  })
end

local function write_export(kind)
  sync_colors_from_dialog()
  local extension = kind == "gpl" and "gpl" or "json"
  local picker = Dialog { title = "导出 " .. string.upper(extension) }
  picker:file {
    id = "filename",
    label = "保存位置",
    save = true,
    entry = true,
    filename = "pixel-palette-" .. state.asset.id .. "." .. extension,
    filetypes = { extension },
  }
  picker:button { id = "save", text = "导出", focus = true }
  picker:button { id = "cancel", text = "取消" }
  picker:show()
  if not picker.data.save or not picker.data.filename or picker.data.filename == "" then return end
  local file, err = io.open(picker.data.filename, "w")
  if not file then
    app.alert("无法写入文件：" .. tostring(err))
    return
  end
  file:write(kind == "gpl" and gpl_text() or json_text())
  file:close()
  app.alert("已导出：" .. picker.data.filename)
end

local function hue_from_pointer(x, y, width, height)
  local cx, cy = width / 2, height / 2
  local dx, dy = x - cx, y - cy
  local radius = math.min(width, height) * 0.5
  local distance = math.sqrt(dx * dx + dy * dy)
  if distance < radius * 0.52 or distance > radius then return nil end
  -- Aseprite embeds Lua 5.4, where math.atan(y, x) is the portable atan2 form.
  return (math.deg(math.atan(dy, dx)) + 90 + 360) % 360
end

local function draw_wheel(ev)
  local gc = ev.context
  local width, height = gc.width, gc.height
  local cx, cy = width / 2, height / 2
  local radius = math.min(width, height) * 0.48
  gc.antialias = true
  gc.color = Color { r = 35, g = 39, b = 48 }
  gc:fillRect(Rectangle(0, 0, width, height))

  for index = 0, 119 do
    local a1 = math.rad(index * 3 - 90)
    local a2 = math.rad((index + 1) * 3 - 90)
    local inner = radius * 0.54
    gc.color = color_from_hsl(index * 3, 86, 54)
    gc:beginPath()
    gc:moveTo(cx + math.cos(a1) * inner, cy + math.sin(a1) * inner)
    gc:lineTo(cx + math.cos(a1) * radius, cy + math.sin(a1) * radius)
    gc:lineTo(cx + math.cos(a2) * radius, cy + math.sin(a2) * radius)
    gc:lineTo(cx + math.cos(a2) * inner, cy + math.sin(a2) * inner)
    gc:closePath()
    gc:fill()
  end

  gc.color = color_from_hsl(state.hue, state.sat, state.light)
  gc:beginPath()
  gc:oval(Rectangle(cx - radius * 0.45, cy - radius * 0.45, radius * 0.9, radius * 0.9))
  gc:fill()
  gc.color = Color { r = 255, g = 255, b = 255, a = 150 }
  gc.strokeWidth = 2
  gc:beginPath()
  gc:oval(Rectangle(cx - radius * 0.45, cy - radius * 0.45, radius * 0.9, radius * 0.9))
  gc:stroke()

  local angle = math.rad(state.hue - 90)
  local marker = radius * 0.77
  local mx, my = cx + math.cos(angle) * marker, cy + math.sin(angle) * marker
  gc.color = Color { r = 22, g = 25, b = 32 }
  gc:beginPath(); gc:oval(Rectangle(mx - 7, my - 7, 14, 14)); gc:fill()
  gc.color = Color { r = 255, g = 255, b = 255 }
  gc.strokeWidth = 2
  gc:beginPath(); gc:oval(Rectangle(mx - 7, my - 7, 14, 14)); gc:stroke()
  gc.color = Color { r = 240, g = 244, b = 255 }
  gc:fillText("拖动外环选择色相 · 当前 " .. round(state.hue) .. "°", 10, height - 18)
end

local function refresh_dialog()
  regenerate()
  if not dlg then return end
  dlg:modify { id = "hue", value = round(state.hue) }
  dlg:modify { id = "sat", value = round(state.sat) }
  dlg:modify { id = "light", value = round(state.light) }
  dlg:modify { id = "swatches", colors = role_colors() }
  dlg:modify { id = "note", text = state.asset.note }
  for index, row in ipairs(state.rows) do
    dlg:modify { id = "role_" .. index, color = row.color }
    dlg:modify { id = "hex_" .. index, text = row.name .. "  " .. hex(row.color) }
  end
  dlg:repaint()
end

local function select_asset(asset_name)
  for _, asset in ipairs(ASSETS) do
    if asset.id == asset_name then
      state.asset = asset
      state.hue, state.sat, state.light = asset.hue, asset.sat, asset.light
      refresh_dialog()
      return
    end
  end
end

local function show_palette_lab()
  regenerate()
  local names = {}
  for _, asset in ipairs(ASSETS) do table.insert(names, asset.id) end
  dlg = Dialog { title = "Pixel Palette Lab · 像素资产配色", resizeable = false }
  dlg:combobox {
    id = "asset", label = "资产预设", option = state.asset.id, options = names,
    onchange = function() select_asset(dlg.data.asset) end,
  }
  dlg:canvas {
    id = "wheel", width = 270, height = 270, autoscaling = true, mousecursor = MouseCursor.CROSSHAIR,
    onpaint = draw_wheel,
    onmousedown = function(ev)
      local hue = hue_from_pointer(ev.x, ev.y, 270, 270)
      if hue then dragging = true; state.hue = hue; refresh_dialog() end
    end,
    onmousemove = function(ev)
      if not dragging then return end
      local hue = hue_from_pointer(ev.x, ev.y, 270, 270)
      if hue then state.hue = hue; refresh_dialog() end
    end,
    onmouseup = function() dragging = false end,
  }
  dlg:slider { id = "hue", label = "色相", min = 0, max = 359, value = state.hue, onchange = function() state.hue = dlg.data.hue; refresh_dialog() end }
  dlg:slider { id = "sat", label = "饱和度", min = 15, max = 95, value = state.sat, onchange = function() state.sat = dlg.data.sat; refresh_dialog() end }
  dlg:slider { id = "light", label = "主体明度", min = 20, max = 75, value = state.light, onchange = function() state.light = dlg.data.light; refresh_dialog() end }
  dlg:label { id = "note", text = state.asset.note }
  dlg:shades {
    id = "swatches", label = "左键前景 / 右键背景", mode = "pick", colors = role_colors(),
    onclick = function(ev)
      if ev.button == MouseButton.LEFT then app.fgColor = ev.color
      elseif ev.button == MouseButton.RIGHT then app.bgColor = ev.color end
    end,
  }
  for index, row in ipairs(state.rows) do
    dlg:color {
      id = "role_" .. index, label = row.name, color = row.color,
      onchange = function() state.rows[index].color = dlg.data["role_" .. index]; dlg:modify { id = "hex_" .. index, text = row.name .. "  " .. hex(state.rows[index].color) }; dlg:modify { id = "swatches", colors = role_colors() } end,
    }
    dlg:label { id = "hex_" .. index, text = row.name .. "  " .. hex(row.color) }
  end
  dlg:button { text = "应用到当前精灵", onclick = apply_to_sprite }
  dlg:newrow()
  dlg:button { text = "导出 GPL（带角色名）", onclick = function() write_export("gpl") end }
  dlg:button { text = "导出 JSON", onclick = function() write_export("json") end }
  dlg:button { text = "关闭", onclick = function() dlg:close() end }
  dlg:show { wait = false }
end

function init(plugin)
  -- `file_scripts` is Aseprite's permanent Scripts submenu declared in gui.xml.
  -- Register directly there so the command does not rely on dynamic menu groups.
  plugin:newCommand {
    id = "PixelPaletteLab",
    title = "打开像素配色助手...",
    group = "file_scripts",
    onclick = show_palette_lab,
  }
end
