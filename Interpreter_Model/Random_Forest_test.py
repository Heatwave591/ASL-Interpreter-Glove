import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, confusion_matrix, accuracy_score
import joblib
import time
import os

def get_dataset_paths():
    """
    Get the paths to the local dataset files.
    
    Returns:
    - train_path: Path to the training data file
    - test_path: Path to the test data file
    """
    # Path to the dataset
    dataset_folder = "Dataset"
    
    # Check if the dataset folder exists
    if not os.path.exists(dataset_folder):
        raise FileNotFoundError(f"Dataset folder '{dataset_folder}' not found. Please ensure it exists in the current directory.")
    
    # Paths to the data files
    train_path = os.path.join(dataset_folder, "train.csv")
    test_path = os.path.join(dataset_folder, "test.csv")
    
    # Check if the files exist
    if not os.path.exists(train_path):
        raise FileNotFoundError(f"Training file '{train_path}' not found.")
    if not os.path.exists(test_path):
        raise FileNotFoundError(f"Test file '{test_path}' not found.")
    
    print(f"Found dataset files in '{dataset_folder}' folder.")
    
    return train_path, test_path

def load_data(train_path, test_path):
    """
    Load the Human Activity Recognition dataset from CSV files.
    
    Parameters:
    - train_path: Path to the training data CSV file
    - test_path: Path to the test data CSV file
    
    Returns:
    - X_train: Training features
    - X_test: Test features
    - y_train: Training labels
    - y_test: Test labels
    - feature_names: Names of the features
    - activity_labels: Mapping of activity IDs to names
    """
    print("Loading dataset...")
    start_time = time.time()
    
    # Load training data
    train_df = pd.read_csv(train_path)
    
    # Load test data
    test_df = pd.read_csv(test_path)
    
    # Assuming the last column is the activity label
    # Extract features and labels
    X_train = train_df.iloc[:, :-1]
    y_train = train_df.iloc[:, -1:].rename(columns={train_df.columns[-1]: 'activity'})
    
    X_test = test_df.iloc[:, :-1]
    y_test = test_df.iloc[:, -1:].rename(columns={test_df.columns[-1]: 'activity'})
    
    # Get feature names
    feature_names = X_train.columns.tolist()
    
    # Create a mapping for activity labels
    # Assuming activities are labeled 1-6 for WALKING, WALKING_UPSTAIRS, etc.
    activity_mapping = {
        1: 'WALKING',
        2: 'WALKING_UPSTAIRS',
        3: 'WALKING_DOWNSTAIRS',
        4: 'SITTING',
        5: 'STANDING',
        6: 'LAYING'
    }
    
    activity_labels = pd.DataFrame({
        'index': list(activity_mapping.keys()),
        'activity_name': list(activity_mapping.values())
    })
    
    print(f"Dataset loaded in {time.time() - start_time:.2f} seconds")
    
    # Print dataset info
    print(f"Training set shape: {X_train.shape}")
    print(f"Test set shape: {X_test.shape}")
    print(f"Number of features: {len(feature_names)}")
    print(f"Number of activity classes: {len(activity_labels)}")
    
    return X_train, X_test, y_train, y_test, feature_names, activity_labels

def preprocess_data(X_train, X_test):
    """
    Preprocess the data: scaling and potentially feature selection.
    
    Parameters:
    - X_train: Training data
    - X_test: Test data
    
    Returns:
    - X_train_scaled: Scaled training data
    - X_test_scaled: Scaled test data
    - scaler: Fitted StandardScaler
    """
    print("Preprocessing data...")
    
    # Standardize the data
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    return X_train_scaled, X_test_scaled, scaler

def train_random_forest(X_train, y_train, quick_mode=False):
    """
    Train a Random Forest classifier with hyperparameter tuning.
    
    Parameters:
    - X_train: Training data
    - y_train: Training labels
    - quick_mode: If True, use a simpler grid search for faster execution
    
    Returns:
    - best_model: The best model from the grid search
    """
    print("Training Random Forest model...")
    start_time = time.time()
    
    # Define the parameter grid for GridSearchCV
    if quick_mode:
        param_grid = {
            'n_estimators': [100],
            'max_depth': [None, 20],
            'min_samples_split': [2],
            'min_samples_leaf': [1]
        }
    else:
        param_grid = {
            'n_estimators': [100, 200, 500],
            'max_depth': [None, 10, 20, 30],
            'min_samples_split': [2, 5, 10],
            'min_samples_leaf': [1, 2, 4]
        }
    
    # Create the base model
    rf = RandomForestClassifier(random_state=42)
    
    # Perform grid search
    grid_search = GridSearchCV(
        rf, param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1
    )
    grid_search.fit(X_train, y_train.values.ravel())
    
    # Get the best model
    best_model = grid_search.best_estimator_
    print(f"Best parameters: {grid_search.best_params_}")
    print(f"Training completed in {time.time() - start_time:.2f} seconds")
    
    return best_model

