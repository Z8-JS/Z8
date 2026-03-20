$path = 'd:\Z8\Z8-app\src\module\node\stream\stream.cpp'
$raw = Get-Content -Raw -Path $path

# Remove early return that prevents 'end' emission
$earlyReturn = '    if (p_internal->m_ended && p_internal->m_buffer_values.empty()) {
        args.GetReturnValue().SetNull();
        return;
    }'

$raw = $raw.Replace($earlyReturn, '')

$raw | Set-Content -Path $path
