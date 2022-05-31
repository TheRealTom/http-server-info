#include "main.h"

int main(int argc, char const *argv[]) {
    // Port value check
    if (argc == 1) {
        perror("ERROR: no port value");
        exit(EXIT_FAILURE);
    }

    int serverSocket, newSocket, readv;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];

    // Creating server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("ERROR: socket failed");
        exit(EXIT_FAILURE);
    }

    // Init server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[PORT_VALUE]));

    // Attaching socket to the given port
    if (bind(serverSocket, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("ERROR: bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 1) < 0) {
        perror("ERROR: listen");
        exit(EXIT_FAILURE);
    }

    // Runtime
    while (1) {
        if ((newSocket = accept(serverSocket, (struct sockaddr *) &address,
                                 (socklen_t *) &addrlen)) < 0) {
            perror("ERROR: accept");
            exit(EXIT_FAILURE);
        }
        readv = read(newSocket, buffer, 1024);

        string responseMessage = "";

        // Get response
        if (readv > 0) {
            responseMessage = processMessage(buffer);
        }

        string response;
        // Return
        if (responseMessage == "") {
            response = "HTTP/1.1 400 BAD_REQUEST \r\n Content-Type: text/html \r\n\r\n400 Bad Request\n";
        } else {
            response = "HTTP/1.1 200 OK \r\n Content-Type: text/html \r\n\r\n";
            response += responseMessage;
        }
        send(newSocket, response.data(), response.length(), 0);

        close(newSocket);
    }
    return 0;
}

/**
 * Process HEADER information
 */
string processMessage(char* buffer) {
    string request = buffer;
    if (request.compare(0, 3, "GET") == 0) {
        switch (getResourceMethod(request)) {
            case GET_HOSTNAME:
                return getHostname();
            case GET_CPU_INFO:
                return getCPUInfo();
            case GET_CPU_USAGE:
                return getCPUUsage();
            default:
                return WRONG_METHOD;
        }
    }
    return WRONG_METHOD;
}

/**
 * Determines which method is passed through HTTP
 */
int getResourceMethod(string method) {
    if (method.compare(4, 10, "/hostname ") == 0) {
        return GET_HOSTNAME;
    } else if (method.compare(4, 10, "/cpu-name ") == 0) {
        return GET_CPU_INFO;
    } else if (method.compare(4, 6, "/load ") == 0) {
        return GET_CPU_USAGE;
    }
    return NOT_RECOGNIZED;
}


string getHostname() {
    // gets the hostname from terminal
    FILE *fp = popen("cat /proc/sys/kernel/hostname", "r");

    if (fp == nullptr) {
        perror("ERROR: cannot get hostname");
        exit(EXIT_FAILURE);
    }

    // gets the line and returns it
    char charLine[1024];
    while (fgets(charLine, 1024,fp)) {
        string stringLine = charLine;
        return stringLine;
    }

    // something went wrong
    return "";
}

/*
 * 1. get cpu info from terminal
 * 2. search for model name
 * 3. gets just one line, since every line is the same
 * 4. gets only the name of CPU from the line
 * 5. trims the whitespace
 */
string getCPUInfo() {
    FILE *fp = popen(
            "cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk '{$1=$2=$3=\"\"; print $0}' | sed -r 's/\\s*(.*?)\\s*$/\\1/'",
            "r");
    if (fp == nullptr) {
        perror("ERROR: cannot get cpuinfo");
        exit(EXIT_FAILURE);
    }

    // gets the line and returns it
    char charLine[1024];
    while (fgets(charLine, 1024,fp)) {
        string stringLine = charLine;
        return stringLine;
    }

    // something went wrong
    return "";
}

/*
 * Parse and calculates CPU Usage from /proc/stat
 */
string getCPUUsage() {
    FILE *fpPrevious = popen("cat /proc/stat | head -n 1 | awk '{$1=\"\"; print $0}' | sed -r 's/\\s*(.*?)\\s*$/\\1/'",
                             "r");
    if (fpPrevious == nullptr) {
        perror("ERROR: cannot get cpuinfo");
        exit(EXIT_FAILURE);
    }

    sleep(1);

    FILE *fpCurrent = popen("cat /proc/stat | head -n 1 | awk '{$1=\"\"; print $0}' | sed -r 's/\\s*(.*?)\\s*$/\\1/'    ",
                            "r");
    if (fpCurrent == nullptr) {
        perror("ERROR: cannot get cpuinfo");
        exit(EXIT_FAILURE);
    }

    // make arrays
    char charLinePrev[1024];
    unsigned long long int prevValues[10];

    while (fgets(charLinePrev, 1024,fpPrevious)) {
        string stringLine = charLinePrev;
        int index = 0;
        stringstream ss(stringLine);
        string word;
        while (ss >> word) {
            prevValues[index] = stoull(word);
            index++;
        }
    }

    char charLineCurrent[1024];
    unsigned long long int currentValues[10];

    while (fgets(charLineCurrent, 1024,fpCurrent)) {
        string stringLine = charLineCurrent;
        int index = 0;
        stringstream ss(stringLine);
        string word;
        while (ss >> word) {
            currentValues[index] = stoull(word);
            index++;
        }
    }

    // Calculate CPU load
    // PrevIdle = previdle + previowait
    signed long long int prevIdle = prevValues[3] + prevValues[4];

    // Idle = idle + iowait
    signed long long int currentIdle = currentValues[3] + currentValues[4];
    // PrevNonIdle = prevuser + prevnice + prevsystem + previrq + prevsoftirq + prevsteal
    signed long long int prevNonIdle =
            prevValues[0] + prevValues[1] + prevValues[2]
            + prevValues[5] + prevValues[6] + prevValues[7];
    // NonIdle = user + nice + system + irq + softirq + steal
    signed long long int currentNonIdle =
            currentValues[0] + currentValues[1] + currentValues[2]
            + currentValues[5] + currentValues[6] + currentValues[7];
    // PrevTotal = PrevIdle + PrevNonIdle
    signed long long int prevTotal = prevIdle + prevNonIdle;
    // Total = Idle + NonIdle
    signed long long int currentTotal = currentIdle + currentNonIdle;
    // Differentiate: actual value minus the previous one
    // totald = Total - PrevTotal
    signed long long int totalD = currentTotal - prevTotal;
    // idled = Idle - PrevIdle
    signed long long int idleD = currentIdle - prevIdle;
    // CPU_Percentage = (totald - idled)/totald
    signed long long int CPULoadPercentage = ((totalD - idleD) * 100) / totalD;
    string CPULoadPercentageString = to_string(CPULoadPercentage) + "%\n";

    // everything OK
    return CPULoadPercentageString;
}