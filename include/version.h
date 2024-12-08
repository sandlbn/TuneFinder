#ifndef VERSION_H
#define VERSION_H

#define VERS "TuneFinder 1.3"
#define VSTRING "TuneFinder  1.3 (12.08.2024)\r\n"
#define VERSTAG "\0$VER: TuneFinder  1.3 (12.08.2024)"
#define AUTHOR "Marcin Spoczynski"
#define TRANSLATION "-German: Thomas Blatt\n-Italian: Samir Hawamdeh\n-French: Eric 'Tarinz' LUCZYSZYN \nIcons: Thomas Blatt\n"
__attribute__((section(".text"))) UBYTE VString[] =
       "" VERSTAG " \r\n";
#endif