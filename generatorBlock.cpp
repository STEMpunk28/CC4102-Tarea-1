#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <chrono>


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <M>\n";
        return 1;
    }
    const size_t TOTAL_BYTES = std::stoll(argv[1]); // N
    const size_t BLOCK_BYTES = 10 * 1024 * 1024; // 10 MB por bloque
    const size_t BLOCK_SIZE = BLOCK_BYTES / sizeof(int64_t); // enteros por bloque
    const size_t TOTAL_ELEMENTS = TOTAL_BYTES / sizeof(int64_t); // total de int64_t
    
    std::ofstream out("input.bin", std::ios::binary);
    if (!out) {
        std::cerr << "Error al crear el archivo input.bin\n";
        return 1;
    }

    std::random_device rd;
    std::mt19937_64 rng(rd());

    std::vector<int64_t> block(BLOCK_SIZE);

    size_t written = 0;
    auto start = std::chrono::high_resolution_clock::now();

    while (written < TOTAL_ELEMENTS) {
        size_t count = std::min(BLOCK_SIZE, TOTAL_ELEMENTS - written);

        for (size_t i = 0; i < count; ++i)
            block[i] = static_cast<int64_t>(written + i);

        std::shuffle(block.begin(), block.begin() + count, rng);
        out.write(reinterpret_cast<char*>(block.data()), count * sizeof(int64_t));
        written += count;

        std::cout << "\rProgreso: " << (100 * written / TOTAL_ELEMENTS) << "%   " << std::flush;
    }

    out.close();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "\nArchivo generado: input.bin (" << TOTAL_BYTES / (1024 * 1024) << " MB)\n";
    std::cout << "Tiempo total: " << elapsed.count() << " segundos\n";
    return 0;
}
