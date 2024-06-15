<?php
session_start();

$file = 'data.txt';

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['data'])) {
        $data = filter_input(INPUT_GET, 'data', FILTER_SANITIZE_NUMBER_INT);
        file_put_contents($file, $data);
        echo $data; // Return the data sent in the GET request
    } else {
        if (!file_exists($file)) {
            file_put_contents($file, '5'); // Initialize with default value
        }
        $data = file_get_contents($file);
        echo $data; // Return the stored data
    }
}
?>
