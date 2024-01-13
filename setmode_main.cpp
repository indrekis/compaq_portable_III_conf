#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>

// #define DEBUG_MODE 1
// #define DEEP_DEBUG_MODE 1
// https://open-watcom.github.io/open-watcom-v2-wikidocs/ctools.pdf
// https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.htm
// https://open-watcom.github.io/open-watcom-v2-wikidocs/clr.html
// https://open-watcom.github.io/open-watcom-v2-wikidocs/lguide.pdf

//==Tools=================================================================

void str_toupper(char* str){
  while (*str) {
    *str = toupper(*str);
    ++str;
  }
}

//==General===============================================================

void print_usage(){
    puts("Compaq Portable III (286) and 386 mode configurator\n");
    puts("Usage (no whitespaces around '=' allowed):");
    puts(" setmode video=CGA|MDA");
    puts("or:");
    puts(" setmode CPU=common|fast|high|auto|toggle|<1-50>");
    puts("or:");
    puts(" setmode print=CPU|video|all");
    puts("or (untested!):");
    puts(" setmode monitor=external|internal");
 }

bool chech_for_compaq() {
    char __far * bios_prtr_port_1 = (char __far *) MK_FP( 0xF000, 0xFFEA );
    const char __far *compaq_str = "COMPAQ";
    const size_t css = _fstrlen(compaq_str);
    int res = _fmemcmp(bios_prtr_port_1, compaq_str, css);
    // "COMPAQ" at  F000:FFEAh
    return !res;
}

bool chech_for_qemm() {
    //  67h * 4 -- != 0
    // Int 67/AH=3Fh/CX=5145h
    // http://www.ctyme.com/intr/rb-7414.htm

    unsigned long __far * int67_IRQ_ptr = (unsigned long __far *) MK_FP( 0x00, 0x67*4 );

    if( *int67_IRQ_ptr == 0){
#ifdef DEBUG_MODE
        puts("int 67 IRQ ptr == 0");
#endif
        return false;
    } else {
#ifdef DEEP_DEBUG_MODE
        printf("int 67 0x%xl\n", int67_IRQ_ptr);
#endif
    }

    union REGS inregs, outregs;

    inregs.h.ah = 0x3F;
    inregs.w.cx = 0x5145; // "QE"
    inregs.w.dx = 0x4D4D; // "MM"

    int86(0x67, &inregs, &outregs);

    if( outregs.h.ah == 0 )
        return true;
    else {
#ifdef DEBUG_MODE
        printf("int 67 ah = 0x%x\n", outregs.h.ah);
#endif
        return false;
    }
}

//==CPU=speed=============================================================

// http://www.ctyme.com/intr/rb-1916.htm
const char* CPU_speeds[]={
    "COMMON", // 0
    "FAST",   // 1
    "HIGH",   // 2
    "Wrong-3",// 3 -- used to toogle speed, should not be returned
    "Wrong-4",// 4
    "Wrong-5",// 5
    "Wrong-6",// 6
    "Wrong-7",// 7
    "AUTO",   // 8
    "Custom:" // 9
};

unsigned read_CPU_speed(){
    union REGS inregs, outregs;
    inregs.h.ah = 0xF1; // http://www.ctyme.com/intr/rb-1918.htm
    int86(0x16, &inregs, &outregs);
    unsigned res = outregs.h.al;
    if(outregs.h.al == 9)
        res |= (outregs.w.cx << 8); // Value should be less then 50
    return res;
}

void print_CPU_speed(){
    unsigned cpu_speed_code = read_CPU_speed();
    unsigned cpu_speed = cpu_speed_code & 0x00FF;

    if( cpu_speed>9 ){
        printf("CPU speed: Wrong code (%0#4.0x)", cpu_speed_code);
    } else {
        printf("CPU speed: %s", CPU_speeds[cpu_speed]);
        if( cpu_speed == 9 )
            printf(" %x", cpu_speed_code >> 8 );
    }
    puts("");
}
int set_CPU_speed(unsigned code, unsigned custom = 0){
    union REGS inregs, outregs;
    if(code != 8 && code !=9 && code > 3)
        return 1;
    if( custom > 50 )
        return 2;
    inregs.h.ah = 0xF0;
    inregs.h.al = (unsigned char)code;
    inregs.w.cx = custom;
    int86(0x16, &inregs, &outregs);
    return 0;
}

//==video================================================================

enum mon_types_t{
    mon_t_none = 0,
    mon_t_dualmode = 1,
    mon_t_CGA = 2,
    mon_t_CCM = 3, // Compaq Color monitor
    mon_t_plasma = 4, //flat panel, 640x400
    // More types, not present in Portable III/386, here: http://www.ctyme.com/intr/rb-0481.htm
};

const char* get_mon_type_str(unsigned t){
    switch(t){
    case mon_t_none:
       return "None";
       // break;
    case mon_t_dualmode:
       return "dual-mode";
       // break;
    case mon_t_CGA:
       return "CGA";
       // break;
    case mon_t_CCM:
       return "Compaq Color monitor";
       // break;
    case mon_t_plasma:
       return "flat panel, 640x400";
       // break;
    default:
       return "Unknown or unsupported";
    }
}

