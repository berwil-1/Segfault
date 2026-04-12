#pragma once

#include <array>
#include <cstring>
#include <fstream>
#include <immintrin.h>

constexpr int BOARD_SIZE_NNUE{768};

struct NetworkWeights {
    std::array<float, 1024 * BOARD_SIZE_NNUE> fc1_weight;
    std::array<float, 1024>                   fc1_bias;
    float                                     fc2_weight_scale;
    std::array<int8_t, 512 * 1024>            fc2_weight;
    std::array<float, 512>                    fc2_bias;
    float                                     fc3_weight_scale;
    std::array<int8_t, 256 * 512>             fc3_weight;
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
    if (!file)
        throw std::runtime_error("Failed to open weights file: " + path);

    auto readFloats = [&](float * data, std::size_t count) {
        file.read(reinterpret_cast<char *>(data),
                  static_cast<std::streamsize>(count * sizeof(float)));
    };
    auto readInt8s = [&](int8_t * data, std::size_t count) {
        file.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(count));
    };

    // Layer 1: float
    readFloats(weights.fc1_weight.data(), weights.fc1_weight.size());
    readFloats(weights.fc1_bias.data(), weights.fc1_bias.size());

    // Layer 2: scale + int8 weights + float bias
    readFloats(&weights.fc2_weight_scale, 1);
    readInt8s(weights.fc2_weight.data(), weights.fc2_weight.size());
    readFloats(weights.fc2_bias.data(), weights.fc2_bias.size());

    // Layer 3: scale + int8 weights + float bias
    readFloats(&weights.fc3_weight_scale, 1);
    readInt8s(weights.fc3_weight.data(), weights.fc3_weight.size());
    readFloats(weights.fc3_bias.data(), weights.fc3_bias.size());

    // Layer 4: float
    readFloats(weights.fc4_weight.data(), weights.fc4_weight.size());
    readFloats(weights.fc4_bias.data(), weights.fc4_bias.size());
}

inline int
hsum_epi32(const __m256i v) {
    const auto hi = _mm256_extracti128_si256(v, 1);
    const auto lo = _mm256_castsi256_si128(v);
    const auto sum128 = _mm_add_epi32(lo, hi);
    const auto shuf = _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2));
    const auto sum64 = _mm_add_epi32(sum128, shuf);
    const auto shuf2 = _mm_shuffle_epi32(sum64, _MM_SHUFFLE(0, 1, 0, 1));
    return _mm_cvtsi128_si32(_mm_add_epi32(sum64, shuf2));
}

inline float
quantizeActivations(const float * input, uint8_t * output, int size) {
    auto max_val = 0.0f;
    for (auto i = 0; i < size; ++i)
        max_val = std::max(max_val, input[i]);

    if (max_val == 0.0f) {
        std::memset(output, 0, size);
        return 1.0f;
    }

    const auto scale = 127.0f / max_val;
    for (auto i = 0; i < size; ++i)
        output[i] = static_cast<uint8_t>(std::min(127.0f, input[i] * scale));

    return scale;
}

template<int INPUTS, int OUTPUTS>
void
matmulBiasReluInt8(const uint8_t * __restrict__ input, const int8_t * __restrict__ weight,
                   const float * __restrict__ bias, const float weight_scale,
                   const float input_scale, float * __restrict__ output) {
    static_assert(INPUTS % 32 == 0);
    static_assert(OUTPUTS % 4 == 0);

    const auto combined_scale = input_scale * weight_scale;
    const auto ones = _mm256_set1_epi16(1);

    for (auto o = 0; o < OUTPUTS; o += 4) {
        const auto * row0 = &weight[(o + 0) * INPUTS];
        const auto * row1 = &weight[(o + 1) * INPUTS];
        const auto * row2 = &weight[(o + 2) * INPUTS];
        const auto * row3 = &weight[(o + 3) * INPUTS];

        auto acc0 = _mm256_setzero_si256();
        auto acc1 = _mm256_setzero_si256();
        auto acc2 = _mm256_setzero_si256();
        auto acc3 = _mm256_setzero_si256();

        for (auto i = 0; i < INPUTS; i += 32) {
            const auto x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&input[i]));

            auto w0 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&row0[i]));
            acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(_mm256_maddubs_epi16(x, w0), ones));

            auto w1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&row1[i]));
            acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(_mm256_maddubs_epi16(x, w1), ones));

            auto w2 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&row2[i]));
            acc2 = _mm256_add_epi32(acc2, _mm256_madd_epi16(_mm256_maddubs_epi16(x, w2), ones));

            auto w3 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&row3[i]));
            acc3 = _mm256_add_epi32(acc3, _mm256_madd_epi16(_mm256_maddubs_epi16(x, w3), ones));
        }

        output[o + 0] = std::max(0.0f, hsum_epi32(acc0) / combined_scale + bias[o + 0]);
        output[o + 1] = std::max(0.0f, hsum_epi32(acc1) / combined_scale + bias[o + 1]);
        output[o + 2] = std::max(0.0f, hsum_epi32(acc2) / combined_scale + bias[o + 2]);
        output[o + 3] = std::max(0.0f, hsum_epi32(acc3) / combined_scale + bias[o + 3]);
    }
}

inline int
featureIndex(chess::Piece piece, chess::Square sq) {
    return static_cast<int>(piece) * 64 + static_cast<int>(sq.index());
}

inline float
forward_from_accumulator(const NetworkWeights & weights, const Accumulator & acc) {
    // ReLU the accumulator
    alignas(32) std::array<float, 1024> relu_out;
    for (auto i = 0; i < 1024; ++i)
        relu_out[i] = std::max(0.0f, acc.values[i]);

    // Quantize to uint8 for int8 matmul
    alignas(32) std::array<uint8_t, 1024> q_input;
    const auto input_scale = quantizeActivations(relu_out.data(), q_input.data(), 1024);

    // Layer 2: 1024 -> 512 (int8)
    alignas(32) std::array<float, 512> hidden2;
    matmulBiasReluInt8<1024, 512>(q_input.data(), weights.fc2_weight.data(),
                                  weights.fc2_bias.data(), weights.fc2_weight_scale, input_scale,
                                  hidden2.data());

    // Quantize for layer 3
    alignas(32) std::array<uint8_t, 512> q_hidden2;
    const auto scale2 = quantizeActivations(hidden2.data(), q_hidden2.data(), 512);

    // Layer 3: 512 -> 256 (int8)
    alignas(32) std::array<float, 256> hidden3;
    matmulBiasReluInt8<512, 256>(q_hidden2.data(), weights.fc3_weight.data(),
                                 weights.fc3_bias.data(), weights.fc3_weight_scale, scale2,
                                 hidden3.data());

    // Layer 4: 256 -> 1 (float, no ReLU)
    auto sum = weights.fc4_bias[0];
    for (auto i = 0; i < 256; ++i)
        sum += weights.fc4_weight[i] * hidden3[i];

    return sum;
}
