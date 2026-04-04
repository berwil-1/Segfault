#pragma once

#include <fstream>
#include <array>

struct NetworkWeights
{
    std::array<float, 1024 * 258> fc1_weight;
    std::array<float, 1024>       fc1_bias;
    std::array<float, 512 * 1024> fc2_weight;
    std::array<float, 512>        fc2_bias;
    std::array<float, 256 * 512>  fc3_weight;
    std::array<float, 256>        fc3_bias;
    std::array<float, 1 * 256>    fc4_weight;
    std::array<float, 1>          fc4_bias;
};

void loadWeights(NetworkWeights & weights, const std::string & path)
{
    std::ifstream file{path, std::ios::binary};
    if (!file)
    {
        throw std::runtime_error("Failed to open weights file: " + path);
    }

    auto readArray = [&](float * data, std::size_t count)
    {
        file.read(reinterpret_cast<char *>(data),
                  static_cast<std::streamsize>(count * sizeof(float)));
    };

    readArray(weights.fc1_weight.data(), weights.fc1_weight.size());
    readArray(weights.fc1_bias.data(),   weights.fc1_bias.size());
    readArray(weights.fc2_weight.data(), weights.fc2_weight.size());
    readArray(weights.fc2_bias.data(),   weights.fc2_bias.size());
    readArray(weights.fc3_weight.data(), weights.fc3_weight.size());
    readArray(weights.fc3_bias.data(),   weights.fc3_bias.size());
    readArray(weights.fc4_weight.data(), weights.fc4_weight.size());
    readArray(weights.fc4_bias.data(),   weights.fc4_bias.size());
}

template <int INPUTS, int OUTPUTS>
void matmulBiasRelu(const float * __restrict__ input,
                    const float * __restrict__ weight,
                    const float * __restrict__ bias,
                    float * __restrict__ output)
{
    for (int o = 0; o < OUTPUTS; ++o)
    {
        auto sum = bias[o];
        for (int i = 0; i < INPUTS; ++i)
        {
            sum += weight[o * INPUTS + i] * input[i];
        }
        output[o] = std::max(0.0f, sum);
    }
}

float forward(const NetworkWeights & weights, const float * input)
{
    std::array<float, 1024> hidden1{};
    matmulBiasRelu<258, 1024>(input,
                              weights.fc1_weight.data(),
                              weights.fc1_bias.data(),
                              hidden1.data());

    std::array<float, 512> hidden2{};
    matmulBiasRelu<1024, 512>(hidden1.data(),
                              weights.fc2_weight.data(),
                              weights.fc2_bias.data(),
                              hidden2.data());

    std::array<float, 256> hidden3{};
    matmulBiasRelu<512, 256>(hidden2.data(),
                             weights.fc3_weight.data(),
                             weights.fc3_bias.data(),
                             hidden3.data());

    // Final layer — no ReLU (sigmoid applied outside)
    auto sum = weights.fc4_bias[0];
    for (int i = 0; i < 256; ++i)
    {
        sum += weights.fc4_weight[i] * hidden3[i];
    }
    return sum;
}