#!/usr/bin/env python3
"""
Simple Test Website for 42 Webserv Project
Creates an HTML page with forms, links, and buttons to test your server.
Run: python3 test_website.py [port]
Then visit: http://localhost:[port] (or the port you specified)
"""

import http.server
import socketserver
import sys
import os
from string import Template


interface = os.environ.get("SERVER_NAME","") + ":" + os.environ.get("SERVER_PORT","")
HTML_CONTENT = """Content-Type: text/html\r\n\r\n<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>⚡️SwiftServe⚡️</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }
        
        body {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 800px;
            width: 100%;
        }
        
        header {
            text-align: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid #f0f0f0;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 2.5em;
        }
        
        .subtitle {
            color: #666;
            font-size: 1.1em;
        }
        
        .server-info {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 30px;
            border-left: 4px solid #667eea;
        }
        
        .server-info h3 {
            color: #333;
            margin-bottom: 10px;
        }
        
        .test-section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
        }
        
        h2 {
            color: #333;
            margin-bottom: 20px;
            font-size: 1.5em;
        }
        
        .button-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        
        .test-btn {
            background: #667eea;
            color: white;
            border: none;
            padding: 15px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            transition: all 0.3s ease;
            text-decoration: none;
            display: inline-block;
            text-align: center;
        }
        
        .test-btn:hover {
            background: #5a67d8;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        
        .test-btn.danger {
            background: #e53e3e;
        }
        
        .test-btn.danger:hover {
            background: #c53030;
        }
        
        .test-btn.success {
            background: #38a169;
        }
        
        .test-btn.success:hover {
            background: #2f855a;
        }
        
        .form-group {
            margin-bottom: 15px;
        }
        
        label {
            display: block;
            margin-bottom: 5px;
            color: #333;
            font-weight: 500;
        }
        
        input, textarea, select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s ease;
        }
        
        input:focus, textarea:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        
        textarea {
            min-height: 100px;
            resize: vertical;
        }
        
        .status {
            padding: 10px;
            border-radius: 5px;
            margin-top: 20px;
            display: none;
        }
        
        .status.success {
            background: #c6f6d5;
            color: #22543d;
            display: block;
        }
        
        .status.error {
            background: #fed7d7;
            color: #742a2a;
            display: block;
        }
        
        footer {
            text-align: center;
            margin-top: 40px;
            padding-top: 20px;
            border-top: 2px solid #f0f0f0;
            color: #666;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>⚡️SwiftServe⚡️ Test Site</h1>
            <p class="subtitle">Test your 42 webserver with various requests</p>
        </header>
        
        <div class="server-info">
            <h3>📍 Server Information</h3>
            <p><strong>Base URL:</strong> <span id="base-url">http://${interface}</span></p>
            <p><strong>Test Page:</strong> Running on port <span id="test-port">8080</span></p>
            <p><strong>Note:</strong> Make sure your webserv is running on port 8080 before testing</p>
        </div>
        
        <!-- GET Request Tests -->
        <div class="test-section">
            <h2>📡 GET Request Tests</h2>
            <div class="button-grid">
                <a href="http://${interface}/" class="test-btn" target="_blank">
                    Test Homepage
                </a>
                <a href="http://${interface}/nonexistent" class="test-btn danger" target="_blank">
                    Test 404 Error
                </a>
                <a href="http://${interface}/dire/" class="test-btn" target="_blank">
                    Test Directory
                </a>
                <a href="http://${interface}/largefile.html" class="test-btn" target="_blank">
                    Large File 
                </a>
                <a href="http://${interface}/video.mp4" class="test-btn" target="_blank">
                    Large Video
                </a>
                <button class="test-btn" onclick="testConcurrentRequests()">
                    Test Concurrent
                </button>
            </div>
        </div>
        
        <!-- POST Request Tests -->
        <div class="test-section">
            <h2>📝 POST Request Tests</h2>
            <form id="post-form">
                <div class="form-group">
                    <label for="username">Username:</label>
                    <input type="text" id="username" name="username" placeholder="Enter username">
                </div>
                <div class="form-group">
                    <label for="message">Message:</label>
                    <textarea id="message" name="message" placeholder="Enter your message">Testing webserver from test page</textarea>
                </div>
                <div class="form-group">
                    <label for="method">HTTP Method:</label>
                    <select id="method" name="method">
                        <option value="POST">POST</option>
                        <option value="PUT">PUT</option>
                        <option value="PATCH">PATCH</option>
                    </select>
                </div>
                <button type="button" class="test-btn success" onclick="sendFormData()">
                    Send Request
                </button>
                <button type="button" class="test-btn" onclick="uploadFile()">
                    Upload File
                </button>
            </form>
            <div id="form-status" class="status"></div>
        </div>
        
        <!-- Response Display -->
        <div class="test-section">
            <h2>📊 Response Information</h2>
            <div class="form-group">
                <label for="custom-url">Custom URL:</label>
                <input type="text" id="custom-url" placeholder="http://${interface}/path" value="http://${interface}">
            </div>
            <div id="fetchurl" class="button-grid">
                <button class="test-btn" onclick="fetchCustomUrl()">
                    Fetch URL
                </button>
                 <button class="test-btn" onclick="goToCustomUrl()">
                    Go To URL
                </button>
                <button class="test-btn" onclick="deleteUrl()">
                    Delete URL
                </button>
                <button class="test-btn" onclick="clearResponses()">
                    Clear Responses
                </button>
            </div>
            <div id="response-display" style="margin-top: 20px; padding: 15px; background: #f8f9fa; border-radius: 8px; min-height: 100px; font-family: monospace; white-space: pre-wrap; word-wrap: break-word;"></div>
        </div>
        
        <footer>
            <p>42 Webserv Project Tester • Created for testing webserver implementations</p>
            <p>Click buttons to test various HTTP requests against your server</p>
        </footer>
    </div>
    
    <script>
        const SERVER_URL = 'http://${interface}';
        const TEST_PORT = window.location.port || '8081';
        
        // Update displayed URLs
        document.getElementById('base-url').textContent = SERVER_URL;
        document.getElementById('test-port').textContent = TEST_PORT;
        
        function showStatus(elementId, message, isSuccess = true) {
            const element = document.getElementById(elementId);
            element.textContent = message;
            element.className = 'status ' + (isSuccess ? 'success' : 'error');
            element.style.display = 'block';
            
            setTimeout(() => {
                element.style.display = 'none';
            }, 5000);
        }
        
        function displayResponse(data) {
            const display = document.getElementById('response-display');
            let output = `Status: ${data.status}\\n`;
            output += `URL: ${data.url}\\n`;
            
            if (data.headers) {
                output += '\\nHeaders:\\n';
                for (const [key, value] of Object.entries(data.headers)) {
                    output += `  ${key}: ${value}\\n`;
                }
            }
            
            if (data.body) {
                output += '\\nBody (first 500 chars):\\n';
                output += data.body.substring(0, 500);
                if (data.body.length > 500) output += '...';
            }
            
            output += '\\n' + '='.repeat(50) + '\\n';
            
            display.textContent = output + display.textContent;
        }
        
        async function makeRequest(url, options = {}) {
            try {
                const startTime = Date.now();
                const response = await fetch(url, {
                    mode: 'cors',
                    ...options
                });
                const endTime = Date.now();
                
                const responseData = {
                    url: url,
                    status: response.status,
                    statusText: response.statusText,
                    headers: {},
                    time: endTime - startTime,
                    body: await response.text()
                };
                
                // Get headers
                response.headers.forEach((value, key) => {
                    responseData.headers[key] = value;
                });
                
                displayResponse(responseData);
                return responseData;
                
            } catch (error) {
                displayResponse({
                    url: url,
                    status: 'Error',
                    body: error.toString()
                });
                return null;
            }
        }
        
        // Test functions
        function testLargeFile() {
            makeRequest(SERVER_URL + '/large_file.bin');
            showStatus('form-status', 'Testing large file download...', true);
        }
        
        async function testConcurrentRequests() {
            showStatus('form-status', 'Sending 5 concurrent requests...', true);
            const urls = Array(5).fill(SERVER_URL + '/');
            const requests = urls.map(url => makeRequest(url));
            await Promise.all(requests);
            showStatus('form-status', 'Concurrent requests completed!', true);
        }
        
        function sendFormData() {
            const form = document.getElementById('post-form');
            const formData = new FormData(form);
            const method = document.getElementById('method').value;
            var url = '/upload';
            var encoded = new URLSearchParams(formData).toString();

            makeRequest(SERVER_URL + url , {
                method: method,
                headers: {"Content-Type": "application/x-www-form-urlencoded" },
                body: encoded
            });

            showStatus('form-status', `${method} request sent to /upload`, true);
        }
        
        function uploadFile() {
            const input = document.createElement('input');
            input.type = 'file';
            
            input.onchange = async (e) => {
                const file = e.target.files[0];
                if (!file) return;
                
                makeRequest(SERVER_URL + '/upload', {
                    method: 'POST',
                    body: file
                });
                
                showStatus('form-status', `File "${file.name}" upload started`, true);
            };
            
            input.click();
        }
    
        function goToCustomUrl() {
            const url = document.getElementById('custom-url').value;
            if (!url) {
                showStatus('form-status', 'Please enter a URL', false);
                return;
            }

           window.open( url,"_blank");
            
        }
        
        function fetchCustomUrl()  {
            const url = document.getElementById('custom-url').value;
            if (!url) {
                showStatus('form-status', 'Please enter a URL', false);
                return;
            }
            
            makeRequest(url);
        }
        
        function deleteUrl(){
            const url = document.getElementById('custom-url').value;
            if (!url) {
                showStatus('form-status', 'Please enter a URL', false);
                return;
            }

            makeRequest( url , {method:'DELETE'});
        }

        function clearResponses() {
            document.getElementById('response-display').textContent = '';
        }
        
        // Auto-test on page load
        window.addEventListener('load', () => {
            // Test connection to webserv
            makeRequest(SERVER_URL + '/')
                .then(data => {
                    if (data && data.status === 200) {
                        console.log('✅ Connected to webserv successfully');
                    } else {
                        console.warn('⚠️ Could not connect to webserv on ' + SERVER_URL);
                    }
                });
        });
    </script>
</body>
</html>
"""
""" 
class TestWebsiteHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(HTML_CONTENT.encode())
        else:
            super().do_GET()
"""
def create_test_files():
    """Create test files for the webserver"""
    test_dir = os.environ.get("SCRIPT_ROOT","/var/www")+"/test_files"
    os.makedirs(test_dir, exist_ok=True)
    
    # Create test HTML file
    with open(f"{test_dir}/index.html", "w") as f:
        f.write("""<!DOCTYPE html>
<html>
<head><title>Test Page</title></head>
<body>
<h1>Test Page for Webserv</h1>
<p>This is a test file for your webserver.</p>
<a href="/test_files/page2.html">Go to page 2</a>
</body>
</html>""")
    
    # Create second page
    with open(f"{test_dir}/page2.html", "w") as f:
        f.write("""<!DOCTYPE html>
<html>
<head><title>Page 2</title></head>
<body>
    <h1>Page 2</h1>
    <p>This is another test page.</p>
    <a href="/test_files/index.html">Back to home</a>
</body>
</html>""")
    
    # Create a large file for testing
    with open(f"{test_dir}/large_file.bin", "wb") as f:
        f.write(b'X' * 1024 * 1024)  # 1MB file
    
    # Create a simple text file
    with open(f"{test_dir}/test.txt", "w") as f:
        f.write("This is a simple text file for testing.\nLine 2\nLine 3")
    
