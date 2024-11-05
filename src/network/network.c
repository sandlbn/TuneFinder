#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <proto/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <json-c/json.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "../../include/network.h"
#include "../../include/config.h"
#include "../../include/gui.h"
#include "../../include/utils.h"

char* url_encode(const char *str) {
    if (!str) return NULL;
    
    int len = strlen(str);
    char *encoded = AllocVec(len * 3 + 1, MEMF_CLEAR);
    if (!encoded) return NULL;
    
    char *penc = encoded;
    while (*str) {
        if ((*str >= 'A' && *str <= 'Z') ||
            (*str >= 'a' && *str <= 'z') ||
            (*str >= '0' && *str <= '9') ||
            *str == '-' || *str == '_' || 
            *str == '.' || *str == '~') {
            *penc++ = *str;
        } else if (*str == ' ') {
            *penc++ = '+';
        } else {
            sprintf(penc, "%%%02X", (unsigned char)*str);
            penc += 3;
        }
        str++;
    }
    *penc = '\0';
    
    return encoded;
}

char* build_search_url(const struct SearchParams *params) {
    char *url;
    int pos = 0;
    int space_left;
    
    url = malloc(MAX_URL_LENGTH);
    if (!url) {
        DEBUG("Failed to allocate URL buffer");
        return NULL;
    }
    
    pos = snprintf(url, MAX_URL_LENGTH, "%s?", API_ENDPOINT);
    space_left = MAX_URL_LENGTH - pos;
    
    if (space_left > 0) {
        pos += snprintf(url + pos, space_left, "hidebroken=%s&", 
                       (params->hidebroken || params->hidebroken == 0) ? "1" : "0");
        space_left = MAX_URL_LENGTH - pos;
    }
    
    if (params->is_https != HTTPS_ALL && space_left > 0) {
        pos += snprintf(url + pos, space_left, "is_https=%s&",
                       params->is_https == HTTPS_TRUE ? "true" : "false");
        space_left = MAX_URL_LENGTH - pos;
    }
    
    if (params->name && strlen(params->name) > 2 && space_left > 0) {
        char *encoded_name = url_encode(params->name);
        if (encoded_name) {
            pos += snprintf(url + pos, space_left, "name=%s&", encoded_name);
            space_left = MAX_URL_LENGTH - pos;
            free(encoded_name);
        }
    }
    
    if (params->country_code && space_left > 0) {
        char upper_country[3];
        upper_country[0] = toupper(params->country_code[0]);
        upper_country[1] = toupper(params->country_code[1]);
        upper_country[2] = '\0';
        pos += snprintf(url + pos, space_left, "countrycode=%s&", upper_country);
        space_left = MAX_URL_LENGTH - pos;
    }
    
    if (params->state && strlen(params->state) >= 2 && space_left > 0) {
        char *encoded_state = url_encode(params->state);
        if (encoded_state) {
            pos += snprintf(url + pos, space_left, "state=%s&", encoded_state);
            space_left = MAX_URL_LENGTH - pos;
            free(encoded_state);
        }
    }
    
    if (params->codec && strlen(params->codec) > 2 && space_left > 0) {
        char *encoded_codec = url_encode(params->codec);
        if (encoded_codec) {
            pos += snprintf(url + pos, space_left, "codec=%s&", encoded_codec);
            space_left = MAX_URL_LENGTH - pos;
            free(encoded_codec);
        }
    }
    
    if (params->tag_list && strlen(params->tag_list) > 2 && space_left > 0) {
        char *encoded_tags = url_encode(params->tag_list);
        if (encoded_tags) {
            pos += snprintf(url + pos, space_left, "tagList=%s&", encoded_tags);
            space_left = MAX_URL_LENGTH - pos;
            free(encoded_tags);
        }
    }
    
    if (space_left > 0) {
        const char *limit = params->limit ? params->limit : DEFAULT_LIMIT;
        pos += snprintf(url + pos, space_left, "limit=%s", limit);
    }
    
    if (pos > 0 && url[pos-1] == '&') {
        url[pos-1] = '\0';
    }
    
    DEBUG("Built URL: %s", url);
    return url;
}

