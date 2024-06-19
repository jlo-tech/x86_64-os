f = open("disk.img", "wb")
f.write(bytes(512) * 256)
f.close()
