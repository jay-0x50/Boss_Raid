param(
    [string]$AssetSourceRoot = (Join-Path $PSScriptRoot '..\SourceAssets\ThirdParty')
)

$ErrorActionPreference = 'Stop'

function Get-CheckedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination,
        [string]$Md5 = ''
    )

    $parent = Split-Path -Parent $Destination
    New-Item -ItemType Directory -Force -Path $parent | Out-Null

    if (Test-Path -LiteralPath $Destination) {
        if (-not $Md5 -or (Get-FileHash -LiteralPath $Destination -Algorithm MD5).Hash -eq $Md5) {
            return
        }
    }

    Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    if ($Md5 -and (Get-FileHash -LiteralPath $Destination -Algorithm MD5).Hash -ne $Md5) {
        throw "Checksum mismatch: $Destination"
    }
}

$AssetSourceRoot = [IO.Path]::GetFullPath($AssetSourceRoot)
New-Item -ItemType Directory -Force -Path $AssetSourceRoot | Out-Null

$kenneyRoot = Join-Path $AssetSourceRoot 'Kenney_ModularDungeon'
$kenneyZip = Join-Path ([IO.Path]::GetTempPath()) 'exception_kenney_modular_dungeon.zip'
Get-CheckedFile `
    -Url 'https://kenney.nl/media/pages/assets/modular-dungeon-kit/7bed87605b-1771926065/kenney_modular-dungeon-kit_1.0.zip' `
    -Destination $kenneyZip

$extractRoot = Join-Path ([IO.Path]::GetTempPath()) ("exception_kenney_" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $extractRoot | Out-Null
Expand-Archive -LiteralPath $kenneyZip -DestinationPath $extractRoot

$kenneyFiles = @(
    'corridor-corner.fbx',
    'corridor-end.fbx',
    'gate.fbx',
    'gate-metal-bars.fbx',
    'room-corner.fbx',
    'stairs-wide.fbx',
    'template-floor-big.fbx',
    'template-floor-detail-a.fbx',
    'template-wall.fbx',
    'template-wall-corner.fbx',
    'template-wall-detail-a.fbx',
    'template-wall-half.fbx',
    'template-wall-top.fbx'
)

New-Item -ItemType Directory -Force -Path $kenneyRoot | Out-Null
foreach ($file in $kenneyFiles) {
    Copy-Item -LiteralPath (Join-Path $extractRoot "Models\FBX format\$file") -Destination (Join-Path $kenneyRoot $file) -Force
}
Copy-Item -LiteralPath (Join-Path $extractRoot 'Models\FBX format\Textures\colormap.png') -Destination (Join-Path $kenneyRoot 'colormap.png') -Force
Copy-Item -LiteralPath (Join-Path $extractRoot 'License.txt') -Destination (Join-Path $kenneyRoot 'LICENSE.txt') -Force

$polyFiles = @(
    @('PolyHaven_ModularFort01\modular_fort_01_1k.fbx', 'https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/modular_fort_01/modular_fort_01_1k.fbx', '4f0429ef44265aa6789c7dffaa1f0cff'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_trim_diff_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_trim_diff_1k.png', 'a004a447a5340588223680a8680da29c'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_wall_diff_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_wall_diff_1k.png', '59dea3b2313c8f7d672aa4625d922ace'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_plaster_diff_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_plaster_diff_1k.png', '2c6061ae400c2fba8130455b40819440'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_trim_nor_gl_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_trim_nor_gl_1k.png', '084044b1606548c186f4e667f4c5d6c7'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_wall_nor_gl_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_wall_nor_gl_1k.png', 'd635d16e286d79d3cc0eb26913d8df6e'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_plaster_nor_gl_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_plaster_nor_gl_1k.png', 'f3b4acd68380487a8cc0757c2e14500d'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_trim_rough_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_trim_rough_1k.png', '437f7cacd435532d0090862861a71d89'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_wall_rough_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_wall_rough_1k.png', 'adcadc5a5767884a26faa1f18587df12'),
    @('PolyHaven_ModularFort01\textures\modular_fort_01_plaster_rough_1k.png', 'https://dl.polyhaven.org/file/ph-assets/Models/png/1k/modular_fort_01/modular_fort_01_plaster_rough_1k.png', '723f3abb7439a4c048f23ea45b614a52'),
    @('PolyHaven_Boulder01\boulder_01_1k.fbx', 'https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/boulder_01/boulder_01_1k.fbx', 'e6079d3ddb9a71716d88274808e2d92c'),
    @('PolyHaven_Boulder01\textures\boulder_01_diff_1k.jpg', 'https://dl.polyhaven.org/file/ph-assets/Models/jpg/1k/boulder_01/boulder_01_diff_1k.jpg', '25f8a843f13369d56d37f49db05ea8c3'),
    @('PolyHaven_Boulder01\textures\boulder_01_nor_gl_1k.exr', 'https://dl.polyhaven.org/file/ph-assets/Models/exr/1k/boulder_01/boulder_01_nor_gl_1k.exr', '8313f3c7c6b1df797512ddeb54f176d2'),
    @('PolyHaven_Boulder01\textures\boulder_01_rough_1k.exr', 'https://dl.polyhaven.org/file/ph-assets/Models/exr/1k/boulder_01/boulder_01_rough_1k.exr', '9c5f53182895bcf50d57e9fe5a2e7379'),
    @('PolyHaven_Mountainside\mountainside_1k.fbx', 'https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/mountainside/mountainside_1k.fbx', '8a5a70807e066b78e4678e20dbdc5cca'),
    @('PolyHaven_Mountainside\textures\mountainside_diff_1k.jpg', 'https://dl.polyhaven.org/file/ph-assets/Models/jpg/1k/mountainside/mountainside_diff_1k.jpg', 'ab12251ec12488e2a0cf2a213d52bd7d'),
    @('PolyHaven_Mountainside\textures\mountainside_nor_gl_1k.exr', 'https://dl.polyhaven.org/file/ph-assets/Models/exr/1k/mountainside/mountainside_nor_gl_1k.exr', '24f1a656a456802422c20660e6aef11c'),
    @('PolyHaven_Mountainside\textures\mountainside_rough_1k.exr', 'https://dl.polyhaven.org/file/ph-assets/Models/exr/1k/mountainside/mountainside_rough_1k.exr', '0d707f18aefcad749d57ba10df8cbe30')
)

foreach ($entry in $polyFiles) {
    Get-CheckedFile -Destination (Join-Path $AssetSourceRoot $entry[0]) -Url $entry[1] -Md5 $entry[2]
}

$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
$resolvedExtract = [IO.Path]::GetFullPath($extractRoot)
if ($resolvedExtract.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
    Remove-Item -LiteralPath $resolvedExtract -Recurse -Force
}

Write-Host "Boss environment sources acquired at $AssetSourceRoot"
