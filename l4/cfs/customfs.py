
class InMemoryFS:
    def __init__(self):
        self.files = {}  # Словник для зберігання файлів (ім'я: дані)
        self.links = {}
        self.open_files = {}  # Словник для відстеження відкритих файлів (fd: (ім'я, зміщення))
        self.next_fd = 0  # Лічильник для наступного доступного дескриптора файлу

    def create(self, name):
        if name in self.files:
            raise FileExistsError(f"Файл '{name}' вже існує")
        self.files[name] = b""

    def open(self, name):
        if name not in self.files:
            raise FileNotFoundError(f"Файл '{name}' не знайдено")
        fd = self.next_fd
        self.next_fd += 1
        self.open_files[fd] = (name, 0)
        return fd

    def close(self, fd):
        if fd not in self.open_files:
            raise ValueError(f"Недійсний дескриптор файлу {fd}")
        del self.open_files[fd]

    def read(self, fd, size):
        if fd not in self.open_files:
            raise ValueError(f"Недійсний дескриптор файлу {fd}")
        name, offset = self.open_files[fd]
        data = self.files[name]
        available = len(data) - offset
        read_size = min(size, available)
        result = data[offset:offset + read_size]
        self.open_files[fd] = (name, offset + read_size)
        return result

    def write(self, fd, data):
        if fd not in self.open_files:
            raise ValueError(f"Недійсний дескриптор файлу {fd}")
        name, offset = self.open_files[fd]
        self.files[name] = self.files[name][:offset] + data + self.files[name][offset + len(data):]

    def ls(self):
        return list(self.files.keys())

    def link(self, name1, name2):
        if name1 not in self.files:
            raise FileNotFoundError(f"Файл '{name1}' не знайдено")
        self.links[name2] = name1

    def unlink(self, name):
        if name not in self.links:
            raise FileNotFoundError(f"Тверде посилання '{name}' не знайдено")
        del self.links[name]

    def truncate(self, name, size):
        if name not in self.files:
            raise FileNotFoundError(f"Файл '{name}' не знайдено")
        self.files[name] = self.files[name][:size] + b"\0" * (size - len(self.files[name]))
