<?php

$projectRoot = dirname(dirname(__DIR__));
$uploadDir = $projectRoot . "/uploads";

$accept = $_SERVER['HTTP_ACCEPT'] ?? '';
$isJson = str_contains($accept, 'application/json') || ($_GET['format'] ?? '') === 'json';

if (!is_dir($uploadDir)) {
    http_response_code(404);
    echo $isJson ? json_encode(['files' => []]) : "<p>Nenhum arquivo encontrado.</p>";
    exit;
}

$files = array_values(array_diff(scandir($uploadDir), ['.', '..']));

/* =======================
   🔹 JSON RESPONSE
======================= */
if ($isJson) {
    header("Content-Type: application/json; charset=UTF-8");
    echo json_encode([
        'files' => $files
    ]);
    exit;
}

/* =======================
   🔹 HTML RESPONSE
======================= */
header("Content-Type: text/html; charset=UTF-8");

function shortName($filename) {
    return strlen($filename) > 20
        ? substr($filename, 0, 17) . '...'
        : $filename;
}

echo <<<HTML
<style>
.file-grid { display: flex; flex-wrap: wrap; gap: 15px; }
.file-card {
    width: 200px;
    border: 1px solid #ccc;
    padding: 10px;
    text-align: center;
    border-radius: 8px;
}
.file-card img, video {
    max-width: 100%;
    max-height: 120px;
    margin-top: 8px;
}
button {
    margin-top: 5px;
    padding: 6px 10px;
}
</style>

<script>
function deleteFile(file) {
    if (!confirm("Excluir " + file + "?")) return;

    fetch("/cgi-bin/php/delete_file.php?file=" + encodeURIComponent(file), {
        method: "DELETE"
    })
    .then(r => {
        if (!r.ok) throw new Error("Erro");
        location.reload();
    })
    .catch(() => alert("Falha ao excluir"));
}
</script>

<div class="file-grid">
HTML;

foreach ($files as $file) {
    $url = "/uploads/" . rawurlencode($file);
    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));

    echo "<div class='file-card'>";
    echo "<strong title='$file'>" . shortName($file) . "</strong>";

    if (in_array($ext, ['jpg','jpeg','png','gif','webp'])) {
        echo "<img src='$url'>";
    } elseif (in_array($ext, ['mp4','webm'])) {
        echo "<video src='$url' controls></video>";
    } else {
        echo "<p>Arquivo</p>";
    }

    echo "<br><button onclick=\"window.open('$url')\">Abrir</button>";
    echo "<button onclick=\"deleteFile('$file')\">Excluir</button>";
    echo "</div>";
}

echo "</div>";
