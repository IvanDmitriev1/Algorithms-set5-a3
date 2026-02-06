# HyperLogLog (A3): анализ зависимости от B

В проекте сравнивается работа HLL при разных `B` (precision bits):

- `B = {6, 8, 10, 12, 14}`
- хеш-функции: `PolyHash32` и `Fnv1a32`

## Анализ

При росте  случайная ошибка уменьшается, но общая относительная ошибка остаётся заметной (особенно для Fnv1a32). Это означает наличие систематического смещения хеша.

Дисперсия в целом падает с ростом . Для PolyHash32 при больших  она хорошо укладывается в теоретические границы, для Fnv1a32 отклонения чаще выше.

Выбранные base сильно влияют на результат. Они уменьшают стабильность и точность для части случаев.

## Установка зависимостей

### Windows

```powershell
python -m pip install matplotlib
winget install Premake.Premake.5.Beta
```

### Linux (Arch)

```bash
sudo pacman -Syu premake python python-pip python-matplotlib base-devel
```

### macOS

```bash
brew install premake python
python3 -m pip install matplotlib
```

## Сборка и запуск

### Windows

```powershell
premake5 vs2022
msbuild Set5.sln /p:Configuration=Release /p:Platform=x64
.\bin\Release\A3.exe
python plot_hll.py
```

### Linux / macOS

```bash
premake5 gmake2
make config=release
./bin/Release/A3
python3 plot_hll.py
```

## Что создается

- CSV в `output/`
- Графики в `plots/`
