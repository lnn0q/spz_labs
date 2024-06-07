from customfs import InMemoryFS

fs = InMemoryFS()

fs.create("file1.txt")
fs.create("file2.txt")
fs.link("file1.txt", "alias1.txt")

fd1 = fs.open("file1.txt")
fd2 = fs.open("file2.txt")

fs.write(fd1, b"File one - notatextanotatext")
fs.write(fd2, b"File two - 123456789abcdefg")

fs.unlink("alias1.txt")

print(fs.ls()) 

data = fs.read(fd1, 10)
print(data.decode())  



fs.close(fd1)

fs.write(fd2, b"Appended content")
print(fs.read(fd2, 20).decode()) 
fs.truncate("file2.txt", 10)
fs.close(fd2)
