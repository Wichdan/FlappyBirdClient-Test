#include <iostream>
#include <SDL.h>
#include <cstdlib>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

SDL_Event event;
using namespace std;

// Variabel untuk burung
int birdY = 300;  // Koordinat Y burung

int birdVelocity = 0;  // Kecepatan vertikal burung

const int gravity = 1;  // Gravitasi untuk menarik burung ke bawah

const int jumpHeight = -10;  // Kecepatan angkat ketika tombol spasi ditekan

// Variabel untuk rintangan
int obstacleX = 800;  // Koordinat X rintangan
int obstacleGap = 300;  // Jarak antara rintangan atas dan bawah

// Variabel skor
int score = 0;

bool isCollision(int birdX, int birdY, int obstacleX, int obstacleGap) {
    // Hitbox (kotak pembatas) burung
    int birdLeft = birdX;
    int birdRight = birdX + 50; // Misalkan lebar burung adalah 50
    int birdTop = birdY;
    int birdBottom = birdY + 50; // Misalkan tinggi burung adalah 50

    // Hitbox rintangan atas
    int obstacleTopLeft = obstacleX;
    int obstacleTopRight = obstacleX + 50; // Misalkan lebar rintangan adalah 50
    int obstacleTopBottom = obstacleGap; // Tinggi rintangan atas

    // Hitbox rintangan bawah
    int obstacleBottomLeft = obstacleX;
    int obstacleBottomRight = obstacleX + 50; // Misalkan lebar rintangan adalah 50
    int obstacleBottomTop = obstacleGap + 100; // Tinggi rintangan bawah

    // Deteksi tabrakan
    if (birdRight >= obstacleTopLeft && birdLeft <= obstacleTopRight && birdBottom >= obstacleTopBottom) {
        return true; // Terjadi tabrakan dengan rintangan atas
    }
    if (birdRight >= obstacleBottomLeft && birdLeft <= obstacleBottomRight && birdTop <= obstacleBottomTop) {
        return true; // Terjadi tabrakan dengan rintangan bawah
    }

    return false; // Tidak terjadi tabrakan
}

// Struktur untuk objek yang akan direplikasi
struct GameObject {
    int id;
    int x;
    int y;
};

// Fungsi untuk mengirim data ke server
void SendData(SOCKET clientSocket, const char* data, int dataSize) {
    send(clientSocket, data, dataSize, 0);
}

int main(int argc, char* argv[]) {
    // Inisialisasi Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return -1;
    }

    // Buat socket client
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create client socket." << std::endl;
        WSACleanup();
        return -1;
    }

    // Set konfigurasi server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Connect ke server
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to server." << std::endl;

    WSANETWORKEVENTS netEvents;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Bird Sync", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    GameObject obj = { 0,0,0 };

    while (true) {

        if (WSAEnumNetworkEvents(clientSocket, NULL, &netEvents) == SOCKET_ERROR) {
            std::cerr << "WSAEnumNetworkEvents failed." << std::endl;
            break;
        }
        // Terima data dari server

        // Data yang dapat dibaca atau koneksi tertutup
        if (netEvents.lNetworkEvents & FD_READ) {
            // Terima data dari server
            char buffer[sizeof(GameObject)];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead > 0) {
                // Data diterima, lakukan replikasi objek
                GameObject* receivedObj = reinterpret_cast<GameObject*>(buffer);

                // Update posisi objek di klien
                obj.x = receivedObj->x;
                obj.y = receivedObj->y;
            }
        }

        // Kirim data objek ke server
        SendData(clientSocket, reinterpret_cast<const char*>(&obj), sizeof(GameObject));

        // Lakukan logika permainan di sini

        // Buat jendela
        //SDL_Window* window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);

        // Buat renderer
        //SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

        // Main game loop
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                // Tombol close jendela ditekan
                // Keluar dari loop utama untuk mengakhiri permainan
                break;
            }
            else if (event.type == SDL_KEYDOWN) {
                // Tombol keyboard ditekan
                if (event.key.keysym.sym == SDLK_w) {
                    // Tombol spasi ditekan, melakukan tindakan seperti mengangkat burung
                    birdVelocity = jumpHeight;
                }
            }
            else if (event.type == SDL_KEYUP) {
                // Tombol keyboard dilepas
                if (event.key.keysym.sym == SDLK_SPACE) {
                    // Tombol spasi dilepas, melakukan tindakan seperti menurunkan burung
                }
            }
        }

        obj.y += birdVelocity;  // Perbarui posisi vertikal burung
        birdVelocity += gravity;  // Terapkan gravitasi pada burung

        // Pergerakan rintangan
        obstacleX -= 5;  // Gerakkan rintangan ke kiri

        // Jika rintangan melewati layar, buat rintangan baru
        if (obstacleX < -100) {
            obstacleX = 800;
            obstacleGap = rand() % 400 + 100;  // Jarak antara rintangan acak
        }

        // Deteksi tabrakan burung dengan rintangan
        if (isCollision(0, obj.y, obstacleX, obstacleGap)) {
            // Terjadi tabrakan, lakukan tindakan sesuai permainan (misalnya, akhir permainan)

        }

        if (obj.y > 600)
            obj.y = 600;
        // Deteksi skor

        //cout << bytesRead << endl;

        // Bersihkan layar
        SDL_SetRenderDrawColor(renderer, 0, 148, 255, 255);  // Warna latar belakang (hitam)
        SDL_RenderClear(renderer);

        // Render burung
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        SDL_Rect birdRect = { obj.x, obj.y, 50, 50 }; // Ukuran dan posisi awal burung
        SDL_RenderFillRect(renderer, &birdRect);

        // Render rintangan atas
        SDL_SetRenderDrawColor(renderer, 76, 255, 0, 255);  // Warna pipa (ijo)
        SDL_Rect obstacleTopRect = { obstacleX, 0, 50, obstacleGap };
        SDL_RenderFillRect(renderer, &obstacleTopRect);

        // Render rintangan bawah
        SDL_SetRenderDrawColor(renderer, 76, 255, 0, 255);  // Warna pipa (ijo)
        SDL_Rect obstacleBottomRect = { obstacleX, obstacleGap + 100, 50, 600 - (obstacleGap + 100) };
        SDL_RenderFillRect(renderer, &obstacleBottomRect);

        // Render skor
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // Warna skor (putih)

        // Implementasikan logika untuk menampilkan skor di layar

        // Render hasilnya
        SDL_RenderPresent(renderer);

        // Delay untuk mengatur kecepatan permainan
        SDL_Delay(16); // Atur sesuai kebutuhan



        // Delay untuk mengatur kecepatan pembaruan
        //Sleep(16); // Sesuaikan sesuai kebutuhan
    }

    // Bersihkan dan hentikan SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    // Delay untuk mengatur kecepatan pembaruan
    //Sleep(16); // Sesuaikan sesuai kebutuhan

    // Clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
