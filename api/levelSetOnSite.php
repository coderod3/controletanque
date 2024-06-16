<?php
session_start();

// Fetch environment variables for database connection
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
    if (isset($_GET['newLevel'])) {
        $newLevel = filter_input(INPUT_GET, 'newLevel', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);

        // Update the water level in the database
        $stmt = $pdo->prepare("INSERT INTO water_levels (level1) VALUES (:newLevel) ON CONFLICT (id) DO UPDATE SET level1 = :newLevel");
        $stmt->execute([':newLevel' => $newLevel]);

        echo $newLevel; // Return the new level sent in the GET request
    } else {
        // Fetch the current water level from the database
        $stmt = $pdo->query("SELECT level1 FROM water_levels ORDER BY id DESC LIMIT 1");
        $result = $stmt->fetch();

        if ($result) {
            echo $result['level1'];
        } else {
            // If no level is found, you can set a default or handle the case as needed
            echo "No data available";
        }
    }
}
?>