char* make_http_request(const struct APISettings *settings, const char *path) {
    int sockfd = -1;
    char *response = NULL;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[512];
    char *chunk_buffer = NULL;
    char *response_buffer = NULL;
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    size_t total_size = 0;
    int bytes_received;
    char *json_start;
    char *new_buffer;
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    char msg[MAX_STATUS_MSG_LEN];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to create socket");
        UpdateStatusMessage(msg);
        return NULL;
    }
    
    
    chunk_buffer = malloc(READ_CHUNK_SIZE);
    response_buffer = malloc(INITIAL_BUFFER_SIZE);
    if (!chunk_buffer || !response_buffer) {
        UpdateStatusMessage("Failed to allocate buffers");
        goto cleanup;
    }

    snprintf(msg, MAX_STATUS_MSG_LEN, "Resolving host: %s", settings->host);
    UpdateStatusMessage(msg);
    DEBUG("Resolving host: %s", settings->host);
    
    server = gethostbyname(settings->host);
    if (!server) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to resolve host");
        UpdateStatusMessage(msg);
        goto cleanup;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(settings->port);

    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    
    DEBUG("Connecting to server");
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        UpdateStatusMessage("Failed to connect to server");
        goto cleanup;
    }
    
    snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: AmigaRadioBrowser/1.0\r\n"
            "Accept: application/json\r\n"
            "Connection: close\r\n"
            "\r\n",
            path, settings->host);
    
    DEBUG("Sending request");
    if (send(sockfd, request, strlen(request), 0) < 0) {
        UpdateStatusMessage("Failed to send request");
        goto cleanup;
    }
    
    DEBUG("Receiving response");
    while (1) {
        struct timeval select_timeout;
        ULONG read_mask;
        
        select_timeout.tv_sec = 5;
        select_timeout.tv_usec = 0;
        
        read_mask = 1L << sockfd;
        
        int ready = WaitSelect(sockfd + 1, &read_mask, NULL, NULL, &select_timeout, NULL);
        if (ready < 0) {
            DEBUG("Select error");
            break;
        }
        if (ready == 0) {
            UpdateStatusMessage("Timeout waiting for data");
            retry_count++;
            if (retry_count >= MAX_RETRIES) {
                DEBUG("Max retries reached");
                break;
            }
            continue;
        }
        
        if (read_mask & (1L << sockfd)) {
            bytes_received = recv(sockfd, chunk_buffer, READ_CHUNK_SIZE - 1, 0);
            if (bytes_received < 0) {
                DEBUG("Error receiving data");
                break;
            }
            if (bytes_received == 0) {
                DEBUG("Connection closed by server");
                break;
            }
            
            DEBUG("Received %d bytes", bytes_received);

            if (total_size + bytes_received + 1 > buffer_size) {
                size_t new_size = buffer_size * 2;
                if (new_size > MAX_BUFFER_SIZE) {
                    DEBUG("Response too large");
                    goto cleanup;
                }
                
                DEBUG("Growing buffer from %lu to %lu bytes", 
                      (unsigned long)buffer_size, (unsigned long)new_size);
                
                new_buffer = realloc(response_buffer, new_size);
                if (!new_buffer) {
                    DEBUG("Failed to grow response buffer");
                    goto cleanup;
                }
                response_buffer = new_buffer;
                buffer_size = new_size;
            }
            
            memcpy(response_buffer + total_size, chunk_buffer, bytes_received);
            total_size += bytes_received;
            response_buffer[total_size] = '\0';
        }
    }
    
    if (total_size == 0) {
        DEBUG("No data received");
        goto cleanup;
    }
    
    DEBUG("Total bytes received: %lu", (unsigned long)total_size);
    
    json_start = strstr(response_buffer, "\r\n\r\n");
    if (json_start) {
        json_start += 4;
        DEBUG("Found JSON content");
    } else {
        DEBUG("Could not find JSON content, using entire response");
        json_start = response_buffer;
    }
    
    response = strdup(json_start);
    
cleanup:
    if (sockfd >= 0) {
        DEBUG("Closing socket");
        CloseSocket(sockfd);
    }
    
    if (response_buffer) {
        free(response_buffer);
    }
    if (chunk_buffer) {
        free(chunk_buffer);
    }
    
    return response;
}

struct RadioStation* parse_stations_json(const char *json_str, int *count) {
    struct json_object *root;
    struct RadioStation *stations = NULL;
    int array_len;
    int i;

    *count = 0;
    
    DEBUG("Parsing JSON string");
    root = json_tokener_parse(json_str);
    if (!root) {
        DEBUG("Failed to parse JSON response");
        return NULL;
    }

    DEBUG("Getting array length");
    array_len = json_object_array_length(root);
    if (array_len <= 0) {
        DEBUG("Empty array or invalid length");
        json_object_put(root);
        return NULL;
    }
    
    DEBUG("Allocating memory for %d stations", array_len);
    stations = calloc(array_len, sizeof(struct RadioStation));
    if (!stations) {
        DEBUG("Memory allocation failed");
        json_object_put(root);
        return NULL;
    }
    
    DEBUG("Parsing individual stations");
    for (i = 0; i < array_len; i++) {
        struct json_object *station_obj = json_object_array_get_idx(root, i);
        struct json_object *name_obj, *url_obj, *codec_obj, *bitrate_obj, *country_obj;
        const char *name, *url, *codec, *country;
        int bitrate;
        
        if (json_object_object_get_ex(station_obj, "name", &name_obj) &&
            json_object_object_get_ex(station_obj, "url", &url_obj) &&
            json_object_object_get_ex(station_obj, "codec", &codec_obj) &&
            json_object_object_get_ex(station_obj, "countrycode", &country_obj) &&
            json_object_object_get_ex(station_obj, "bitrate", &bitrate_obj)) {
            
            name = json_object_get_string(name_obj);
            url = json_object_get_string(url_obj);
            codec = json_object_get_string(codec_obj);
            country = json_object_get_string(country_obj);
            bitrate = json_object_get_int(bitrate_obj);
            
            stations[i].name = strdup(name);
            stations[i].url = strdup(url);
            stations[i].codec = strdup(codec);
            stations[i].country = strdup(country);
            stations[i].bitrate = bitrate;
            
            (*count)++;
        }
    }
    
    json_object_put(root);
    return stations;
}