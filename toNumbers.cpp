#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int main() {
    std::ifstream archivo("salida.bin", std::ios::binary);
    if (!archivo) {
        std::cerr << "No se pudo abrir el archivo." << std::endl;
        return 1;
    }

    std::vector<int64_t> numeros;
    int64_t numero;

    // Lee el archivo de 8 bytes en 8 bytes
    while (archivo.read(reinterpret_cast<char*>(&numero), sizeof(int64_t))) {
        numeros.push_back(numero);
    }

    archivo.close();

    // Imprimir los números leídos
    for (int64_t n : numeros) {
        std::cout << n << std::endl;
    }

    return 0;
}
