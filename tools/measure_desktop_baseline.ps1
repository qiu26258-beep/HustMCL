param(
    [string]$Executable = 'build/desktop-release/src/app/hustmcl_desktop.exe',
    [string]$Output = 'out/m0-next/desktop-baseline.json',
    [ValidateRange(1, 10)][int]$Runs = 5,
    [ValidateRange(1, 120)][int]$IdleSeconds = 30,
    [ValidateRange(1, 30)][int]$SampleSeconds = 5
)

$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path -LiteralPath $Executable).Path
$outputPath = [IO.Path]::GetFullPath($Output)
$rows = @()
$oldPlatform = $env:QT_QPA_PLATFORM
try {
    $env:QT_QPA_PLATFORM = 'windows'
    for ($run = 1; $run -le $Runs; $run++) {
        $process = $null
        try {
            $timer = [Diagnostics.Stopwatch]::StartNew()
            $process = Start-Process -FilePath $exePath -WorkingDirectory (Split-Path $exePath) `
                -ArgumentList @('--scenario', 'home-ready', '--window-size', '1120x720') `
                -WindowStyle Hidden -PassThru
            if (-not $process.WaitForInputIdle(15000)) { throw 'GUI input-idle timeout' }
            $inputIdleMs = $timer.Elapsed.TotalMilliseconds
            Start-Sleep -Seconds $IdleSeconds
            $process.Refresh()
            if ($process.HasExited) { throw 'Demo exited before the idle sample' }
            $startCpu = $process.TotalProcessorTime.TotalSeconds
            $sampleTimer = [Diagnostics.Stopwatch]::StartNew()
            Start-Sleep -Seconds $SampleSeconds
            $process.Refresh()
            if ($process.HasExited) { throw 'Demo exited during the idle sample' }
            $row = [ordered]@{
                run = $run
                inputIdleMs = [Math]::Round($inputIdleMs, 2)
                privateBytes = $process.PrivateMemorySize64
                workingSetBytes = $process.WorkingSet64
                cpuPercentWholeMachine = [Math]::Round(100 * `
                    ($process.TotalProcessorTime.TotalSeconds - $startCpu) / `
                    $sampleTimer.Elapsed.TotalSeconds / [Environment]::ProcessorCount, 4)
            }
            $rows += $row
            Write-Output ($row | ConvertTo-Json -Compress)
        } finally {
            # Only stop the process created by this iteration. No process-name cleanup.
            if ($null -ne $process) {
                if (-not $process.HasExited) {
                    $process.Kill()
                    $process.WaitForExit()
                }
                $process.Dispose()
            }
        }
    }
    $system = Get-CimInstance Win32_OperatingSystem
    $computer = Get-CimInstance Win32_ComputerSystem
    $report = [ordered]@{
        recordedAt = [DateTimeOffset]::Now.ToString('o')
        executable = $exePath
        executableSha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash.ToLowerInvariant()
        os = "$($system.Caption) $($system.Version) build $($system.BuildNumber)"
        cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1).Name
        logicalProcessors = [Environment]::ProcessorCount
        physicalMemoryBytes = $computer.TotalPhysicalMemory
        gpu = @((Get-CimInstance Win32_VideoController).Name)
        idleSeconds = $IdleSeconds
        sampleSeconds = $SampleSeconds
        conditions = 'Windows QPA; hidden launch; 1120x720 logical client; input-idle proxy, not first paint or interactive-ready; OS caches not cleared; GPU memory not measured'
        runs = $rows
    }
    [IO.Directory]::CreateDirectory((Split-Path $outputPath)) | Out-Null
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $outputPath -Encoding utf8
    Write-Output "Saved $outputPath"
} finally {
    $env:QT_QPA_PLATFORM = $oldPlatform
}
