import torch

model = torch.jit.load("model_best.pt", map_location="cpu")

for name, param in model.named_parameters():
    print(f"{name}: {param.shape}, mean={param.mean():.6f}, std={param.std():.6f}")