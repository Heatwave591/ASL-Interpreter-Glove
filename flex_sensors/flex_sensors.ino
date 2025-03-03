<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Finger Sensor Simulation</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .dashboard {
            background-color: #fff;
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 20px;
        }
        .sensor-readings {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
            margin-bottom: 20px;
        }
        .sensor-card {
            background-color: #e9f7fe;
            border-radius: 5px;
            padding: 15px;
            text-align: center;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        }
        .sensor-title {
            font-weight: bold;
            margin-bottom: 10px;
            color: #0277bd;
        }
        .sensor-value {
            font-size: 24px;
            font-weight: bold;
            color: #01579b;
        }
        .output-section {
            background-color: #333;
            color: #fff;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            margin-bottom: 20px;
            min-height: 180px;
            overflow-y: auto;
        }
        .output-line {
            margin: 5px 0;
            white-space: pre;
        }
        .status {
            background-color: #4caf50;
            color: white;
            padding: 15px;
            border-radius: 5px;
            font-size: 24px;
            text-align: center;
            margin-top: 20px;
            font-weight: bold;
        }
        .controls {
            display: flex;
            justify-content: space-between;
            margin-top: 20px;
        }
        button {
            background-color: #0277bd;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #01579b;
        }
    </style>
</head>
<body>
    <div class="dashboard">
        <h1>Finger Sensor Simulation</h1>
        
        <div class="sensor-readings">
            <div class="sensor-card">
                <div class="sensor-title">Finger 1</div>
                <div class="sensor-value" id="f_volt1">0</div>
            </div>
            <div class="sensor-card">
                <div class="sensor-title">Finger 2</div>
                <div class="sensor-value" id="f_volt2">0</div>
            </div>
            <div class="sensor-card">
                <div class="sensor-title">Finger 3</div>
                <div class="sensor-value" id="f_volt3">0</div>
            </div>
            <div class="sensor-card">
                <div class="sensor-title">Finger 4</div>
                <div class="sensor-value" id="f_volt4">0</div>
            </div>
            <div class="sensor-card">
                <div class="sensor-title">Finger 5</div>
                <div class="sensor-value" id="f_volt5">0</div>
            </div>
        </div>
        
        <div class="status" id="status">Initializing...</div>
        
        <div class="output-section" id="serialOutput">
            <div class="output-line">Serial monitor initialized at 115200 baud rate</div>
        </div>
        
        <div class="controls">
            <button id="startBtn">Start Simulation</button>
            <button id="stopBtn">Stop Simulation</button>
            <button id="clearBtn">Clear Output</button>
        </div>
    </div>

    <script>
        // Sensor values
        let f_volt1 = 0;
        let f_volt2 = 0;
        let f_volt3 = 0;
        let f_volt4 = 0;
        let f_volt5 = 0;
        
        // Simulation variables
        let simulationInterval;
        let isRunning = false;
        
        // DOM elements
        const startBtn = document.getElementById('startBtn');
        const stopBtn = document.getElementById('stopBtn');
        const clearBtn = document.getElementById('clearBtn');
        const serialOutput = document.getElementById('serialOutput');
        const statusElement = document.getElementById('status');
        
        // Random number generator (similar to Arduino's random function)
        function random(max) {
            return Math.floor(Math.random() * max);
        }
        
        // Add a line to the serial output
        function serialPrint(text, newLine = false) {
            const line = document.createElement('div');
            line.className = 'output-line';
            line.textContent = text + (newLine ? '' : '');
            serialOutput.appendChild(line);
            serialOutput.scrollTop = serialOutput.scrollHeight;
        }
        
        // Clear the serial output
        function clearOutput() {
            serialOutput.innerHTML = '<div class="output-line">Serial monitor cleared</div>';
        }
        
        // Update the UI with new sensor values
        function updateUI() {
            document.getElementById('f_volt1').textContent = f_volt1;
            document.getElementById('f_volt2').textContent = f_volt2;
            document.getElementById('f_volt3').textContent = f_volt3;
            document.getElementById('f_volt4').textContent = f_volt4;
            document.getElementById('f_volt5').textContent = f_volt5;
        }
        
        // Simulate the Arduino loop function
        function loop() {
            // Generate random values (0-1023) like in the Arduino code
            f_volt1 = random(1024);
            f_volt2 = random(1024);
            f_volt3 = random(1024);
            f_volt4 = random(1024);
            f_volt5 = random(1024);
            
            // Update the UI
            updateUI();
            
            // Print to serial (similar to Serial.print in Arduino)
            let output = `Finger 1: ${f_volt1}\tFinger 2: ${f_volt2}\tFinger 3: ${f_volt3}\tFinger 4: ${f_volt4}\tFinger 5: ${f_volt5}\t`;
            
            // Determine status based on the same logic as your Arduino code
            let status = "";
            if (f_volt1 > 200 && f_volt2 < 200) {
                status = "Water";
            } else if (f_volt2 > 200 && f_volt1 < 200) {
                status = "Food";
            } else if (f_volt1 > 200 && f_volt2 > 200) {
                status = "Water and Food";
            } else {
                status = "3, 4, 5";
            }
            
            // Update status display
            statusElement.textContent = status;
            
            // Add complete output to serial monitor
            serialPrint(output + status);
        }
        
        // Start the simulation
        function startSimulation() {
            if (!isRunning) {
                serialPrint("Starting simulation...");
                simulationInterval = setInterval(loop, 2500); // Same delay as in Arduino
                isRunning = true;
                startBtn.disabled = true;
                stopBtn.disabled = false;
            }
        }
        
        // Stop the simulation
        function stopSimulation() {
            if (isRunning) {
                clearInterval(simulationInterval);
                serialPrint("Simulation stopped.");
                isRunning = false;
                startBtn.disabled = false;
                stopBtn.disabled = true;
            }
        }
        
        // Event listeners
        startBtn.addEventListener('click', startSimulation);
        stopBtn.addEventListener('click', stopSimulation);
        clearBtn.addEventListener('click', clearOutput);
        
        // Initialize
        stopBtn.disabled = true;
        
        // Run setup once at the beginning
        serialPrint("Setup completed. Press 'Start Simulation' to begin.");
    </script>
</body>
</html>