<?php
header("Content-Type: text/plain; charset=UTF-8");

if (!isset($_GET['file'])) {
    echo "Arquivo não especificado.";
    exit;
}

$projectRoot = dirname(dirname(__DIR__));
$uploadDir = $projectRoot . "/uploads";
$fileName = basename($_GET['file']); // evita caminhos maliciosos
$filePath = $uploadDir . "/" . $fileName;

if (!file_exists($filePath)) {
    echo "Arquivo não encontrado.";
    exit;
}

if (unlink($filePath)) {
    echo "Arquivo excluído com sucesso.";
} else {
    echo "Falha ao excluir o arquivo.";
}
?>
