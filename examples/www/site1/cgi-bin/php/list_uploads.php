<?php
header("Content-Type: text/html; charset=UTF-8");

$projectRoot = dirname(dirname(__DIR__));
$uploadDir = $projectRoot . "/uploads";

if (!is_dir($uploadDir)) {
    echo "<p>Nenhum arquivo encontrado.</p>";
    exit;
}

$files = array_diff(scandir($uploadDir), array('.', '..'));

if (empty($files)) {
    echo "<p>Nenhum arquivo carregado.</p>";
    exit;
}

function filePreview($filename) {
    $ext = pathinfo($filename, PATHINFO_EXTENSION);
    $name = pathinfo($filename, PATHINFO_FILENAME);
    if (strlen($name) > 5) {
        $name = substr($name, 0, 5) . '...';
    }
    return $name . '.' . $ext;
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
    .file-card img, .file-card video, .file-card audio {
        max-width: 100%;
        max-height: 120px;
        display: block;
        margin: 5px auto;
    }
    button { margin: 3px; padding: 5px 8px; cursor: pointer; }
</style>

<script>
    function deleteFile(fileName) {
        if (!confirm('Tem certeza que deseja excluir "' + fileName + '"?')) return;

        fetch('/cgi-bin/php/delete_file.php', {
            method: 'DELETE',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: 'file=' + encodeURIComponent(fileName)
        })
        .then(resp => {
            if (!resp.ok) throw new Error('Falha na exclusão');
            return resp.text();
        })
        .then(() => location.reload())
        .catch(err => alert('Erro ao excluir: ' + err.message));
    }
</script>

<div class="file-grid">
HTML;

foreach ($files as $file) {
    $filePath = $uploadDir . "/" . $file;
    $fileUrl  = "/uploads/" . urlencode($file);
    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
    $preview = filePreview($file);

    echo "<div class='file-card'>";
    echo "<strong title='$file'>$preview</strong>";

    if (in_array($ext, ['jpg','jpeg','png','gif','webp'])) {
        echo "<img src='$fileUrl' alt='$file'>";
    } elseif (in_array($ext, ['mp4','webm','ogg'])) {
        echo "<video controls src='$fileUrl'></video>";
    } elseif (in_array($ext, ['mp3','wav','ogg'])) {
        echo "<audio controls src='$fileUrl'></audio>";
    } else {
        echo "<p>Arquivo não visualizável</p>";
    }

    echo "<div>";
    echo "<button onclick=\"window.open('$fileUrl', '_blank')\">View</button>";
    echo "<button onclick=\"deleteFile('$file')\">Excluir</button>";
    echo "</div>";
    echo "</div>";
}

echo "</div>";
?>
