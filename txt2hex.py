x = int(input());

buffer = []

for i in range(x):
	buffer.append(hex(int(input(), 2)))

print("uint8_t[] txtBuffer = {")
for i in buffer:
	print("{},".format(i))
print("};")