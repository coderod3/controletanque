<?php
session_start();

// Fetch environment variables
$dsn = getenv('POSTGRES_URL');
$user = getenv('POSTGRES_USER');
$password = getenv('POSTGRES_PASSWORD');

// Debug: Print environment variables
var_dump($dsn);
var_dump($user);
var_dump($password);

try {
    // Create a PDO instance
    $pdo = new PDO($dsn, $user, $password, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    ]);
} catch (PDOException $e) {
    die("Could not connect to the database: " . $e->getMessage());
}

// Debug: Print database connection status
if ($pdo) {
    echo "Connected to the database successfully!";
} else {
    echo "Failed to connect to the database.";
}

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['data'])) {
        $data = filter_input(INPUT_GET, 'data', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);

        // Prepare and execute SQL statement to insert or update data
        $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2) ON CONFLICT (id) DO UPDATE SET level1 = :level1, level2 = :level2");
        $stmt->execute([':level1' => $data, ':level2' => $data]);

        echo $data;
    } else {
        // Retrieve the latest level1 value from the database
        $stmt = $pdo->query("SELECT level1 FROM water_levels ORDER BY id DESC LIMIT 1");
        $result = $stmt->fetch();

        if ($result) {
            echo $result['level1'];
        } else {
            // Insert default data if no records found
            $data = 5;
            $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2)");
            $stmt->execute([':level1' => $data, ':level2' => $data]);
            echo $data;
        }
    }
}
?>
