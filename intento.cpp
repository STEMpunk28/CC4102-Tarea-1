#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>      // fopen, fread, fwrite, fseek, remove
#include <cstdlib>     // atoi, atol
#include <cstdint>
#include <ctime>
#include <algorithm>
#include <random>

#define BLOCK_SIZE 4096
#define ELEMENT_SIZE sizeof(int64_t)
#define ELEMENTS_PER_BLOCK (BLOCK_SIZE / ELEMENT_SIZE)

int64_t io_counter = 0;

// Función auxiliar para verificar fopen
void check_file_open(FILE* file, const std::string& context, const std::string& filename) {
    if (!file) {
        std::cerr << "Error abriendo archivo en contexto '" << context << "': " << filename << "\n";
        perror("fopen");
        exit(1);
    }
}

// Leer un bloque desde archivo
void read_block(FILE* file, int64_t* buffer, size_t block_id) {
    fseek(file, block_id * BLOCK_SIZE, SEEK_SET);
    fread(buffer, ELEMENT_SIZE, ELEMENTS_PER_BLOCK, file);
    io_counter++;
}

// Escribir un bloque al archivo
void write_block(FILE* file, int64_t* buffer, size_t block_id) {
    fseek(file, block_id * BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, ELEMENT_SIZE, ELEMENTS_PER_BLOCK, file);
    io_counter++;
}

// Ordenar en memoria usando std::sort
void sort_in_memory(int64_t* array, size_t size) {
    std::sort(array, array + size);
}

void external_quicksort(const std::string& input_filename, size_t total_elements, size_t memory_limit, int a, const std::string& output_filename) {
    if (total_elements <= memory_limit) {
        FILE* file = fopen(input_filename.c_str(), "rb");
        check_file_open(file, "lectura total en memoria", input_filename);

        std::vector<int64_t> data(total_elements);
        fread(data.data(), ELEMENT_SIZE, total_elements, file);
        io_counter += (total_elements * ELEMENT_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
        fclose(file);

        sort_in_memory(data.data(), total_elements);

        FILE* out = fopen(output_filename.c_str(), "wb");
        check_file_open(out, "escritura ordenada en memoria", output_filename);
        fwrite(data.data(), ELEMENT_SIZE, total_elements, out);
        io_counter += (total_elements * ELEMENT_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
        fclose(out);
        return;
    }

    FILE* input = fopen(input_filename.c_str(), "rb");
    check_file_open(input, "lectura para pivotes", input_filename);

    size_t total_blocks = (total_elements + ELEMENTS_PER_BLOCK - 1) / ELEMENTS_PER_BLOCK;
    size_t random_block = rand() % total_blocks;

    std::vector<int64_t> buffer(ELEMENTS_PER_BLOCK);
    read_block(input, buffer.data(), random_block);

    std::vector<int64_t> pivots;
    for (int i = 0; i < a - 1; i++) {
        pivots.push_back(buffer[rand() % ELEMENTS_PER_BLOCK]);
    }

    std::sort(pivots.begin(), pivots.end());
    fclose(input);

    std::vector<FILE*> parts(a);
    std::vector<std::string> part_filenames(a);
    for (int i = 0; i < a; i++) {
        part_filenames[i] = input_filename + "_part_" + std::to_string(i) + ".bin";
        parts[i] = fopen(part_filenames[i].c_str(), "wb");
        check_file_open(parts[i], "creación de partición", part_filenames[i]);
    }

    input = fopen(input_filename.c_str(), "rb");
    check_file_open(input, "relectura para particionar", input_filename);
    std::vector<int64_t> read_buf(ELEMENTS_PER_BLOCK);

    for (size_t i = 0; i < total_blocks; i++) {
        size_t elems = fread(read_buf.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, input);
        io_counter++;

        for (size_t j = 0; j < elems; j++) {
            int k = 0;
            while (k < a - 1 && read_buf[j] >= pivots[k]) k++;
            fwrite(&read_buf[j], ELEMENT_SIZE, 1, parts[k]);
        }
    }

    fclose(input);
    for (int i = 0; i < a; i++) fclose(parts[i]);

    std::vector<size_t> sizes(a);
    for (int i = 0; i < a; i++) {
        FILE* part = fopen(part_filenames[i].c_str(), "rb");
        check_file_open(part, "medición de tamaño", part_filenames[i]);
        fseek(part, 0, SEEK_END);
        sizes[i] = ftell(part) / ELEMENT_SIZE;
        fclose(part);
    }

    std::vector<std::string> sorted_part_filenames(a);
    for (int i = 0; i < a; i++) {
        sorted_part_filenames[i] = input_filename + "_sorted_part_" + std::to_string(i) + ".bin";
        external_quicksort(part_filenames[i], sizes[i], memory_limit, a, sorted_part_filenames[i]);
    }

    FILE* output = fopen(output_filename.c_str(), "wb");
    check_file_open(output, "escritura final", output_filename);
    std::vector<int64_t> write_buf(ELEMENTS_PER_BLOCK);

    for (int i = 0; i < a; i++) {
        FILE* part = fopen(sorted_part_filenames[i].c_str(), "rb");
        check_file_open(part, "lectura para merge final", sorted_part_filenames[i]);

        size_t read;
        while ((read = fread(write_buf.data(), ELEMENT_SIZE, ELEMENTS_PER_BLOCK, part)) > 0) {
            fwrite(write_buf.data(), ELEMENT_SIZE, read, output);
            io_counter += 2;
        }
        fclose(part);
        remove(sorted_part_filenames[i].c_str());
        remove(part_filenames[i].c_str());
    }

    fclose(output);
}

// Punto de entrada
int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Uso: " << argv[0] << " archivo.bin num_elementos memoria_max a\n";
        return 1;
    }

    srand(time(NULL));

    std::string filename = argv[1];
    size_t N = std::stoll(argv[2]);
    size_t M = std::stoll(argv[3]);
    int a = std::stoi(argv[4]);

    external_quicksort(filename, N, M, a, "output.bin");

    std::cout << "Total de I/Os: " << io_counter << std::endl;
    return 0;
}
