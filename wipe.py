f = open("disk.img", "wb")
f.write(bytes(512) * 64)
f.close()
