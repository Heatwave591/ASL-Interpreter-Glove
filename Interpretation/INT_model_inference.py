import serial
import pickle
import time
import numpy as np
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import argparse
from sklearn.preprocessing import StandardScaler

def load_model(model_path):
    """Load the trained RandomForest model"""
    print(f"Loading model from {model_path}...")
    try:
        with open(model_path, 'rb') as f:
            model = pickle.load(f)
        print("Model loaded successfully!")
        return model
    except Exception as e:
        print(f"Error loading model: {e}")
        return None

def initialize_firebase():
    """Initialize Firebase connection"""
    try:
        # Path to your Firebase service account key file
        cred = credentials.Certificate("asl-interpreter-glove-firebase-key.json")
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'https://asl-interpreter-glove-default-rtdb.firebaseio.com/'
        })
        print("Firebase initialized successfully!")
        return True
    except Exception as e:
        print(f"Error initializing Firebase: {e}")
        return False

def process_sensor_data(sensor_values, model, scaler=None):
    """Process sensor data and run inference with the model"""
    # Convert to numpy array and reshape for model prediction
    features = np.array(sensor_values).reshape(1, -1)
    
    # Scale the features if a scaler is provided
    if scaler:
        features = scaler.transform(features)
    
    # Make prediction
    prediction = model.predict(features)[0]
    
    # Get probability scores
    probabilities = model.predict_proba(features)[0]
    max_prob = max(probabilities)
    
    return prediction, max_prob

def update_firebase(prediction, confidence, sensor_values):
    """Update Firebase with prediction and sensor values"""
    try:
        # Reference to the inference results node
        ref = db.reference('/inference_results')
        
        # Create data to send
        data = {
            'predicted_sign': prediction,
            'confidence': float(confidence),
            'finger1': int(sensor_values[0]),
            'finger2': int(sensor_values[1]),
            'finger3': int(sensor_values[2]),
            'finger4': int(sensor_values[3]),
            'finger5': int(sensor_values[4]),
            'timestamp': int(time.time() * 1000)
        }
        
        # Update Firebase
        ref.set(data)
        return True
    except Exception as e:
        print(f"Error updating Firebase: {e}")
        return False

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='ASL Interpreter - Inference')
    parser.add_argument('--port', type=str, required=True, 
                        help='Serial port for ESP32 (e.g., COM3 on Windows or /dev/ttyUSB0 on Linux)')
    parser.add_argument('--baud', type=int, default=115200, 
                        help='Baud rate for serial communication')
    parser.add_argument('--model', type=str, required=True, 
                        help='Path to the trained model pickle file')
    parser.add_argument('--scaler', type=str, default=None,
                        help='Path to the scaler pickle file (optional)')
    
    args = parser.parse_args()
    
    # Load the trained model
    model = load_model(args.model)
    if model is None:
        return
    
    # Load scaler if provided
    scaler = None
    if args.scaler:
        try:
            with open(args.scaler, 'rb') as f:
                scaler = pickle.load(f)
            print("Scaler loaded successfully!")
        except Exception as e:
            print(f"Error loading scaler: {e}")
    
    # Initialize Firebase
    if not initialize_firebase():
        return
    
    # Open serial connection to ESP32
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"Connected to {args.port} at {args.baud} baud")
    except Exception as e:
        print(f"Error opening serial port: {e}")
        return
    
    # Main loop
    print("Starting inference loop... Press Ctrl+C to exit")
    try:
        while True:
            # Read a line from the serial port
            line = ser.readline().decode('utf-8').strip()
            
            # Parse sensor values
            if line and ',' in line:
                try:
                    sensor_values = [float(x) for x in line.split(',')]
                    
                    if len(sensor_values) == 5:  # Make sure we have 5 values
                        # Process data and get prediction
                        prediction, confidence = process_sensor_data(sensor_values, model, scaler)
                        
                        # Print results
                        print(f"Sensor values: {sensor_values}")
                        print(f"Prediction: {prediction}, Confidence: {confidence:.4f}")
                        
                        # Update Firebase
                        if update_firebase(prediction, confidence, sensor_values):
                            print(f"Firebase updated with prediction: {prediction}")
                        
                        print("-------------------")
                    else:
                        print(f"Invalid number of sensor values: {len(sensor_values)}")
                except ValueError as e:
                    print(f"Error parsing sensor values: {e}")
                    print(f"Raw data: {line}")
            
            # Small delay to prevent excessive CPU usage
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        if ser.is_open:
            ser.close()
            print("Serial connection closed")

if __name__ == "__main__":
    main()