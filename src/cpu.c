#include <cpu.h>
#include <time.h>
#include <stdio.h>
#include <peripherals.h>

void cpu_set_freq(CPU *cpu, u32 hz) {
    float freq_pause_nanos_f = (1.0 / hz) * NANOS_IN_SECOND;
    cpu->freq_pause_nanos = (int)freq_pause_nanos_f;
    cpu->freq_hz = hz;
}

void cpu_error_illegal_opcode() {
    error("Got illegal opcode");
}

// Read instruction from ROM at address `addr`
u16 cpu_get_instruction(CPU *cpu, u16 addr) {
    if (addr >= cpu->rom_size_words) {
        char s[64];
        sprintf(s, "cpu_get_instruction called with addr too large: 0x%04X", addr);
        error(s);
    }

    return cpu->rom[addr];
}

u8 cpu_ram_get_unsafe(CPU *cpu, u8 addr) { return cpu->ram[addr]; }
void cpu_ram_write_unsafe(CPU *cpu, u8 addr, u8 data) { cpu->ram[addr] = data; }
void cpu_stack_push8_unsafe(CPU *cpu, u8 data) { cpu_ram_write_unsafe(cpu, eSP++, data); }
u8 cpu_stack_pop8_unsafe(CPU *cpu) { return cpu_ram_get_unsafe(cpu, --eSP); }

// Read memory from RAM at address `addr`
u8 cpu_ram_get(CPU *cpu, u8 addr) {
    if (addr >= cpu->ram_size_bytes) {
        char s[64];
        sprintf(s, "cpu_ram_get called with addr too large: 0x%02X", addr);
        error(s);
    }

    return cpu->ram[addr];
}

// Write to from at address `addr` with data `data`
void cpu_ram_write(CPU *cpu, u8 addr, u8 data) {
    if (addr >= cpu->ram_size_bytes) {
        char s[64];
        sprintf(s, "cpu_ram_write called with addr too large: 0x%02X", addr);
        error(s);
    }

    cpu->ram[addr] = data;
}

// Push byte `data` on the stack
void cpu_stack_push8(CPU *cpu, u8 data) {
    u8 new_sp = eSP + 1;

    if (new_sp >= cpu->ram_size_bytes) {
        char s[80];
        sprintf(s, "cpu_stack_push8 called with new stack pointer too large: 0x%02X", new_sp);
        error(s); 
    }

    cpu_ram_write_unsafe(cpu, eSP++, data);
}

// Pop byte off from the stack
u8 cpu_stack_pop8(CPU *cpu) {
    if ((i8)eSP - 1 < 0) {
        error("cpu_stack_pop8 called with nothing on the stack");
    }

    return cpu_ram_get_unsafe(cpu, --eSP);
}

// Push word `data` on the stack
void cpu_stack_push16(CPU *cpu, u16 data) {
    u8 new_sp = eSP + 2;

    if (new_sp >= cpu->ram_size_bytes) {
        char s[80];
        sprintf(s, "cpu_stack_push16 called with new stack pointer too large: 0x%02X", new_sp);
        error(s); 
    }

    cpu_stack_push8_unsafe(cpu, data);
    cpu_stack_push8_unsafe(cpu, data >> 8);
}

// Pop word off the stack
u16 cpu_stack_pop16(CPU *cpu) {
    if ((i8)eSP - 2 < 0) {
        error("cpu_stack_pop16 called with nothing on the stack");
    }

    return (cpu_stack_pop8_unsafe(cpu) << 8) | cpu_stack_pop8_unsafe(cpu);
}

// Read byte from IO register
u8 cpu_io_get(CPU *cpu, u8 addr) {
    if (addr >= cpu->io_size_bytes) {
        char s[48];
        sprintf(s, "Invalid IO read with addr: 0x%02X", addr);
        error(s); 
    }

    return peripheral_read(cpu, addr);
}

// Write byte to IO register
void cpu_io_write(CPU *cpu, u8 addr, u8 data) {
    if (addr >= cpu->io_size_bytes) {
        char s[48];
        sprintf(s, "Invalid IO write with addr: 0x%02X", addr);
        error(s);
    }

    peripheral_write(cpu, addr, data);
}

// The cpu_pdk14_execute function is mainly from: https://github.com/free-pdk/fppa-pdk-tools
// The flag solving functions are also from fppa-pdk-tools
//
// MIT License
//
// Copyright (c) 2018 free-pdk
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

