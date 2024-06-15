<?php
session_start();

$dsn = getenv('POSTGRES_URL');
$user = getenv('POSTGRES_USER');
$password = getenv('POSTGRES_PASSWORD');

try {
    $pdo = new PDO($dsn, $user, $password, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    ]);
} catch (PDOException $e) {
    die("Could not connect to the database: " . $e->getMessage());
}

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['data'])) {
        $data = filter_input(INPUT_GET, 'data', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);

        $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2) ON CONFLICT (id) DO UPDATE SET level1 = :level1, level2 = :level2");
        $stmt->execute([':level1' => $data, ':level2' => $data]);

        echo $data;
    } else {
        $stmt = $pdo->query("SELECT level1 FROM water_levels ORDER BY id DESC LIMIT 1");
        $result = $stmt->fetch();

        if ($result) {
            echo $result['level1'];
        } else {
            $data = 5;
            $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2)");
            $stmt->execute([':level1' => $data, ':level2' => $data]);
            echo $data;
        }
    }
}
?>
