#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <bits/stdc++.h>

using namespace std;

#define PORT_VALUE 1

#define GET_HOSTNAME     0
#define GET_CPU_INFO     1
#define GET_CPU_USAGE    2
#define NOT_RECOGNIZED  -1
#define WRONG_METHOD    ""


string processMessage(char* buffer);
int getResourceMethod(string method);
string getHostname();
string getCPUInfo();
string getCPUUsage();