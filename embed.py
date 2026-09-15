import sys
data = open(sys.argv[1], 'rb').read()
out = open(sys.argv[2], 'w')
out.write('#include "types.h"\n\n')
out.write('const uint8 user_hello_elf[] = {\n')
for i in range(0, len(data), 12):
    chunk = data[i:i+12]
    hex_bytes = ', '.join('0x{:02x}'.format(b) for b in chunk)
    out.write('    ' + hex_bytes + ',\n')
out.write('};\n')
out.write('const uint32 user_hello_elf_size = sizeof(user_hello_elf);\n')
out.close()
print('Done, {} bytes embedded'.format(len(data)))
