<?php

$MAX_SIZE = 10 * 1024 * 1024;

/* Detectar formato de resposta */
$format = $_GET['format'] ?? '';
$accept = $_SERVER['HTTP_ACCEPT'] ?? '';

if ($format === 'plain')
{
    $responseType = 'text/plain';
} else if (strpos($accept, 'text/html') !== false) {
    $responseType = 'text/html';
} else {
    $responseType = 'text/plain';
}

header("Content-Type: $responseType; charset=UTF-8");

/* Helpers de resposta */
function respond($status, $message, $extra = [])
{
    http_response_code($status);

    global $responseType;

    if ($responseType === 'application/json')
    {
        echo json_encode(array_merge([
            'status' => $status < 400 ? 'ok' : 'error',
            'message' => $message
        ], $extra));
    }
    else if ($responseType === 'text/html')
    {
        echo "<p>$message</p>";
    }
    else
    {
        echo $message;
    }

    exit;
}

/* Validações */
if ($_SERVER['REQUEST_METHOD'] !== 'POST')
    respond(405, 'Método não permitido');

if (!isset($_FILES['file']))
    respond(400, 'Nenhum arquivo enviado');

if ($_FILES['file']['error'] !== UPLOAD_ERR_OK) {
    respond(400, 'Erro no upload: ' . $_FILES['file']['error']);
}

if ($_FILES['file']['size'] > $MAX_SIZE)
    respond(413, 'Arquivo excede o limite de 10MB');

/* Upload */
$projectRoot = dirname(dirname(__DIR__));
$uploadDir   = $projectRoot . "/uploads";

if (!is_dir($uploadDir))
    mkdir($uploadDir, 0755, true);

$name   = basename($_FILES['file']['name']);
$target = $uploadDir . "/" . $name;

if (!move_uploaded_file($_FILES['file']['tmp_name'], $target))
    respond(500, 'Falha ao salvar o arquivo');

/* Resposta final */
if ($responseType === 'text/html' && file_exists(__DIR__ . "/sucessUpload.html"))
{
    readfile(__DIR__ . "/sucessUpload.html");
    exit;
}

respond(201, 'Upload realizado com sucesso', [
    'file' => $name,
    'size' => $_FILES['file']['size']
]);