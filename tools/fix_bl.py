import argparse
import re
import struct


def generate_func_list(func_s: str) -> dict:
    with open(func_s, 'r', encoding='utf-8') as f:
        content = f.read()
    matches = re.findall(r'\.equiv (\w+), (\w+)', content)
    func_dict = {name: int(addr, 16) for name, addr in matches}
    return func_dict

def generate_fix_list(fix_lst: str) -> list:
	fix_list = []
	with open(fix_lst, 'r', encoding='utf-8') as f:
		for line in f:
			parts = line.strip().split()
			if len(parts) == 2:
				addr, func = parts
				fix_list.append((int(addr, 16), func))
	return fix_list

def fix_BL(rom: bytearray, func_dict: dict, fix_list: list) -> None:
	for addr, func in fix_list:
		diff = (func_dict[func] - (addr + 4)) >> 1 & 0x3FFFFF
		bl = 0xF8000000 | (diff & 0x7FF) << 16 | 0xF000 | diff >> 11
		rom[addr: addr + 4] = struct.pack('I', bl)

def main() -> None:
    parser = argparse.ArgumentParser(description='Fix BL call')
    parser.add_argument('func_s', type=str, help='Path to the .s file')
    parser.add_argument('fix_list', type=str, help='Path to the list file')
    parser.add_argument('rom', type=str, help='Path to the ROM')

    args = parser.parse_args()

    func_dict = generate_func_list(args.func_s)
    fix_list = generate_fix_list(args.fix_list)

    with open(args.rom, 'rb') as f:
        rom = f.read()
    rom = bytearray(rom)
    fix_BL(rom, func_dict, fix_list)

    with open(args.rom, 'wb') as f:
        f.write(rom)
    print(f"Fix {args.rom} successfully.")


if __name__ == '__main__':
    main()
