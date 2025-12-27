<?php
header("Content-Type: text/html");

if (!isset($_FILES['file'])) {
    echo "<p>Nenhum arquivo recebido</p>";
    exit;
}

// Caminho relativo ao projeto: sobe duas pastas a partir do script
$projectRoot = dirname(dirname(__DIR__)); // __DIR__ é o diretório do script
$uploadDir = $projectRoot . "/uploads";

if (!is_dir($uploadDir)) {
    mkdir($uploadDir, 0777, true);
}

// Nome original do arquivo
$originalName = basename($_FILES['file']['name']);
$pathInfo = pathinfo($originalName);

// Adiciona timestamp ao nome do arquivo para evitar sobrescrever
$timestamp = time();
$dest = $uploadDir . "/" . $pathInfo['filename'] . "_" . $timestamp;
if (isset($pathInfo['extension']) && $pathInfo['extension'] !== '') {
    $dest .= "." . $pathInfo['extension'];
}

if (move_uploaded_file($_FILES['file']['tmp_name'], $dest)) {
    echo "<p>Upload feito com sucesso!</p>";
    echo "<p>Salvo em: $dest</p>";
} else {
    echo "<p>Falha ao mover o arquivo.</p>";
}
?>
