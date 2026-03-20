Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$assetsRoot = Join-Path $root "Assets"
$unitsDir = Join-Path $assetsRoot "Units"
$buildingsDir = Join-Path $assetsRoot "Buildings"
$nodesDir = Join-Path $assetsRoot "Nodes"
$tilesDir = Join-Path $assetsRoot "Tiles"
$uiDir = Join-Path $assetsRoot "UI"
$iconsDir = Join-Path $uiDir "Icons"
$portraitsDir = Join-Path $uiDir "Portraits"
$effectsDir = Join-Path $assetsRoot "Effects"

foreach ($dir in @($assetsRoot, $unitsDir, $buildingsDir, $nodesDir, $tilesDir, $uiDir, $iconsDir, $portraitsDir, $effectsDir)) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
    }
}

function New-Brush([string]$hex) {
    return New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml($hex))
}

function New-Pen([string]$hex, [float]$width = 2) {
    return New-Object System.Drawing.Pen ([System.Drawing.ColorTranslator]::FromHtml($hex)), $width
}

function Fill-RoundedRect($g, $brush, [float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    $g.FillPath($brush, $path)
    $path.Dispose()
}

function Draw-UnitSheet(
    [string]$path,
    [string]$csvPath,
    [string]$bodyColor,
    [string]$accentColor,
    [string]$outlineColor,
    [string]$weaponColor,
    [string]$helmetColor
) {
    $bmp = New-Object System.Drawing.Bitmap 64, 32
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $shadowBrush = New-Brush "#33111111"
    $bodyBrush = New-Brush $bodyColor
    $accentBrush = New-Brush $accentColor
    $outlinePen = New-Pen $outlineColor 2
    $weaponBrush = New-Brush $weaponColor
    $helmetBrush = New-Brush $helmetColor

    for ($frame = 0; $frame -lt 2; $frame++) {
        $offsetX = $frame * 32
        $lift = if ($frame -eq 0) { 0 } else { -1 }

        $g.FillEllipse($shadowBrush, $offsetX + 7, 24, 18, 5)
        Fill-RoundedRect $g $bodyBrush ($offsetX + 10) (10 + $lift) 12 12 4
        Fill-RoundedRect $g $accentBrush ($offsetX + 12) (14 + $lift) 8 5 2
        $g.FillEllipse($helmetBrush, $offsetX + 11, (4 + $lift), 10, 9)
        $g.DrawEllipse($outlinePen, $offsetX + 11, (4 + $lift), 10, 9)
        $g.FillRectangle($weaponBrush, $offsetX + 20, (13 + $lift), 6, 2)
        $g.FillRectangle($weaponBrush, $offsetX + 23, (11 + $lift), 2, 6)
        $g.DrawLine($outlinePen, $offsetX + 14, 22 + $lift, $offsetX + 11, 28 + $lift)
        $g.DrawLine($outlinePen, $offsetX + 18, 22 + $lift, $offsetX + 21, 28 + $lift)
        $g.DrawLine($outlinePen, $offsetX + 10, 14 + $lift, $offsetX + 7, 19 + $lift)
        $g.DrawLine($outlinePen, $offsetX + 22, 14 + $lift, $offsetX + 26, 18 + $lift)
        $g.DrawRectangle($outlinePen, $offsetX + 10, (10 + $lift), 12, 12)
    }

    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()

    Set-Content -Path $csvPath -Value @("0,0,32,32", "32,0,32,32")
}

function Draw-Building(
    [string]$path,
    [int]$size,
    [string]$baseColor,
    [string]$accentColor,
    [string]$roofColor
) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $baseBrush = New-Brush $baseColor
    $accentBrush = New-Brush $accentColor
    $roofBrush = New-Brush $roofColor
    $outlinePen = New-Pen "#1B2632" 3

    Fill-RoundedRect $g $baseBrush 10 18 ($size - 20) ($size - 28) 10
    $g.FillPolygon($roofBrush, @(
        (New-Object System.Drawing.PointF 16, 26),
        (New-Object System.Drawing.PointF ($size / 2), 6),
        (New-Object System.Drawing.PointF ($size - 16), 26)
    ))
    Fill-RoundedRect $g $accentBrush 20 34 ($size - 40) 18 6
    Fill-RoundedRect $g $accentBrush 24 58 18 18 4
    Fill-RoundedRect $g $accentBrush ($size - 42) 58 18 18 4
    $g.DrawRectangle($outlinePen, 18, 32, ($size - 36), ($size - 46))
    $g.DrawLine($outlinePen, $size / 2, 38, $size / 2, ($size - 14))

    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-Node(
    [string]$path,
    [string]$ringColor,
    [string]$coreColor,
    [string]$highlightColor
) {
    $bmp = New-Object System.Drawing.Bitmap 72, 72
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $ringBrush = New-Brush $ringColor
    $coreBrush = New-Brush $coreColor
    $highlightBrush = New-Brush $highlightColor
    $outlinePen = New-Pen "#17222C" 3

    $g.FillEllipse($ringBrush, 8, 8, 56, 56)
    $g.FillEllipse($coreBrush, 18, 18, 36, 36)
    $g.FillEllipse($highlightBrush, 28, 16, 12, 12)
    $g.DrawEllipse($outlinePen, 8, 8, 56, 56)
    $g.DrawEllipse($outlinePen, 18, 18, 36, 36)

    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-Tile(
    [string]$path,
    [string]$baseColor,
    [string]$detailColor,
    [string]$lineColor
) {
    $bmp = New-Object System.Drawing.Bitmap 96, 96
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.ColorTranslator]::FromHtml($baseColor))

    $detailBrush = New-Brush $detailColor
    $linePen = New-Pen $lineColor 2

    for ($i = 0; $i -lt 12; $i++) {
        $x = (($i * 17) + 11) % 90
        $y = (($i * 23) + 7) % 90
        $w = 6 + ($i % 4)
        $h = 4 + (($i + 1) % 5)
        $g.FillEllipse($detailBrush, $x, $y, $w, $h)
    }

    $g.DrawRectangle($linePen, 0, 0, 95, 95)
    $g.DrawLine($linePen, 48, 0, 48, 96)
    $g.DrawLine($linePen, 0, 48, 96, 48)

    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-SelectionRing([string]$path, [string]$color) {
    $bmp = New-Object System.Drawing.Bitmap 96, 96
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $pen = New-Pen $color 6
    $g.DrawEllipse($pen, 12, 12, 72, 72)
    $g.DrawEllipse((New-Pen "#66FFFFFF" 2), 20, 20, 56, 56)
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-RallyMarker([string]$path, [string]$color) {
    $bmp = New-Object System.Drawing.Bitmap 64, 64
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $brush = New-Brush $color
    $pen = New-Pen "#183040" 3
    $g.FillEllipse($brush, 18, 6, 28, 28)
    $g.DrawEllipse($pen, 18, 6, 28, 28)
    $g.FillPolygon($brush, @(
        (New-Object System.Drawing.PointF 32, 56),
        (New-Object System.Drawing.PointF 22, 28),
        (New-Object System.Drawing.PointF 42, 28)
    ))
    $g.DrawPolygon($pen, @(
        (New-Object System.Drawing.PointF 32, 56),
        (New-Object System.Drawing.PointF 22, 28),
        (New-Object System.Drawing.PointF 42, 28)
    ))
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-Panel([string]$path, [int]$width, [int]$height, [string]$baseColor, [string]$accentColor) {
    $bmp = New-Object System.Drawing.Bitmap $width, $height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    Fill-RoundedRect $g (New-Brush $baseColor) 4 4 ($width - 8) ($height - 8) 12
    Fill-RoundedRect $g (New-Brush $accentColor) 10 10 ($width - 20) 16 8
    $g.DrawRectangle((New-Pen "#17232C" 3), 6, 6, ($width - 12), ($height - 12))
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-Icon(
    [string]$path,
    [string]$bgColor,
    [string]$shapeColor,
    [string]$shape
) {
    $bmp = New-Object System.Drawing.Bitmap 64, 64
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    Fill-RoundedRect $g (New-Brush $bgColor) 4 4 56 56 10
    $pen = New-Pen "#17232C" 3
    $shapeBrush = New-Brush $shapeColor

    switch ($shape) {
        "hammer" {
            $g.FillRectangle($shapeBrush, 30, 16, 8, 26)
            $g.FillRectangle($shapeBrush, 18, 14, 24, 8)
        }
        "sword" {
            $g.FillPolygon($shapeBrush, @(
                (New-Object System.Drawing.PointF 32, 12),
                (New-Object System.Drawing.PointF 40, 28),
                (New-Object System.Drawing.PointF 32, 44),
                (New-Object System.Drawing.PointF 24, 28)
            ))
            $g.FillRectangle($shapeBrush, 18, 44, 28, 6)
        }
        "shield" {
            $g.FillPolygon($shapeBrush, @(
                (New-Object System.Drawing.PointF 32, 12),
                (New-Object System.Drawing.PointF 46, 20),
                (New-Object System.Drawing.PointF 42, 44),
                (New-Object System.Drawing.PointF 32, 52),
                (New-Object System.Drawing.PointF 22, 44),
                (New-Object System.Drawing.PointF 18, 20)
            ))
        }
        "gear" {
            $g.FillEllipse($shapeBrush, 18, 18, 28, 28)
            $g.FillEllipse((New-Brush "#17232C"), 26, 26, 12, 12)
            $g.FillRectangle($shapeBrush, 28, 8, 8, 10)
            $g.FillRectangle($shapeBrush, 28, 46, 8, 10)
            $g.FillRectangle($shapeBrush, 8, 28, 10, 8)
            $g.FillRectangle($shapeBrush, 46, 28, 10, 8)
        }
        "node" {
            $g.FillEllipse($shapeBrush, 16, 16, 32, 32)
            $g.FillEllipse((New-Brush "#DFF8F4"), 26, 20, 10, 10)
        }
        "worker" {
            $g.FillEllipse($shapeBrush, 22, 12, 20, 18)
            $g.FillRectangle($shapeBrush, 20, 30, 24, 16)
        }
    }

    $g.DrawRectangle($pen, 4, 4, 56, 56)
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

function Draw-Portrait([string]$path, [string]$bgColor, [string]$mainColor, [string]$accentColor) {
    $bmp = New-Object System.Drawing.Bitmap 96, 96
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    Fill-RoundedRect $g (New-Brush $bgColor) 4 4 88 88 14
    $g.FillEllipse((New-Brush $accentColor), 26, 18, 44, 32)
    $g.FillRectangle((New-Brush $mainColor), 24, 46, 48, 28)
    $g.DrawRectangle((New-Pen "#18242E" 3), 6, 6, 84, 84)
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

Draw-UnitSheet (Join-Path $unitsDir "worker.png") (Join-Path $unitsDir "worker_idle.csv") "#B38F62" "#E6D4A9" "#5A4025" "#7F5E3D" "#F2E2B6"
Draw-UnitSheet (Join-Path $unitsDir "soldier.png") (Join-Path $unitsDir "soldier_walk.csv") "#4A7A8C" "#B7D7E8" "#213744" "#D3A04F" "#DCE9F0"
Draw-UnitSheet (Join-Path $unitsDir "defender.png") (Join-Path $unitsDir "defender_walk.csv") "#556270" "#A8B8C8" "#24303A" "#7A8DA1" "#E4EEF4"
Draw-UnitSheet (Join-Path $unitsDir "raider.png") (Join-Path $unitsDir "raider_walk.csv") "#9B5E39" "#E7B86A" "#4A2412" "#C94D3F" "#F1DDC2"
Draw-UnitSheet (Join-Path $unitsDir "ranger_mech.png") (Join-Path $unitsDir "ranger_mech_walk.csv") "#507A5B" "#A4D7AF" "#203828" "#D9E27A" "#D7F1DD"
Draw-UnitSheet (Join-Path $unitsDir "siege_unit.png") (Join-Path $unitsDir "siege_unit_walk.csv") "#6D586F" "#C6A7C8" "#332535" "#D66F5E" "#E7D8E8"

Draw-Building (Join-Path $buildingsDir "hq.png") 112 "#6F8B9E" "#DDE7EE" "#D9B36F"
Draw-Building (Join-Path $buildingsDir "barracks.png") 96 "#7E6F58" "#E3D5BB" "#B65A48"
Draw-Building (Join-Path $buildingsDir "factory.png") 104 "#69777F" "#D7E0E5" "#889F56"
Draw-Building (Join-Path $buildingsDir "node_hub.png") 82 "#5F7A6B" "#CEE7D6" "#6DA2C7"
Draw-Building (Join-Path $buildingsDir "defense_tower.png") 72 "#6E5D76" "#D9D0E2" "#C87859"

Draw-Node (Join-Path $nodesDir "metal_node.png") "#7A8F9E" "#C9D7E0" "#F3F8FB"
Draw-Node (Join-Path $nodesDir "energy_core.png") "#547D7A" "#7BE3D6" "#D7FFF7"

Draw-Tile (Join-Path $tilesDir "sand_base.png") "#CDBB8A" "#B79F6E" "#8D7850"
Draw-Tile (Join-Path $tilesDir "battlefield_grass.png") "#6E8760" "#86A172" "#4D6545"
Draw-Tile (Join-Path $tilesDir "metal_plating.png") "#72808A" "#94A4AF" "#495761"

Draw-SelectionRing (Join-Path $effectsDir "selection_ring_friendly.png") "#66D7FF"
Draw-SelectionRing (Join-Path $effectsDir "selection_ring_enemy.png") "#FF7A66"
Draw-RallyMarker (Join-Path $effectsDir "rally_marker.png") "#F2D16B"

Draw-Panel (Join-Path $uiDir "hud_panel.png") 512 160 "#CC1E2A33" "#665A7382"
Draw-Panel (Join-Path $uiDir "production_panel.png") 384 120 "#CC2A2320" "#667E6859"
Draw-Panel (Join-Path $uiDir "tooltip_panel.png") 280 100 "#D81F252B" "#667B98A9"

Draw-Icon (Join-Path $iconsDir "icon_build.png") "#7B5E4A" "#F0D29D" "hammer"
Draw-Icon (Join-Path $iconsDir "icon_worker.png") "#8A775F" "#F4E0B7" "worker"
Draw-Icon (Join-Path $iconsDir "icon_soldier.png") "#4F7389" "#E8F0F5" "sword"
Draw-Icon (Join-Path $iconsDir "icon_defender.png") "#66707F" "#DDE4EB" "shield"
Draw-Icon (Join-Path $iconsDir "icon_factory.png") "#63727E" "#CFDCE4" "gear"
Draw-Icon (Join-Path $iconsDir "icon_nodehub.png") "#5A7F73" "#D9FFF8" "node"

Draw-Portrait (Join-Path $portraitsDir "portrait_worker.png") "#7A664F" "#B59872" "#EAD8AF"
Draw-Portrait (Join-Path $portraitsDir "portrait_soldier.png") "#48697D" "#7DA4BA" "#DCEAF2"
Draw-Portrait (Join-Path $portraitsDir "portrait_defender.png") "#5D6672" "#8894A4" "#E0E8EE"
Draw-Portrait (Join-Path $portraitsDir "portrait_ranger_mech.png") "#4F735A" "#79B08A" "#D7F0DD"
Draw-Portrait (Join-Path $portraitsDir "portrait_hq.png") "#697E8F" "#AFC7D7" "#F0E0B2"
Draw-Portrait (Join-Path $portraitsDir "portrait_barracks.png") "#7F6A52" "#B79D79" "#E9D6BA"
