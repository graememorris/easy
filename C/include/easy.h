#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <db.h>

DB *db;
int ns;

int dbcmd(char *str, char *code);

