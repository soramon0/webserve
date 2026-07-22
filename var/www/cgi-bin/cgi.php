<?php
$name = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : null;
echo  "status: 200 OK\n";
echo  "Content-Type: text/html\n\n";
echo "<!DOCTYPE html>";
echo "<html lang='en'>";
echo "<head><meta charset='UTF-8'><title>Welcome Page</title></head>";
echo "<body>";
echo "<h1>Welcome, $name!</h1>";
echo "<p>This is a simple CGI-like PHP script.</p>";
echo "</body>";
echo "</html>";
?>

