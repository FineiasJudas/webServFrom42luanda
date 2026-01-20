<?php

$MAX_SIZE = 10 * 1024 * 1024; // 10 MB

/* Detectar formato de resposta */
$format = $_GET['format'] ?? '';
$accept = $_SERVER['HTTP_ACCEPT'] ?? '';

if ($format === 'plain') {
    $responseType = 'text/plain';
} else if (strpos($accept, 'text/html') !== false) {
    $responseType = 'text/html';
} else {
    $responseType = 'text/plain';
}

header("Content-Type: $responseType; charset=UTF-8");

/* Helpers de resposta */
function respond($status, $message, $extra = []) {
    http_response_code($status);

    global $responseType;

    if ($responseType === 'application/json') {
        echo json_encode(array_merge([
            'status' => $status < 400 ? 'ok' : 'error',
            'message' => $message
        ], $extra));
    } else if ($responseType === 'text/html') {
        echo "<p>$message</p>";
    } else {
        echo $message;
    }

    exit;
}

/* Validações básicas */
if ($_SERVER['REQUEST_METHOD'] !== 'POST')
    respond(405, 'Método não permitido');

if (!isset($_FILES['file']))
    respond(400, 'Nenhum arquivo enviado');

if ($_FILES['file']['error'] !== UPLOAD_ERR_OK)
    respond(400, 'Erro no upload: ' . $_FILES['file']['error']);

if ($_FILES['file']['size'] > $MAX_SIZE)
    respond(413, 'Arquivo excede o limite de 10MB');

/* Definir diretório de upload usando variável de ambiente */
$projectRoot = dirname(dirname(__DIR__));
$uploadDir_ = getenv('UPLOAD_DIR') ?: "/uploads_";
$uploadDir = $projectRoot . $uploadDir_;

if (!is_dir($uploadDir)) {
    mkdir($uploadDir, 0755, true);
}

/* Preparar nome do arquivo com timestamp */
$originalName = basename($_FILES['file']['name']);
$pathInfo = pathinfo($originalName);

// Normalizar nome (somente letras, números, ., _, -)
$filename = preg_replace('/[^A-Za-z0-9._-]/', '_', $pathInfo['filename']);
$timestamp = time();
$dest = $uploadDir . "/" . $filename . "_" . $timestamp;

if (!empty($pathInfo['extension'])) {
    $dest .= "." . $pathInfo['extension'];
}

/* Mover arquivo */
if (!move_uploaded_file($_FILES['file']['tmp_name'], $dest))
    respond(500, 'Falha ao salvar o arquivo');

/* Resposta final */
if ($responseType === 'text/html' && file_exists(__DIR__ . "/sucessUpload.html")) {
    readfile(__DIR__ . "/sucessUpload.html");
    exit;
}

respond(201, 'Upload realizado com sucesso', [
    'file' => basename($dest),
    'size' => $_FILES['file']['size']
]);
