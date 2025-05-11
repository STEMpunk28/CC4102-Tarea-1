#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <queue>
#include <algorithm>
#include <cassert>

using namespace std::chrono;

// Constantes globales
const int64_t ELEMENT_SIZE = sizeof(int64_t);  // Tamaño de cada elemento (8 bytes)
const int64_t BLOCK_SIZE = 4096;               // Tamaño de bloque de E/S (4KB)
const int64_t ELEMENTS_PER_BLOCK = BLOCK_SIZE / ELEMENT_SIZE; // Elementos por bloque

// Contadores de operaciones de E/S
long read_io = 0, write_io = 0;

/**
 * Ordena un archivo completo en memoria (para archivos pequeños)
 * @param input_file Archivo de entrada
 * @param output_file Archivo de salida ordenado
 * @param N Número de elementos a ordenar
 */
void sort_in_memory(const std::string& input_file, const std::string& output_file, int64_t N) {
    // Abrir archivo de entrada
    FILE* f = fopen(input_file.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para lectura\n", input_file.c_str());
        exit(1);
    }

    // Leer datos en buffer
    std::vector<int64_t> buf(N);
    for (int64_t i = 0; i < N; i += ELEMENTS_PER_BLOCK) {
        int64_t chunk = std::min(ELEMENTS_PER_BLOCK, N - i);
        fread(&buf[i], ELEMENT_SIZE, chunk, f);
    }
    fclose(f);

    // Ordenar en memoria
    std::sort(buf.begin(), buf.end());

    // Escribir archivo de salida
    FILE* out = fopen(output_file.c_str(), "wb");
    if (!out) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para escritura\n", output_file.c_str());
        exit(1);
    }

    for (int64_t i = 0; i < N; i += ELEMENTS_PER_BLOCK) {
        int64_t chunk = std::min(ELEMENTS_PER_BLOCK, N - i);
        fwrite(&buf[i], ELEMENT_SIZE, chunk, out);
    }
    fclose(out);
}

/**
 * Mezcla múltiples archivos ordenados en uno solo
 * @param input_files Vector con nombres de archivos a mezclar
 * @param output_file Archivo de salida combinado
 */
void merge_external(const std::vector<std::string>& input_files, const std::string& output_file) {
    // Preparar estructuras para cada archivo de entrada
    std::vector<FILE*> input_fps(input_files.size());
    std::vector<std::vector<int64_t>> buffers(input_files.size());
    std::vector<size_t> buffer_pos(input_files.size(), 0);
    std::vector<size_t> buffer_size(input_files.size(), 0);

    // Inicializar: abrir archivos y leer primer bloque de cada uno
    for (size_t i = 0; i < input_files.size(); i++) {
        input_fps[i] = fopen(input_files[i].c_str(), "rb");
        if (!input_fps[i]) {
            fprintf(stderr, "[ERROR] No se pudo abrir %s para lectura\n", input_files[i].c_str());
            exit(1);
        }
        buffers[i].resize(ELEMENTS_PER_BLOCK);
        buffer_size[i] = fread(buffers[i].data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, input_fps[i]);
        read_io++;
    }

    // Preparar archivo de salida
    FILE* out = fopen(output_file.c_str(), "wb");
    if (!out) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para escritura\n", output_file.c_str());
        exit(1);
    }

    // Función de comparación para el min-heap
    auto compare = [](const std::pair<int64_t, size_t>& a, const std::pair<int64_t, size_t>& b) {
        return a.first > b.first;
    };
    
    // Min-heap para manejar el merge
    std::priority_queue<std::pair<int64_t, size_t>, 
                       std::vector<std::pair<int64_t, size_t>>, 
                       decltype(compare)> min_heap(compare);

    // Inicializar heap con primeros elementos de cada archivo
    for (size_t i = 0; i < input_files.size(); i++) {
        if (buffer_size[i] > 0) {
            min_heap.push({buffers[i][0], i});
            buffer_pos[i] = 1;
        }
    }

    // Buffer de salida para escritura por bloques
    std::vector<int64_t> out_buffer;
    out_buffer.reserve(ELEMENTS_PER_BLOCK);

    // Proceso principal de mezcla
    while (!min_heap.empty()) {
        auto [val, idx] = min_heap.top();
        min_heap.pop();
        out_buffer.push_back(val);

        // Escribir bloque completo
        if (out_buffer.size() == ELEMENTS_PER_BLOCK) {
            fwrite(out_buffer.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, out);
            write_io++;
            out_buffer.clear();
        }

        // Avanzar en el buffer actual o leer nuevo bloque
        if (buffer_pos[idx] < buffer_size[idx]) {
            min_heap.push({buffers[idx][buffer_pos[idx]++], idx});
        } else {
            buffer_size[idx] = fread(buffers[idx].data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, input_fps[idx]);
            read_io++;
            buffer_pos[idx] = 0;
            if (buffer_size[idx] > 0) {
                min_heap.push({buffers[idx][buffer_pos[idx]++], idx});
            }
        }
    }

    // Escribir datos restantes
    if (!out_buffer.empty()) {
        fwrite(out_buffer.data(), ELEMENT_SIZE, out_buffer.size(), out);
        write_io++;
    }

    // Limpieza: cerrar archivos
    for (auto fp : input_fps) fclose(fp);
    fclose(out);
}

