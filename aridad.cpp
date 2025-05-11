#include <cstdio>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <unistd.h>
#include <iostream>

// Contador global de operaciones de I/O (lecturas y escrituras)
size_t IO_COUNT = 0;

// Tamaño de bloque en bytes (4KB típico para sistemas modernos)
const size_t B = 4096;

// Número de enteros de 64 bits que caben en un bloque
const size_t ints_per_block = B / sizeof(int64_t);

// Estructura para los nodos del heap usado en el merge
struct HeapNode {
    int64_t value;    // El valor del elemento
    size_t index;     // Índice del archivo del que proviene
    
    // Operador para comparar nodos (necesario para el min-heap)
    bool operator>(const HeapNode& other) const {
        return value > other.value;
    }
};

/**
 * Función que mezcla múltiples archivos ordenados en uno solo
 * @param files Vector con los nombres de los archivos a mezclar
 * @param output_file Nombre del archivo de salida
 * @param start_index Posición inicial en el archivo de salida para escribir
 */
void merge_files(const std::vector<std::string>& files, const std::string& output_file, size_t start_index) {
    size_t k = files.size();
    std::vector<FILE*> ins(k);                     // Handles de archivos de entrada
    std::vector<std::vector<int64_t>> buffers(k);  // Buffers para cada archivo
    std::vector<size_t> indices(k, 0);            // Índices actuales en cada buffer
    std::vector<size_t> sizes(k, 0);              // Tamaños actuales de cada buffer
    
    // Min-heap para obtener siempre el menor elemento disponible
    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<>> heap;

    // Inicialización: abrir archivos y llenar buffers iniciales
    for (size_t i = 0; i < k; ++i) {
        ins[i] = fopen(files[i].c_str(), "rb");
        buffers[i].resize(ints_per_block);
        sizes[i] = fread(buffers[i].data(), sizeof(int64_t), ints_per_block, ins[i]);
        IO_COUNT++;
        
        if (sizes[i] > 0) {
            heap.push({buffers[i][0], i});
        }
    }

    // Preparar archivo de salida
    FILE* out = fopen(output_file.c_str(), "rb+");
    fseek(out, start_index * sizeof(int64_t), SEEK_SET);

    // Buffer de salida para escritura por bloques
    std::vector<int64_t> out_buffer(ints_per_block);
    size_t out_index = 0;

    // Proceso principal de mezcla
    while (!heap.empty()) {
        // Obtener el menor elemento del heap
        auto [val, i] = heap.top();
        heap.pop();

        // Escribir al buffer de salida
        out_buffer[out_index++] = val;
        
        // Si el buffer de salida está lleno, escribir a disco
        if (out_index == ints_per_block) {
            fwrite(out_buffer.data(), sizeof(int64_t), out_index, out);
            IO_COUNT++;
            out_index = 0;
        }

        // Avanzar en el buffer de entrada actual
        if (++indices[i] == sizes[i]) {
            // Leer nuevo bloque si hemos consumido el buffer actual
            sizes[i] = fread(buffers[i].data(), sizeof(int64_t), ints_per_block, ins[i]);
            IO_COUNT++;
            indices[i] = 0;
        }

        // Si hay más elementos en este archivo, agregar al heap
        if (sizes[i] > 0) {
            heap.push({buffers[i][indices[i]], i});
        }
    }

    // Escribir cualquier dato restante en el buffer de salida
    if (out_index > 0) {
        fwrite(out_buffer.data(), sizeof(int64_t), out_index, out);
        IO_COUNT++;
    }

    // Limpieza: cerrar archivos y eliminar temporales
    for (FILE* f : ins) fclose(f);
    fclose(out);

    for (const auto& file : files) std::remove(file.c_str());
}

/**
 * Ordenamiento externo principal
 * @param input_file Archivo de entrada a ordenar
 * @param output_file Archivo de salida ordenado
 * @param a Factor que determina el tamaño de cada run (en bloques)
 */
void external_mergesort(const std::string& input_file, const std::string& output_file, size_t a) {
    // Abrir archivo de entrada y determinar su tamaño
    FILE* in = fopen(input_file.c_str(), "rb");
    fseek(in, 0, SEEK_END);
    size_t file_size = ftell(in);
    size_t total_ints = file_size / sizeof(int64_t);
    rewind(in);

    // Calcular tamaño de cada run (en número de enteros)
    size_t run_size = ints_per_block * a;
    std::vector<std::string> run_files;  // Nombres de archivos temporales
    std::vector<int64_t> buffer(run_size); // Buffer para cada run

    // Fase 1: Crear runs ordenados
    while (true) {
        // Leer un chunk del archivo
        size_t read = fread(buffer.data(), sizeof(int64_t), run_size, in);
        if (read == 0) break;
        IO_COUNT++;

        // Ordenar en memoria
        std::sort(buffer.begin(), buffer.begin() + read);

        // Escribir run ordenado a archivo temporal
        std::string run_file = "run_" + std::to_string(run_files.size()) + ".bin";
        FILE* out = fopen(run_file.c_str(), "wb");
        fwrite(buffer.data(), sizeof(int64_t), read, out);
        IO_COUNT++;
        fclose(out);

        run_files.push_back(run_file);
    }

    fclose(in);

    // Preparar archivo de salida con el tamaño correcto
    FILE* final = fopen(output_file.c_str(), "wb");
    ftruncate(fileno(final), file_size);
    fclose(final);

    // Fase 2: Mezclar todos los runs
    merge_files(run_files, output_file, 0);
}

/**
 * Función auxiliar para ejecutar y medir I/Os
 * @return Cantidad de operaciones I/O realizadas
 */
int run_and_measure(const std::string& input_file, const std::string& output_file, int a) {
    IO_COUNT = 0;
    external_mergesort(input_file, output_file, a);
    return IO_COUNT;
}

/**
 * Encuentra el valor óptimo del parámetro 'a' mediante búsqueda ternaria
 * @return Mejor valor encontrado para 'a'
 */
int best_a(const std::string& input_file, const std::string& output_file) {
    int low = 2, high = B / 8;  // Rango inicial para la búsqueda
    
    // Búsqueda ternaria para reducir el espacio de búsqueda
    while (high - low > 3) {
        int m1 = low + (high - low) / 3;
        int m2 = high - (high - low) / 3;

        int io1 = run_and_measure(input_file, output_file, m1);
        int io2 = run_and_measure(input_file, output_file, m2);

        if (io1 < io2)
            high = m2;
        else
            low = m1;
    }

    // Búsqueda lineal en el rango reducido
    int best = low;
    int min_io = run_and_measure(input_file, output_file, low);
    
    for (int i = low + 1; i <= high; ++i) {
        int io = run_and_measure(input_file, output_file, i);
        if (io < min_io) {
            min_io = io;
            best = i;
        }
    }
    
    std::cout << "Cantidad de I/Os con mejor a: " << min_io << std::endl;
    return best;
}

int main() {
    std::string input = "salida.bin";
    std::string output = "salidas.bin";

    // Encontrar el mejor parámetro 'a' y mostrar resultados
    int mejor_a = best_a(input, output);
    std::cout << "Mejor valor de a encontrado: " << mejor_a << std::endl;

    return 0;
}