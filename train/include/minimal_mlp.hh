// minimal_mlp.hpp – single‑header MLP for 64‑input chess evaluation
// Public domain / MIT‑0.  Compile with any C++17 compiler.
#pragma once
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <random>
#include <vector>

namespace minimal_mlp {

/**
 * Single‑output multilayer perceptron.
 * Default topology: 64 inputs → hidden (32) → 1 output (−12 … 12)
 */
class MLP {
public:
    /// Construct with given hidden units (defaults to 32)
    explicit MLP(std::size_t hidden = 32)
        : in_(64),
          hid_(hidden),
          out_(1),
          w1_(hid_ * in_),
          b1_(hid_),
          w2_(out_ * hid_),
          b2_(out_),
          rng_(std::random_device{}()) {
        init_weights();
    }

    /// Forward inference – returns evaluation in range [−12, 12]
    [[nodiscard]] float
    forward(const std::array<float, 64> & x) const {
        std::vector<float> h(hid_);
        // Hidden layer
        for (std::size_t i = 0; i < hid_; ++i) {
            float         s = b1_[i];
            const float * w = &w1_[i * in_];
            for (std::size_t j = 0; j < in_; ++j)
                s += w[j] * x[j];
            h[i] = std::tanh(s);
        }
        // Output layer (single neuron)
        float         o = b2_[0];
        const float * w = &w2_[0];
        for (std::size_t i = 0; i < hid_; ++i)
            o += w[i] * h[i];
        o = std::tanh(o); // in [−1, 1]
        return o * 12.0f; // scale to [−12, 12]
    }

    /// SGD on a single (x, target) sample – returns loss (MSE)
    float
    train_sample(const std::array<float, 64> & x, float target, float lr = 0.01f) {
        // ---- Forward ----
        std::vector<float> h(hid_);
        for (std::size_t i = 0; i < hid_; ++i) {
            float   s = b1_[i];
            float * w = &w1_[i * in_];
            for (std::size_t j = 0; j < in_; ++j)
                s += w[j] * x[j];
            h[i] = std::tanh(s);
        }
        float o = b2_[0];
        for (std::size_t i = 0; i < hid_; ++i)
            o += w2_[i] * h[i];
        float o_act = std::tanh(o);
        float pred = o_act * 12.0f;

        // ---- Back‑prop ----
        float t_scaled = target / 12.0f; // back to [−1,1]
        float err = pred - target; // error in eval space
        float grad_o = 2.0f * err / 12.0f; // dMSE/d(o_act)
        float delta_o = grad_o * (1.0f - o_act * o_act); // tanh′

        // Update output weights/bias
        for (std::size_t i = 0; i < hid_; ++i) {
            w2_[i] -= lr * delta_o * h[i];
        }
        b2_[0] -= lr * delta_o;

        // Hidden layer deltas
        for (std::size_t i = 0; i < hid_; ++i) {
            float   delta_h = delta_o * w2_[i] * (1.0f - h[i] * h[i]);
            float * w1 = &w1_[i * in_];
            for (std::size_t j = 0; j < in_; ++j) {
                w1[j] -= lr * delta_h * x[j];
            }
            b1_[i] -= lr * delta_h;
        }

        return err * err; // squared error
    }

    /// Train on dataset for epochs (simple SGD pass‑through)
    void
    train(const std::vector<std::array<float, 64>> & X, const std::vector<float> & y,
          std::size_t epochs = 10, float lr = 0.01f) {
        assert(X.size() == y.size());
        std::uniform_int_distribution<std::size_t> uni(0, X.size() - 1);
        for (std::size_t e = 0; e < epochs; ++e) {
            for (std::size_t i = 0; i < X.size(); ++i) {
                std::size_t idx = uni(rng_); // shuffle
                train_sample(X[idx], y[idx], lr);
            }
        }
    }

    /// Save / load weights to a binary file (optional)
    void
    save(const std::string & path) const {
        std::ofstream f(path, std::ios::binary);
        write_vec(f, w1_);
        write_vec(f, b1_);
        write_vec(f, w2_);
        write_vec(f, b2_);
    }

    void
    load(const std::string & path) {
        std::ifstream f(path, std::ios::binary);
        read_vec(f, w1_);
        read_vec(f, b1_);
        read_vec(f, w2_);
        read_vec(f, b2_);
    }

private:
    std::size_t          in_, hid_, out_;
    std::vector<float>   w1_, b1_, w2_, b2_;
    mutable std::mt19937 rng_;

    void
    init_weights() {
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        for (auto & v : w1_)
            v = dist(rng_);
        for (auto & v : b1_)
            v = 0.0f;
        for (auto & v : w2_)
            v = dist(rng_);
        for (auto & v : b2_)
            v = 0.0f;
    }

    template<typename Stream>
    static void
    write_vec(Stream & s, const std::vector<float> & v) {
        s.write(reinterpret_cast<const char *>(v.data()), v.size() * sizeof(float));
    }

    template<typename Stream>
    static void
    read_vec(Stream & s, std::vector<float> & v) {
        s.read(reinterpret_cast<char *>(v.data()), v.size() * sizeof(float));
    }
};

} // namespace minimal_mlp
