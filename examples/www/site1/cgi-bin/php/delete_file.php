<?php
header("Content-Type: text/plain; charset=UTF-8");

$method = $_SERVER['REQUEST_METHOD'];

// Aceita DELETE (oficial) e GET (fallback browser)
if ($method !== 'DELETE' && $method !== 'GET') {
    http_response_code(405);
    echo "Método não permitido.";
    exit;
}

// GET → via query string
if (!isset($_GET['file'])) {
    http_response_code(400);
    echo "Arquivo não especificado.";
    exit;
}

$projectRoot = dirname(dirname(__DIR__));
$uploadDir = $projectRoot . "/uploads";

// Segurança: remove qualquer path malicioso
$fileName = basename($_GET['file']);
$filePath = $uploadDir . "/" . $fileName;

if (!file_exists($filePath)) {
    http_response_code(404);
    echo "Arquivo não encontrado.";
    exit;
}

if (!is_writable($filePath)) {
    http_response_code(403);
    echo "Permissão negada.";
    exit;
}

if (unlink($filePath)) {
    http_response_code(200);
    echo "Arquivo excluído com sucesso.";
} else {
    http_response_code(500);
    echo "Falha ao excluir o arquivo.";
}
