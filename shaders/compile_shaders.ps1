Get-ChildItem ".\" -Filter *.slang |

Foreach-Object {
    $out = $_.BaseName + ".spv"
    $vert = Select-String -Path $_.Name -Pattern '[shader("vertex")]' -SimpleMatch
    $frag = Select-String -Path $_.Name -Pattern '[shader("fragment")]' -SimpleMatch

    if ($vert -ne $null) {
        $out = $_.BaseName + ".vert.spv"

        Write-Host ("Compiling " + $_.Name + " to " + $out ) 
        slangc $_.Name -profile spirv_1_4 -fvk-use-scalar-layout -matrix-layout-column-major -target spirv -o $out -entry "vertexMain" -stage "vertex"

    }
    if ($frag -ne $null) {
        $out = $_.BaseName + ".frag.spv"
        
        Write-Host ("Compiling " + $_.Name + " to " + $out ) 
        slangc $_.Name -profile spirv_1_4 -fvk-use-scalar-layout -matrix-layout-column-major -target spirv -o $out -entry "fragmentMain" -stage "fragment"
    }
}
# Might set -O at some point