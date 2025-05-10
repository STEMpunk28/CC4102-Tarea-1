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

size_t IO_COUNT = 0;  // contador global de I/Os
const size_t B = 4096; // Tamaño de bloque en bytes
const size_t ints_per_block = B / sizeof(int64_t);  // número de enteros por bloque

struct HeapNode {
    int64_t value;
    size_t index;
    bool operator>(const HeapNode& other) const {
        return value > other.value;
    }
};

void merge_files(const std::vector<std::string>& files, const std::string& output_file, size_t start_index) {
    size_t k = files.size();
    std::vector<FILE*> ins(k);
    std::vector<std::vector<int64_t>> buffers(k);
    std::vector<size_t> indices(k, 0), sizes(k, 0);
    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<>> heap;

    for (size_t i = 0; i < k; ++i) {
        ins[i] = fopen(files[i].c_str(), "rb");
        buffers[i].resize(ints_per_block);
        sizes[i] = fread(buffers[i].data(), sizeof(int64_t), ints_per_block, ins[i]); IO_COUNT++;
        if (sizes[i] > 0) heap.push({buffers[i][0], i});
    }

    FILE* out = fopen(output_file.c_str(), "rb+");
    fseek(out, start_index * sizeof(int64_t), SEEK_SET);

    std::vector<int64_t> out_buffer(ints_per_block);
    size_t out_index = 0;

    while (!heap.empty()) {
        auto [val, i] = heap.top(); heap.pop();

        out_buffer[out_index++] = val;
        if (out_index == ints_per_block) {
            fwrite(out_buffer.data(), sizeof(int64_t), out_index, out); IO_COUNT++;
            out_index = 0;
        }

        if (++indices[i] == sizes[i]) {
            sizes[i] = fread(buffers[i].data(), sizeof(int64_t), ints_per_block, ins[i]); IO_COUNT++;
            indices[i] = 0;
        }

        if (sizes[i] > 0) {
            heap.push({buffers[i][indices[i]], i});
        }
    }

    if (out_index > 0) {
        fwrite(out_buffer.data(), sizeof(int64_t), out_index, out); IO_COUNT++;
    }

    for (FILE* f : ins) fclose(f);
    fclose(out);

    for (const auto& file : files) std::remove(file.c_str());
}

void external_mergesort(const std::string& input_file, const std::string& output_file, size_t a) {
    FILE* in = fopen(input_file.c_str(), "rb");
    fseek(in, 0, SEEK_END);
    size_t file_size = ftell(in);
    size_t total_ints = file_size / sizeof(int64_t);
    rewind(in);

    size_t run_size = ints_per_block * a;
    std::vector<std::string> run_files;
    std::vector<int64_t> buffer(run_size);

    while (true) {
        size_t read = fread(buffer.data(), sizeof(int64_t), run_size, in);
        if (read == 0) break;
        IO_COUNT++;

        std::sort(buffer.begin(), buffer.begin() + read);

        std::string run_file = "run_" + std::to_string(run_files.size()) + ".bin";
        FILE* out = fopen(run_file.c_str(), "wb");
        fwrite(buffer.data(), sizeof(int64_t), read, out); IO_COUNT++;
        fclose(out);

        run_files.push_back(run_file);
    }

    fclose(in);

    FILE* final = fopen(output_file.c_str(), "wb");
    ftruncate(fileno(final), file_size);
    fclose(final);

    merge_files(run_files, output_file, 0);
}

int run_and_measure(const std::string& input_file, const std::string& output_file, int a) {
    IO_COUNT = 0;
    external_mergesort(input_file, output_file, a);
    return IO_COUNT;
}

int best_a(const std::string& input_file, const std::string& output_file) {
    int low = 2, high = B / 8;
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

    int mejor_a = best_a(input, output);
    std::cout << "Mejor valor de a encontrado: " << mejor_a << std::endl;

    return 0;
}
