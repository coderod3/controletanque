<?php
session_start();

$file = 'data.txt';

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (isset($_GET['data'])) {
        $data = filter_input(INPUT_GET, 'data', FILTER_SANITIZE_NUMBER_FLOAT);
        file_put_contents($file, $data);
        echo $data; // Return the data sent in the GET request
    } else {
        if (file_exists($file)) {
            $data = file_get_contents($file);
            echo $data; // Return the stored data
        } else {
            $data = 5;
            file_put_contents($file, $data);
            echo $data; // Return the default data if no stored data exists
        }
    }
}
?>
