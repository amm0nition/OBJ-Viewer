param(
    [string]$VertPath = "shaders\model.vert",
    [string]$FragPath = "shaders\model.frag",
    [string]$OutHeader = "src\shaders.h"
)

if ((Test-Path $VertPath) -and (Test-Path $FragPath)) {
    $vert = (Get-Content $VertPath -Raw).Trim()
    $frag = (Get-Content $FragPath -Raw).Trim()

    $content = @"
#pragma once

const char* vertexShaderSource = R"(
$vert
)";

const char* fragmentShaderSource = R"(
$frag
)";
"@
    Set-Content -Path $OutHeader -Value $content -Encoding utf8
}
