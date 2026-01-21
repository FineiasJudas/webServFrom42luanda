<?php
header("Content-Type: application/json; charset=UTF-8");

// - SOMENTE DELETE OUVIU? -
if ($_SERVER['REQUEST_METHOD'] !== 'DELETE') {
    http_response_code(405);
    echo json_encode(['error' => 'Método não permitido']);
    exit;
}

// - LER BODY -
parse_str(file_get_contents("php://input"), $data);

if (empty($data['file'])) {
    http_response_code(400);
    echo json_encode(['error' => 'Arquivo não especificado']);
    exit;
}

// - PROJECT ROOT -
$projectRoot = dirname(dirname(__DIR__));

// - UPLOAD DIR (ENV) -
$uploadDirEnv = getenv('UPLOAD_DIR') ?: 'uploads_';
$uploadDirEnv = trim($uploadDirEnv, '/');

// - PATH FISICO -
$uploadDir = $projectRoot . '/' . $uploadDirEnv;

// - GARANTIR QUE EXISTE -
$realUploadDir = realpath($uploadDir);
if ($realUploadDir === false) {
    http_response_code(500);
    echo json_encode(['error' => 'Diretório de upload inválido']);
    exit;
}

// - SEGURANÇA -
$fileName = basename($data['file']);
$filePath = $realUploadDir . '/' . $fileName;
$realFilePath = realpath($filePath);

if ($realFilePath === false || strpos($realFilePath, $realUploadDir) !== 0) {
    http_response_code(403);
    echo json_encode(['error' => 'Acesso negado']);
    exit;
}

// - EXISTE? -
if (!file_exists($realFilePath)) {
    http_response_code(404);
    echo json_encode(['error' => 'Arquivo não encontrado']);
    exit;
}

// - DELETE -
if (unlink($realFilePath)) {
    echo json_encode(['success' => true]);
} else {
    http_response_code(500);
    echo json_encode(['error' => 'Falha ao excluir arquivo']);
}
