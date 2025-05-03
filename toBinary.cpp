#include <iostream>
#include <fstream>
#include <cstdint>

int main() {
    // Arreglo de 10 números enteros de 64 bits
    int64_t numeros[10] = {10, 1, 3, 5, 4, 100, -20, 9999, 0, -1};

    // Abrimos el archivo en modo binario
    std::ofstream archivo("test.bin", std::ios::binary);

    if (!archivo) {
        std::cerr << "No se pudo abrir el archivo para escritura.\n";
        return 1;
    }

    // Escribimos todos los números de una vez
    archivo.write(reinterpret_cast<char*>(numeros), sizeof(numeros));

    archivo.close();
    std::cout << "Archivo 'salida.bin' creado correctamente.\n";

    return 0;
}
