<?php
header("Content-Type: text/html; charset=UTF-8");

if (!isset($_FILES['file'])) {
    exit("<p>Nenhum arquivo recebido</p>");
}

$projectRoot = dirname(dirname(__DIR__));
$uploadDir_ = getenv('UPLOAD_DIR') ?: "/uploads_";
$uploadDir = $projectRoot . $uploadDir_;

if (!is_dir($uploadDir)) {
    mkdir($uploadDir, 0777, true);
}

$originalName = basename($_FILES['file']['name']);
$pathInfo = pathinfo($originalName);

// (opcional) normalizar nome
$filename = $pathInfo['filename'];
$filename = preg_replace('/[^A-Za-z0-9._-]/', '_', $filename);

$timestamp = time();
$dest = $uploadDir . "/" . $filename . "_" . $timestamp;

if (!empty($pathInfo['extension'])) {
    $dest .= "." . $pathInfo['extension'];
}

if (move_uploaded_file($_FILES['file']['tmp_name'], $dest)) {
    echo file_get_contents("./sucessUpload.html");
} else {
    echo "<p>Falha ao mover o arquivo.</p>";
}
?>
