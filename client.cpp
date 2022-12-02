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
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
using std::cerr;

/**
 * Print a message to stderr telling the user how to
 * properly run the program and then exiting with a
 * return code of 1.
 *
 * @brief Print a message detailing how to run program.
 *
 ********************************************************************************/
static void usage() {
  cerr << "Usage: client [-s server-ip] server-port" << std::endl;
  cerr << "    -s Specify ther servers IPv4 number in dotted-quad format"
       << std::endl;
  cerr << "    -The server port number to which the client must connect"
       << std::endl;
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
 *
 * create buffer and place bytes of the server
 * response into that buffer until nothing is left
 * to write, then print that buffer to stdout.
 *
 * @brief print response from server
 *
 * @param fd The file descriptor.
 *
 * @return status
 ********************************************************************************/
static int print_response(int fd) {
  char buf[1024];
  int rval = 1; // prime the while loop

  while (rval > 0) {
    if ((rval = read(fd, buf, sizeof(buf) - 1)) == -1) {
      perror("reading stream message");
      return -1; // error happened
    } else if (rval > 0) {
      buf[rval] = '\0';
      std::cout << buf << std::endl;
    }
  }
  return 0; // all went ok
}
/**
 *
 * Use the syscall read() and plug in the file descriptor
 * the buffer to hold the data and the amount of data
 * we want to read. If the return value of read() is -1,
 * the function will return that value.
 * Then it will use the safe_write() to send data to
 * server, finally it will return the numbers
 * written from read().
 *
 * @brief read from stdin
 *
 * @param fd The file descriptor.
 * @param buffer buffer to hold file data.
 * @param len Size of the chunk you want to read from the buffer.
 *
 * @return numbers written from the read syscall.
 ********************************************************************************/
int r_std_input(const char *buffer, int fd, size_t len) {
  // read standard input
  ssize_t total = 0;
  ssize_t nr;
  // read stdinput until EOF is indicated
  do {
    nr = read(0, &buffer, sizeof(buffer));
    total += nr;
    if (nr == -1) {
      return nr;
    }

    safe_write(fd, buffer, len);

  } while (nr != 0);

  return nr;
}

/**
 * This program creates a socket and initiates a connection with
 * the socket given in the command line. Data from stdin will be sent
 * over the connection and await a response from the server, after the
 * response is read, the connection will close.
 *
 * @brief connect to a server and send data
 *
 * @param argc argument count.
 * @param argv array containing all arguments.
 *
 * @return Error or success of connection
 *
 ********************************************************************************/
int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in server;
  std::string ip_addr = "127.0.0.1";
  char buf[1024] = {0};

  int opt;
  while ((opt = getopt(argc, argv, "s:")) != -1) {
    switch (opt) {
    case 's': {
      std::istringstream iss(optarg);
      iss >> ip_addr;
      std::cout << "ip: " << ip_addr << std::endl;
    } break;

    default:
      usage();
    }
  }

  if (optind >= argc) { // missing port
    usage();
  }
  /* Create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("opening stream socket");
    exit(1);
  }
  /* Connect socket using name specified by command line. */
  server.sin_family = AF_INET;
  const char *conv_ip_addr = ip_addr.c_str(); // convert string ip to c_str

  if (inet_pton(AF_INET, conv_ip_addr, &server.sin_addr) <= 0) {
    std::cerr << conv_ip_addr << ": unknown host" << std::endl;
    exit(2);
  }

  server.sin_port = htons(atoi(argv[optind]));

  if (connect(sock, (sockaddr *)&server, sizeof(server)) < 0) {
    perror("connecting stream socket");
    exit(1);
  }
  ssize_t len;
  len = r_std_input(buf, sock, sizeof(buf));

  if (len == -1) {
    perror("Reading input: ");
    exit(5);
  }

  if (safe_write(sock, buf, sizeof(buf)) < 0)
    perror("writing on stream socket");

  // half close the socket
  shutdown(
      sock,
      SHUT_WR); // cause any subsequent read() calls to return EOF on serverside

  print_response(sock);

  close(sock);

  return 0;
}
