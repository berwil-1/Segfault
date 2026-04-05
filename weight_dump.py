import torch

model = torch.jit.load("model_best.pt", map_location="cpu")

with open("weights.bin", "wb") as f:
    for name, param in model.named_parameters():
        data = param.detach().float().cpu()
        if name == "seq.0.weight":
            data = torch.nn.functional.pad(data, (0, 6))  # 258 -> 264 cols
        f.write(data.contiguous().numpy().tobytes())
        print(f"wrote {name}: {data.shape}, {data.numel() * 4} bytes")
