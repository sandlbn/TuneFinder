#ifndef VERSION_H
#define VERSION_H

#define VERS "TuneFinder 1.3a"
#define VSTRING "TuneFinder  1.3a (12.09.2024)\r\n"
#define VERSTAG "\0$VER: TuneFinder  1.3a (12.09.2024)"
#define AUTHOR "Marcin Spoczynski"
#define TRANSLATION "-German: Thomas Blatt\n-Italian: Samir Hawamdeh\n-French: Eric 'Tarzin' LUCZYSZYN \nIcons: Thomas Blatt\n"
__attribute__((section(".text"))) UBYTE VString[] =
       "" VERSTAG " \r\n";
#endif