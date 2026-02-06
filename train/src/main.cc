#include <iostream>
#include <torch/torch.h>

struct FenEvalDataset : torch::data::datasets::Dataset<FenEvalDataset> {
    rocksdb::DB *            database_;
    std::vector<std::string> keys_;

    FenEvalDataset(rocksdb::DB * const database, const std::vector<std::string> && keys)
        : database_(database), keys_(std::move(keys)) {}

    torch::data::Example<>
    get(std::size_t index) override {
        std::string value;
        const auto  fen = keys_[index];
        database_->Get(rocksdb::ReadOptions(), fen, &value);

        int cp = 0;
        try {
            cp = std::stoi(value);
        } catch (...) {
            cp = 0;
        }

        constexpr auto k = 0.00368208f;
        const auto     score = 1.0f / (1.0f + std::exp(-k * cp));

        Board      board = Board::fromFen(fen);
        const auto enc = encode_board(board); // std::array<float, board_size>

        auto x = torch::from_blob((void *)enc.data(), {static_cast<int64_t>(board_size)},
                                  torch::TensorOptions().dtype(torch::kFloat32))
                     .clone();

        auto y = torch::tensor({score}, torch::TensorOptions().dtype(torch::kFloat32));
        return {x, y};
    }

    torch::optional<size_t>
    size() const override {
        return keys_.size();
    }
};

int
main() {
    torch::Tensor tensor = torch::rand({2, 3});
    std::cout << tensor << std::endl;
}
