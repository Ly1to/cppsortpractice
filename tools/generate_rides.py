#!/usr/bin/env python3
"""Generate ride records in the format consumed by OrdCsltion."""

from __future__ import annotations

import argparse
import random
import secrets
import sys
from pathlib import Path
from typing import TextIO

VEHICLE_ID_MIN = 1001
VEHICLE_ID_MAX = 3000

# The existing samples have heavier traffic during morning and evening commutes.
HOUR_WEIGHTS = [
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
    6.5,
    6.5,
    3.5,
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
    2.5,
    6.0,
    6.5,
    1.0,
    1.0,
    1.0,
    1.0,
    1.0,
]

AREAS = ("A1", "A2", "A3", "A4", "A5", "B1", "B2", "C1")
AREA_WEIGHTS = (172, 164, 175, 105, 106, 111, 59, 108)


def positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("必须是整数") from error
    if number <= 0:
        raise argparse.ArgumentTypeError("必须大于 0")
    return number


def random_start_time(rng: random.Random) -> int:
    hour = rng.choices(range(24), weights=HOUR_WEIGHTS, k=1)[0]
    return hour * 60 + rng.randrange(60)


def random_duration(rng: random.Random) -> int:
    if rng.random() < 0.97:
        return round(rng.triangular(5, 45, 18))
    return rng.randint(50, 180)


def random_cost(rng: random.Random) -> float:
    if rng.random() < 0.99:
        return round(rng.triangular(1.15, 4.5, 2.1), 2)
    return round(rng.uniform(4.5, 14.0), 2)


def write_rides(output: TextIO, count: int, rng: random.Random) -> None:
    output.write(f"{count}\n")
    for _ in range(count):
        vehicle_id = (
            f"NB-{rng.randint(VEHICLE_ID_MIN, VEHICLE_ID_MAX):04d}"
        )
        start_time = random_start_time(rng)
        duration = random_duration(rng)
        cost = random_cost(rng)
        area = rng.choices(AREAS, weights=AREA_WEIGHTS, k=1)[0]
        output.write(
            f"{vehicle_id} {start_time:4d} {duration:3d} {cost:6.2f} {area}\n"
        )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="生成 OrdCsltion 可读取的随机骑行测试数据"
    )
    parser.add_argument(
        "-n",
        "--count",
        type=positive_int,
        default=10000,
        help="记录条数（默认：10000）",
    )
    parser.add_argument(
        "-s",
        "--seed",
        type=int,
        help="随机种子；省略时自动生成并打印",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="输出文件；使用 - 输出到标准输出（默认：rides_<数量>_random.txt）",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    seed = args.seed if args.seed is not None else secrets.randbits(32)
    destination = args.output or Path(f"rides_{args.count}_random.txt")
    rng = random.Random(seed)

    if destination == Path("-"):
        write_rides(sys.stdout, args.count, rng)
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("w", encoding="ascii", newline="\n") as output:
            write_rides(output, args.count, rng)
        print(
            f"已生成 {args.count} 条记录：{destination}（seed={seed}）",
            file=sys.stderr,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
