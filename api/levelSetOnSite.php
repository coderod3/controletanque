<?php
session_start();

$file = 'level.txt';
$logFile = 'log.txt';
date_default_timezone_set('America/Sao_Paulo');

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['newLevel']) && isset($_GET['userAgent']) && isset($_GET['userIP'])) {
        $newLevel = filter_input(INPUT_GET, 'newLevel', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);
        $userAgent = filter_input(INPUT_GET, 'userAgent', FILTER_SANITIZE_STRING);
        $userIP = filter_input(INPUT_GET, 'userIP', FILTER_SANITIZE_STRING);
        $previousLevelCm = file_get_contents($file);
        // Update the water level
        file_put_contents($file, $newLevel);
        
        // Log the information
        $logEntry = sprintf(
            "User IP: %s, User Agent: %s, Previous Level: %s, New Level: %s, Timestamp: %s\n",
            $userIP,
            $userAgent,
            $previousLevelCm,
            $newLevel,
            date("Y-m-d H:i:s")
        );
        file_put_contents($logFile, $logEntry, FILE_APPEND);

        echo $newLevel; // Return the new level sent in the GET request
    } else {
        $contents = file_get_contents($file);
        echo $contents;
    }
}
?>
