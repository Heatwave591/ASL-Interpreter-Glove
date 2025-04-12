import pandas as pd
import numpy as np
import os
import pickle
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.preprocessing import StandardScaler
import argparse


def load_data(file_path):
    """Load and prepare the ASL dataset from CSV without header."""
    print(f"Loading data from {file_path}...")
    try:
        # Add header names since the file doesn't have them
        # Assuming columns are in order: sign, finger1, finger2, finger3, finger4, finger5
        df = pd.read_csv(file_path, header=None, 
                         names=['sign', 'finger1', 'finger2', 'finger3', 'finger4', 'finger5'])
        
        print(f"Dataset loaded: {df.shape[0]} samples, {df.shape[1]} columns")
        return df
    except Exception as e:
        print(f"Error loading dataset: {e}")
        return None


def preprocess_data(df):
    """Preprocess the dataset for training."""
    # Select features and target
    X = df[['finger1', 'finger2', 'finger3', 'finger4', 'finger5']]
    y = df['sign']
    
    # Split the data
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    
    # Scale the features
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    print(f"Training set: {X_train.shape[0]} samples")
    print(f"Testing set: {X_test.shape[0]} samples")
    
    return X_train_scaled, X_test_scaled, y_train, y_test, scaler


def train_model(X_train, y_train, X_test, y_test, output_dir='models'):
    """Train a Random Forest model with hyperparameter tuning and save only the model file."""
    os.makedirs(output_dir, exist_ok=True)
    
    print("Training Random Forest model...")
    
    # Define parameter grid for hyperparameter tuning
    param_grid = {
        'n_estimators': [50, 100, 200],
        'max_depth': [None, 10, 20, 30],
        'min_samples_split': [2, 5, 10],
        'min_samples_leaf': [1, 2, 4]
    }
    
    # Create and train the model
    rf = RandomForestClassifier(random_state=42)
    
    # Use GridSearchCV for hyperparameter tuning
    print("Performing hyperparameter tuning...")
    grid_search = GridSearchCV(
        estimator=rf,
        param_grid=param_grid,
        cv=5,
        n_jobs=-1,
        verbose=1
    )
    grid_search.fit(X_train, y_train)
    
    # Get the best model
    best_model = grid_search.best_estimator_
    print(f"Best parameters: {grid_search.best_params_}")
    
    # Evaluate the model and print results to console
    y_pred = best_model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)
    
    print(f"Model accuracy: {accuracy:.4f}")
    print("\nClassification report:")
    print(classification_report(y_test, y_pred))
    
    # Show feature importance in console
    feature_importance = pd.DataFrame({
        'Feature': ['finger1', 'finger2', 'finger3', 'finger4', 'finger5'],
        'Importance': best_model.feature_importances_
    }).sort_values('Importance', ascending=False)
    
    print("\nFeature importance:")
    print(feature_importance)
    
    # Save only the model file
    model_file = os.path.join(output_dir, 'asl_model.pkl')
    with open(model_file, 'wb') as f:
        pickle.dump(best_model, f)
    
    print(f"Model saved to {model_file}")
    
    return best_model

def main():
    parser = argparse.ArgumentParser(description='Train and export ASL recognition model for ESP32')
    parser.add_argument('--data', type=str, required=True, help='Path to the ASL dataset CSV file')
    parser.add_argument('--output', type=str, default='output', help='Output directory for model file')
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output, exist_ok=True)
    
    # Load dataset
    df = load_data(args.data)
    if df is None:
        return
    
    # Preprocess data
    X_train, X_test, y_train, y_test, scaler = preprocess_data(df)
    
    # Train model and save only the model file
    model = train_model(X_train, y_train, X_test, y_test, args.output)
    
    print("\nProcess completed successfully!")
    print(f"Model saved to '{os.path.join(args.output, 'asl_model.pkl')}'")


if __name__ == "__main__":
    main()