/**
 * Implementación principal del MergeSort externo
 * @param input_file Archivo de entrada
 * @param output_file Archivo de salida ordenado
 * @param N Número total de elementos a ordenar
 * @param M Tamaño de memoria disponible (en elementos)
 * @param a Aridad (número de vías para el merge)
 */
void mergesort_external(const std::string& input_file, const std::string& output_file, int64_t N, int64_t M, int64_t a) {
    printf("[INFO] Iniciando mergesort externo con N=%ld, M=%ld, aridad=%ld\n", N, M, a);
    fflush(stdout);

    // Caso base: si cabe en memoria, ordenar directamente
    if (N <= M) {
        sort_in_memory(input_file, output_file, N);
        return;
    }

    // Dividir el archivo en 'a' partes
    int64_t block_size = (N + a - 1) / a;
    std::vector<std::string> temp_files;

    FILE* f = fopen(input_file.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s\n", input_file.c_str());
        exit(1);
    }

    // Crear archivos temporales para cada partición
    for (int i = 0; i < a && N > 0; ++i) {
        int64_t current_size = std::min(N, block_size);
        std::vector<int64_t> buffer(current_size);

        // Leer partición
        for (int64_t j = 0; j < current_size; j += ELEMENTS_PER_BLOCK) {
            int64_t chunk = std::min(ELEMENTS_PER_BLOCK, current_size - j);
            fread(&buffer[j], ELEMENT_SIZE, chunk, f);
            read_io++;
        }

        // Escribir archivo temporal
        std::string temp_file = input_file + "_part_" + std::to_string(i);
        FILE* tf = fopen(temp_file.c_str(), "wb");
        if (!tf) {
            fprintf(stderr, "[ERROR] No se pudo crear %s\n", temp_file.c_str());
            exit(1);
        }

        for (int64_t j = 0; j < current_size; j += ELEMENTS_PER_BLOCK) {
            int64_t chunk = std::min(ELEMENTS_PER_BLOCK, current_size - j);
            fwrite(&buffer[j], ELEMENT_SIZE, chunk, tf);
            write_io++;
        }

        fclose(tf);
        temp_files.push_back(temp_file);
        N -= current_size;
    }
    fclose(f);

    // Ordenar recursivamente cada partición
    for (size_t i = 0; i < temp_files.size(); ++i) {
        std::string sorted_temp = temp_files[i] + "_sorted";
        FILE* tf = fopen(temp_files[i].c_str(), "rb");
        fseek(tf, 0, SEEK_END);
        int64_t file_size = ftell(tf);
        fclose(tf);

        int64_t num_elements = file_size / ELEMENT_SIZE;
        mergesort_external(temp_files[i], sorted_temp, num_elements, M, a);
        remove(temp_files[i].c_str());
        temp_files[i] = sorted_temp;
    }

    // Mezclar todas las particiones ordenadas
    merge_external(temp_files, output_file);

    // Eliminar archivos temporales
    for (const auto& temp_file : temp_files) {
        remove(temp_file.c_str());
    }

    printf("[RESULTADO] I/Os totales: %ld (lecturas: %ld, escrituras: %ld)\n", 
           read_io + write_io, read_io, write_io);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida> <N_bytes> <M_bytes> <aridad_inicial>\n", argv[0]);
        return 1;
    }

    // Parsear argumentos
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int64_t N_bytes = atoll(argv[3]);
    int64_t M_bytes = atoll(argv[4]);
    int64_t N = N_bytes / ELEMENT_SIZE;

    printf("[MAIN] Archivo de entrada: %s\n", input_file.c_str());
    printf("[MAIN] Archivo de salida:  %s\n", output_file.c_str());
    printf("[MAIN] Tamaño total de entrada: %ld elementos (%ld bytes)\n", N, N_bytes);
    printf("[MAIN] Memoria disponible:  %ld bytes\n", M_bytes);
    fflush(stdout);

    // Función para probar una aridad específica
    auto test_aridad = [&](int64_t a) -> std::pair<long, long> {
        read_io = 0;
        write_io = 0;
        mergesort_external(input_file, output_file, N, M_bytes / ELEMENT_SIZE, a);
        remove(output_file.c_str()); // limpiar archivo de salida
        return {read_io + write_io, a};
    };

    // Búsqueda ternaria para encontrar la mejor aridad
    int64_t low = 2, high = 512;
    std::pair<long, long> best_result = {LONG_MAX, -1};

    while (high - low > 3) {
        int64_t m1 = low + (high - low) / 3;
        int64_t m2 = high - (high - low) / 3;

        auto r1 = test_aridad(m1);
        auto r2 = test_aridad(m2);

        if (r1.first < r2.first) {
            high = m2;
            best_result = std::min(best_result, r1);
        } else {
            low = m1;
            best_result = std::min(best_result, r2);
        }
    }

    // Búsqueda lineal en el rango reducido
    for (int64_t a = low; a <= high; ++a) {
        auto result = test_aridad(a);
        if (result.first < best_result.first) {
            best_result = result;
        }
    }

    printf("[RESULTADO FINAL] Mejor a = %ld con %ld I/Os totales\n", 
           best_result.second, best_result.first);
    fflush(stdout);

    return 0;
}