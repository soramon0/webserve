#!/bin/bash
name="${QUERY_STRING:-}"

echo  "status: 200 OK"
echo "Content-Type: text/html"
echo ""

cat <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Welcome Page</title>
</head>
<body>
    <h1>Welcome, $name!</h1>
    <p>This is a simple CGI-like Bash script.</p>
</body>
</html>
EOF


