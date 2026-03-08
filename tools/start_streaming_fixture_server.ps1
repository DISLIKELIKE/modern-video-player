param(
    [string]$RootPath = "samples/streaming/hls_local",
    [int]$Port = 8765
)

$ErrorActionPreference = 'Stop'

$resolvedRoot = (Resolve-Path $RootPath).Path
$listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse('127.0.0.1'), $Port)
$listener.Start()

Write-Host "Streaming fixture server root: $resolvedRoot"
Write-Host "Streaming fixture server listening on: http://127.0.0.1:$Port/"

function Get-ContentType([string]$Path) {
    switch ([System.IO.Path]::GetExtension($Path).ToLowerInvariant()) {
        '.m3u8' { return 'application/vnd.apple.mpegurl' }
        '.mpd' { return 'application/dash+xml' }
        '.ts' { return 'video/mp2t' }
        '.m4s' { return 'video/iso.segment' }
        '.mp4' { return 'video/mp4' }
        default { return 'application/octet-stream' }
    }
}

try {
    while ($true) {
        $client = $listener.AcceptTcpClient()
        try {
            $client.NoDelay = $true
            $stream = $client.GetStream()
            $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::ASCII, $false, 1024, $true)
            $requestLine = $reader.ReadLine()
            if ([string]::IsNullOrWhiteSpace($requestLine)) {
                continue
            }

            while (($line = $reader.ReadLine()) -ne $null -and $line.Length -gt 0) {
            }

            $parts = $requestLine.Split(' ')
            $requestPath = if ($parts.Length -ge 2) { $parts[1].TrimStart('/') } else { '' }
            if ([string]::IsNullOrWhiteSpace($requestPath)) {
                $requestPath = 'sample.m3u8'
            }

            $fullPath = Join-Path $resolvedRoot ($requestPath -replace '/', '\')
            if (-not (Test-Path $fullPath -PathType Leaf)) {
                $body = [System.Text.Encoding]::UTF8.GetBytes('not found')
                $header = "HTTP/1.1 404 Not Found`r`nContent-Type: text/plain; charset=utf-8`r`nContent-Length: $($body.Length)`r`nConnection: close`r`n`r`n"
                $headerBytes = [System.Text.Encoding]::ASCII.GetBytes($header)
                $stream.Write($headerBytes, 0, $headerBytes.Length)
                $stream.Write($body, 0, $body.Length)
                continue
            }

            $body = [System.IO.File]::ReadAllBytes($fullPath)
            $header = "HTTP/1.1 200 OK`r`nContent-Type: $(Get-ContentType $fullPath)`r`nContent-Length: $($body.Length)`r`nConnection: close`r`n`r`n"
            $headerBytes = [System.Text.Encoding]::ASCII.GetBytes($header)
            $stream.Write($headerBytes, 0, $headerBytes.Length)
            $stream.Write($body, 0, $body.Length)
        }
        finally {
            $client.Close()
        }
    }
}
finally {
    $listener.Stop()
}
