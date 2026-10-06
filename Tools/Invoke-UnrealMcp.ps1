param([string]$Tool = 'list_toolsets', [string]$ArgumentsJson = '{}', [string]$OutputPath)
$ErrorActionPreference = 'Stop'
$headers = @{Accept='application/json, text/event-stream'}
function Rpc($message) {
    $response = Invoke-WebRequest -UseBasicParsing -Uri 'http://127.0.0.1:8000/mcp' -Method Post -ContentType 'application/json' -Headers $headers -Body ($message | ConvertTo-Json -Depth 50 -Compress) -TimeoutSec 120
    $session = $response.Headers['Mcp-Session-Id']
    if ($session) { $headers['Mcp-Session-Id'] = [string]($session -join '') }
    if (-not $response.Content) { return }
    $content = [string]$response.Content
    if ($content.TrimStart().StartsWith('event:') -or $content.TrimStart().StartsWith('data:')) {
        $content = (($content -split "`n" | Where-Object { $_.StartsWith('data:') }) -replace '^data:\s*','') -join "`n"
    }
    $result = $content | ConvertFrom-Json
    if ($result.error) { throw ($result.error | ConvertTo-Json -Depth 10) }
    $result.result
}
$init = Rpc @{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2025-11-25';capabilities=@{};clientInfo=@{name='QiantongTools';version='1.0'}}}
$headers['MCP-Protocol-Version'] = $init.protocolVersion
$null = Rpc @{jsonrpc='2.0';method='notifications/initialized'}
$result = Rpc @{jsonrpc='2.0';id=2;method='tools/call';params=@{name=$Tool;arguments=($ArgumentsJson | ConvertFrom-Json)}}
$json = $result | ConvertTo-Json -Depth 60
if ($OutputPath) { $json | Set-Content -Encoding utf8 $OutputPath }
$json
if ($result.isError) { exit 1 }
