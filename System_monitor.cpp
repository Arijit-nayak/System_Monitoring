#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <csignal>
#include <vector>
#include <algorithm>
using namespace std;

// Structure to store process info
struct Process {
    int pid;
    string name;
    long memory;     // in KB
    float cpuUsage;  // CPU %
};

// ================= MEMORY INFO =================
void getMemoryInfo() {
    ifstream meminfo("/proc/meminfo");
    string key, unit;
    long value, totalMem = 0, freeMem = 0;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") totalMem = value;
        if (key == "MemAvailable:") freeMem = value;
    }

    cout << "Total Memory: " << totalMem / 1024 << " MB\n";
    cout << "Used Memory : " << (totalMem - freeMem) / 1024 << " MB\n";
}

// ================= CPU USAGE =================
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

// ================= UPTIME =================
void showUptime() {
    ifstream uptimeFile("/proc/uptime");
    double uptimeSeconds;
    uptimeFile >> uptimeSeconds;

    int hours = uptimeSeconds / 3600;
    int minutes = ((int)uptimeSeconds % 3600) / 60;

    cout << "Uptime: " << hours << " hrs " << minutes << " mins\n";
}

// ================= KILL PROCESS =================
void killProcess() {
    int pid;
    cout << "\nEnter PID to kill (0 to skip): ";
    cin >> pid;

    if (pid == 0) return;

    int result = kill(pid, SIGKILL);
    if (result == 0)
        cout << "✅ Process " << pid << " terminated successfully.\n";
    else
        perror("❌ Failed to kill process");

    sleep(2);
}

// ================= GET PROCESS LIST =================
vector<Process> getProcesses() {
    DIR* dir = opendir("/proc");
    struct dirent* entry;
    vector<Process> processes;

    while ((entry = readdir(dir)) != nullptr) {
        if (isdigit(entry->d_name[0])) {
            string pidStr = entry->d_name;
            int pid = stoi(pidStr);

            string commPath = "/proc/" + pidStr + "/comm";
            string statusPath = "/proc/" + pidStr + "/status";

            ifstream commFile(commPath);
            ifstream statusFile(statusPath);

            if (!commFile || !statusFile) continue;

            string processName;
            getline(commFile, processName);

            string key, unit;
            long mem = 0;
            while (statusFile >> key >> mem >> unit) {
                if (key == "VmRSS:") break;
            }

            // CPU usage per process (approximation)
            string statPath = "/proc/" + pidStr + "/stat";
            ifstream statFile(statPath);
            if (!statFile) continue;
            string tmp;
            long utime, stime;
            for (int i = 0; i < 13; i++) statFile >> tmp; // skip first 13 columns
            statFile >> utime >> stime;

            float cpuUsage = (utime + stime) / 100.0; // rough estimate

            processes.push_back({pid, processName, mem, cpuUsage});
        }
    }

    closedir(dir);
    return processes;
}

// ================= DISPLAY PROCESSES =================
void listProcesses(const vector<Process>& processes) {
    cout << "\nPID\tProcess Name\tMemory (KB)\tCPU Usage (%)\n";
    cout << "-------------------------------------------------------\n";
    for (const auto& p : processes) {
        cout << p.pid << "\t" << p.name << "\t" << p.memory << "\t\t" << p.cpuUsage << endl;
    }
}

// ================= MAIN =================
int main() {
    while (true) {
        system("clear");

        cout << "===== SYSTEM MONITOR =====\n";
        getMemoryInfo();
        showUptime();
        cout << "CPU Usage: " << calculateCPUUsage() << "%\n";

        vector<Process> processes = getProcesses();

        int choice;
        cout << "\nSort by: 1) Memory  2) CPU usage → ";
        cin >> choice;

        if (choice == 1)
            sort(processes.begin(), processes.end(),
                 [](const Process& a, const Process& b) { return a.memory > b.memory; });
        else if (choice == 2)
            sort(processes.begin(), processes.end(),
                 [](const Process& a, const Process& b) { return a.cpuUsage > b.cpuUsage; });

        listProcesses(processes);
        killProcess();

        cout << "\nRefreshing in 3 seconds... (Ctrl+C to exit)\n";
        sleep(3);
    }

    return 0;
}
