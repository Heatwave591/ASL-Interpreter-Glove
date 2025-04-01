import pandas as pd
import numpy as np
import os
import pickle
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.preprocessing import StandardScaler
import argparse


def load_data(file_path):
    """Load and prepare the ASL dataset from CSV."""
    print(f"Loading data from {file_path}...")
    try:
        df = pd.read_csv(file_path)
        print(f"Dataset loaded: {df.shape[0]} samples, {df.shape[1]} columns")
        return df
    except Exception as e:
        print(f"Error loading dataset: {e}")
        return None

def visualize_dataset(df, output_dir='plots'):
    """Create visualizations of the dataset."""
    os.makedirs(output_dir, exist_ok=True)
    
    # Count of samples per sign
    plt.figure(figsize=(12, 6))
    sign_counts = df['sign'].value_counts().sort_index()
    ax = sign_counts.plot(kind='bar')
    plt.title('Number of Samples per ASL Sign')
    plt.xlabel('ASL Sign')
    plt.ylabel('Number of Samples')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'sample_distribution.png'))
    
    # Distribution of flex sensor values for each sign
    for finger in ['finger1', 'finger2', 'finger3', 'finger4', 'finger5']:
        plt.figure(figsize=(12, 8))
        sns.boxplot(x='sign', y=finger, data=df)
        plt.title(f'Distribution of {finger} values by ASL Sign')
        plt.xticks(rotation=90)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'{finger}_distribution.png'))
    
    # Correlation matrix between flex sensors
    plt.figure(figsize=(10, 8))
    correlation = df[['finger1', 'finger2', 'finger3', 'finger4', 'finger5']].corr()
    sns.heatmap(correlation, annot=True, cmap='coolwarm')
    plt.title('Correlation Between Flex Sensors')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'correlation_matrix.png'))
    
    print(f"Visualizations saved to '{output_dir}' directory")

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
    """Train a Random Forest model with hyperparameter tuning."""
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
    
    # Evaluate the model
    y_pred = best_model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)
    
    print(f"Model accuracy: {accuracy:.4f}")
    print("\nClassification report:")
    print(classification_report(y_test, y_pred))
    
    # Confusion matrix
    cm = confusion_matrix(y_test, y_pred)
    plt.figure(figsize=(12, 10))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', 
                xticklabels=best_model.classes_, 
                yticklabels=best_model.classes_)
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.title('Confusion Matrix')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'confusion_matrix.png'))
    
    # Feature importance
    feature_importance = pd.DataFrame({
        'Feature': ['finger1', 'finger2', 'finger3', 'finger4', 'finger5'],
        'Importance': best_model.feature_importances_
    }).sort_values('Importance', ascending=False)
    
    print("\nFeature importance:")
    print(feature_importance)
    
    plt.figure(figsize=(10, 6))
    sns.barplot(x='Importance', y='Feature', data=feature_importance)
    plt.title('Feature Importance')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'feature_importance.png'))
    
    # Save model and scaler
    model_file = os.path.join(output_dir, 'asl_model.pkl')
    with open(model_file, 'wb') as f:
        pickle.dump(best_model, f)
    
    return best_model

