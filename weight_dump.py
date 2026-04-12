import torch
import numpy as np

model = torch.jit.load("model_best.pt", map_location="cpu")

with open("weights.bin", "wb") as f:
    for name, param in model.named_parameters():
        data = param.detach().float().cpu()

        # Quantize only the big hidden layer weights
        if "weight" in name and name not in ("seq.0.weight", "seq.6.weight"):
            scale = 127.0 / data.abs().max().item()
            quantized = torch.round(data * scale).clamp(-128, 127).to(torch.int8)
            f.write(np.array([scale], dtype=np.float32).tobytes())
            f.write(quantized.contiguous().numpy().tobytes())
            print(f"wrote {name}: {data.shape}, scale={scale:.4f}, int8")
        else:
            f.write(data.contiguous().numpy().tobytes())
            print(f"wrote {name}: {data.shape}, float32")
