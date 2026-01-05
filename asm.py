import sys
import struct

# Opcode Mapping
ALU_OPS = {"MOV": 0, "ADD": 1, "AND": 2, "XOR": 3}
MEM_OPS = {"LOAD": 0, "STORE": 1, "LEA": 2}
CTRL_OPS = {"RET": 0, "CJMP": 1, "JMP": 2, "CALL": 3}

# Register Mapping (NNN)
REGS = {f"R{i}": i for i in range(8)}

def encode_mem_mod(mode_str, reg_nnn, size, pre_dec=False, post_inc=False):
    """Encodes the reg/mod byte per the project table"""
    # Mode: bits 7-6
    modes = {"REG": 0b00, "BP": 0b01, "SP": 0b10, "IMM": 0b11}
    mod_byte = modes[mode_str] << 6
    
    # W (Size): bit 5 (1 for Word, 0 for Byte)
    if size == 2:
        mod_byte |= (1 << 5)
    
    # I (Post-inc): bit 4
    if post_inc:
        mod_byte |= (1 << 4)
        
    # D (Pre-dec): bit 3
    if pre_dec:
        mod_byte |= (1 << 3)
        
    # NNN: bits 2-0
    mod_byte |= REGS[reg_nnn]
    
    return mod_byte

def assemble_instruction(line):
    tokens = line.replace(",", " ").split()
    if not tokens: return b""
    
    mnemonic = tokens[0].upper()
    
    # Example ALU: ADD R1 R2 -> [op:01, d:001, s:010] -> 01001010
    if mnemonic in ALU_OPS:
        op = ALU_OPS[mnemonic]
        d = REGS[tokens[1]]
        s = REGS[tokens[2]]
        byte = (op << 6) | (d << 3) | s
        return struct.pack("B", byte)

    # Example Memory: LOAD R1 [BP+R2++] (Word)
    # Syntax: LOAD <dst_reg> <mode> <index_reg> <size> <pre/post>
    if mnemonic in MEM_OPS:
        op = MEM_OPS[mnemonic]
        dst_s = REGS[tokens[1]]
        # Group 1 instructions have d=0
        instr_byte = (op << 6) | (0 << 3) | dst_s 
        
        # mod byte details
        mod = encode_mem_mod(tokens[2], tokens[3], int(tokens[4]), "PRE" in tokens, "POST" in tokens)
        
        output = struct.pack("BB", instr_byte, mod)
        
        # If mode is IMM, we need to append a 16-bit word
        if tokens[2] == "IMM":
            output += struct.pack("<H", int(tokens[5], 0))
        return output

    # Control flow instructions
    if mnemonic in CTRL_OPS:
        op = CTRL_OPS[mnemonic]
        # RET: op=0, d=0, s=0 -> 00 000 000 = 0x00
        if mnemonic == "RET":
            return struct.pack("B", 0x00)
        # JMP: op=2, d=0, s=0 -> 10 000 000 = 0x80, then 16-bit address
        if mnemonic == "JMP":
            instr_byte = (op << 6) | (0 << 3) | 0
            addr = int(tokens[1], 0)
            return struct.pack("<BH", instr_byte, addr)
        # CALL: op=3, d=0, s=0 -> 11 000 000 = 0xC0, then 16-bit address
        if mnemonic == "CALL":
            instr_byte = (op << 6) | (0 << 3) | 0
            addr = int(tokens[1], 0)
            return struct.pack("<BH", instr_byte, addr)
    
    return b""

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 asm.py <input.lavm> -o <output.bin>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = None
    
    # Parse arguments
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "-o" and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        else:
            i += 1
    
    if not output_file:
        print("Error: Output file not specified. Use -o <output.bin>")
        sys.exit(1)
    
    # Read and assemble
    binary = b""
    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            instr = assemble_instruction(line)
            binary += instr
    
    # Write output (create directory if needed)
    import os
    os.makedirs(os.path.dirname(output_file) if os.path.dirname(output_file) else '.', exist_ok=True)
    with open(output_file, 'wb') as f:
        f.write(binary)
    
    print(f"Assembled {len(binary)} bytes -> {output_file}")

if __name__ == "__main__":
    main()