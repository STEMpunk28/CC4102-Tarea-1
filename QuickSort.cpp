#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <bits/stdc++.h>
using namespace std::chrono;

// Tamaño de un entero de 64 bits (en bytes)
const int64_t ELEMENT_SIZE = sizeof(int64_t);
// Tamaño de un bloque de disco (en bytes)
const int64_t BLOCK_SIZE = 4096;
// Cantidad de elementos que caben en un bloque
const int64_t ELEMENTS_PER_BLOCK = BLOCK_SIZE / ELEMENT_SIZE;

// Contador global de operaciones de lecturas y escrituras realizadas en disco
long total_read_io = 0, total_write_io = 0;

// Contadores de operaciones de lectura y escritura por llamada a quicksort_external
long read_io = 0, write_io = 0;

/**
 * Ordena un archivo binario que contiene enteros de 64 bits usando una versión de Quicksort multi-pivote en memoria externa.
 *
 * Parámetros:
 * @param input_file  Nombre del archivo de entrada que contiene los enteros a ordenar.
 * @param output_file Nombre del archivo de salida donde se guardarán los enteros ya ordenados.
 * @param a           Número de pivotes + 1 que se utilizarán en cada nivel de recursión.
 * @param N           Número total de elementos presentes en el archivo de entrada.
 * @param M           Número máximo de elementos que se pueden cargar en memoria principal.
 *
 */
void quicksort_external(const std::string& input_file, const std::string& output_file, int a, int64_t N, int64_t M) {

    if (N <= M) {
        // Cargar, ordenar en memoria y escribir
        FILE* f = fopen(input_file.c_str(), "rb");
        if (!f) {
            fprintf(stderr, "[ERROR] No se pudo abrir %s para lectura\n", input_file.c_str());
            exit(1);
        }

        std::vector<int64_t> buf(N);
        fread(buf.data(), ELEMENT_SIZE, N, f);
        fclose(f);

        std::sort(buf.begin(), buf.end());

        FILE* out = fopen(output_file.c_str(), "wb");
        if (!out) {
            fprintf(stderr, "[ERROR] No se pudo abrir %s para escritura\n", output_file.c_str());
            exit(1);
        }

        fwrite(buf.data(), ELEMENT_SIZE, N, out);
        fclose(out);
        read_io++;
        return;
    }

    // Seleccionar pivotes aleatoriamente
    int64_t total_blocks = N / ELEMENTS_PER_BLOCK;
    int64_t random_block = rand() % total_blocks;

    FILE* f = fopen(input_file.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para lectura de pivotes\n", input_file.c_str());
        exit(1);
    }
    fseek(f, random_block * BLOCK_SIZE, SEEK_SET);

    std::vector<int64_t> pivot_buf(ELEMENTS_PER_BLOCK);
    fread(pivot_buf.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, f);
    fclose(f);

    // Se mezclan para simular aleatoriedad
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(pivot_buf.begin(), pivot_buf.end(), g);

    // Tomamos los a-1 primeros como pivotes
    std::vector<int64_t> pivots(pivot_buf.begin(), pivot_buf.begin() + a - 1);
    std::sort(pivots.begin(), pivots.end());

    // Preparar archivos de partición
    std::vector<std::string> part_files;
    std::vector<FILE*> parts(a);
    for (int i = 0; i < a; i++) {
        std::string part_name = input_file + "_part_" + std::to_string(i);
        part_files.push_back(part_name);
        parts[i] = fopen(part_name.c_str(), "wb");
        if (!parts[i]) {
            fprintf(stderr, "[ERROR] No se pudo crear archivo de partición %s\n", part_name.c_str());
            exit(1);
        }
    }

    // Leer y repartir los datos según los pivotes
    f = fopen(input_file.c_str(), "rb");
    std::vector<int64_t> read_buf(ELEMENTS_PER_BLOCK);
    std::vector<std::vector<int64_t>> part_buffers(a);

    while (true) {
        size_t elems = fread(read_buf.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, f);
        if (elems == 0) break;
        read_io++;

        for (size_t j = 0; j < elems; j++) {
            int k = 0;
            while (k < a - 1 && read_buf[j] >= pivots[k]) k++;
            part_buffers[k].push_back(read_buf[j]);

            if (part_buffers[k].size() == ELEMENTS_PER_BLOCK) {
                fwrite(part_buffers[k].data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, parts[k]);
                write_io++;
                part_buffers[k].clear();
            }
        }
    }
    fclose(f);

    for (int i = 0; i < a; i++) {
        if (!part_buffers[i].empty()) {
            fwrite(part_buffers[i].data(), ELEMENT_SIZE, part_buffers[i].size(), parts[i]);
            write_io++;
        }
        fclose(parts[i]);
    }

    // Subdividir cada partición
    std::vector<std::string> sorted_parts;
    for (int i = 0; i < a; i++) {
        FILE* pf = fopen(part_files[i].c_str(), "rb");
        if (!pf) {
            fprintf(stderr, "[ERROR] No se pudo abrir %s\n", part_files[i].c_str());
            exit(1);
        }
        fseek(pf, 0L, SEEK_END);
        int64_t bytes = ftell(pf);
        fclose(pf);
    
        int64_t part_n = bytes / ELEMENT_SIZE;
        std::string sorted_name = part_files[i] + "_sorted";
    
        quicksort_external(part_files[i], sorted_name, a, part_n, M);
    
        sorted_parts.push_back(sorted_name);
        remove(part_files[i].c_str());
    }

    // Mezclar las partes ordenadas
    FILE* out = fopen(output_file.c_str(), "wb");
    std::vector<int64_t> merge_buf(ELEMENTS_PER_BLOCK);

    for (int i = 0; i < a; i++) {
        FILE* pf = fopen(sorted_parts[i].c_str(), "rb");
        while (true) {
            size_t elems = fread(merge_buf.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, pf);
            if (elems == 0) break;
            read_io++;

            fwrite(merge_buf.data(), ELEMENT_SIZE, elems, out);
            write_io++;
        }
        fclose(pf);
        remove(sorted_parts[i].c_str());
    }

    fclose(out);

    total_read_io += read_io;
    total_write_io += write_io;
}

/**
 * Función principal. Maneja argumentos de línea de comandos, prepara variables
 * y llama a la función de ordenamiento externo.
 *
 * Parámetros:
 * @param argc Número de argumentos.
 * @param argv Lista de argumentos. Se espera:
 * * argv[1]: nombre del archivo de entrada
 * * argv[2]: nombre del archivo de salida
 * * argv[3]: número de particiones (a)
 * * argv[4]: tamaño en bytes del archivo de entrada
 *
 * @return 0 si todo fue exitoso, 1 si hubo error de uso.
 */
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida> <a> <N_bytes>\n", argv[0]);
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int a = atoi(argv[3]);
    int64_t N_bytes = atoll(argv[4]);
    int64_t N = N_bytes / ELEMENT_SIZE;

    int64_t M = 50 * 1024 * 1024; // 50MB de memoria
    M = M / ELEMENT_SIZE;

    auto start = high_resolution_clock::now();

    quicksort_external(input_file, output_file, a, N, M);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    printf("Tiempo total: %lld ms\n", duration.count());
    printf("I/Os totales: %ld (lecturas: %ld, escrituras: %ld)\n",
        total_read_io + total_write_io, total_read_io, total_write_io);
    
    return 0;
}