def evaluate_model(model, X_train, y_train, X_test, y_test, feature_names, activity_labels):
    """
    Evaluate the trained model.
    
    Parameters:
    - model: Trained Random Forest model
    - X_train: Training data
    - y_train: Training labels
    - X_test: Test data
    - y_test: Test labels
    - feature_names: Names of the features
    - activity_labels: Activity class names
    
    Returns:
    - train_accuracy: Training accuracy
    - test_accuracy: Test accuracy
    """
    print("Evaluating model...")
    
    # Training accuracy
    train_pred = model.predict(X_train)
    train_accuracy = accuracy_score(y_train, train_pred)
    print(f"Training accuracy: {train_accuracy:.4f}")
    
    # Test accuracy
    test_pred = model.predict(X_test)
    test_accuracy = accuracy_score(y_test, test_pred)
    print(f"Test accuracy: {test_accuracy:.4f}")
    
    # Detailed classification report
    print("\nClassification Report:")
    
    # Map numeric labels to activity names for readability
    activity_mapping = dict(zip(activity_labels['index'], activity_labels['activity_name']))
    y_test_labels = y_test['activity'].map(activity_mapping)
    test_pred_labels = pd.Series(test_pred).map(activity_mapping)
    
    print(classification_report(y_test_labels, test_pred_labels))
    
    # Confusion matrix
    plt.figure(figsize=(12, 10))
    cm = confusion_matrix(y_test_labels, test_pred_labels)
    
    # Plot confusion matrix as a heatmap
    sns.heatmap(
        cm, 
        annot=True, 
        fmt='d', 
        cmap='Blues', 
        xticklabels=activity_labels['activity_name'], 
        yticklabels=activity_labels['activity_name']
    )
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.title('Confusion Matrix')
    plt.tight_layout()
    plt.savefig('har_confusion_matrix.png')
    print("Confusion matrix saved as 'har_confusion_matrix.png'")
    
    # Feature importance
    feature_importance = model.feature_importances_
    
    # Get the top 30 features (since there are 561 features)
    indices = np.argsort(feature_importance)[-30:]
    
    plt.figure(figsize=(12, 8))
    plt.barh(range(len(indices)), feature_importance[indices], align='center')
    plt.yticks(range(len(indices)), [feature_names[i] for i in indices])
    plt.title('Top 30 Feature Importance')
    plt.tight_layout()
    plt.savefig('har_feature_importance.png')
    print("Feature importance plot saved as 'har_feature_importance.png'")
    
    return train_accuracy, test_accuracy

def save_model(model, scaler, model_path='har_model.pkl', scaler_path='har_scaler.pkl'):
    """
    Save the trained model and scaler to disk.
    
    Parameters:
    - model: Trained model
    - scaler: Fitted scaler
    - model_path: Path to save the model
    - scaler_path: Path to save the scaler
    """
    print("SAVE MODEL FLAG!!!")
    joblib.dump(model, model_path)
    joblib.dump(scaler, scaler_path)
    print(f"Model saved to {model_path}")
    print(f"Scaler saved to {scaler_path}")

def predict_activity(model, scaler, data):
    """
    Predict activity from new data.
    
    Parameters:
    - model: Trained model
    - scaler: Fitted scaler
    - data: New data to predict
    
    Returns:
    - predicted_activity: Predicted activity label
    - confidence: Prediction confidence (probability)
    """
    # Scale the data
    data_scaled = scaler.transform(data)
    
    # Predict
    prediction = model.predict(data_scaled)[0]
    probabilities = model.predict_proba(data_scaled)[0]
    confidence = np.max(probabilities) * 100
    
    return prediction, confidence

def main():
    """Main function to run the entire pipeline."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Human Activity Recognition using Random Forest')
    parser.add_argument('--mode', choices=['train', 'predict'], default='train',
                        help='Mode of operation: train a new model or predict (default: train)')
    parser.add_argument('--quick', action='store_true',
                        help='Use a smaller grid search for faster training')
    
    args = parser.parse_args()
    
    if args.mode == 'train':
        # Get dataset paths
        train_path, test_path = get_dataset_paths()
        
        # Load the data
        X_train, X_test, y_train, y_test, feature_names, activity_labels = load_data(
            train_path, test_path
        )
        
        # Preprocess the data
        X_train_scaled, X_test_scaled, scaler = preprocess_data(X_train, X_test)
        
        # Train the model
        model = train_random_forest(X_train_scaled, y_train, quick_mode=args.quick)
        
        # Evaluate the model
        train_accuracy, test_accuracy = evaluate_model(
            model, X_train_scaled, y_train, X_test_scaled, y_test, feature_names, activity_labels
        )
        
        # Save the model
        save_model(model, scaler)
    
    elif args.mode == 'predict':
        # In a real-world scenario, you would load the model and make predictions
        # on new data. For demonstration purposes, we'll predict on the test set.
        print("Loading model and data for prediction...")
        
        # Check if model exists
        if not os.path.exists('har_model.pkl') or not os.path.exists('har_scaler.pkl'):
            print("Error: Model files not found. Please train the model first.")
            return
        
        # Load the model and scaler
        model = joblib.load('har_model.pkl')
        scaler = joblib.load('har_scaler.pkl')
        
        # Get dataset paths
        _, test_path = get_dataset_paths()
        
        # Load just the test data
        _, X_test, _, y_test, feature_names, activity_labels = load_data(
            test_path, test_path  # We only need test data, but function expects both paths
        )
        
        # Make predictions on a few examples
        print("\nPredicting on sample test data:")
        activity_mapping = dict(zip(activity_labels['index'], activity_labels['activity_name']))
        
        for i in range(min(5, len(X_test))):
            sample = X_test.iloc[i:i+1]
            true_label = y_test.iloc[i]['activity']
            true_activity = activity_mapping[true_label]
            
            prediction, confidence = predict_activity(model, scaler, sample)
            predicted_activity = activity_mapping[prediction]
            
            print(f"Sample {i+1}:")
            print(f"  True activity: {true_activity}")
            print(f"  Predicted activity: {predicted_activity}")
            print(f"  Confidence: {confidence:.2f}%")
            print()

if __name__ == "__main__":
    main()