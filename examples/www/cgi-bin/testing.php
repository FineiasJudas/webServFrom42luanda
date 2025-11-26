<?php
echo "Content-Type: text/plain\r\n\r\n";

echo "=== PHP CGI TEST ===\n\n";

echo "REQUEST_METHOD: " . getenv("REQUEST_METHOD") . "\n";
echo "QUERY_STRING: " . getenv("QUERY_STRING") . "\n";
echo "CONTENT_LENGTH: " . getenv("CONTENT_LENGTH") . "\n";
echo "SCRIPT_NAME: " . getenv("SCRIPT_NAME") . "\n";
echo "SCRIPT_FILENAME: " . getenv("SCRIPT_FILENAME") . "\n";
echo "REQUEST_URI: " . getenv("REQUEST_URI") . "\n";
echo "DOCUMENT_ROOT: " . getenv("DOCUMENT_ROOT") . "\n\n";

if (getenv("REQUEST_METHOD") === "POST") {
    $len = intval(getenv("CONTENT_LENGTH"));
    $data = file_get_contents("php://input", false, null, 0, $len);
    echo "POST DATA:\n";
    echo $data;
}
?>
