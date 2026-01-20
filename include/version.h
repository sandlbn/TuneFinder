#ifndef VERSION_H
#define VERSION_H

#define VERS "TuneFinder 1.4"
#define VSTRING "TuneFinder  1.4 (20.1.2026)\r\n"
#define VERSTAG "\0$VER: TuneFinder  1.4 (20.1.2026)"
#define AUTHOR "Marcin Spoczynski"
#define TRANSLATION "-German: Thomas Blatt\n-Italian: Samir Hawamdeh\n-French: Eric 'Tarzin' LUCZYSZYN \nIcons: Thomas Blatt\n"
__attribute__((section(".text"))) UBYTE VString[] =
       "" VERSTAG " \r\n";
#endif