"""   print(f"✅ Test files created in '{test_dir}/' directory")
    print(f"   You can access them at: http://${interface}/{test_dir}/") """

def run_test_website(port=8080):
    """Run the test website server
    print(f"\n🚀 Starting test website on port {port}...")
    print(f"📌 Open your browser and visit: http://localhost:{port}")
    print(f"📌 Make sure your webserv is running on port 8080")
    print("\n📋 Available tests on the page:")
    print("   • GET requests (homepage, 404, directories)")
    print("   • POST/PUT/DELETE form submissions")
    print("   • File uploads")
    print("   • Header manipulation")
    print("   • Concurrent requests")
    print("   • Custom URL testing")
    print("\nPress Ctrl+C to stop the test server\n")
    """
	
    content = Template(HTML_CONTENT).safe_substitute(interface=interface)
    print(content)
	

"""  
    # Change to current directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
     
    # Start the server
    with socketserver.TCPServer(("", port), TestWebsiteHandler) as httpd:
        print(f"✅ Server ready at http://localhost:{port}")
        print("   (The page will automatically test connection to your webserv)")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\n👋 Server stopped")
"""
if __name__ == "__main__":
    port = os.environ.get("SERVER_PORT", "8080")
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            print(f"Usage: {sys.argv[0]} [port]")
            print(f"Default port: 8081")
            sys.exit(1)
    
    run_test_website(port)