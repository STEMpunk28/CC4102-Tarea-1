#include <iostream>
#include <fstream>
#include <cstdint>

bool verificarOrden(const std::string &archivo) {
    std::ifstream file(archivo, std::ios::binary); // Abrimos el archivo en modo binario
    if (!file) {
        std::cerr << "No se pudo abrir el archivo." << std::endl;
        return false;
    }

    int64_t anterior, actual;
    // Leer el primer elemento
    if (!file.read(reinterpret_cast<char*>(&anterior), sizeof(anterior))) {
        file.close();
        return true; // Si solo hay un elemento, consideramos que está ordenado
    }

    // Leer los elementos restantes y verificar el orden
    while (file.read(reinterpret_cast<char*>(&actual), sizeof(actual))) {
        if (anterior > actual) {
            file.close();
            return false; // El archivo no está ordenado
        }
        anterior = actual;
    }

    file.close();
    return true; // El archivo está ordenado
}

int main() {
    std::string archivo = "salidas.bin";
    if (verificarOrden(archivo)) {
        std::cout << "El archivo está ordenado." << std::endl;
    } else {
        std::cout << "El archivo NO está ordenado." << std::endl;
    }
    return 0;
}
