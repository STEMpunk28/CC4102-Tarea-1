#include <iostream>
#include <fstream>
#include <cstdint>

/**
 * Verifica si los enteros contenidos en un archivo binario están ordenados de forma creciente.
 *
 * @param archivo Nombre del archivo binario a verificar.
 * 
 * @return `true` si el archivo está ordenado o contiene 0/1 elementos, `false` si hay al menos un par fuera de orden.
 */
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

/**
 * Función principal que recibe como argumento el nombre de un archivo binario
 * y verifica si su contenido está ordenado.
 *
 * Parámetros:
 * @param argc Número de argumentos.
 * @param argv Lista de argumentos. Se espera:
 * * argv[1]: Nombre del archivo a ordenar
 * 
 * @return 0 si se ejecuta correctamente, 1 si hay error de uso.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <nombre_del_archivo>\n";
        return 1;
    }
    std::string archivo = argv[1];
    if (verificarOrden(archivo)) {
        std::cout << "El archivo está ordenado." << std::endl;
    } else {
        std::cout << "El archivo NO está ordenado." << std::endl;
    }
    return 0;
}