//! Zero -- has capability, one -- do not have it.
enum video_capabs_masks{
    vc_has_CGA=1, vc_has_MDA=(1<<3), vc_has_bitblt=(1<<4),
    vc_has_132cols=(1<<5),  vc_has_640x480x256=(1<<6),
    vc_has_8DAC=(1<<7)
};

enum video_adapter_code_masks{
    va_master_mode=0x07, // 0b0111
    va_is_internal_mon=(1<<3),
    va_has_extended=(1<<6)
};

enum video_adapter_code_t{
     va_CGA = 0,
     va_EGA = 1,
     va_MDA = 3
};

const char* get_adapter_type_str(unsigned t){
    switch(t){
    case va_CGA:
       return "CGA";
       // break;
    case va_EGA:
       return "EGA";
       // break;
    case va_MDA:
       return "MDA";
       // break;
    default:
       return "Unknown or unsupported";
    }
}

struct video_mode_t{
    unsigned mode; // https://www.ctyme.com/intr/rb-0069.htm#Table10
    unsigned cols;
    unsigned page;
};

//! See https://www.ctyme.com/intr/rb-0108.htm
// "If mode was set with bit 7 set ("no blanking"), the returned mode will also have bit 7 set."
//  "EGA, VGA, and UltraVision return either AL=03h (color) or AL=07h (monochrome)
//   in all extended-row text modes."
video_mode_t get_cur_video_mode(){
    union REGS inregs, outregs;
    inregs.h.ah = 0x0F;
    int86(0x10, &inregs, &outregs);

    video_mode_t res;
    res.mode = outregs.h.al;
    res.cols = outregs.h.ah;
    res.page = outregs.h.bh;
    return res;
}

void print_video_subsys_cap(){
    unsigned capabilities = inp(0x17C6);
    printf("Video capabilities (%0#2x):", capabilities);
    if( !(capabilities & vc_has_CGA) ) printf(" CGA");
    if( !(capabilities & vc_has_MDA) ) printf(" MDA");
    if( !(capabilities & vc_has_bitblt) ) printf(" BitBlt");
    if( !(capabilities & vc_has_132cols) ) printf(" 132-cols");
    if( !(capabilities & vc_has_640x480x256) ) printf(" 640x480x256-mode");
    if( !(capabilities & vc_has_8DAC) ) printf(" 8-bit-DAC");
    puts("");

    unsigned monitors = inp(0x1BC6);
    unsigned external = monitors & 0x0F;
    unsigned internal = ((monitors & 0xF0)>>5);
    printf("Internal monitor (%0#1x): %s\n", internal, get_mon_type_str(internal));
    printf("External monitor (%0#1x): %s\n", external, get_mon_type_str(external));
}

void print_video_subsys_info(){
    unsigned video_mode_code = inp(0x13C6);

    printf("Current adapter (%0#2x): %s, monitor: %s, \n\textended capabilities: %s \n",
            video_mode_code,
            get_adapter_type_str(video_mode_code & va_master_mode),
            (video_mode_code & va_is_internal_mon) ? "internal" : "external",
            (video_mode_code & va_has_extended) ? "yes" : "no"
    );
    video_mode_t vm = get_cur_video_mode();
    printf("Current video mode: %u, columns: %u, current page: %u\n",
           vm.mode, vm.cols, vm.page);
}

void set_to_CGA(){
    unsigned pv = inp(0x13C6);
    pv &= 0xF8;
    outp(0x13C6, pv);

    // BIOS Data Area segment (https://stanislavs.org/helppc/bios_data_area.html):
    char __far * bda_10_ptr = (char __far *) MK_FP( 0x40, 0x10 );
    *bda_10_ptr &= 0x0CF;
    *bda_10_ptr |= 0x020; // Set bits 4-5 to 10b, color 80x25

    union REGS inregs, outregs;
    inregs.w.ax = 0x03;     // Set video mode to 80x25/16 colors (CGA)
    int86(0x10, &inregs, &outregs);// http://www.ctyme.com/intr/rb-0069.htm#Table10
}

void set_to_MDA(){
    unsigned pv = inp(0x13C6);
    pv &= 0xF8;
    pv |= 0x03;
    outp(0x13C6, pv);

    // BIOS Data Area segment (https://stanislavs.org/helppc/bios_data_area.html):
    char __far * bda_10_ptr = (char __far *) MK_FP( 0x40, 0x10 );
    *bda_10_ptr &= 0x0CF;
    *bda_10_ptr |= 0x030; // Set bits 4-5 to 10b, mono 80x25

    union REGS inregs, outregs;
    inregs.w.ax = 0x07;     // Set video mode to 80x25/mono (MDA)
    int86(0x10, &inregs, &outregs);// http://www.ctyme.com/intr/rb-0069.htm#Table10
}

