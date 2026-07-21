#include <iostream>

#include <vector>
#include <thread>

#include <atomic>
#include <chrono>
#include <csignal>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/resource.h>

#include <cstring>

// Global atomic variables for thread-safe state tracking
std::atomic<bool> g_running{true};
std::atomic<int> g_active_connections{0};
std::atomic<int> g_failed_connections{0};

// Handle Ctrl+C to trigger a graceful exit
void signal_handler(int signum) {
    std::cout << "\n[TEST] Stopping threads gracefully... Please wait.\n";
    g_running = false;
}

// Function to configure timeouts so client threads don't block forever
void set_socket_timeouts(int sock) {
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

void worker_thread(std::string ip, int port, int target_connections, int delay_ms) {
    std::vector<int> my_sockets;
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    // Phase 1: Connect
    for (int i = 0; i < target_connections && g_running; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) continue;

        set_socket_timeouts(sock);

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            my_sockets.push_back(sock);
            g_active_connections++;
            
            // Send a ping to register with the reactor
            std::string msg = "Ping";
            send(sock, msg.c_str(), msg.size(), 0);
        } else {
            g_failed_connections++;
            close(sock);
        }

        // Delay to prevent network interface flooding
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }

    // Phase 2: Hold connections open until the test stops
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Phase 3: Cleanup
    for (int sock : my_sockets) {
        close(sock);
        g_active_connections--;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./stress_test <TOTAL_CONNECTIONS>\n";
        return 1;
    }

    std::string ip = "127.0.0.1";
    int port = 8080;
    int total_conns = std::stoi(argv[1]);
    
    // SAFETY CHECK: Verify OS File Descriptor limits
    struct rlimit rlim;
    getrlimit(RLIMIT_NOFILE, &rlim);
    if (total_conns > rlim.rlim_cur) {
        std::cerr << "[ERROR] Requested " << total_conns << " connections, but OS limit is " << rlim.rlim_cur << ".\n";
        std::cerr << "Run 'ulimit -n " << (total_conns + 100) << "' before testing.\n";
        return 1;
    }

    std::signal(SIGINT, signal_handler);

    int num_threads = 10;
    int conns_per_thread = total_conns / num_threads;
    std::vector<std::thread> threads;

    std::cout << "[TEST] Launching " << total_conns << " connections across " << num_threads << " threads...\n";

    for (int i = 0; i < num_threads; i++) {
        // 5ms delay between connections per thread for a smooth ramp-up
        threads.emplace_back(worker_thread, ip, port, conns_per_thread, 5); 
    }

    // Monitoring Loop
    while (g_running) {
        std::cout << "Active: " << g_active_connections << " | Failed: " << g_failed_connections << "\r" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (g_active_connections + g_failed_connections >= total_conns) {
            std::cout << "\n[TEST] Target reached. Holding connections open. Press Ctrl+C to exit.\n";
            while(g_running) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    std::cout << "\n[TEST] Shutdown complete. Final Active: " << g_active_connections << "\n";
    return 0;
}