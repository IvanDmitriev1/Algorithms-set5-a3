import csv
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

plt.rcParams["font.family"] = "DejaVu Sans"
plt.rcParams["axes.unicode_minus"] = False


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as file:
        return list(csv.DictReader(file))


def col(rows: list[dict[str, str]], key: str) -> list[float]:
    return [float(row[key]) for row in rows]


def plot_series(csv_path: Path, out_png: Path, title_suffix: str) -> None:
    rows = read_rows(csv_path)

    x = col(rows, "prefix_size")
    ft0 = col(rows, "ft0_exact")
    nt = col(rows, "nt_estimate")
    mean_nt = col(rows, "mean_nt")
    lower_nt = col(rows, "lower_nt")
    upper_nt = col(rows, "upper_nt")

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    ax1.plot(x, ft0, linewidth=2.0, label="Истинное значение Ft0")
    ax1.plot(x, nt, linewidth=2.0, label="Оценка Nt")
    ax1.set_title("График №1 сравнения оценки Nt и Ft0")
    ax1.set_xlabel("Размер обработанной части потока (момент времени t)")
    ax1.set_ylabel("Количество уникальных элементов")
    ax1.grid(alpha=0.3)
    ax1.legend()

    ax2.plot(x, mean_nt, linewidth=2.0, label="Линия E(Nt)")
    ax2.fill_between(x, lower_nt, upper_nt, alpha=0.25, label="Область E(Nt)+σNt и E(Nt)-σNt")
    ax2.set_title("График №2 статистик оценки")
    ax2.set_xlabel("Размер обработанной части потока (момент времени t)")
    ax2.set_ylabel("Количество уникальных элементов")
    ax2.grid(alpha=0.3)
    ax2.legend()

    fig.suptitle(title_suffix)
    fig.tight_layout()
    fig.savefig(out_png, dpi=180)
    plt.close(fig)


def plot_b_analysis(summary_csv: Path, out_png: Path) -> None:
    rows = read_rows(summary_csv)
    grouped: dict[str, list[dict[str, str]]] = defaultdict(list)

    for row in rows:
        grouped[row["hasher"]].append(row)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    theory_x = []
    theory_104 = []
    theory_13 = []

    for hasher, items in grouped.items():
        items.sort(key=lambda r: int(r["b"]))
        b_vals = [int(r["b"]) for r in items]
        mean_err = [float(r["mean_abs_rel_err"]) * 100.0 for r in items]
        mean_sigma = [float(r["mean_sigma_over_e"]) * 100.0 for r in items]

        ax1.plot(b_vals, mean_err, marker="o", linewidth=2.0, label=hasher)
        ax2.plot(b_vals, mean_sigma, marker="o", linewidth=2.0, label=hasher)

        if not theory_x:
            theory_x = b_vals
            theory_104 = [float(r["theory_104"]) * 100.0 for r in items]
            theory_13 = [float(r["theory_13"]) * 100.0 for r in items]

    if theory_x:
        ax2.plot(theory_x, theory_104, "k--", linewidth=1.5, label="1.04/sqrt(2^B)")
        ax2.plot(theory_x, theory_13, "k:", linewidth=1.5, label="1.3/sqrt(2^B)")

    ax1.set_title("Зависимость средней относительной ошибки от B")
    ax1.set_xlabel("B")
    ax1.set_ylabel("Средняя ошибка, %")
    ax1.grid(alpha=0.3)
    ax1.legend()

    ax2.set_title("Зависимость относительной дисперсии от B")
    ax2.set_xlabel("B")
    ax2.set_ylabel("Среднее σ/E, %")
    ax2.grid(alpha=0.3)
    ax2.legend()

    fig.suptitle("Анализ HyperLogLog по B")
    fig.tight_layout()
    fig.savefig(out_png, dpi=180)
    plt.close(fig)


def main() -> None:
    root_dir = Path(__file__).resolve().parent
    output_dir = root_dir / "output"
    plots_dir = root_dir / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)

    csv_files = sorted(output_dir.glob("*_B*.csv"))
    if not csv_files:
        raise SystemExit("Файлы *_B*.csv не найдены в output/. Сначала запусти A3.")

    print("Сохранены файлы:")
    for csv_path in csv_files:
        title_suffix = csv_path.stem.replace("_", " ")
        out_png = plots_dir / f"{csv_path.stem}.png"
        plot_series(csv_path, out_png, title_suffix)
        print(out_png)

    summary_csv = output_dir / "b_analysis.csv"
    if summary_csv.exists():
        summary_png = plots_dir / "b_analysis.png"
        plot_b_analysis(summary_csv, summary_png)
        print(summary_png)


if __name__ == "__main__":
    main()
