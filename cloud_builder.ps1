# ESP32 GitHub Cloud Build & Flash (PowerShell Version)
$ErrorActionPreference = "Stop"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     ESP32 GitHub Cloud Build & Flash    " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Load config
if (-not (Test-Path "builder_config.json")) {
    Write-Error "builder_config.json not found!"
    exit 1
}
$config = Get-Content "builder_config.json" | ConvertFrom-Json
$projectName = $config.project_name
$comPort = $config.com_port
$baudRate = $config.baud_rate
$githubRepo = $config.github_repo
$githubUsername = $config.github_username
$githubToken = $config.github_token

$repoPath = $githubRepo.Replace("https://github.com/", "").Replace(".git", "")

# 2. Setup GitHub Actions workflow
$workflowPath = ".github/workflows/build.yml"
$workflowDir = [System.IO.Path]::GetDirectoryName($workflowPath)
if (-not (Test-Path $workflowDir)) {
    New-Item -ItemType Directory -Path $workflowDir | Out-Null
}

$workflowContent = @"
name: Build ESP32 Firmware

on:
  push:
    branches: [ "main", "master" ]
  pull_request:
    branches: [ "main", "master" ]

jobs:
  build:
    runs-on: ubuntu-latest
    container: espressif/idf:release-v5.1

    steps:
      - name: Checkout Code
        uses: actions/checkout@v4

      - name: Reconstruct ESP-IDF Project Structure
        run: |
          mkdir -p project/main
          mkdir -p project/components

          cp $projectName.cpp project/main/main.cpp
          cp *.h project/main/ || true
          cp idf_component.yml project/main/
          cp CMakeLists_main.txt project/main/CMakeLists.txt
          cp sdkconfig.defaults project/
          cp partitions.csv project/ || true

          printf 'cmake_minimum_required(VERSION 3.16)\nadd_compile_definitions("esp_wifi_set_band_mode=(int)" "WIFI_BAND_MODE_2G_ONLY=0")\ninclude(`$ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject($projectName)\n' > project/CMakeLists.txt

          git clone --depth 1 -b release/v3.0.x https://github.com/espressif/arduino-esp32.git project/components/arduino

      - name: Build Firmware (ESP32-S3)
        shell: bash
        run: |
          . `$IDF_PATH/export.sh
          cd project
          idf.py set-target esp32s3
          idf.py build

      - name: Upload Firmware Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: esp32-firmware
          path: |
            project/build/bootloader/bootloader.bin
            project/build/partition_table/partition-table.bin
            project/build/*.bin
"@

Set-Content -Path $workflowPath -Value $workflowContent
Write-Host "Updated GitHub Actions workflow at $workflowPath" -ForegroundColor Green

# 3. Setup MinGit
$gitDir = Join-Path $env:TEMP "mingit"
$gitExe = Join-Path $gitDir "cmd\git.exe"
if (-not (Test-Path $gitExe)) {
    Write-Host "Downloading portable Git (~28MB)..." -ForegroundColor Yellow
    $zipPath = Join-Path $env:TEMP "mingit.zip"
    Invoke-WebRequest -Uri "https://github.com/git-for-windows/git/releases/download/v2.45.2.windows.1/MinGit-2.45.2-64-bit.zip" -OutFile $zipPath
    Expand-Archive -Path $zipPath -DestinationPath $gitDir -Force
    Remove-Item $zipPath
}

# 4. Get previous run ID before pushing
$headers = @{
    "Authorization" = "Bearer $githubToken"
    "Accept"        = "application/vnd.github+json"
    "X-GitHub-Api-Version" = "2022-11-28"
    "User-Agent"    = "ESP32-Builder-Agent"
}

$previousRunId = $null
try {
    $runsApi = "https://api.github.com/repos/$repoPath/actions/runs?per_page=1"
    $res = Invoke-RestMethod -Uri $runsApi -Headers $headers -Method Get
    if ($res.workflow_runs -and $res.workflow_runs.Count -gt 0) {
        $previousRunId = $res.workflow_runs[0].id
    }
} catch {
    Write-Host "Note: Could not fetch initial run ID" -ForegroundColor Gray
}

# 5. Push to GitHub
Write-Host "`n>>> Pushing code to GitHub..." -ForegroundColor Cyan
if (-not (Test-Path ".git")) {
    & $gitExe init
    & $gitExe branch -M main
    & $gitExe remote add origin $githubRepo
}

& $gitExe config user.name "ESP Builder"
& $gitExe config user.email "builder@esp.local"
& $gitExe add .

$status = & $gitExe status --porcelain
if ($status) {
    & $gitExe commit -m "Build trigger"
} else {
    Write-Host "No changes to commit. Pushing current state." -ForegroundColor Yellow
}

$cleanRepo = $githubRepo.Replace("https://", "")
$authUrl = "https://${githubUsername}:${githubToken}@${cleanRepo}"
& $gitExe push -u $authUrl main --force

Write-Host "Push successful!" -ForegroundColor Green

# 6. Poll GitHub Actions build
Write-Host "`n>>> Monitoring build on GitHub Actions..." -ForegroundColor Cyan
$runId = $null
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 5
    try {
        $res = Invoke-RestMethod -Uri $runsApi -Headers $headers -Method Get
        if ($res.workflow_runs -and $res.workflow_runs.Count -gt 0) {
            $latestRun = $res.workflow_runs[0]
            if ($latestRun.id -ne $previousRunId) {
                $runId = $latestRun.id
                Write-Host "Found active workflow run: ID $runId, Status: $($latestRun.status)" -ForegroundColor Green
                break
            }
        }
    } catch {}
}

if (-not $runId) {
    Write-Error "Could not find active build on GitHub Actions."
    exit 1
}

