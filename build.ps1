# Build & Flash script for TFT_video_STM32F7
param (
    [string]$Action = "all"
)

$projDir = $PSScriptRoot
$gcc = "D:\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin\arm-none-eabi-gcc.exe"
$objcopy = "D:\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin\arm-none-eabi-objcopy.exe"
$size = "D:\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin\arm-none-eabi-size.exe"
$stlink = "D:\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.200.202503041107\tools\bin\STM32_Programmer_CLI.exe"

$inc = "-I$projDir\Inc"
$cflags = @("-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-sp-d16", "-mfloat-abi=hard", $inc, "-O2", "-Wall", "-fdata-sections", "-ffunction-sections")

if ($Action -eq "clean") {
    Remove-Item "$projDir\Src\*.o", "$projDir\Startup\*.o", "$projDir\*.elf", "$projDir\*.hex", "$projDir\*.bin", "$projDir\*.map" -ErrorAction SilentlyContinue
    Write-Host "Clean completed."
    exit 0
}

Write-Host "Compiling C sources..."
$srcs = @("Src/main.c", "Src/sys_clock.c", "Src/sdram.c", "Src/ltdc.c", "Src/dma2d.c", "Src/sdmmc.c", "Src/media_player.c", "Src/diskio.c", "Src/ff.c")
$objs = @()
foreach ($src in $srcs) {
    $obj = $src -replace "\.c$", ".o"
    $fullSrc = Join-Path $projDir $src
    $fullObj = Join-Path $projDir $obj
    & $gcc $cflags -c $fullSrc -o $fullObj
    if ($LASTEXITCODE -ne 0) { Write-Error "Compile error: $src"; exit 1 }
    $objs += $fullObj
}

Write-Host "Compiling Startup..."
$startupSrc = "$projDir\Startup\startup_stm32f746nghx.s"
$startupObj = "$projDir\Startup\startup_stm32f746nghx.o"
& $gcc -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -x assembler-with-cpp -c $startupSrc -o $startupObj
$objs += $startupObj

Write-Host "Linking..."
$ld = "$projDir\STM32F746NGHX_FLASH.ld"
$elf = "$projDir\tft_video_f7.elf"
$hex = "$projDir\tft_video_f7.hex"
$bin = "$projDir\tft_video_f7.bin"
$map = "$projDir\tft_video_f7.map"

$ldflags = @("-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-sp-d16", "-mfloat-abi=hard", "-T$ld", "-Wl,-Map=$map,--cref", "-Wl,--gc-sections")
& $gcc $ldflags $objs -o $elf
if ($LASTEXITCODE -ne 0) { Write-Error "Link error"; exit 1 }

& $size $elf
& $objcopy -O ihex $elf $hex
& $objcopy -O binary -S $elf $bin
Write-Host "BUILD COMPLETED: tft_video_f7.hex generated."

if ($Action -eq "flash") {
    Write-Host "Flashing via STM32_Programmer_CLI (Connect Under Reset)..."
    & $stlink -c port=SWD mode=UR -w $hex -v -rst
}
