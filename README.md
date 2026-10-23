# 骑行数据排序练习

这是一个使用 C++11 练习排序、查找和统计的示例项目。

## 构建和运行

```sh
make
make run
```

默认读取 `rides_1k.txt`。也可以指定其他数据文件：

```sh
make run RIDES=rides_10k.txt
```

或直接运行：

```sh
./build/OrdCsltion rides_50k_B.txt
```

## 生成随机测试数据

项目包含一个只依赖 Python 3 标准库的数据生成器。以下命令会生成 100000 条可复现的测试记录：

```sh
python3 tools/generate_rides.py \
  --count 100000 \
  --seed 2026 \
  --output rides_100k_random.txt
```

也可以使用 Makefile：

```sh
make generate GENERATE_COUNT=100000 GENERATE_SEED=2026 \
  GENERATED_RIDES=rides_100k_random.txt
make run RIDES=rides_100k_random.txt
```

生成数据会沿用现有文件的字段格式和近似分布。完整参数与说明见 [`docs/随机测试数据生成器.md`](docs/随机测试数据生成器.md)。
