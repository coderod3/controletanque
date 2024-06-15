<?php
session_start();

$file = 'level.txt';

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['newLevel'])) {
        $newLevel = filter_input(INPUT_GET, 'newLevel', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);
        file_put_contents($file, $newLevel);
        echo $newLevel; // Return the new level sent in the GET request
    } else {
        if (!file_exists($file)) {
            file_put_contents($file, '0'); // Initialize with default value
        }
        $level = file_get_contents($file);
        echo $level; // Return the stored level
    }
}
?>
