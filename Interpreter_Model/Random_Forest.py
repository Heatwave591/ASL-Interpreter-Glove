# RANDOM FOREST DECISION ALGORITHM: Training + Test script 

import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, confusion_matrix
import matplotlib.pyplot as plt
import seaborn as sns
import joblib
import serial
import time

# Part 1: Training the model with data from CSV
def train_model(csv_path, test_size=0.2, random_state=42):
    """
    Train a Random Forest model using data from a CSV file
    
    Parameters:
    - csv_path: Path to the CSV file with training data
    - test_size: Portion of data to be used for testing (default 0.2 = 20%)
    - random_state: Random seed for reproducibility
    
    Returns:
    - trained_model: The trained RandomForestClassifier
    - scaler: The fitted StandardScaler for preprocessing new data
    - feature_names: List of feature names used for training
    - train_accuracy: Training set accuracy
    - test_accuracy: Test set accuracy
    """
    # Load the data
    print(f"Loading data from {csv_path}...")
    df = pd.read_csv(csv_path)
    
    # Display basic info about the dataset
    print(f"Dataset shape: {df.shape}")
    print(f"Number of unique signs: {df['LABEL'].nunique()}")
    print(f"Signs in dataset: {', '.join(df['LABEL'].unique())}")
    
    # Split features and target
    X = df[['THUMB', 'INDEX', 'MIDDLE', 'RING', 'LITTLE', 'G1', 'G2', 'G3']]
    y = df['LABEL']
    feature_names = X.columns.tolist()
    
    # Normalize/standardize the features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    
    # Split into training and test sets
    X_train, X_test, y_train, y_test = train_test_split(
        X_scaled, y, test_size=test_size, random_state=random_state, stratify=y
    )
    
    print(f"Training set size: {X_train.shape[0]} samples")
    print(f"Test set size: {X_test.shape[0]} samples")
    
    # Define the model with hyperparameter search
    param_grid = {
        'n_estimators': [100, 200],
        'max_depth': [None, 10, 20],
        'min_samples_split': [2, 5],
        'min_samples_leaf': [1, 2]
    }
    
    # If dataset is large, you might want to use a simpler grid
    if len(df) > 1000:
        param_grid = {
            'n_estimators': [100],
            'max_depth': [None],
            'min_samples_split': [2],
            'min_samples_leaf': [1]
        }
    
    # Create the base model
    rf = RandomForestClassifier(random_state=random_state)
    
    # Search for best parameters
    print("Finding best hyperparameters...")
    grid_search = GridSearchCV(
        rf, param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1
    )
    grid_search.fit(X_train, y_train)
    
    # Get the best model
    best_model = grid_search.best_estimator_
    print(f"Best parameters: {grid_search.best_params_}")
    
    # Evaluate the model
    train_accuracy = best_model.score(X_train, y_train)
    test_accuracy = best_model.score(X_test, y_test)
    
    print(f"Training accuracy: {train_accuracy:.4f}")
    print(f"Test accuracy: {test_accuracy:.4f}")
    
    # Detailed evaluation on test set
    y_pred = best_model.predict(X_test)
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred))
    
    # Plot feature importance
    feature_importance = best_model.feature_importances_
    sorted_idx = np.argsort(feature_importance)
    
    plt.figure(figsize=(10, 6))
    plt.barh(
        range(len(sorted_idx)), 
        feature_importance[sorted_idx], 
        align='center'
    )
    plt.yticks(range(len(sorted_idx)), [feature_names[i] for i in sorted_idx])
    plt.title('Feature Importance')
    plt.tight_layout()
    plt.savefig('feature_importance.png')
    print("Feature importance plot saved as 'feature_importance.png'")
    
    # Plot confusion matrix
    plt.figure(figsize=(12, 10))
    cm = confusion_matrix(y_test, y_pred)
    sns.heatmap(
        cm, 
        annot=True, 
        fmt='d', 
        cmap='Blues', 
        xticklabels=best_model.classes_, 
        yticklabels=best_model.classes_
    )
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.title('Confusion Matrix')
    plt.tight_layout()
    plt.savefig('confusion_matrix.png')
    print("Confusion matrix plot saved as 'confusion_matrix.png'")
    
    return best_model, scaler, feature_names, train_accuracy, test_accuracy


def save_model(model, scaler, feature_names, model_path='asl_model.pkl', scaler_path='asl_scaler.pkl'):
    """
    Save the trained model, scaler, and feature names to disk
    
    Parameters:
    - model: Trained RandomForestClassifier
    - scaler: Fitted StandardScaler
    - feature_names: List of feature names used for training
    - model_path: Path to save the model
    - scaler_path: Path to save the scaler
    """
    # Save the model
    joblib.dump(model, model_path)
    
    # Save the scaler
    joblib.dump(scaler, scaler_path)
    
    # Save feature names
    with open('feature_names.txt', 'w') as f:
        for feature in feature_names:
            f.write(f"{feature}\n")
    
    print(f"Model saved to {model_path}")
    print(f"Scaler saved to {scaler_path}")
    print("Feature names saved to feature_names.txt")


# Part 2: Real-time prediction using the trained model
def load_model(model_path='asl_model.pkl', scaler_path='asl_scaler.pkl'):
    """
    Load the trained model and scaler from disk
    
    Parameters:
    - model_path: Path to the saved model
    - scaler_path: Path to the saved scaler
    
    Returns:
    - model: Loaded RandomForestClassifier
    - scaler: Loaded StandardScaler
    """
    # Load the model
    model = joblib.load(model_path)
    
    # Load the scaler
    scaler = joblib.load(scaler_path)
    
    print(f"Model loaded from {model_path}")
    print(f"Scaler loaded from {scaler_path}")
    
    return model, scaler


