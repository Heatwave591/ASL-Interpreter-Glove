import requests
import pandas as pd
import time
import os
import tkinter as tk
from tkinter import ttk, messagebox
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class ASLDataCollector:
    def __init__(self, root, esp32_ip="192.168.1.100"):
        self.root = root
        self.root.title("ASL Training Data Collector")
        self.root.geometry("800x600")
        self.esp32_ip = esp32_ip
        self.esp32_url = f"http://{esp32_ip}/data"
        self.data = []
        self.collecting = False
        self.current_sign = ""
        self.samples_collected = 0
        
        # Define ASL signs to collect data for
        self.asl_signs = [
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
            "SPACE", "DELETE"
        ]
        
        self.setup_ui()
        
        # Create a dataframe to store collected data
        self.df = pd.DataFrame(columns=["finger1", "finger2", "finger3", "finger4", "finger5", "sign"])
        
        # Load existing data if available
        self.data_file = "asl_training_data.csv"
        if os.path.exists(self.data_file):
            self.df = pd.read_csv(self.data_file)
            self.update_status(f"Loaded {len(self.df)} existing samples")
        
        # Setup real-time plotting
        self.setup_plot()
        
        # Start sensor data polling
        self.sensor_thread = threading.Thread(target=self.poll_sensor_data, daemon=True)
        self.sensor_thread.start()
    
    def setup_ui(self):
        # Create main frame
        main_frame = ttk.Frame(self.root, padding=10)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Connection settings
        conn_frame = ttk.LabelFrame(main_frame, text="ESP32 Connection", padding=10)
        conn_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(conn_frame, text="ESP32 IP:").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.ip_entry = ttk.Entry(conn_frame, width=15)
        self.ip_entry.insert(0, self.esp32_ip)
        self.ip_entry.grid(row=0, column=1, padx=5, pady=5, sticky=tk.W)
        
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.connect_to_esp32)
        self.connect_btn.grid(row=0, column=2, padx=5, pady=5)
        
        self.status_var = tk.StringVar(value="Not connected")
        ttk.Label(conn_frame, textvariable=self.status_var).grid(row=0, column=3, padx=5, pady=5)
        
        # Data collection frame
        collect_frame = ttk.LabelFrame(main_frame, text="Data Collection", padding=10)
        collect_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(collect_frame, text="Select ASL Sign:").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.sign_combo = ttk.Combobox(collect_frame, values=self.asl_signs, width=10)
        self.sign_combo.current(0)
        self.sign_combo.grid(row=0, column=1, padx=5, pady=5, sticky=tk.W)
        
        ttk.Label(collect_frame, text="Samples to collect:").grid(row=0, column=2, padx=5, pady=5, sticky=tk.W)
        self.samples_entry = ttk.Entry(collect_frame, width=5)
        self.samples_entry.insert(0, "20")
        self.samples_entry.grid(row=0, column=3, padx=5, pady=5, sticky=tk.W)
        
        self.collect_btn = ttk.Button(collect_frame, text="Start Collection", command=self.toggle_collection)
        self.collect_btn.grid(row=0, column=4, padx=5, pady=5)
        
        self.progress = ttk.Progressbar(collect_frame, orient=tk.HORIZONTAL, length=300, mode='determinate')
        self.progress.grid(row=1, column=0, columnspan=5, padx=5, pady=5, sticky=tk.EW)
        
        # Dataset statistics frame
        stats_frame = ttk.LabelFrame(main_frame, text="Dataset Statistics", padding=10)
        stats_frame.pack(fill=tk.X, pady=5)
        
        self.stats_text = tk.Text(stats_frame, height=5, width=60)
        self.stats_text.pack(fill=tk.X, expand=True)
        self.update_stats()
        
        # Action buttons
        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(btn_frame, text="Save Dataset", command=self.save_dataset).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="Export for Training", command=self.export_for_training).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="View Data Distribution", command=self.show_distribution).pack(side=tk.LEFT, padx=5)
        
        # Current readings display area (will be replaced with a plot)
        self.readings_frame = ttk.LabelFrame(main_frame, text="Current Sensor Readings", padding=10)
        self.readings_frame.pack(fill=tk.BOTH, expand=True, pady=5)
    
    def setup_plot(self):
        # Create a figure for the plot
        self.fig, self.ax = plt.subplots(figsize=(8, 3))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.readings_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Initialize the plot
        self.sensor_names = ['Finger 1', 'Finger 2', 'Finger 3', 'Finger 4', 'Finger 5']
        self.bar_container = self.ax.bar(self.sensor_names, [0, 0, 0, 0, 0], color='skyblue')
        self.ax.set_ylim(0, 4095)  # ESP32 ADC range
        self.ax.set_title('Flex Sensor Readings')
        self.canvas.draw()
    
    def update_plot(self, values):
        # Update the bar heights with new sensor values
        for i, bar in enumerate(self.bar_container):
            bar.set_height(values[i])
        self.canvas.draw()
    
    def connect_to_esp32(self):
        self.esp32_ip = self.ip_entry.get()
        self.esp32_url = f"http://{self.esp32_ip}/data"
        self.update_status("Connecting...")
        
        try:
            response = requests.get(self.esp32_url, timeout=3)
            if response.status_code == 200:
                self.update_status("Connected successfully")
                return True
            else:
                self.update_status(f"Error: {response.status_code}")
                return False
        except Exception as e:
            self.update_status(f"Connection failed: {str(e)}")
            return False
    
    def update_status(self, message):
        self.status_var.set(message)
        self.root.update_idletasks()
    
    def poll_sensor_data(self):
        # Continuously poll sensor data from ESP32
        while True:
            try:
                response = requests.get(self.esp32_url, timeout=1)
                if response.status_code == 200:
                    data = response.json()
                    # Update plot with new sensor values
                    sensor_values = [data['f1'], data['f2'], data['f3'], data['f4'], data['f5']]
                    self.root.after(0, lambda: self.update_plot(sensor_values))
                    
                    # If collecting data, store the readings
                    if self.collecting:
                        self.data.append({
                            'finger1': data['f1'],
                            'finger2': data['f2'],
                            'finger3': data['f3'],
                            'finger4': data['f4'],
                            'finger5': data['f5'],
                            'sign': self.current_sign
                        })
                        
                        self.samples_collected += 1
                        max_samples = int(self.samples_entry.get())
                        self.progress['value'] = (self.samples_collected / max_samples) * 100
                        
                        if self.samples_collected >= max_samples:
                            self.root.after(0, self.stop_collection)
                    
            except Exception as e:
                # Silently fail - will retry on next iteration
                pass
            
            time.sleep(0.2)  # Poll at 5 Hz
    
    def toggle_collection(self):
        if not self.collecting:
            self.start_collection()
        else:
            self.stop_collection()
    
    def start_collection(self):
        self.current_sign = self.sign_combo.get()
        if not self.current_sign:
            messagebox.showerror("Error", "Please select an ASL sign")
            return
        
        self.collecting = True
        self.samples_collected = 0
        self.data = []
        self.progress['value'] = 0
        self.collect_btn.config(text="Stop Collection")
        self.update_status(f"Collecting data for sign: {self.current_sign}")
    
    def stop_collection(self):
        if not self.collecting:
            return
            
        self.collecting = False
        self.collect_btn.config(text="Start Collection")
        
        # Add collected data to dataframe
        if self.data:
            new_df = pd.DataFrame(self.data)
            self.df = pd.concat([self.df, new_df], ignore_index=True)
            self.update_stats()
            self.save_dataset()  # Auto-save after collection
            
        self.update_status(f"Added {len(self.data)} samples for sign {self.current_sign}")
    
    def update_stats(self):
        if len(self.df) == 0:
            self.stats_text.delete(1.0, tk.END)
            self.stats_text.insert(tk.END, "No data collected yet.")
            return
            
        # Calculate statistics
        total_samples = len(self.df)
        signs_count = self.df['sign'].value_counts()
        
        stats = f"Total samples: {total_samples}\n\n"
        stats += "Samples per sign:\n"
        
        for sign, count in signs_count.items():
            percent = (count / total_samples) * 100
            stats += f"{sign}: {count} ({percent:.1f}%)\n"
        
        self.stats_text.delete(1.0, tk.END)
        self.stats_text.insert(tk.END, stats)
    
    def save_dataset(self):
        if len(self.df) == 0:
            messagebox.showinfo("Info", "No data to save")
            return
            
        self.df.to_csv(self.data_file, index=False)
        self.update_status(f"Saved {len(self.df)} samples to {self.data_file}")
    
    def export_for_training(self):
        if len(self.df) == 0:
            messagebox.showinfo("Info", "No data to export")
            return
            
        # Export to timestamped file for training
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        export_file = f"asl_dataset_{timestamp}.csv"
        self.df.to_csv(export_file, index=False)
        messagebox.showinfo("Export Complete", f"Dataset exported to {export_file}")
    
    def show_distribution(self):
        if len(self.df) == 0:
            messagebox.showinfo("Info", "No data to visualize")
            return
            
        # Create a new window for the distribution plot
        plot_window = tk.Toplevel(self.root)
        plot_window.title("ASL Sign Distribution")
        plot_window.geometry("800x600")
        
        # Create plot
        fig, ax = plt.subplots(figsize=(10, 6))
        sign_counts = self.df['sign'].value_counts().sort_index()
        sign_counts.plot(kind='bar', ax=ax)
        ax.set_title('Number of Samples per ASL Sign')
        ax.set_xlabel('ASL Sign')
        ax.set_ylabel('Number of Samples')
        
        # Embed plot in window
        canvas = FigureCanvasTkAgg(fig, master=plot_window)
        canvas.draw()
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

def main():
    root = tk.Tk()
    app = ASLDataCollector(root)
    root.mainloop()

if __name__ == "__main__":
    main()