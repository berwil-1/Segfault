#pragma once

#include <array>
#include <fstream>
#include <immintrin.h>

constexpr int BOARD_SIZE_NNUE{768};

struct NetworkWeights {
    std::array<float, 1024 * BOARD_SIZE_NNUE> fc1_weight;
    std::array<float, 1024>                   fc1_bias;
    std::array<float, 512 * 1024>             fc2_weight;
    std::array<float, 512>                    fc2_bias;
    std::array<float, 256 * 512>              fc3_weight;
    std::array<float, 256>                    fc3_bias;
    std::array<float, 1 * 256>                fc4_weight;
    std::array<float, 1>                      fc4_bias;
};

struct Accumulator {
    alignas(32) std::array<float, 1024> values;

    void
    refresh(const NetworkWeights & weights, const float * input) {
        std::copy(weights.fc1_bias.begin(), weights.fc1_bias.end(), values.begin());
        for (auto i = 0; i < BOARD_SIZE_NNUE; ++i) {
            if (input[i] == 0.0f)
                continue;
            for (auto j = 0; j < 1024; ++j)
                values[j] += input[i] * weights.fc1_weight[j * BOARD_SIZE_NNUE + i];
        }
    }

    void
    add_feature(const NetworkWeights & weights, int feature_index) {
        for (auto j = 0; j < 1024; ++j)
            values[j] += weights.fc1_weight[j * BOARD_SIZE_NNUE + feature_index];
    }

    void
    sub_feature(const NetworkWeights & weights, int feature_index) {
        for (auto j = 0; j < 1024; ++j)
            values[j] -= weights.fc1_weight[j * BOARD_SIZE_NNUE + feature_index];
    }
};

inline void
loadWeights(NetworkWeights & weights, const std::string & path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        std::cerr << "Failed to open weights file: " + path << std::endl;
    }

    auto readArray = [&](float * data, std::size_t count) {
        file.read(reinterpret_cast<char *>(data),
                  static_cast<std::streamsize>(count * sizeof(float)));
    };

    readArray(weights.fc1_weight.data(), weights.fc1_weight.size());
    readArray(weights.fc1_bias.data(), weights.fc1_bias.size());
    readArray(weights.fc2_weight.data(), weights.fc2_weight.size());
    readArray(weights.fc2_bias.data(), weights.fc2_bias.size());
    readArray(weights.fc3_weight.data(), weights.fc3_weight.size());
    readArray(weights.fc3_bias.data(), weights.fc3_bias.size());
    readArray(weights.fc4_weight.data(), weights.fc4_weight.size());
    readArray(weights.fc4_bias.data(), weights.fc4_bias.size());
}

template<int INPUTS, int OUTPUTS>
void
matmulBiasRelu(const float * __restrict__ input, const float * __restrict__ weight,
               const float * __restrict__ bias, float * __restrict__ output) {
    for (int o = 0; o < OUTPUTS; ++o) {
        auto sum = bias[o];
        for (int i = 0; i < INPUTS; ++i) {
            sum += weight[o * INPUTS + i] * input[i];
        }
        output[o] = std::max(0.0f, sum);
    }
}

template<int INPUTS, int OUTPUTS>
void
matmulBiasReluTiled(const float * __restrict__ input, const float * __restrict__ weight,
                    const float * __restrict__ bias, float * __restrict__ output) {
    static_assert(INPUTS % 8 == 0);
    static_assert(OUTPUTS % 4 == 0);

    for (int o = 0; o < OUTPUTS; o += 4) {
        const auto * row0 = &weight[(o + 0) * INPUTS];
        const auto * row1 = &weight[(o + 1) * INPUTS];
        const auto * row2 = &weight[(o + 2) * INPUTS];
        const auto * row3 = &weight[(o + 3) * INPUTS];

        auto sum0 = _mm256_setzero_ps();
        auto sum1 = _mm256_setzero_ps();
        auto sum2 = _mm256_setzero_ps();
        auto sum3 = _mm256_setzero_ps();

        for (int i = 0; i < INPUTS; i += 8) {
            const auto x = _mm256_loadu_ps(&input[i]);
            sum0 = _mm256_fmadd_ps(x, _mm256_loadu_ps(&row0[i]), sum0);
            sum1 = _mm256_fmadd_ps(x, _mm256_loadu_ps(&row1[i]), sum1);
            sum2 = _mm256_fmadd_ps(x, _mm256_loadu_ps(&row2[i]), sum2);
            sum3 = _mm256_fmadd_ps(x, _mm256_loadu_ps(&row3[i]), sum3);
        }

        auto hsum = [](const __m256 v) -> float {
            const auto hi4 = _mm256_extractf128_ps(v, 1);
            const auto lo4 = _mm256_castps256_ps128(v);
            const auto s4 = _mm_add_ps(lo4, hi4);
            const auto hi2 = _mm_movehl_ps(s4, s4);
            const auto s2 = _mm_add_ps(s4, hi2);
            const auto hi1 = _mm_shuffle_ps(s2, s2, 0x1);
            return _mm_cvtss_f32(_mm_add_ss(s2, hi1));
        };

        output[o + 0] = std::max(0.0f, hsum(sum0) + bias[o + 0]);
        output[o + 1] = std::max(0.0f, hsum(sum1) + bias[o + 1]);
        output[o + 2] = std::max(0.0f, hsum(sum2) + bias[o + 2]);
        output[o + 3] = std::max(0.0f, hsum(sum3) + bias[o + 3]);
    }
}

inline float
forward(const NetworkWeights & weights, const float * input) {
    std::array<float, BOARD_SIZE_NNUE> padded_input{};
    std::copy_n(input, BOARD_SIZE_NNUE, padded_input.begin());

    std::array<float, 1024> hidden1{};
    matmulBiasReluTiled<BOARD_SIZE_NNUE, 1024>(padded_input.data(), weights.fc1_weight.data(),
                                               weights.fc1_bias.data(), hidden1.data());

    std::array<float, 512> hidden2{};
    matmulBiasReluTiled<1024, 512>(hidden1.data(), weights.fc2_weight.data(),
                                   weights.fc2_bias.data(), hidden2.data());

    std::array<float, 256> hidden3{};
    matmulBiasReluTiled<512, 256>(hidden2.data(), weights.fc3_weight.data(),
                                  weights.fc3_bias.data(), hidden3.data());

    // Final layer — no ReLU (sigmoid applied outside)
    auto sum = weights.fc4_bias[0];
    for (int i = 0; i < 256; ++i) {
        sum += weights.fc4_weight[i] * hidden3[i];
    }
    return sum;
}

inline int
featureIndex(chess::Piece piece, chess::Square sq) {
    return static_cast<int>(piece) * 64 + static_cast<int>(sq.index());
}

inline float
forward_from_accumulator(const NetworkWeights & weights, const Accumulator & acc) {
    alignas(32) std::array<float, 1024> relu_out;
    for (auto i = 0; i < 1024; ++i)
        relu_out[i] = std::max(0.0f, acc.values[i]);

    std::array<float, 512> hidden2{};
    matmulBiasReluTiled<1024, 512>(relu_out.data(), weights.fc2_weight.data(),
                                   weights.fc2_bias.data(), hidden2.data());

    std::array<float, 256> hidden3{};
    matmulBiasReluTiled<512, 256>(hidden2.data(), weights.fc3_weight.data(),
                                  weights.fc3_bias.data(), hidden3.data());

    auto sum = weights.fc4_bias[0];
    for (auto i = 0; i < 256; ++i)
        sum += weights.fc4_weight[i] * hidden3[i];
    return sum;
}
