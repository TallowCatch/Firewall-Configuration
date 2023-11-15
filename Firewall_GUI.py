import tkinter as tk
import customtkinter
from tkinter import ttk, messagebox, scrolledtext, Menu
from PIL import Image, ImageTk
import re
import socket
import datetime
import requests
import webbrowser

customtkinter.set_appearance_mode("dark") 
customtkinter.set_default_color_theme("dark-blue") 

class FirewallGUI:
    def __init__(self, master):
        self.master = master
        master.title("Firewall Rule Manager")

        # Styling
        style = ttk.Style()
        style.configure('TButton', background='#4a7a8c', font=('Arial', 10))
        style.configure('TLabel', font=('Arial', 12), background='#334257', foreground='#ffffff')  # Enlarged font size for labels
        master.configure(bg='#334257')

        # Server connection details
        self.server_host = '127.0.0.1'
        self.server_port = 2000

        #Labels
        ttk.Label(master, text="Insert the IP Address here \u279C", style='TLabel').grid(row=0, column=0, padx=10, pady=10)  # Unicode arrow
        ttk.Label(master, text="Insert the Port here \u279C", style='TLabel').grid(row=1, column=0, padx=10, pady=10)  # Unicode arrow

        # Entry fields
        self.ip_entry = ttk.Entry(master)
        self.port_entry = ttk.Entry(master)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=10)
        self.port_entry.grid(row=1, column=1, padx=10, pady=10)

        # Load icons for buttons
        # Note: Make sure to update these paths to your icons
        self.add_icon = ImageTk.PhotoImage(Image.open("/Users/ameerfiras/Desktop/C_Project/Images/plus-icon-vector.png").resize((20, 20), Image.Resampling.LANCZOS))
        self.delete_icon = ImageTk.PhotoImage(Image.open("/Users/ameerfiras/Desktop/C_Project/Images/delete-icon-vector.png").resize((20, 20), Image.Resampling.LANCZOS))
        self.list_icon = ImageTk.PhotoImage(Image.open("/Users/ameerfiras/Desktop/C_Project/Images/list-icon-vector.png").resize((20, 20), Image.Resampling.LANCZOS))
        self.check_icon = ImageTk.PhotoImage(Image.open("/Users/ameerfiras/Desktop/C_Project/Images/IP_Image.png").resize((20, 20), Image.Resampling.LANCZOS))

        # Set fixed button sizes
        button_width = 190  # or any suitable value
        button_height = 40  # or any suitable value

        # Buttons
        customtkinter.CTkButton(master, image=self.add_icon, text="Add", width=button_width, height=button_height, compound="top", command=self.add_rule).grid(row=3, column=0, padx=10, pady=10, sticky="nsew")
        customtkinter.CTkButton(master, image=self.delete_icon, text="Delete", width=button_width, height=button_height, compound="top", command=self.delete_rule).grid(row=3, column=1, padx=10, pady=10, sticky="nsew")
        customtkinter.CTkButton(master, image=self.list_icon, text="List", width=button_width, height=button_height, compound="top", command=self.list_rules).grid(row=4, column=0, padx=10, pady=10, sticky="nsew")
        customtkinter.CTkButton(master, image=self.check_icon, text="Check", width=button_width, height=button_height, compound="top", command=self.check_rule).grid(row=4, column=1, padx=10, pady=10, sticky="nsew")

        # Configure the grid to control stretching
        master.grid_rowconfigure(3, weight=0)
        master.grid_rowconfigure(4, weight=0)
        master.grid_columnconfigure(0, weight=1)
        master.grid_columnconfigure(1, weight=1)

        # Menu
        menubar = Menu(master)
        helpmenu = Menu(menubar, tearoff=0)
        helpmenu.add_command(label="About", command=self.show_about)
        helpmenu.add_command(label="Usage", command=self.show_usage)
        menubar.add_cascade(label="Help", menu=helpmenu)
        master.config(menu=menubar)

        # Configure the grid to control stretching
        for i in range(5):  # Configure rows 0 to 4 to not expand
            master.grid_rowconfigure(i, weight=0)
        master.grid_columnconfigure(0, weight=1)
        master.grid_columnconfigure(1, weight=1)

        # Log window
        self.log_text = scrolledtext.ScrolledText(master, height=10, state=tk.DISABLED)
        self.log_text.grid(row=5, column=0, columnspan=2, padx=10, pady=10, sticky="nsew")
        master.grid_rowconfigure(5, weight=1)  # Make the log window row expand

        # Make the log window expandable
        self.log_text.grid(row=5, column=0, columnspan=2, padx=10, pady=10, sticky="nsew")

    # Help section methods
    def show_about(self):
        messagebox.showinfo("About", "Firewall Rule Manager\nVersion 1.0")

    def show_usage(self):
        # Use webbrowser to open the README.md file in the default browser
        readme_path = 'https://github.com/TallowCatch/Firewall-Configuration/blob/main/README.md'
        webbrowser.open_new(readme_path)

     # Validation functions
    def validate_ip(self):
        ip_pattern = re.compile(r'^(\d{1,3}\.){3}\d{1,3}$')
        ip = self.ip_entry.get()
        if not ip_pattern.match(ip):
            messagebox.showwarning("Invalid IP", "Please enter a valid IPv4 address.")
            self.ip_entry.delete(0, tk.END)
            return False
        return True

    def validate_port(self):
        port = self.port_entry.get()
        if not port.isdigit() or not 1 <= int(port) <= 65535:
            messagebox.showwarning("Invalid Port", "Please enter a valid port number (1-65535).")
            self.port_entry.delete(0, tk.END)
            return False
        return True

    # Add the log_message method
        
    def log_message(self, message):
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Check if the message contains rule-query pairs
        if "Rule:" in message:
            # Split the message into lines and process each line
            lines = message.split('\n')
            formatted_lines = []
            first_rule = True
            for line in lines:
                # Insert a newline before each "Rule:" line except the first
                if line.startswith("Rule:") and not first_rule:
                    formatted_lines.append("\n" + line)
                else:
                    formatted_lines.append(line)
                if line.startswith("Rule:"):
                    first_rule = False
            # Join the processed lines back into a single string
            message = '\n'.join(formatted_lines)

        # Prepare the message with dashed lines
        formatted_message = f"-----\n\n{timestamp} - {message}\n\n-----\n"

        # Temporarily enable the widget to add text
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, formatted_message)

        # Disable the widget to prevent user input
        self.log_text.config(state=tk.DISABLED)
        self.log_text.yview(tk.END)  # Auto-scroll to the bottom

    def communicate_with_server(self, message):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((self.server_host, self.server_port))
                s.sendall(message.encode())
                response = s.recv(1024).decode()
                return response
        except ConnectionError as e:
            messagebox.showerror("Connection Error", str(e))
            return None

    def add_rule(self):
        if self.validate_ip() and self.validate_port():
            ip = self.ip_entry.get()
            port = self.port_entry.get()
            response = self.communicate_with_server(f"A {ip} {port}")
            if response:
                self.log_message(f"Add: {response}")

    def delete_rule(self):
        if self.validate_ip() and self.validate_port():
            ip = self.ip_entry.get()
            port = self.port_entry.get()
            response = self.communicate_with_server(f"D {ip} {port}")
            if response:
                self.log_message(f"Delete: {response}")

    def list_rules(self):
        response = self.communicate_with_server("L")
        if response:
            self.log_message(f"List:\n{response}")

    def check_rule(self):
        if self.validate_ip() and self.validate_port():
            ip = self.ip_entry.get()
            port = self.port_entry.get()
            response = self.communicate_with_server(f"C {ip} {port}")
            if response:
                self.log_message(f"Check: {response}")

root = tk.Tk()
gui = FirewallGUI(root)
root.mainloop()