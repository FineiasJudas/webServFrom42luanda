<?php
header("Content-Type: text/plain; charset=UTF-8");

echo "=== PHP CGI TEST ===\n\n";

/* ============================
   Variáveis de ambiente CGI
   ============================ */
$method = getenv("REQUEST_METHOD");
$contentType = getenv("CONTENT_TYPE");
$contentLength = intval(getenv("CONTENT_LENGTH"));

echo "REQUEST_METHOD: $method\n";
echo "CONTENT_TYPE: $contentType\n";
echo "CONTENT_LENGTH: $contentLength\n";
echo "QUERY_STRING: " . getenv("QUERY_STRING") . "\n";
echo "SCRIPT_NAME: " . getenv("SCRIPT_NAME") . "\n";
echo "SCRIPT_FILENAME: " . getenv("SCRIPT_FILENAME") . "\n";
echo "REQUEST_URI: " . getenv("REQUEST_URI") . "\n";
echo "DOCUMENT_ROOT: " . getenv("DOCUMENT_ROOT") . "\n\n";

/* ============================
   Processar POST
   ============================ */
if ($method !== "POST")
{
    echo "Não é POST. Nada a processar.\n";
    exit(0);
}

if ($contentLength <= 0)
{
    echo "POST sem conteúdo (Content-Length = 0).\n";
    exit(0);
}

// Ler raw input
$rawData = file_get_contents("php://input", false, null, 0, $contentLength);
$actualSize = strlen($rawData);

echo "DADOS RECEBIDOS: $actualSize bytes\n\n";

/* ============================
   Detectar tipo de conteúdo
   ============================ */
$projectRoot = dirname(dirname(__DIR__));
$uploadDir = $projectRoot . "/uploads";

// Criar diretório se não existir
if (!is_dir($uploadDir))
    mkdir($uploadDir, 0755, true);

$saved = false;
$savedPath = "";

// 1️MULTIPART/FORM-DATA
if (stripos($contentType, 'multipart/form-data') !== false)
{
    echo "TIPO: Multipart Form Data\n";
    
    if (isset($_FILES['file']))
    {
        $fileName = basename($_FILES['file']['name']);
        $tmpName = $_FILES['file']['tmp_name'];
        $fileSize = $_FILES['file']['size'];
        $error = $_FILES['file']['error'];
        
        echo "   Arquivo: $fileName\n";
        echo "   Tamanho: $fileSize bytes\n";
        echo "   Error Code: $error\n";
        
        if ($error === UPLOAD_ERR_OK)
        {
            $savedPath = $uploadDir . "/" . $fileName;
            if (move_uploaded_file($tmpName, $savedPath))
            {
                echo " Salvo em: $savedPath!\n";
                $saved = true;
            }
            else
            {
                echo " Erro ao salvar arquivo\n";
            }
        }
    }
    else
    {
        echo "  Nenhum arquivo detectado em \$_FILES\n";
    }
}

// 2 APPLICATION/JSON
else if (stripos($contentType, 'application/json') !== false)
{
    echo " TIPO: JSON\n";
    
    $json = json_decode($rawData, true);
    
    if ($json !== null)
    {
        echo "   JSON válido com " . count($json) . " campos:\n";
        echo "   " . json_encode($json, JSON_PRETTY_PRINT) . "\n";
    }
    else
    {
        echo "  JSON inválido\n";
        echo "   Raw: " . substr($rawData, 0, 200) . "...\n";
    }
}

// 3 APPLICATION/OCTET-STREAM ou BINARY
else if (
    stripos($contentType, 'application/octet-stream') !== false ||
    stripos($contentType, 'image/') !== false ||
    stripos($contentType, 'video/') !== false ||
    stripos($contentType, 'audio/') !== false ||
    stripos($contentType, 'application/pdf') !== false ||
    stripos($contentType, 'application/zip') !== false
)
{
    echo " TIPO: Binário ($contentType)\n";
    
    // Gerar nome baseado em timestamp + tipo
    $ext = "bin";
    
    if (stripos($contentType, 'image/png') !== false) $ext = "png";
    else if (stripos($contentType, 'image/jpeg') !== false) $ext = "jpg";
    else if (stripos($contentType, 'image/gif') !== false) $ext = "gif";
    else if (stripos($contentType, 'application/pdf') !== false) $ext = "pdf";
    else if (stripos($contentType, 'application/zip') !== false) $ext = "zip";
    else if (stripos($contentType, 'video/mp4') !== false) $ext = "mp4";
    else if (stripos($contentType, 'audio/mpeg') !== false) $ext = "mp3";
    
    $fileName = "binary_" . time() . "_" . rand(1000, 9999) . "." . $ext;
    $savedPath = $uploadDir . "/" . $fileName;
    
    if (file_put_contents($savedPath, $rawData) !== false)
    {
        echo " Salvo em: $savedPath\n";
        echo "   Tamanho: " . filesize($savedPath) . " bytes\n";
        $saved = true;
    }
    else
    {
        echo " Erro ao salvar\n";
    }
}

// 4 APPLICATION/X-WWW-FORM-URLENCODED
else if (stripos($contentType, 'application/x-www-form-urlencoded') !== false)
{
    echo " TIPO: Form URL Encoded\n";
    
    parse_str($rawData, $formData);
    
    echo "   Campos recebidos:\n";
    foreach ($formData as $key => $value)
    {
        echo "   - $key = " . substr($value, 0, 100) . "\n";
    }
}

// 5 TEXT/PLAIN ou outro
else
{
    echo " TIPO: Texto ou desconhecido ($contentType)\n";
    
    // Verificar se parece binário
    $isBinary = false;
    for ($i = 0; $i < min(512, $actualSize); $i++)
    {
        $byte = ord($rawData[$i]);
        if ($byte < 32 && $byte != 9 && $byte != 10 && $byte != 13)
        {
            $isBinary = true;
            break;
        }
    }
    
    if ($isBinary)
    {
        echo "  Parece ser binário, salvando como .bin\n";
        
        $fileName = "unknown_" . time() . "_" . rand(1000, 9999) . ".bin";
        $savedPath = $uploadDir . "/" . $fileName;
        
        if (file_put_contents($savedPath, $rawData) !== false)
        {
            echo " Salvo em: $savedPath\n";
            $saved = true;
        }
    }
    else
    {
        echo " Conteúdo texto:\n";
        echo "   " . substr($rawData, 0, 500) . "\n";
        if ($actualSize > 500)
            echo "   ... (+" . ($actualSize - 500) . " bytes)\n";
    }
}

/* ============================
   Resumo final
   ============================ */
echo "\n" . str_repeat("=", 50) . "\n";
echo "RESUMO:\n";
echo "- Bytes recebidos: $actualSize\n";
echo "- Tipo detectado: $contentType\n";

if ($saved)
{
    echo "- Arquivo salvo: \n";
    echo "- Localização: $savedPath\n";
    echo "- Tamanho final: " . filesize($savedPath) . " bytes\n";
}
else
{
    echo "- Arquivo salvo: (não aplicável ou erro)\n";
}

echo str_repeat("=", 50) . "\n";
?>