u8 cpu_add_solve_flags_vacz(u16 value1, i16 value2, i16 c) {
    int zr = !(((value1&0xFF)+(value2&0xFF)+c)&0xFF);
    int cy = (((value1&0xFF)+(value2&0xFF)+c)>>8)&1;
    int ac = (((value1&0xF)+(value2&0xF)+c)>>4)&1;
    int ov = ((((value1&0x7F)+(value2&0x7F)+c)>>7)&1)^cy;
    return( (ov<<3)|(ac<<2)|(cy<<1)|zr );
}

u8 cpu_sub_solve_flags_vacz(i16 value1, i16 value2, i16 c) {
    int zr = !(((value1&0xFF)-(value2&0xFF)-c)&0xFF);
    int cy = (((value1&0xFF)-(value2&0xFF)-c)>>8)&1;
    int ac = ((value1&0xF)<((value2&0xF)+c))?1:0;
    int ov = ((((value1&0x7F)-(value2&0x7F))>>7)&1)^cy;
    return( (ov<<3)|(ac<<2)|(cy<<1)|zr );
}

// Returns how many cycles the instruction should take.
int cpu_pdk14_execute(CPU *cpu) {
    i16 T; // Temp register.
    u16 opcode = cpu_get_instruction(cpu, cpu->pc++);
    u64 start_cycles = eCycle; // Used to check if cpu_cycle increased, if so, we have extra cycles we need to skip.

    //14 bit opcodes 0x0000 - 0x00BF
    if (opcode<=0x00BF) {
        switch (opcode) {
            case 0x0000: break;                                                                                                           //NOP

            case 0x0006: eA=cpu_get_instruction(cpu,cpu_ram_get(cpu,eSP)|(((u16)cpu_ram_get(cpu,eSP+1))<<8))&0xFF; eCycle++; break;      //LDSPTL //TODO: verify, Z?
            case 0x0007: eA=cpu_get_instruction(cpu,cpu_ram_get(cpu,eSP)|(((u16)cpu_ram_get(cpu,eSP+1))<<8))>>8; eCycle++; break;        //LDSPTH //TODO: verify, Z?

            case 0x0060: T=(eF>>1)&1;eF=cpu_add_solve_flags_vacz(eA,0,T); eA=(eA+T)&0xFF; break;                                           //ADDC A
            case 0x0061: T=(eF>>1)&1;eF=cpu_sub_solve_flags_vacz(eA,0,T); eA=(eA-T)&0xFF; break;                                           //SUBC A
            case 0x0062: eF=cpu_add_solve_flags_vacz(eA, 1, 0); eA=(eA+1)&0xFF; if (!eA){ePC++; eCycle++;} break;                           //IZSN A
            case 0x0063: eF=cpu_sub_solve_flags_vacz(eA, 1, 0); eA=(eA-1)&0xFF; if (!eA){ePC++; eCycle++;} break;                           //DZSN A

            case 0x0067: ePC=(ePC-1)+eA; break;                                                                                           //PCADD A
            case 0x0068: eA=(~eA)&0xFF; eF=(eF&0xE)|(!eA); break;                                                                         //NOT A
            case 0x0069: eA=(-((i8)eA))&0xFF; eF=(eF&0xE)|(!eA); break;                                                               //NEG A
            case 0x006A: eF=(eF&0xD)|((eA<<1)&2); eA>>=1; break;                                                                          //SR A
            case 0x006B: eF=(eF&0xD)|((eA>>6)&2); eA=(eA<<1)&0xFF; break;                                                                 //SL A
            case 0x006C: eA|=(eF&2)<<7; eF=(eF&0xD)|((eA<<1)&2); eA>>=1; break;                                                           //SRC A
            case 0x006D: T=(eF>>1)&1; eF=(eF&0xD)|((eA>>6)&2); eA=((eA<<1)&0xFF)|T; break;                                                //SLC A
            case 0x006E: eA=((eA<<4)|(eA>>4))&0xFF; break;                                                                                //SWAP A

            case 0x0070: /*TODO*/ break; //WDRESET

            case 0x0072: cpu_stack_push8(cpu,eA); cpu_stack_push8(cpu,eF); break;                                                         //PUSHAF
            case 0x0073: eF=cpu_stack_pop8(cpu); eA=cpu_stack_pop8(cpu); break;                                                           //POPAF

            case 0x0075: return 1;                                                                                                        //RESET
            case 0x0076: ePC--; return 2;                                                                                                 //STOPSYS
            case 0x0077: ePC--; return 3;                                                                                                 //STOPEXE
            case 0x0078: eGINTEnabled=true;  break;                                                                                       //ENGINT
            case 0x0079: eGINTEnabled=false;  break;                                                                                      //DISGINT
            case 0x007A: ePC=cpu_stack_pop16(cpu); break;                                                                                 //RET
            case 0x007B: ePC=cpu_stack_pop16(cpu); eGINTEnabled=true; break;                                                              //RETI
            case 0x007C: /*TODO*/ break; //MUL

            default:
                cpu_error_illegal_opcode();
                return -1;
        }
    } else if ((opcode>=0x00C0) && (opcode<=0x01FF)) { //8 bit opcodes 0x00C0 - 0x01FF
        u8 addr = opcode&0x3F;
        switch (opcode&0x3FC0) {
            case 0x00C0: cpu_io_write(cpu,addr,cpu_io_get(cpu,addr)^eA);break;                                                            //XOR IO, A
            case 0x0180: cpu_io_write(cpu,addr,eA);break;                                                                                  //MOV IO, A
            case 0x01C0: eA=cpu_io_get(cpu,addr);eF=(eF&0xE)|(!eA);break;                                                                //MOV A, IO
            default:
                cpu_error_illegal_opcode();
                return -1;
        }
    } else if (0x0300 == (opcode&0x3F00)) { //7 bit opcodes 0x03..
        u8 addr = opcode&0x7E;
        switch (opcode & 0x3F81) {
            case 0x0300: cpu_ram_write(cpu, addr, eT16&0xFF); cpu_ram_write(cpu, addr, eT16>>8); break;                                     //STT16 M
            case 0x0301: eT16 = cpu_ram_get(cpu, addr) | (((u16)cpu_ram_get(cpu, addr+1))<<8); break;                              //LDT16 M
            case 0x0380: T=cpu_ram_get(cpu, addr); eCycle++; cpu_ram_write(cpu, T,eA); break;                                             //IDXM M,A
            case 0x0381: T=cpu_ram_get(cpu, addr); eCycle++; eA=cpu_ram_get(cpu, T); break;                                             //IDXM A,M
        }
    } else if ((opcode>=0x0600) && (opcode<=0x17FF)) { //7 bit opcodes 0x0600 - 0x17FF
        i8 addr = opcode&0x7F;
        i16 M = cpu_ram_get(cpu, addr);
        switch (opcode & 0x3F80) {
            case 0x0600: eF=cpu_sub_solve_flags_vacz(eA,M,0); break;                                                                       //COMP A, M
            case 0x0680: eF=cpu_sub_solve_flags_vacz(M,eA,0); break;                                                                       //COMP M, A
            case 0x0700: eF=cpu_add_solve_flags_vacz(eA,(-M)&0xFF,0); eA=(eA+((-M)&0xFF))&0xFF; break;                                     //NADD A, M //TODO: verify
            case 0x0780: eF=cpu_add_solve_flags_vacz(M,(-eA)&0xFF,0); M=(M+((-eA)&0xFF))&0xFF; break;                                      //NADD M, A //TODO: verify
            case 0x0800: eF=cpu_add_solve_flags_vacz(M,eA,0); cpu_ram_write(cpu,addr,M+eA); break;                                          //ADD M, A
            case 0x0880: eF=cpu_sub_solve_flags_vacz(M,eA,0); cpu_ram_write(cpu,addr,M-eA); break;                                          //SUB M, A
            case 0x0900: T=(eF>>1)&1;eF=cpu_add_solve_flags_vacz(M,eA,T); cpu_ram_write(cpu,addr,M+eA+T); break;                            //ADDC M, A
            case 0x0980: T=(eF>>1)&1;eF=cpu_sub_solve_flags_vacz(M,eA,T); cpu_ram_write(cpu,addr,M-eA-T); break;                            //SUBC M, A
            case 0x0A00: M&=eA;cpu_ram_write(cpu,addr,M);eF=(eF&0xE)|(!M); break;                                                          //AND M, A
            case 0x0A80: M|=eA;cpu_ram_write(cpu,addr,M);eF=(eF&0xE)|(!M); break;                                                          //OR M, A
            case 0x0B00: M^=eA;cpu_ram_write(cpu,addr,M);eF=(eF&0xE)|(!M); break;                                                          //XOR M, A
            case 0x0B80: cpu_ram_write(cpu,addr,eA); break;                                                                                //MOV M, A
            case 0x0C00: eF=cpu_add_solve_flags_vacz(eA,M,0);eA=(eA+M)&0xFF; break;                                                        //ADD A, M
            case 0x0C80: eF=cpu_sub_solve_flags_vacz(eA,M,0);eA=(eA-M)&0xFF; break;                                                        //SUB A, M
            case 0x0D00: T=(eF>>1)&1;eF=cpu_add_solve_flags_vacz(eA,M,T);eA=(eA+M+T)&0xFF; break;                                          //ADDC A, M
            case 0x0D80: T=(eF>>1)&1;eF=cpu_sub_solve_flags_vacz(eA,M,T);eA=(eA-M-T)&0xFF; break;                                          //SUBC A, M
            case 0x0E00: eA=eA&M;eF=(eF&0xE)|(!eA); break;                                                                                //AND A, M
            case 0x0E80: eA=eA|M;eF=(eF&0xE)|(!eA); break;                                                                                //OR A, M
            case 0x0F00: eA=eA^M;eF=(eF&0xE)|(!eA); break;                                                                                //XOR A, M
            case 0x0F80: eA=M;eF=(eF&0xE)|(!eA); break;                                                                                   //MOV A, M
            case 0x1000: T=(eF>>1)&1;eF=cpu_add_solve_flags_vacz(M,0,T); cpu_ram_write(cpu,addr,M+T); break;                                //ADDC M
            case 0x1080: T=(eF>>1)&1;eF=cpu_sub_solve_flags_vacz(M,0,T); cpu_ram_write(cpu,addr,M-T); break;                                //SUBC M
            case 0x1100: eF=cpu_add_solve_flags_vacz(M,1,0);cpu_ram_write(cpu,addr,M+1);if (!(eF&1)){ePC++;eCycle++;}break;                  //IZSN M
            case 0x1180: eF=cpu_sub_solve_flags_vacz(M,1,0);cpu_ram_write(cpu,addr,M-1);if (!(eF&1)){ePC++;eCycle++;}break;                  //DZSN M
            case 0x1200: eF=cpu_add_solve_flags_vacz(M,1,0);cpu_ram_write(cpu,addr,M+1);break;                                              //INC M
            case 0x1280: eF=cpu_sub_solve_flags_vacz(M,1,0);cpu_ram_write(cpu,addr,M-1);break;                                              //DEC M
            case 0x1300: cpu_ram_write(cpu,addr,0); break;                                                                                 //CLEAR M
            case 0x1380: cpu_ram_write(cpu,addr,eA); eA=M; break;                                                                          //XCH A,M
            case 0x1400: M=(~M)&0xFF; eF=(eF&0xE)|(!M); cpu_ram_write(cpu,addr,M); break;                                                  //NOT M
            case 0x1480: M=(-((i8)M))&0xFF; eF=(eF&0xE)|(!M); cpu_ram_write(cpu,addr,M); break;                                        //NEG M
            case 0x1500: eF=(eF&0xD)|((M<<1)&2); cpu_ram_write(cpu,addr,M>>1); break;                                                      //SR M
            case 0x1580: eF=(eF&0xD)|((M>>6)&2); cpu_ram_write(cpu,addr,M<<1); break;                                                      //SL M
            case 0x1600: M|=(eF&2)<<7; eF=(eF&0xD)|((M<<1)&2); cpu_ram_write(cpu,addr,M>>1); break;                                        //SRC M
            case 0x1680: T=(eF>>1)&1; eF=(eF&0xD)|((M>>6)&2); cpu_ram_write(cpu,addr,(M<<1)|T); break;                                     //SLC M
            case 0x1700: eF=cpu_sub_solve_flags_vacz(eA, M, 0); if (!((eA-M)&0xFF)){ ePC++; eCycle++;} break;                             //CEQSN A,M
            case 0x1780: eF=cpu_sub_solve_flags_vacz(eA, M, 0); if ((eA-M)&0xFF){ ePC++; eCycle++;} break;                                //CNEQSN A,M
        }
    } else if ((0x0200 == (opcode&0x3F00)) || (0x2800 == (opcode&0x3800))) {     //6 bit opcodes 0x02.. , 0x2800 - 0x2FFF
        u8 k = opcode&0xFF;
        switch (opcode & 0x3F00) {
            case 0x0200: eA=k; ePC=cpu_stack_pop16(cpu); break;                                                                           //RET k
            case 0x2800: eF=cpu_add_solve_flags_vacz(eA, k, 0); eA=(eA+k)&0xFF; break;                                                     //ADD A,k
            case 0x2900: eF=cpu_sub_solve_flags_vacz(eA, k, 0); eA=(eA-k)&0xFF; break;                                                     //SUB A,k
            case 0x2A00: eF=cpu_sub_solve_flags_vacz(eA, k, 0); if (!((eA-k)&0xFF)){ ePC++; eCycle++;} break;                             //CEQSN A,k
            case 0x2B00: eF=cpu_sub_solve_flags_vacz(eA, k, 0); if ((eA-k)&0xFF){ ePC++; eCycle++;} break;                                //CNEQSN A,k
            case 0x2C00: eA &= k; eF=(eF&0xE)|(!eA); break;                                                                               //AND A,k
            case 0x2D00: eA |= k; eF=(eF&0xE)|(!eA); break;                                                                               //OR A,k
            case 0x2E00: eA ^= k; eF=(eF&0xE)|(!eA); break;                                                                               //XOR A,k
            case 0x2F00: eA  = k; break;                                                                                                  //MOV A,k
        }
    } else if ((0x0400 == (opcode&0x3E00)) || ((opcode>=0x1800) && (opcode<=0x27FF))) { //5 bit opcodes 0x0400 - 0x0500, 0x1800 - 0x27FF
        u8 bit = 1<<((opcode>>6)&7);
        u8 addr = opcode&0x3F;
        switch (opcode & 0x3E00) {
            case 0x0400: T=cpu_io_get(cpu,addr);cpu_io_write(cpu,addr,eF&2?T|bit:T&~bit);eF=(eF&0xD)|((T&bit)?2:0); break;                //SWAPC IO.n
            case 0x1800: if (!(cpu_io_get(cpu,addr)&bit)) { ePC++; eCycle++; } break;                                                   //T0SN IO.n
            case 0x1A00: if (cpu_io_get(cpu,addr)&bit) { ePC++; eCycle++; } break;                                                      //T1SN IO.n
            case 0x1C00: cpu_io_write(cpu,addr,cpu_io_get(cpu,addr)&~bit); break;                                                         //SET0 IO.n
            case 0x1E00: cpu_io_write(cpu,addr,cpu_io_get(cpu,addr)|bit); break;                                                          //SET1 IO.n
            case 0x2000: if (!(cpu_ram_get(cpu,addr)&bit)) { ePC++; eCycle++; } break;                                                  //T0SN M.n
            case 0x2200: if (cpu_ram_get(cpu,addr)&bit) { ePC++; eCycle++; } break;                                                     //T1SN M.n
            case 0x2400: cpu_ram_write(cpu,addr,cpu_ram_get(cpu,addr)&~bit);break;                                                        //SET0 M.n
            case 0x2600: cpu_ram_write(cpu,addr,cpu_ram_get(cpu,addr)|bit);break;                                                         //SET1 M.n
        }
    } else if ((0x3000 == (opcode&0x3000))) {     //3 bit opcodes 0x3000 - 0x3FFF
        if (opcode & 0x0800) //CALL needs to put current PC on stack                                                                   //CALL p
            cpu_stack_push16(cpu,ePC);
        eCycle++;
        ePC = opcode & 0x07FF;                                                                                                          //GOTO p
    } else {
        error("Got illegal opcode");
        return -1;
    }

    //         Check for extra cycles to skip.
    return 1 + (eCycle - start_cycles);
}

// Returns how many cycles the instruction should take.
int cpu_clock_run(CPU *cpu) {
    return cpu_pdk14_execute(cpu);
}

int skip_cycles = 0;

void cpu_clock_loop(CPU *cpu) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        i64 elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000 + now.tv_nsec - start.tv_nsec;
        if (elapsed_ns >= cpu->freq_pause_nanos) {
            if (skip_cycles > 0) {
                skip_cycles--;
                goto end;
            }

            skip_cycles += cpu_clock_run(cpu) - 1;

        end:
            cpu->cpu_cycle++;
            start = now;
        }
    }
}
