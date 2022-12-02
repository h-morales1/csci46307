//***************************************************************************
//
//  Herbert Morales
//  Z1959955
//  CSCI-463-MSTR
//
//  I certify that this is my own work and where appropriate an extension
//  of the starter code provided for the assignment.
//
//***************************************************************************

/*
 Copyright (c) 1986, 1993
 The Regents of the University of California.  All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
    must display the following acknowledgement:
 This product includes software developed by the University of
 California, Berkeley and its contributors.
 4. Neither the name of the University nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

    Modifications to make this build & run on Linux by John Winans, 2021
*/

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using std::cerr;
/*
 * This program creates a socket and then begins an infinite loop. Each time
 * through the loop it accepts a connection and prints out messages from it.
 * When the connection breaks, or a termination message comes through, the
 * program accepts a new connection.
 */
/**
 * Print a message to stderr telling the user how to
 * properly run the program and then exiting with a
 * return code of 1.
 *
 * @brief Print a message detailing how to run program.
 *
 ********************************************************************************/
static void usage() {
  cerr << "Usage: server [-l listener-port]" << std::endl;
  exit(1);
}
/**
 *
 * call write() syscall with the fd, buffer and length
 * of bytes we want to write. This is done in a loop
 * until there is nothing left in the buffer to write.
 *
 * @brief used to write all bytes in a buffer
 *
 * @param fd The file descriptor.
 * @param buf buffer holding data
 * @param len size of chunk we want to write.
 *
 * @return status
 ********************************************************************************/
static ssize_t safe_write(int fd, const char *buf, size_t len) {
  while (len > 0) {
    ssize_t wlen = write(fd, buf, len);
    if (wlen == -1) {
      return -1; // write returned an unrecoverable error, errno will be set
    }
    len -= wlen; // reduce the remaining number of bytes to send
    buf += wlen; // advance the buffer pointer past the written data
  }
  return len; // if we get here then we sent the full requested length
}
/**
 * Loop through the chunk of bytes
 * inside the respective buffer and
 * calculate the check sum of with each
 * byte in the buffer and keep count
 * of how many bytes have been processed
 *
 * @brief calculate checksum and byte count
 *
 * @param buffer buffer to process
 * @param rval argument count
 * @param byte_count store byte_count
 * @param check_sum store check sum
 *
 *
 ********************************************************************************/
void process_buf(const char *buf, int rval, uint32_t byte_count,
                 uint16_t check_sum) {
  //
  uint8_t bt;
  for (int i = 0; i < rval; i++) {
    if ((int)buf[i] != 0)
      byte_count++;
    bt = buf[i];
    check_sum += bt;
  }
}
/**
 * This program creates a server with the specified port
 * and accept connections, data will be read and proccessed
 * then a response will be sent to the client, afterwards
 * the connection is terminated and the server listens
 * for another connection.
 *
 * @brief create a server that accepts data
 *
 * @param argc argument count.
 * @param argv array containing all arguments.
 *
 * @return Error or success of connection
 *
 ********************************************************************************/
int main(int argc, char *argv[]) {
  int sock;
  socklen_t length;
  struct sockaddr_in server;
  int msgsock;
  char buf[1024];
  int rval;
  uint32_t byte_count = 0;
  uint16_t check_sum = 0;
  uint32_t port_num = 0;

  int opt;
  while ((opt = getopt(argc, argv, "l:")) != -1) {
    switch (opt) {
    case 'l': {
      std::istringstream iss(optarg);
      iss >> port_num;
    } break;

    default:
      usage();
    }
  }

  signal(SIGPIPE, SIG_IGN); // handle SIGPIPE, dont terminate

  /* Create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("opening stream socket");
    exit(1);
  }
  /* Name socket using wildcards */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port_num);

  if (bind(sock, (sockaddr *)&server, sizeof(server))) {
    perror("binding stream socket");
    exit(1);
  }
  /* Find out assigned port number and print it out */
  length = sizeof(server);
  if (getsockname(sock, (sockaddr *)&server, &length)) {
    perror("getting socket name");
    exit(1);
  }
  printf("Socket has port #%d\n", ntohs(server.sin_port));

  /* Start accepting connections */
  listen(sock, 5);
  do {
    struct sockaddr_in from; // used to display address of the connection peer
    socklen_t from_len = sizeof(from);
    msgsock = accept(sock, (struct sockaddr *)&from, &from_len);
    if (msgsock == -1)
      perror("accept");
    else {
      inet_ntop(from.sin_family, &from.sin_addr, buf, sizeof(buf));
      printf("Accepted connection from '%s', port %d\n", buf,
             ntohs(from.sin_port));
      do {
        if ((rval = read(msgsock, buf, sizeof(buf))) < 0)
          perror("reading stream message");
        if (rval == 0)
          std::cout << "Ending connection" << std::endl;
        else {
          process_buf(buf, rval, byte_count, check_sum);
        }
      } while (rval != 0);

      sleep(1);
      // one of these is likely to receive a SIGPIPE and terminate the server
      std::ostringstream os;
      os << "Sum: " << check_sum << " Len: " << byte_count << ".";
      std::string str = os.str();
      const char *ch = str.c_str();

      rval = safe_write(msgsock, ch, str.size());

      close(msgsock);
    }
  } while (true);
  /*
   * Since this program has an infinite loop, the socket "sock" is
   * never explicitly closed.  However, all sockets will be closed
   * automatically when a process is killed or terminates normally.
   */
  return 0;
}
