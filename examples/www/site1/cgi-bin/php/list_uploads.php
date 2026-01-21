<?php
header("Content-Type: application/json; charset=UTF-8");

// - PROJECT ROOT -
$projectRoot = dirname(dirname(__DIR__));

// - UPLOAD DIR (ENV) -
$uploadDirEnv = getenv('UPLOAD_DIR') ?: 'uploads_';
$uploadDirEnv = trim($uploadDirEnv, '/');

// - PATH FISICO -
$uploadDir = $projectRoot . '/' . $uploadDirEnv;

// - URL PUBLICA -
$uploadUrl = '/' . $uploadDirEnv;

// - CHECK -
if (!is_dir($uploadDir)) {
    echo json_encode(['files' => []]);
    exit;
}

// - LIST FILES -
$files = array_diff(scandir($uploadDir), ['.', '..']);
$data = [];

foreach ($files as $file) {
    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));

    $data[] = [
        'name' => $file,
        'url'  => $uploadUrl . '/' . rawurlencode($file),
        'type' => in_array($ext, ['jpg','jpeg','png','gif','webp']) ? 'image'
               : (in_array($ext, ['mp4','webm']) ? 'video'
               : (in_array($ext, ['mp3','wav']) ? 'audio'
               : 'file'))
    ];
}

echo json_encode(['files' => $data], JSON_UNESCAPED_UNICODE);