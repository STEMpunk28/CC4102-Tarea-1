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

// Tamaño de un entero de 64 bits
const int64_t ELEMENT_SIZE = sizeof(int64_t);

// Tamaño de un bloque (en bytes)
const int64_t BLOCK_SIZE = 4096;

// Cantidad de elementos int64_t que caben en un bloque
const int64_t ELEMENTS_PER_BLOCK = BLOCK_SIZE / ELEMENT_SIZE;

// Contador global de operaciones de lecturas y escrituras totales realizadas en disco
long total_read_io = 0, total_write_io = 0;

// Contadores globales de operaciones de lectura/escritura en disco
long read_io = 0, write_io = 0;

/**
 * Ordena en memoria un archivo binario.
 * 
 * @param input_file Nombre del archivo de entrada con los datos a ordenar.
 * @param output_file Nombre del archivo donde se guardarán los datos ordenados.
 * @param N Número total de elementos (int64_t) a ordenar.
 * 
 * Esta función lee el archivo en bloques, los carga en un vector,
 * los ordena en memoria usando std::sort y los escribe al archivo de salida.
 */
void sort_in_memory(const std::string& input_file, const std::string& output_file, int64_t N) {
    FILE* f = fopen(input_file.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para lectura\n", input_file.c_str());
        exit(1);
    }

    std::vector<int64_t> buf(N);
    for (int64_t i = 0; i < N; i += ELEMENTS_PER_BLOCK) {
        int64_t chunk = std::min(ELEMENTS_PER_BLOCK, N - i);
        fread(&buf[i], ELEMENT_SIZE, chunk, f);
    }
    fclose(f);

    std::sort(buf.begin(), buf.end());

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
 * Mezcla varios archivos ordenados en un solo archivo de salida ordenado.
 * 
 * @param input_files Vector de nombres de archivos que ya están ordenados individualmente.
 * @param output_file Nombre del archivo donde se escribirá la mezcla final ordenada.
 * 
 * Usa un heap mínimo para realizar la fusión de k-vías. 
 * Se leen y escriben los datos en bloques de tamaño fijo.
 */
void merge_external(const std::vector<std::string>& input_files, const std::string& output_file) {
    std::vector<FILE*> input_fps(input_files.size());
    std::vector<std::vector<int64_t>> buffers(input_files.size());
    std::vector<size_t> buffer_pos(input_files.size(), 0);
    std::vector<size_t> buffer_size(input_files.size(), 0);

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

    FILE* out = fopen(output_file.c_str(), "wb");
    if (!out) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s para escritura\n", output_file.c_str());
        exit(1);
    }

    auto compare = [](const std::pair<int64_t, size_t>& a, const std::pair<int64_t, size_t>& b) {
        return a.first > b.first;
    };
    std::priority_queue<std::pair<int64_t, size_t>, std::vector<std::pair<int64_t, size_t>>, decltype(compare)> min_heap(compare);

    for (size_t i = 0; i < input_files.size(); i++) {
        if (buffer_size[i] > 0) {
            min_heap.push({buffers[i][0], i});
            buffer_pos[i] = 1;
        }
    }

    std::vector<int64_t> out_buffer;
    out_buffer.reserve(ELEMENTS_PER_BLOCK);

    while (!min_heap.empty()) {
        auto [val, idx] = min_heap.top();
        min_heap.pop();
        out_buffer.push_back(val);

        if (out_buffer.size() == ELEMENTS_PER_BLOCK) {
            fwrite(out_buffer.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, out);
            write_io++;
            out_buffer.clear();
        }

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

    if (!out_buffer.empty()) {
        fwrite(out_buffer.data(), ELEMENT_SIZE, out_buffer.size(), out);
        write_io++;
    }

    for (auto fp : input_fps) fclose(fp);
    fclose(out);
}

/**
 * Implementa el algoritmo de mergesort externo sobre archivos.
 * 
 * @param input_file Nombre del archivo de entrada (datos no ordenados).
 * @param output_file Nombre del archivo de salida donde se guardarán los datos ordenados.
 * @param N Número total de elementos (int64_t) en el archivo de entrada.
 * @param M Cantidad máxima de elementos que caben en memoria (según M_bytes / ELEMENT_SIZE).
 * @param a Aridad del algoritmo: número de particiones a generar (divide el archivo en 'a' bloques).
 * 
 * Si los datos caben en memoria, usa `sort_in_memory`.
 * Si no, divide el archivo en 'a' partes, ordena cada parte recursivamente y luego las fusiona.
 */
void mergesort_external(const std::string& input_file, const std::string& output_file, int64_t N, int64_t M, int64_t a) {

    if (N <= M) {
        sort_in_memory(input_file, output_file, N);
        return;
    }

    int64_t block_size = (N + a - 1) / a;
    std::vector<std::string> temp_files;

    FILE* f = fopen(input_file.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir %s\n", input_file.c_str());
        exit(1);
    }

    for (int i = 0; i < a && N > 0; ++i) {
        int64_t current_size = std::min(N, block_size);
        std::vector<int64_t> buffer(current_size);

        for (int64_t j = 0; j < current_size; j += ELEMENTS_PER_BLOCK) {
            int64_t chunk = std::min(ELEMENTS_PER_BLOCK, current_size - j);
            fread(&buffer[j], ELEMENT_SIZE, chunk, f);
            read_io++;
        }

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

    merge_external(temp_files, output_file);

    for (const auto& temp_file : temp_files) {
        remove(temp_file.c_str());
    }

    total_read_io += read_io;
    total_write_io += write_io;
}

/**
 * Función principal del programa.
 * 
 * @param argc Número de argumentos (debe ser 6).
 * @param argv Argumentos: 
 *    [1] archivo de entrada,
 *    [2] archivo de salida,
 *    [3] N_bytes: tamaño total del archivo de entrada en bytes,
 *    [4] M_bytes: memoria disponible en bytes,
 *    [5] aridad a (cantidad de divisiones recursivas).
 * 
 * @return 0 si termina exitosamente, 1 en caso de error.
 */
int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida> <N_bytes> <M_bytes> <aridad_a>\n", argv[0]);
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int64_t N_bytes = atoll(argv[3]);
    int64_t M_bytes = atoll(argv[4]);
    int64_t a = atoll(argv[5]);
    int64_t N = N_bytes / ELEMENT_SIZE;

    auto start = high_resolution_clock::now();
    mergesort_external(input_file, output_file, N, M_bytes / ELEMENT_SIZE, a);
    auto end = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);

    printf("Tiempo total: %lld ms\n", duration.count());
    printf("I/Os totales: %ld (lecturas: %ld, escrituras: %ld)\n",
        total_read_io + total_write_io, total_read_io, total_write_io);

    return 0;
}
