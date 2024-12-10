#ifndef CONFIG_H
#define CONFIG_H

#define V_List 19561
#define V_Node 20079
#define V_cstr 19938
#define API_HOST "de1.api.radio-browser.info"
#define API_PORT 80
#define API_ENDPOINT "/json/stations/search"
#define MAX_URL_LENGTH 2048
#define DEFAULT_LIMIT 100
#define MAX_LIMIT 9999
#define HTTPS_ALL -1
#define HTTPS_TRUE 1
#define HTTPS_FALSE 0
#define INITIAL_BUFFER_SIZE (32 * 1024)      // Start with 32KB
#define MAX_BUFFER_SIZE (2 * 1024 * 1024)    // Maximum 2MB now
#define PREFERRED_BUFFER_SIZE (128 * 1024)   // Prefer 128KB
#define MIN_FREE_MEMORY (2 * 1024 * 1024)    // Require at least 2MB free
#define READ_CHUNK_SIZE (4 * 1024)           // 4KB chunks
#define MAX_STATUS_MSG_LEN 256 
#define PLS_HEADER "[playlist]\n"
#define PLS_NUMBER_OF_ENTRIES "NumberOfEntries=%d\n"
#define PLS_FILE_ENTRY "File%d=%s\n"
#define PLS_TITLE_ENTRY "Title%d=%s\n"
#define PLS_LENGTH_ENTRY "Length%d=-1\n"
#define DEFAULT_PLS_FILENAME "radio.pls"
#define MAX_STATION_NAME 40
#define NO_POSITION 0xFFFFFFFF

#ifdef DEBUG_BUILD
    #define DEBUG(msg, ...) printf("DEBUG [%s:%d]: " msg "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG(msg, ...) ((void)0)  // Does nothing in release build
#endif

#endif /* CONFIG_H */