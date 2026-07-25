import tkinter as tk
from tkinter import ttk
import json
import os

class StickyNote:
    def __init__(self, root, x=100, y=100, color="#FFE66D", text=""):
        self.root = root
        self.color = color
        self.x = x
        self.y = y
        
        # Создаем окно без рамки (как стикер)
        self.window = tk.Toplevel(root)
        self.window.overrideredirect(True)  # Убираем рамку окна
        self.window.attributes('-topmost', True)  # Всегда поверх других окон
        self.window.geometry(f"250x250+{x}+{y}")
        
        # Основной фрейм с цветом стикера
        self.frame = tk.Frame(self.window, bg=color, highlightbackground="#ddd", 
                            highlightthickness=1)
        self.frame.pack(fill=tk.BOTH, expand=True)
        
        # Текстовое поле
        self.text_widget = tk.Text(self.frame, bg=color, fg="#333", 
                                 font=("Segoe UI", 12), 
                                 wrap=tk.WORD, bd=0, padx=10, pady=10)
        self.text_widget.pack(fill=tk.BOTH, expand=True)
        self.text_widget.insert("1.0", text)
        
        # Кнопка закрытия (крестик в углу)
        close_btn = tk.Button(self.frame, text="×", command=self.close_note,
                            bg=color, fg="#999", font=("Arial", 14, "bold"),
                            bd=0, cursor="hand2")
        close_btn.place(x=220, y=5)
        
        # Привязываем события для перетаскивания
        self.frame.bind("<ButtonPress-1>", self.start_drag)
        self.frame.bind("<B1-Motion>", self.do_drag)
        self.text_widget.bind("<ButtonPress-1>", self.start_drag)
        self.text_widget.bind("<B1-Motion>", self.do_drag)
    
    def start_drag(self, event):
        self.drag_x = event.x_root - self.window.winfo_x()
        self.drag_y = event.y_root - self.window.winfo_y()
    
    def do_drag(self, event):
        new_x = event.x_root - self.drag_x
        new_y = event.y_root - self.drag_y
        self.window.geometry(f"+{new_x}+{new_y}")
    
    def close_note(self):
        self.save_to_file()
        self.window.destroy()
    
    def save_to_file(self):
        notes_data = load_notes()
        note_id = id(self.window)
        notes_data[str(note_id)] = {
            'x': self.window.winfo_x(),
            'y': self.window.winfo_y(),
            'color': self.color,
            'text': self.text_widget.get("1.0", tk.END).strip()
        }
        save_notes(notes_data)

def load_notes():
    if os.path.exists('notes.json'):
        with open('notes.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    return {}

def save_notes(data):
    with open('notes.json', 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

def create_new_note(root, color="#FFE66D"):
    notes_data = load_notes()
    if notes_data:
        last_note = list(notes_data.values())[-1]
        x = last_note['x'] + 30
        y = last_note['y'] + 30
    else:
        x, y = 100, 100
    
    StickyNote(root, x=x, y=y, color=color)

def main():
    root = tk.Tk()
    root.withdraw()  # Скрываем главное окно
    
    # Загружаем сохраненные заметки
    notes_data = load_notes()
    for note_id, data in notes_data.items():
        try:
            StickyNote(root, x=data['x'], y=data['y'], 
                      color=data['color'], text=data['text'])
        except:
            pass
    
    # Создаем меню для добавления новых заметок
    menu_bar = tk.Menu(root)
    file_menu = tk.Menu(menu_bar, tearoff=0)
    file_menu.add_command(label="Новая заметка (желтая)", 
                         command=lambda: create_new_note(root, "#FFE66D"))
    file_menu.add_command(label="Новая заметка (зеленая)", 
                         command=lambda: create_new_note(root, "#95E1D3"))
    file_menu.add_command(label="Новая заметка (розовая)", 
                         command=lambda: create_new_note(root, "#F38BA8"))
    file_menu.add_separator()
    file_menu.add_command(label="Выход", command=root.quit)
    menu_bar.add_cascade(label="Файл", menu=file_menu)
    
    root.config(menu=menu_bar)
    root.mainloop()

if __name__ == "__main__":
    main()