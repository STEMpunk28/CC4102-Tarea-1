#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>

const size_t M = 50 * 1024 * 1024; // Memoria principal = 50MB
const size_t N = 60 * M / sizeof(int64_t); // 60M bytes → cantidad de int64_t que caben

int main() {
    std::vector<int64_t> data(N);

    // Llenar secuencia ordenada
    for (int64_t i = 0; i < static_cast<int64_t>(N); ++i)
        data[i] = i;

    // Desordenar la secuencia (Fisher-Yates shuffle)
    std::random_device rd;
    std::mt19937_64 g(rd());
    std::shuffle(data.begin(), data.end(), g);

    // Guardar en archivo binario
    std::ofstream out("salida.bin", std::ios::binary);
    if (!out) {
        std::cerr << "Error al crear el archivo input.bin\n";
        return 1;
    }

    out.write(reinterpret_cast<char*>(data.data()), N * sizeof(int64_t));
    out.close();

    std::cout << "Archivo input.bin generado con " << N << " enteros de 64 bits.\n";
    return 0;
}
