#!/usr/bin/env python3
"""Split a PGN file into n files with (nearly) equal numbers of games."""

import argparse
import sys
from pathlib import Path
from typing import Iterator, TextIO


def iterate_games(stream: TextIO) -> Iterator[str]:
    """Yield each game from a PGN stream as a string.

    A new game is detected when a tag line (starting with '[') is seen
    after movetext content has been accumulated for the current game.
    """
    current: list[str] = []
    in_movetext = False
    for line in stream:
        stripped = line.lstrip()
        if stripped.startswith('[') and in_movetext:
            yield ''.join(current)
            current = [line]
            in_movetext = False
            continue
        current.append(line)
        if stripped and not stripped.startswith('['):
            in_movetext = True
    if any(s.strip() for s in current):
        yield ''.join(current)


def count_games(path: Path) -> int:
    """Return the number of games in a PGN file."""
    with path.open('r', encoding='utf-8', errors='replace') as stream:
        return sum(1 for _ in iterate_games(stream))


def split_pgn(input_path: Path, num_files: int, output_dir: Path) -> None:
    """Split input_path into num_files PGN files placed in output_dir."""
    if num_files <= 0:
        raise ValueError(f'num_files must be positive, got {num_files}')

    total = count_games(input_path)
    if total == 0:
        raise ValueError(f'No games found in {input_path}')
    if num_files > total:
        raise ValueError(
            f'Requested {num_files} files but only {total} games available'
        )

    base, remainder = divmod(total, num_files)
    sizes = [base + 1 if i < remainder else base for i in range(num_files)]

    output_dir.mkdir(parents=True, exist_ok=True)
    stem = input_path.stem
    width = len(str(num_files))

    with input_path.open('r', encoding='utf-8', errors='replace') as src:
        games = iterate_games(src)
        for index, size in enumerate(sizes, start=1):
            out_path = output_dir / f'{stem}_part_{index:0{width}d}.pgn'
            with out_path.open('w', encoding='utf-8') as dst:
                for _ in range(size):
                    game = next(games)
                    dst.write(game)
                    if not game.endswith('\n\n'):
                        dst.write('\n' if game.endswith('\n') else '\n\n')
            print(f'Wrote {size} games to {out_path}')


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Split a PGN file into n files with equal game counts.',
    )
    parser.add_argument('input', type=Path, help='Input PGN file')
    parser.add_argument('n', type=int, help='Number of output files')
    parser.add_argument(
        '-o',
        '--output-dir',
        type=Path,
        default=Path.cwd(),
        help='Output directory (default: current directory)',
    )
    args = parser.parse_args()

    if not args.input.is_file():
        print(f'error: input file not found: {args.input}', file=sys.stderr)
        return 1

    try:
        split_pgn(args.input, args.n, args.output_dir)
    except ValueError as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
