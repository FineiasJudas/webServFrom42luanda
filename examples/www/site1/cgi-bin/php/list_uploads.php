<?php
header("Content-Type: text/html; charset=UTF-8");

/*
 -- UPLOAD DIR (ENV) --
*/
$projectRoot = dirname(dirname(__DIR__));

$uploadDirEnv = getenv('UPLOAD_DIR');
if (empty($uploadDirEnv)) {
    $uploadDirEnv = '/uploads_';
}

//Caminho físico
$uploadDir = rtrim($projectRoot . $uploadDirEnv, '/');

//Caminho público (URL)
$uploadUrl = rtrim($uploadDirEnv, '/');

// -- JSON DETECTION --
$accept = $_SERVER['HTTP_ACCEPT'] ?? '';
$isJson = str_contains($accept, 'application/json')
    || ($_GET['format'] ?? '') === 'json';

// -- DIR CHECK --
if (!is_dir($uploadDir)) {
    http_response_code(404);

    if ($isJson) {
        header("Content-Type: application/json; charset=UTF-8");
        echo json_encode([
            'files' => [],
            'error' => 'Diretório não encontrado'
        ]);
    } else {
        echo "<p>Nenhum arquivo encontrado.</p>";
    }
    exit;
}

// -- FILE LIST --
$files = array_values(array_diff(scandir($uploadDir), ['.', '..']));

// -- JSON RESPONSE --
if ($isJson) {
    header("Content-Type: application/json; charset=UTF-8");

    $data = [];

    foreach ($files as $file) {
        $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));

        $data[] = [
            'name' => $file,
            'extension' => $ext,
            'url' => $uploadUrl . '/' . rawurlencode($file),
            'type' => in_array($ext, ['jpg', 'jpeg', 'png', 'gif', 'webp']) ? 'image'
                : (in_array($ext, ['mp4', 'webm', 'ogg']) ? 'video'
                    : (in_array($ext, ['mp3', 'wav', 'ogg']) ? 'audio'
                        : 'file'))
        ];
    }

    echo json_encode([
        'count' => count($data),
        'files' => $data
    ], JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
    exit;
}

// -- HTML RESPONSE --
function filePreview($filename)
{
    $ext = pathinfo($filename, PATHINFO_EXTENSION);
    $name = pathinfo($filename, PATHINFO_FILENAME);
    return strlen($name) > 5 ? substr($name, 0, 5) . '....' . $ext : $filename;
}

echo <<<HTML
<style>
.file-grid { display: flex; flex-wrap: wrap; gap: 15px; }
.file-card {
    border: 1px solid #ccc;
    padding: 10px;
    width: 200px;
    text-align: center;
    font-family: sans-serif;
    word-break: break-word;
}
.file-card img, video, audio {
    max-width: 100%;
    max-height: 120px;
    display: block;
    margin: 6px auto;
}
button { margin: 3px; padding: 5px 8px; cursor: pointer; }
</style>

<script>
function deleteFile(fileName) {
    if (!confirm('Excluir "' + fileName + '"?')) return;

    fetch('/cgi-bin/php/delete_file.php', {
        method: 'DELETE',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'file=' + encodeURIComponent(fileName)
    })
    .then(r => {
        if (!r.ok) throw new Error();
        location.reload();
    })
    .catch(() => alert('Erro ao excluir'));
}
</script>

<div class="file-grid">
HTML;

foreach ($files as $file) {
    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
    $url = $uploadUrl . '/' . rawurlencode($file);
    $safeFile = htmlspecialchars($file, ENT_QUOTES);

    echo "<div class='file-card'>";
    echo "<strong title='{$safeFile}'>" . filePreview($file) . "</strong>";

    if (in_array($ext, ['jpg', 'jpeg', 'png', 'gif', 'webp'])) {
        echo "<img src='{$url}'>";
    } elseif (in_array($ext, ['mp4', 'webm', 'ogg'])) {
        echo "<video src='{$url}' controls></video>";
    } elseif (in_array($ext, ['mp3', 'wav', 'ogg'])) {
        echo "<audio src='{$url}' controls></audio>";
    } else {
        echo "<p>Arquivo</p>";
    }

    echo "<div>";
    echo "<button onclick=\"window.open('{$url}', '_blank')\">Ver</button>";
    echo "<button onclick=\"deleteFile('{$safeFile}')\">Excluir</button>";
    echo "</div>";
    echo "</div>";
}

echo "</div>";