$completed = $false
$buildSuccess = $false
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Seconds 10
    try {
        $runDetailApi = "https://api.github.com/repos/$repoPath/actions/runs/$runId"
        $run = Invoke-RestMethod -Uri $runDetailApi -Headers $headers -Method Get
        Write-Host "Build status: $($run.status), Conclusion: $(if ($run.conclusion) { $run.conclusion } else { 'Running...' })" -ForegroundColor Yellow
        if ($run.status -eq "completed") {
            $completed = $true
            $buildSuccess = ($run.conclusion -eq "success")
            break
        }
    } catch {
        Write-Host "Warning: Failed to fetch run status" -ForegroundColor Gray
    }
}

if (-not $completed) {
    Write-Error "Build timed out on GitHub Actions."
    exit 1
}

if (-not $buildSuccess) {
    Write-Host "`n[ERROR] Build FAILED on GitHub Actions. Fetching error log..." -ForegroundColor Red
    try {
        $jobsApi = "https://api.github.com/repos/$repoPath/actions/runs/$runId/jobs"
        $jobsData = Invoke-RestMethod -Uri $jobsApi -Headers $headers -Method Get
        $jobId = $jobsData.jobs[0].id
        $logPath = "build_error.log"
        $wc = New-Object System.Net.WebClient
        $wc.Headers.Add("Authorization", "Bearer $githubToken")
        $wc.Headers.Add("User-Agent", "ESP32-Builder")
        $wc.DownloadFile("https://api.github.com/repos/$repoPath/actions/jobs/$jobId/logs", $logPath)
        Write-Host "Build log saved to: $logPath" -ForegroundColor Yellow
        Write-Host "`n--- BUILD ERRORS ---" -ForegroundColor Red
        Select-String -Path $logPath -Pattern "error:" -Context 1,1 | ForEach-Object { Write-Host $_.ToString() -ForegroundColor Red }
        Write-Host "--- END ---`n" -ForegroundColor Red
    } catch {
        Write-Host "Could not fetch build log: $_" -ForegroundColor Gray
    }
    exit 1
}

# 7. Download Artifact
Write-Host "Build successful! Fetching artifact link..." -ForegroundColor Green
$artifactApi = "https://api.github.com/repos/$repoPath/actions/runs/$runId/artifacts"
$artifacts = Invoke-RestMethod -Uri $artifactApi -Headers $headers -Method Get
$targetArt = $artifacts.artifacts | Where-Object { $_.name -eq "esp32-firmware" }

if (-not $targetArt) {
    Write-Error "Artifact 'esp32-firmware' not found."
    exit 1
}

Write-Host "Downloading firmware zip..." -ForegroundColor Cyan
$downloadUrl = $targetArt.archive_download_url
$tempZip = "esp32_firmware_temp.zip"

# Download zip using WebClient to follow redirects properly
$webClient = New-Object System.Net.WebClient
$webClient.Headers.Add("Authorization", "Bearer $githubToken")
$webClient.Headers.Add("Accept", "application/vnd.github+json")
$webClient.Headers.Add("User-Agent", "ESP32-Builder-Agent")
$webClient.DownloadFile($downloadUrl, $tempZip)

Write-Host "Extracting firmware binaries..." -ForegroundColor Cyan
if (Test-Path "bootloader") { Remove-Item "bootloader" -Recurse -Force }
if (Test-Path "partition_table") { Remove-Item "partition_table" -Recurse -Force }
Expand-Archive -Path $tempZip -DestinationPath "." -Force
Remove-Item $tempZip

Write-Host "Firmware downloaded and extracted successfully!" -ForegroundColor Green

# 8. Flash using standalone esptool
$esptoolDir = Join-Path $env:TEMP "esptool"
$esptoolExe = Join-Path $esptoolDir "esptool-win64\esptool.exe"
if (-not (Test-Path $esptoolExe)) {
    Write-Host "Downloading standalone esptool..." -ForegroundColor Yellow
    $zipPath = Join-Path $env:TEMP "esptool.zip"
    Invoke-WebRequest -Uri "https://github.com/espressif/esptool/releases/download/v4.7.0/esptool-v4.7.0-win64.zip" -OutFile $zipPath
    Expand-Archive -Path $zipPath -DestinationPath $esptoolDir -Force
    Remove-Item $zipPath
}

$bootloaderBin = Join-Path "bootloader" "bootloader.bin"
$partitionBin = Join-Path "partition_table" "partition-table.bin"
$appBin = "$projectName.bin"

if (-not (Test-Path $bootloaderBin) -or -not (Test-Path $partitionBin) -or -not (Test-Path $appBin)) {
    Write-Error "Missing firmware binaries for flashing!"
    exit 1
}

$otaDataBin = "ota_data_initial.bin"
Write-Host "`n>>> Flashing firmware to ESP32-S3 on port $comPort..." -ForegroundColor Cyan
if (Test-Path $otaDataBin) {
    & $esptoolExe --chip esp32s3 --port $comPort --baud $baudRate write_flash 0x0 $bootloaderBin 0x8000 $partitionBin 0xe000 $otaDataBin 0x10000 $appBin
} else {
    & $esptoolExe --chip esp32s3 --port $comPort --baud $baudRate write_flash 0x0 $bootloaderBin 0x8000 $partitionBin 0x10000 $appBin
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n========================================================" -ForegroundColor Green
    Write-Host " SUCCESS! Firmware flashed to your ESP32 on $comPort" -ForegroundColor Green
    Write-Host "========================================================" -ForegroundColor Green
} else {
    Write-Error "Flashing failed. Check COM port or press BOOT button on ESP32."
}
