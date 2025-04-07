import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
import pickle
import matplotlib.pyplot as plt
import seaborn as sns

# Load the dataset
# Replace 'asl_data.csv' with your actual dataset filename
# Expected format: 5 columns for flex sensor values and 1 column for the sign label
def load_and_prepare_data(filename):
    try:
        data = pd.read_csv(filename)
        print(f"Successfully loaded dataset with {data.shape[0]} samples and {data.shape[1]} columns")
        print("First few rows:")
        print(data.head())
        return data
    except Exception as e:
        print(f"Error loading dataset: {e}")
        # Create a sample dataset as fallback
        print("Creating a sample dataset for demonstration...")
        # This is just a placeholder - you'll need your real data
        labels = ['A', 'B', 'C', 'D', 'E']
        samples_per_label = 20
        
        data = []
        for label in labels:
            for _ in range(samples_per_label):
                # Generate random flex sensor values (0-4095, typical ADC range for ESP32)
                flex_values = np.random.randint(0, 4095, size=5)
                data.append(list(flex_values) + [label])
        
        df = pd.DataFrame(data, columns=['finger1', 'finger2', 'finger3', 'finger4', 'finger5', 'sign'])
        print("Sample dataset created:")
        print(df.head())
        return df

# Visualize the dataset
def visualize_data(data):
    # Plot distribution of each flex sensor for each sign
    plt.figure(figsize=(15, 10))
    for i, sensor in enumerate(['finger1', 'finger2', 'finger3', 'finger4', 'finger5']):
        plt.subplot(2, 3, i+1)
        sns.boxplot(x='sign', y=sensor, data=data)
        plt.title(f'Distribution of {sensor} values by sign')
    
    plt.tight_layout()
    plt.savefig('sensor_distributions.png')
    print("Saved sensor distribution visualization to 'sensor_distributions.png'")
    
    # Correlation matrix
    plt.figure(figsize=(10, 8))
    correlation = data.drop('sign', axis=1).corr()
    sns.heatmap(correlation, annot=True, cmap='coolwarm')
    plt.title('Correlation Matrix of Flex Sensors')
    plt.savefig('correlation_matrix.png')
    print("Saved correlation matrix to 'correlation_matrix.png'")

# Train the random forest model
def train_model(data):
    # Split features and target
    X = data.drop('sign', axis=1)
    y = data['sign']
    
    # Split data into training and testing sets
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    print(f"Training on {X_train.shape[0]} samples, testing on {X_test.shape[0]} samples")
    
    # Initialize and train the Random Forest model
    model = RandomForestClassifier(n_estimators=100, random_state=42)
    model.fit(X_train, y_train)
    
    # Evaluate the model
    y_pred = model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)
    
    print(f"Model accuracy: {accuracy:.4f}")
    print("\nClassification report:")
    print(classification_report(y_test, y_pred))
    
    # Confusion matrix
    cm = confusion_matrix(y_test, y_pred)
    plt.figure(figsize=(10, 8))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=model.classes_, yticklabels=model.classes_)
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.title('Confusion Matrix')
    plt.savefig('confusion_matrix.png')
    print("Saved confusion matrix to 'confusion_matrix.png'")
    
    # Feature importance
    feature_importance = pd.DataFrame({
        'Feature': X.columns,
        'Importance': model.feature_importances_
    }).sort_values('Importance', ascending=False)
    
    print("\nFeature importance:")
    print(feature_importance)
    
    plt.figure(figsize=(10, 6))
    sns.barplot(x='Importance', y='Feature', data=feature_importance)
    plt.title('Feature Importance')
    plt.savefig('feature_importance.png')
    print("Saved feature importance plot to 'feature_importance.png'")
    
    return model

# Export the model for ESP32 integration
def export_model(model, filename='asl_model.pkl'):
    # Save the full model
    with open(filename, 'wb') as file:
        pickle.dump(model, file)
    
    # Extract decision paths for a more embedded-friendly format
    # This is a simplified version - actual ESP32 integration will need more work
    n_trees = len(model.estimators_)
    n_classes = len(model.classes_)
    
    print(f"\nExported random forest model with {n_trees} trees and {n_classes} classes")
    print(f"Model saved to '{filename}'")
    
    # Generate a simplified C/C++ representation for ESP32
    # This is just a skeleton - you'll need to adapt it further
    with open('asl_model_esp32.h', 'w') as file:
        file.write(f"// ASL Recognition Model for ESP32\n")
        file.write(f"// Number of trees: {n_trees}\n")
        file.write(f"// Number of classes: {n_classes}\n\n")
        
        # Class labels
        file.write("const char* CLASS_LABELS[] = {")
        for i, label in enumerate(model.classes_):
            file.write(f'"{label}"')
            if i < len(model.classes_) - 1:
                file.write(", ")
        file.write("};\n\n")
        
        # Feature importances
        file.write("const float FEATURE_IMPORTANCES[] = {")
        for i, importance in enumerate(model.feature_importances_):
            file.write(f"{importance:.6f}")
            if i < len(model.feature_importances_) - 1:
                file.write(", ")
        file.write("};\n\n")
        
        # Note: A complete implementation would include the decision trees themselves
        file.write("// Function to predict ASL sign from flex sensor values\n")
        file.write("String predictASLSign(int finger1, int finger2, int finger3, int finger4, int finger5) {\n")
        file.write("  // Replace this with actual model inference code\n")
        file.write("  // This is just a placeholder\n")
        file.write("  // For full implementation, you can use TensorFlow Lite for Microcontrollers or similar\n")
        file.write("  return \"Placeholder\";\n")
        file.write("}\n")
    
    print(f"Generated ESP32-compatible header file 'asl_model_esp32.h'")

# Main function
def main():
    print("ASL Hand Sign Recognition - Random Forest Training")
    print("=" * 50)
    
    data = load_and_prepare_data('asl_data.csv')
    visualize_data(data)
    model = train_model(data)
    export_model(model)
    
    print("\nModel training complete!")
    print("Next steps:")
    print("1. Integrate the generated model with your ESP32 code")
    print("2. Test recognition accuracy with live sensor data")
    print("3. Fine-tune the model if needed")

if __name__ == "__main__":
    main()