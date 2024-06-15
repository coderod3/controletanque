<?php
session_start();

// Retrieve database connection details from environment variables
$dsn = getenv('POSTGRES_URL');
$user = getenv('POSTGRES_USER');
$password = getenv('POSTGRES_PASSWORD');
$db = getenv('POSTGRES_DATABASE');

// Establish a new PDO connection
try {
    $pdo = new PDO($dsn, $user, $password);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
    die("Could not connect to the database: " . $e->getMessage());
}

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['data'])) {
        // Sanitize input data
        $data = filter_input(INPUT_GET, 'data', FILTER_SANITIZE_NUMBER_FLOAT);
        
        // Insert or update the data in the database
        $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2) ON CONFLICT (id) DO UPDATE SET level1 = :level1, level2 = :level2");
        $stmt->execute([':level1' => $data, ':level2' => $data]); // Assuming you're storing both levels similarly
        
        echo $data; // Return the data sent in the GET request
    } else {
        // Retrieve the latest data from the database
        $stmt = $pdo->query("SELECT level1 FROM water_levels ORDER BY id DESC LIMIT 1");
        $result = $stmt->fetch(PDO::FETCH_ASSOC);
        
        if ($result) {
            echo $result['level1']; // Return the stored data
        } else {
            $data = 5;
            // Insert the default data into the database
            $stmt = $pdo->prepare("INSERT INTO water_levels (level1, level2) VALUES (:level1, :level2)");
            $stmt->execute([':level1' => $data, ':level2' => $data]);
            echo $data; // Return the default data if no stored data exists
        }
    }
}
?>
