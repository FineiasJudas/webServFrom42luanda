<?php
header("Content-Type: text/plain; charset=UTF-8");

$uploadDir_ = getenv('UPLOAD_DIR');
if (empty($uploadDir_)) {$uploadDir_ = "/uploads_";}
$uploadDir = $uploadDir_;
// aceita SOMENTE DELETE
if ($_SERVER['REQUEST_METHOD'] !== 'DELETE') {
    http_response_code(405);
    echo "Método não permitido.";
    exit;
}

//ler o corpo da requisição DELETE
$rawInput = file_get_contents("php://input");
parse_str($rawInput, $data);

// verifica se o arquivo foi enviado
if (!isset($data['file']) || empty($data['file'])) {
    http_response_code(400);
    echo "Arquivo não especificado.";
    exit;
}

// raiz do projeto
$projectRoot = dirname(dirname(__DIR__));
$fullUploadDir   = $projectRoot . $uploadDir;//"/uploads";

// segurança contra path traversal
$fileName = basename($data['file']);
$filePath = $fullUploadDir . "/" . $fileName;

// garante que está dentro da pasta uploads
$realUploadDir = realpath($fullUploadDir);
$realFilePath  = realpath($filePath);

if ($realFilePath === false || strpos($realFilePath, $realUploadDir) !== 0) {
    http_response_code(403);
    echo "Acesso negado.";
    exit;


}

//verifica existencia
if (!file_exists($realFilePath)) {
    http_response_code(404);
    echo "Arquivo não encontrado.";
    exit;
}

//tenta excluir
if (unlink($realFilePath)) {
    echo "Arquivo excluído com sucesso.";
} else {
    http_response_code(500);
    echo "Falha ao excluir o arquivo.";
}
?>  