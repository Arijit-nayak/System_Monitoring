#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <signal.h>  // for kill() and SIGKILL
using namespace std;

// Function to get total and used memory
void getMemoryInfo() {
    ifstream meminfo("/proc/meminfo");
    string key;
    long value;
    string unit;
    long totalMem = 0, freeMem = 0;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") totalMem = value;
        if (key == "MemAvailable:") freeMem = value;
    }

    cout << "Total Memory: " << totalMem / 1024 << " MB\n";
    cout << "Used Memory : " << (totalMem - freeMem) / 1024 << " MB\n";
}




void killProcess() {
    int pid;
    cout << "\nEnter PID to kill (0 to skip): ";
    cin >> pid;

    if (pid == 0) return;  // skip if user doesn’t want to kill anything

    int result = kill(pid, SIGKILL);
    if (result == 0)
        cout << "✅ Process " << pid << " terminated successfully.\n";
    else
        perror("❌ Failed to kill process");
    
    sleep(2); // small delay before refreshing
}

// Function to list processes
void listProcesses() {
    DIR* dir = opendir("/proc");
    struct dirent* entry;

    cout << "\nPID\tProcess Name\tMemory (KB)\n";
    cout << "----------------------------------\n";

    while ((entry = readdir(dir)) != nullptr) {
        // Check if folder name is a number (PID)
        if (isdigit(entry->d_name[0])) {
            string pid = entry->d_name;
            string commPath = "/proc/" + pid + "/comm";
            string statusPath = "/proc/" + pid + "/status";

            ifstream commFile(commPath);
            ifstream statusFile(statusPath);

            string processName;
            getline(commFile, processName);

            string key;
            long mem = 0;
            string unit;
            while (statusFile >> key >> mem >> unit) {
                if (key == "VmRSS:") break;
            }

            cout << pid << "\t" << processName << "\t" << mem << endl;
        }
    }
    closedir(dir);
}

// CPU info (for later enhancement)
void getCPUUsage() {
    ifstream statFile("/proc/stat");
    string cpu;
    long user, nice, system, idle;
    statFile >> cpu >> user >> nice >> system >> idle;

    cout << "\nCPU Stats (user+system): " << (user + system)
         << " | Idle Time: " << idle << endl;
}
float calculateCPUUsage() {
    ifstream statFile("/proc/stat");
    string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    statFile >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    statFile.close();

    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long idleTime = idle + iowait;

    static long prevTotal = 0, prevIdle = 0;
    float usage = 0.0;

    if (prevTotal != 0) {
        long totalDiff = total - prevTotal;
        long idleDiff = idleTime - prevIdle;
        usage = (float)(totalDiff - idleDiff) / totalDiff * 100.0;
    }

    prevTotal = total;
    prevIdle = idleTime;
    return usage;
}
void showUptime() {
    ifstream uptimeFile("/proc/uptime");
    double uptimeSeconds;
    uptimeFile >> uptimeSeconds;

    int hours = uptimeSeconds / 3600;
    int minutes = ((int)uptimeSeconds % 3600) / 60;
    cout << "Uptime: " << hours << " hrs " << minutes << " mins\n";
}



int main() {
    while (true) {
        system("clear");  // Clear the screen

        cout << "===== System Monitor =====\n";
        getMemoryInfo();
        getCPUUsage();
        showUptime();
        cout << "CPU Usage: " << calculateCPUUsage() << "%\n";
        listProcesses();
        killProcess();

        cout << "\nRefreshing in 3 seconds... (Press Ctrl+C to exit)\n";
        sleep(3);  // Wait 3 seconds before refreshing
    }
    return 0;
}


