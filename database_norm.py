from rocksdict import Options, Rdict

old_opts = Options(raw_mode=True)
new_opts = Options(raw_mode=True)
new_opts.create_if_missing(True)
new_opts.set_write_buffer_size(256 * 1024 * 1024)

old_db = Rdict("./fens-32m", old_opts)
new_db = Rdict("./fens-32m-norm", new_opts)

count = 0
for key in old_db.keys():
    fen = key if isinstance(key, str) else key.decode()
    parts = fen.split()
    # Keep only board/side/castling/ep, normalize counters
    normalized = " ".join(parts[:4]) + " 0 1"
    value = old_db[key]
    new_db[normalized.encode()] = value
    count += 1
    if count % 1_000_000 == 0:
        print(f"{count}...")

new_db.close()
old_db.close()
print(f"Done: {count} entries")