//==monitor===============================================================
void use_external_monitor(){
    union REGS inregs, outregs;
    inregs.w.ax = 0xBF00;     // https://www.ctyme.com/intr/rb-0478.htm
    int86(0x10, &inregs, &outregs);
}

void use_internal_monitor(){
    union REGS inregs, outregs;
    inregs.w.ax = 0xBF01;     // https://www.ctyme.com/intr/rb-0479.htm
    int86(0x10, &inregs, &outregs);
}

//=================================================================================

int main(int argc, char* argv[]){
    if(argc == 1 || argc > 2){
        if(argc > 2){
            puts("Error, too many arguments\n");
        }
        print_usage();
        if(argc > 2){
            exit(7);
        }
    }
    if(!chech_for_compaq()){
        puts("Compaq computer not detected, exiting.\n");
        exit(1);
    }
    if(chech_for_qemm()){
        puts("Compaq computer found but QEMM386 or similar DOS extended detected, "
             "it prohibits using this tool, exiting.\n");
        exit(2);
    }
    if(argc == 1){
        puts("\nCurrent mode:");
        print_CPU_speed();
        print_video_subsys_cap();
        print_video_subsys_info();

        exit(0);
    }

    if(argc == 2){
        str_toupper(argv[1]);

        const char* video_str="VIDEO=";
        size_t video_str_size = strlen(video_str);
        const char* CPU_str="CPU=";
        size_t CPU_str_size = strlen(CPU_str);
        const char* monitor_str="MONITOR=";
        size_t monitor_str_size = strlen(monitor_str);
        const char* print_str="PRINT=";
        size_t print_str_size = strlen(print_str);


        if( strncmp(argv[1], video_str, video_str_size) == 0 ){
            char* arg_ptr = argv[1] + video_str_size;
            if( strcmp(arg_ptr, "CGA") == 0 ){
                set_to_CGA();
                // printf("CGA");
                exit(0);
            } else if ( strcmp(arg_ptr, "MDA") == 0 ){
                set_to_MDA();
                // printf("MDA");
                exit(0);
            } else {
                puts("Wrong arguments for 'video='.\n");
                print_usage();
                exit(9);
            }
        } else
        if( strncmp(argv[1], CPU_str, CPU_str_size) == 0 ){
            char* arg_ptr = argv[1] + CPU_str_size;
            unsigned speed_code = 0;
            unsigned custom_speedcode = 0;
            if( strcmp(arg_ptr, "COMMON") == 0 )
                speed_code = 0;
            else if( strcmp(arg_ptr, "FAST") == 0 )
                speed_code = 1;
            else if( strcmp(arg_ptr, "HIGH") == 0 )
                speed_code = 2;
            else if( strcmp(arg_ptr, "TOGGLE") == 0 )
                speed_code = 3;
            else if( strcmp(arg_ptr, "AUTO") == 0 )
                speed_code = 8;
            else {
                char* end_ptr;
                custom_speedcode = (unsigned)strtol(arg_ptr, &end_ptr, 10);
                    if( end_ptr == arg_ptr ){
                    puts("Wrong arguments for 'CPU='.\n");
                    print_usage();
                    exit(10);
                }
                if( custom_speedcode == 0 || custom_speedcode > 50 ){
                    puts("Wrong arguments for 'CPU='.\n");
                    print_usage();
                    exit(11);
                }
                set_CPU_speed(9, custom_speedcode);
                exit(0);
            }
#ifdef DEBUG_MODE
            printf("\tSpeed code: %u\n", speed_code);
#endif
            int res = set_CPU_speed(speed_code);
            if(res != 0){
                puts("Switching CPU speed failed.\n");
                exit(13);
            } else {
                exit(0);
            }
        } else // puts(" setmode monitor=external|internal");
        if( strncmp(argv[1], monitor_str, monitor_str_size) == 0 ){
            char* arg_ptr = argv[1] + monitor_str_size;
            if( strcmp(arg_ptr, "EXTERNAL") == 0 ){
                use_external_monitor();
                exit(0);
            } else if ( strcmp(arg_ptr, "INTERNAL") == 0 ){
                use_internal_monitor();
                exit(0);
            } else {
                puts("Wrong arguments for 'monitor='.\n");
                print_usage();
                exit(13);
            }
        } else
        if( strncmp(argv[1], print_str, print_str_size) == 0 ){
            char* arg_ptr = argv[1] + video_str_size;
            if( strcmp(arg_ptr, "CPU") == 0 ){
                print_CPU_speed();
                exit(0);
            } else if ( strcmp(arg_ptr, "VIDEO") == 0 ){
                print_video_subsys_info();
                exit(0);
            } else if ( strcmp(arg_ptr, "ALL") == 0 ){
                print_CPU_speed();
                print_video_subsys_cap();
                print_video_subsys_info();
                exit(0);
            } else {
                puts("Wrong arguments for 'print='.\n");
                print_usage();
                exit(12);
            }
        }
        else {
            puts("Wrong arguments.\n");
            print_usage();
            exit(8);
        }
    }

    return 0;
}