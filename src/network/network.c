#include <exec/memory.h>
#include <exec/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "../../include/config.h"
#include "../../include/gui.h"
#include "../../include/locale.h"
#include "../../include/network.h"
#include "../../include/utils.h"

// Not all SDK headers define these; values are the standard bsdsocket ones
#ifndef FIONBIO
#define FIONBIO 0x8004667eUL
#endif
#ifndef SO_ERROR
#define SO_ERROR 0x1007
#endif

#define CONNECT_TIMEOUT_SECS 10

// connect() blocks until the TCP stack gives up, which can take minutes for
// a dead server. Connect non-blocking and wait with a timeout instead.
static BOOL ConnectWithTimeout(int sockfd, struct sockaddr_in *addr, LONG seconds) {
    LONG nonBlocking = 1;
    BOOL connected = FALSE;

    IoctlSocket(sockfd, FIONBIO, (char *)&nonBlocking);

    if (connect(sockfd, (struct sockaddr *)addr, sizeof(*addr)) == 0) {
        connected = TRUE;
    } else if (Errno() == EINPROGRESS) {  // bsdsocket error, errno may not be wired up
        struct timeval tv;
        ULONG write_mask = 1L << sockfd;

        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        if (WaitSelect(sockfd + 1, NULL, &write_mask, NULL, &tv, NULL) > 0) {
            LONG error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                connected = TRUE;
            }
        } else {
            DEBUG("Connect timed out");
        }
    }

    nonBlocking = 0;
    IoctlSocket(sockfd, FIONBIO, (char *)&nonBlocking);
    return connected;
}

static void UpdateSearchStatus(int chunk_count) {
  static int dot_count = 0;
  char dot_buffer[MAX_STATUS_MSG_LEN];
  char dots[4] = {0};
  int i;

  // Create dots string
  for (i = 0; i < (dot_count % 4); i++) {
    dots[i] = '.';
  }
  // Format message with dots
  sprintf(dot_buffer, "%s%s", GetTFString(MSG_SEARCHING), dots);
  UpdateStatusMessage(dot_buffer);
  // Increment dot counter
  dot_count++;
  if (dot_count >= 4) {
    dot_count = 0;
  }
}

char *make_http_request(const struct APISettings *settings, const char *path) {
    int sockfd = -1;
    char *response = NULL;
    struct sockaddr_in server_addr;
    struct hostent *server = NULL;
    char *request = NULL;
    size_t request_len;
    char *chunk_buffer = NULL;
    char *response_buffer = NULL;
    BOOL dns_ok = FALSE;
    size_t buffer_size = PREFERRED_BUFFER_SIZE;
    size_t total_size = 0;
    int bytes_received;
    char *json_start;
    char *new_buffer;
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    int chunk_count = 0;
    
    DEBUG("Starting HTTP request for: %s", settings->host);

    // Create socket first
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_CR_SOC));
        return NULL;
    }

    // Set socket options BEFORE connect
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        DEBUG("Failed to set receive timeout");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        DEBUG("Failed to set send timeout");
    }

    // Allocate buffers
    chunk_buffer = malloc(READ_CHUNK_SIZE);
    response_buffer = malloc(buffer_size);
    if (!chunk_buffer || !response_buffer) {
        if (!response_buffer && buffer_size > INITIAL_BUFFER_SIZE) {
            buffer_size = INITIAL_BUFFER_SIZE;
            response_buffer = malloc(buffer_size);
        }
        if (!chunk_buffer || !response_buffer) {
            UpdateStatusMessage(GetTFString(MSG_FAILED_ALL_BUFF));
            goto cleanup;
        }
    }

    // DNS resolution with protection
    UpdateStatusMessage("Resolving host...");
    DEBUG("DNS lookup for: %s", settings->host);
    
    // bsdsocket keeps the result per library base, no Forbid() needed
    server = gethostbyname((STRPTR)settings->host);
    if (server && server->h_addr) {
        dns_ok = TRUE;
    }

    if (!dns_ok || !server) {
        DEBUG("DNS lookup failed");
        UpdateStatusMessage(GetTFString(MSG_FAILED_RESOLV_HOST));
        goto cleanup;
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    CopyMem(server->h_addr, &server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(settings->port);

    DEBUG("Connecting to server...");
    if (!ConnectWithTimeout(sockfd, &server_addr, CONNECT_TIMEOUT_SECS)) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_CONN_SERV));
        goto cleanup;
    }

    // Prepare and send HTTP request; the path can be up to MAX_URL_LENGTH
    request_len = strlen(path) + strlen(settings->host) + 256;
    request = AllocVec(request_len, MEMF_ANY);
    if (!request) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_ALL_BUFF));
        goto cleanup;
    }
    snprintf(request, request_len,
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "User-Agent: TuneFinder/" VERSION_NUMBER "\r\n"
             "Accept: application/json\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, settings->host);

    DEBUG("Sending request");
    if (send(sockfd, request, strlen(request), 0) < 0) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_SEND_REQ));
        goto cleanup;
    }

    // Receive response
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
            UpdateStatusMessage(GetTFString(MSG_TIMEOUT));
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

            UpdateSearchStatus(chunk_count);
            chunk_count++;

            // Check if buffer needs to grow
            if (total_size + bytes_received + 1 > buffer_size) {
                size_t new_size = buffer_size * 2;
                if (new_size > MAX_BUFFER_SIZE) {
                    new_size = buffer_size + (1024 * 1024);
                    if (new_size > MAX_BUFFER_SIZE) {
                        DEBUG("Response too large");
                        goto cleanup;
                    }
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

    // Only accept "HTTP/1.x 200" responses
    {
        int status = 0;
        if (sscanf(response_buffer, "HTTP/%*d.%*d %d", &status) != 1 || status != 200) {
            DEBUG("Unexpected HTTP status: %d", status);
            goto cleanup;
        }
    }

    // Find start of JSON content
    json_start = strstr(response_buffer, "\r\n\r\n");
    if (!json_start) {
        DEBUG("Malformed HTTP response");
        goto cleanup;
    }
    json_start += 4;

    // Return the body in place instead of duplicating a possibly large buffer
    memmove(response_buffer, json_start, total_size - (json_start - response_buffer) + 1);
    response = response_buffer;
    response_buffer = NULL;

cleanup:
    if (request) {
        FreeVec(request);
    }
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

// Tells Radio Browser a station was played, as its API asks clients to do.
// The answer is not needed; this only counts the click.
BOOL ReportStationClick(const struct APISettings *settings, const char *uuid) {
  char path[64];
  char *response;

  if (!settings || !IsStationUUID(uuid)) return FALSE;

  snprintf(path, sizeof(path), "/json/url/%s", uuid);
  response = make_http_request(settings, path);
  if (!response) return FALSE;
  free(response);
  return TRUE;
}