def predict_sign_from_csv(model, scaler, csv_path):
    """
    Make predictions on data from a CSV file
    
    Parameters:
    - model: Trained RandomForestClassifier
    - scaler: Fitted StandardScaler
    - csv_path: Path to the CSV file with new data
    
    Returns:
    - predictions: List of predicted signs
    - probabilities: List of prediction probabilities
    """
    # Load the data
    df = pd.read_csv(csv_path)
    
    # Make sure all required features are present
    required_features = ['THUMB', 'INDEX', 'MIDDLE', 'RING', 'LITTLE', 'G1', 'G2', 'G3']
    if not all(feature in df.columns for feature in required_features):
        missing = [f for f in required_features if f not in df.columns]
        raise ValueError(f"Missing required features in CSV: {missing}")
    
    # Extract features
    X = df[required_features]
    
    # Scale the features
    X_scaled = scaler.transform(X)
    
    # Make predictions
    predictions = model.predict(X_scaled)
    probabilities = model.predict_proba(X_scaled)
    
    # Get the confidence (probability of predicted class)
    confidence = np.max(probabilities, axis=1)
    
    return predictions, confidence


def predict_sign_from_serial(model, scaler, port='COM5', baudrate=9600, timeout=1):
    """
    Predict ASL signs from real-time serial data (from Arduino)
    
    Parameters:
    - model: Trained RandomForestClassifier
    - scaler: Fitted StandardScaler
    - port: Serial port to read from
    - baudrate: Baud rate for serial communication
    - timeout: Serial timeout
    
    This function will continuously read data from the serial port
    and make predictions until interrupted.
    """
    try:
        # Set up serial connection
        ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud")
        print("Waiting for data... (Press Ctrl+C to stop)")
        
        # Smoothing window for predictions
        recent_predictions = []
        window_size = 5
        
        while True:
            # Read a line from serial
            line = ser.readline().decode('ascii').strip()
            
            # If we got data
            if line:
                try:
                    # Parse the values
                    values = line.split(',')
                    if len(values) == 8:  # Make sure we have all 8 values
                        # Convert to float
                        sensor_data = [float(val) for val in values]
                        
                        # Reshape for prediction
                        sensor_data = np.array(sensor_data).reshape(1, -1)
                        
                        # Scale the data
                        sensor_data_scaled = scaler.transform(sensor_data)
                        
                        # Make prediction
                        prediction = model.predict(sensor_data_scaled)[0]
                        probabilities = model.predict_proba(sensor_data_scaled)[0]
                        confidence = np.max(probabilities) * 100
                        
                        # Add to recent predictions for smoothing
                        recent_predictions.append(prediction)
                        if len(recent_predictions) > window_size:
                            recent_predictions.pop(0)
                        
                        # Get the most common prediction in the window
                        from collections import Counter
                        smoothed_prediction = Counter(recent_predictions).most_common(1)[0][0]
                        
                        # Print the prediction
                        print(f"Raw values: {values}")
                        print(f"Predicted sign: {prediction} (Confidence: {confidence:.1f}%)")
                        print(f"Smoothed prediction: {smoothed_prediction}")
                        print("-" * 50)
                    else:
                        print(f"Invalid data format. Expected 8 values, got {len(values)}: {line}")
                except Exception as e:
                    print(f"Error processing data: {e}")
            
            # Small delay to prevent CPU hogging
            time.sleep(0.1)
    
    except KeyboardInterrupt:
        print("\nStopped by user")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Close the serial connection if it was opened
        if 'ser' in locals():
            ser.close()
            print("Serial connection closed")


# Example usage
if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='ASL Sign Recognition using Random Forest')
    parser.add_argument('--mode', choices=['train', 'predict-csv', 'predict-serial'], required=True,
                        help='Mode of operation: train a new model, predict from CSV, or predict from serial')
    parser.add_argument('--csv', help='Path to the CSV file with training or prediction data')
    parser.add_argument('--port', default='COM5', help='Serial port for real-time prediction (default: COM5)')
    parser.add_argument('--model', default='asl_model.pkl', help='Path to the model file (default: asl_model.pkl)')
    parser.add_argument('--scaler', default='asl_scaler.pkl', help='Path to the scaler file (default: asl_scaler.pkl)')
    
    args = parser.parse_args()
    
    if args.mode == 'train':
        if not args.csv:
            parser.error("--csv is required for training mode")
        
        # Train the model
        model, scaler, feature_names, train_accuracy, test_accuracy = train_model(args.csv)
        
        # Save the model and scaler
        save_model(model, scaler, feature_names, args.model, args.scaler)
    
    elif args.mode == 'predict-csv':
        if not args.csv:
            parser.error("--csv is required for predict-csv mode")
        
        # Load the model and scaler
        model, scaler = load_model(args.model, args.scaler)
        
        # Make predictions
        predictions, confidence = predict_sign_from_csv(model, scaler, args.csv)
        
        # Print predictions
        print("\nPredictions:")
        for i, (pred, conf) in enumerate(zip(predictions, confidence)):
            print(f"Sample {i+1}: {pred} (Confidence: {conf*100:.1f}%)")
    
    elif args.mode == 'predict-serial':
        # Load the model and scaler
        model, scaler = load_model(args.model, args.scaler)
        
        # Make predictions from serial data
        predict_sign_from_serial(model, scaler, port=args.port)