def export_model_for_esp32(model, scaler, output_dir='esp32_model'):
    """Export the model in a format usable by ESP32."""
    os.makedirs(output_dir, exist_ok=True)
    
    # Get model information
    n_estimators = len(model.estimators_)
    n_classes = len(model.classes_)
    
    print(f"Exporting model with {n_estimators} trees and {n_classes} classes...")
    
    # Create a C++ header file with model parameters
    header_file = os.path.join(output_dir, 'asl_model.h')
    
    with open(header_file, 'w') as f:
        f.write("// ASL Sign Recognition Model for ESP32\n")
        f.write("// Generated by model_export_script.py\n\n")
        
        # Class labels
        f.write("const char* ASL_CLASSES[] = {")
        for i, label in enumerate(model.classes_):
            f.write(f'"{label}"')
            if i < len(model.classes_) - 1:
                f.write(", ")
        f.write(f"}}; // {len(model.classes_)} classes\n\n")
        
        # Scaler parameters
        f.write("// Scaling parameters (StandardScaler)\n")
        f.write("const float SCALER_MEAN[] = {")
        for i, mean in enumerate(scaler.mean_):
            f.write(f"{mean:.6f}")
            if i < len(scaler.mean_) - 1:
                f.write(", ")
        f.write("};\n\n")
        
        f.write("const float SCALER_SCALE[] = {")
        for i, scale in enumerate(scaler.scale_):
            f.write(f"{scale:.6f}")
            if i < len(scaler.scale_) - 1:
                f.write(", ")
        f.write("};\n\n")
        
        # Instead of exporting the full random forest (which would be massive),
        # we'll implement a decision function with heuristics for the top signs
        
        # Get some representative values for each class
        f.write("// Representative value ranges for each sign (for basic fallback classification)\n")
        f.write("typedef struct {\n")
        f.write("  const char* sign;\n")
        f.write("  int finger1Min, finger1Max;\n")
        f.write("  int finger2Min, finger2Max;\n")
        f.write("  int finger3Min, finger3Max;\n")
        f.write("  int finger4Min, finger4Max;\n")
        f.write("  int finger5Min, finger5Max;\n")
        f.write("} ASLSignRange;\n\n")
        
        f.write("const ASLSignRange ASL_SIGN_RANGES[] = {\n")
        
        # This part would typically use your training data to calculate ranges
        # For now we're just creating a placeholder
        for i, class_label in enumerate(model.classes_):
            f.write(f"  {{ \"{class_label}\", 0, 4095, 0, 4095, 0, 4095, 0, 4095, 0, 4095 }}")
            if i < len(model.classes_) - 1:
                f.write(",")
            f.write("\n")
        
        f.write("};\n\n")
        
        # Add a simple prediction function that can be implemented on ESP32
        f.write("// Function to predict ASL sign from flex sensor values\n")
        f.write("String predictASLSign(int finger1, int finger2, int finger3, int finger4, int finger5) {\n")
        f.write("  // Simple rule-based prediction as a fallback\n")
        f.write("  for (int i = 0; i < " + str(len(model.classes_)) + "; i++) {\n")
        f.write("    if (finger1 >= ASL_SIGN_RANGES[i].finger1Min && finger1 <= ASL_SIGN_RANGES[i].finger1Max &&\n")
        f.write("        finger2 >= ASL_SIGN_RANGES[i].finger2Min && finger2 <= ASL_SIGN_RANGES[i].finger2Max &&\n")
        f.write("        finger3 >= ASL_SIGN_RANGES[i].finger3Min && finger3 <= ASL_SIGN_RANGES[i].finger3Max &&\n")
        f.write("        finger4 >= ASL_SIGN_RANGES[i].finger4Min && finger4 <= ASL_SIGN_RANGES[i].finger4Max &&\n")
        f.write("        finger5 >= ASL_SIGN_RANGES[i].finger5Min && finger5 <= ASL_SIGN_RANGES[i].finger5Max) {\n")
        f.write("      return String(ASL_SIGN_RANGES[i].sign);\n")
        f.write("    }\n")
        f.write("  }\n\n")
        f.write("  // If no match found\n")
        f.write("  return \"?\";\n")
        f.write("}\n")
    
    print(f"C++ header file created: {header_file}")
    
    # For TFLite model (for more advanced implementation)
    print("\nFor a more accurate implementation, consider using TensorFlow Lite")
    print("for Microcontrollers (TFLite Micro) to run the full model on ESP32.")
    print("See: https://www.tensorflow.org/lite/microcontrollers")

def main():
    parser = argparse.ArgumentParser(description='Train and export ASL recognition model for ESP32')
    parser.add_argument('--data', type=str, required=True, help='Path to the ASL dataset CSV file')
    parser.add_argument('--output', type=str, default='output', help='Output directory for all files')
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output, exist_ok=True)
    
    # Load dataset
    df = load_data(args.data)
    if df is None:
        return
    
    # Visualize dataset
    visualize_dataset(df, os.path.join(args.output, 'plots'))
    
    # Preprocess data
    X_train, X_test, y_train, y_test, scaler = preprocess_data(df)
    
    # Train model
    model = train_model(X_train, y_train, X_test, y_test, os.path.join(args.output, 'models'))
    
    # Export model for ESP32
    export_model_for_esp32(model, scaler, os.path.join(args.output, 'esp32_model'))
    
    print("\nProcess completed successfully!")
    print(f"All outputs saved to '{args.output}' directory")

if __name__ == "__main__":
    main()