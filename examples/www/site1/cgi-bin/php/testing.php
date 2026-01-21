#!/usr/bin/php
<?php
header("Content-Type: text/html; charset=UTF-8");

// Variáveis de ambiente
$env_vars = [
    "GATEWAY_INTERFACE" => getenv("GATEWAY_INTERFACE"),
    "SERVER_PROTOCOL" => getenv("SERVER_PROTOCOL"),
    "REQUEST_METHOD" => getenv("REQUEST_METHOD"),
    "SERVER_SOFTWARE" => getenv("SERVER_SOFTWARE"),
    "REDIRECT_STATUS" => getenv("REDIRECT_STATUS"),
    "UPLOAD_DIR" => getenv("UPLOAD_DIR"),
    "SCRIPT_NAME" => getenv("SCRIPT_NAME"),
    "QUERY_STRING" => getenv("QUERY_STRING"),
];

// Parâmetros GET
parse_str(getenv("QUERY_STRING"), $params);
$nome = isset($params['nome']) ? htmlspecialchars($params['nome']) : "Fulano";

// Cores
$primaryColor = "#4f46e5"; // cor do sinalizador
$textColor = "#161616";
$bgColor = "#a5b3b1b8";
?>

<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Olá, <?php echo $nome; ?>!</title>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/carbon-components/10.58.12/css/carbon-components.min.css">
<script src="https://cdn.jsdelivr.net/npm/sweetalert2@11"></script>
<style>
body {
    font-family: system-ui, -apple-system, "Segoe UI", Roboto, Arial;
    background: <?php echo $bgColor; ?>;
    color: <?php echo $textColor; ?>;
    margin: 0;
    padding: 2rem;
}

h1 { font-size: 2.5rem; margin-bottom: 1rem; color: <?php echo $primaryColor; ?>; }

.example-box {
    background: #ffffff;
    border-left: 4px solid <?php echo $primaryColor; ?>;
    padding: 1rem 1.5rem;
    border-radius: 8px;
    margin-bottom: 1rem;
    box-shadow: 0 2px 6px rgba(0,0,0,0.08);
    font-family: 'IBM Plex Mono', monospace;
    font-size: 0.95rem;
    line-height: 1.5;
    color: #161616;
}

.example-box p { margin: 0.25rem 0; }
.example-box code {
    background: #e5e7eb;
    color: #161616;
    padding: 0.1rem 0.3rem;
    border-radius: 3px;
    font-size: 0.9rem;
}

.env-table {
    border-collapse: collapse;
    width: 100%;
    margin-top: 0.5rem;
}

.env-table th, .env-table td {
    border: 1px solid rgba(0,0,0,0.1);
    padding: 0.5rem 0.75rem;
}

.env-table th {
    background-color: rgba(79,70,229,0.1);
}
</style>
</head>
<body>

<h1>Olá, <?php echo $nome; ?>! 👋</h1>

<div class="example-box">
    <p>💡 Você pode adicionar parâmetros na URL depois da URI:</p>
    <p>Exemplo: <code>URI?nome=manandre&peso=70&altura=1.75</code></p>
</div>

<?php if (!empty($params)): ?>
    <div class="example-box">✨ Parâmetros GET recebidos: <?php echo count($params); ?> ✨</div>

    <div class="example-box">
        <ul>
        <?php foreach ($params as $k => $v): ?>
            <li><?php echo htmlspecialchars($k); ?> = <?php echo htmlspecialchars($v); ?></li>
        <?php endforeach; ?>
        </ul>
    </div>

    <?php
    if (isset($params['peso']) && isset($params['altura'])) {
        $peso = floatval($params['peso']);
        $altura = floatval($params['altura']);
        if ($altura > 0) {
            $imc = $peso / ($altura * $altura);
            echo "<div class='example-box'><p>IMC calculado: " . round($imc,2) . "</p>";

            if ($imc < 18.5) echo "<p>Classificação: Abaixo do peso</p>";
            else if ($imc < 25) echo "<p>Classificação: Peso normal</p>";
            else if ($imc < 30) echo "<p>Classificação: Sobrepeso</p>";
            else if ($imc < 35) echo "<p>Classificação: Obesidade grau I</p>";
            else if ($imc < 40) echo "<p>Classificação: Obesidade grau II</p>";
            else echo "<p>Classificação: Obesidade grau III</p>";

            echo "</div>";
        } else {
            echo "<div class='example-box'><p>Altura inválida.</p></div>";
        }
    } else {
        echo "<div class='example-box'><p>ℹ Informe os parâmetros <code>peso</code> e <code>altura</code> na URL para calcular o IMC.</p></div>";
    }
    ?>
<?php endif; ?>

<div class="example-box">
    <p>Variáveis de ambiente recebidas pelo CGI:</p>
    <table class="env-table">
        <tr>
            <th>Variável</th>
            <th>Valor</th>
        </tr>
        <?php foreach ($env_vars as $key => $value): ?>
        <tr>
            <td><?php echo htmlspecialchars($key); ?></td>
            <td><?php echo htmlspecialchars($value); ?></td>
        </tr>
        <?php endforeach; ?>
    </table>
</div>

</body>
</html>
