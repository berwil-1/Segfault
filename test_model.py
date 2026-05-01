import math
import torch
import torch.nn as nn
import chess

BOARD_SIZE_NNUE = 768

def encode_board(board):
    inp = [0.0] * BOARD_SIZE_NNUE
    for sq in chess.SQUARES:
        piece = board.piece_at(sq)
        if piece is not None:
            piece_index = (piece.piece_type - 1) + (0 if piece.color == chess.WHITE else 6)
            inp[piece_index * 64 + sq] = 1.0
    return inp

class Net(nn.Module):
    def __init__(self):
        super().__init__()
        self.seq = nn.Sequential(
            nn.Linear(768, 512), nn.ReLU(),
            nn.Linear(512, 512), nn.ReLU(),
            nn.Linear(512, 256), nn.ReLU(),
            nn.Linear(256, 1),
        )

    def forward(self, x):
        return self.seq(x)

# Load from TorchScript archive into regular module
archive = torch.jit.load("model_best.pt", map_location="cpu")
state_dict = {}
for name, param in archive.named_parameters():
    state_dict[name] = param

model = Net()
model.load_state_dict(state_dict)
model.eval()

test_fens = [
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "~0"),
    ("1B1K4/8/2P1k3/8/6p1/7p/7P/8 w - - 0 47", "+1395"),
    ("1B1K4/1bP2p2/6kp/6p1/8/8/4n3/8 b - - 2 43", "-712"),
    ("3r1rk1/pp3pp1/2n1bn1p/2b1q3/8/2NP2PP/PPP1N1BK/R1BQ1R2 w - - 0 14", "~+30"),
]

for fen, expected in test_fens:
    board = chess.Board(fen)
    enc = encode_board(board)
    x = torch.tensor(enc).unsqueeze(0)
    with torch.no_grad():
        pred = model(x).item()

    k = 0.00368208
    pred_clamped = max(1e-6, min(1 - 1e-6, pred))
    cp_est = math.log(pred_clamped / (1 - pred_clamped)) / k

    print(f"expected={expected:>8s}  raw={pred:.6f}  cp={cp_est:.1f}  FEN: {fen}")

'''for fen, expected in test_fens:
    board = chess.Board(fen)
    enc = encode_board(board)
    x = torch.tensor(enc).unsqueeze(0)
    with torch.no_grad():
        pred = model(x).item()
    print(f"expected={expected:>8s}  raw={pred:.6f}  cp={(pred - 0.5) * 10000:.1f}  FEN: {fen}")
'''
