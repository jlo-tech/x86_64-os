f = open("disk.img", "wb")
f.write(bytes(512 * 8) * 16)
f.close()
