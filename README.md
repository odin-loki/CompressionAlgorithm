# CompressionAlgorithm

Context-mixing compressors and the research behind them, in one repository.

- **hp** ([`hp/`](hp/)) is the Hutter Prize lab: an integer-exact
  context-mixing compressor, tuned against enwik. No floating point in the codec.
- **hpd** ([`hpd/`](hpd/)) is the everyday compressor built from the HP40
  study: the same model family, rebuilt for general files (speed, memory,
  start-up, binary data).

The documentation is a small Markdown site in [`docs/`](docs/index.md)
(start there). The paper, the Zenodo release artifacts and the experiments
sit beside the code.

## Layout

```
hp/          hp: the lab compressor (C++23), tests, tools, third_party/xsimd
hpd/         hpd: the everyday compressor (C++20), tests, GPU build, LICENSE (AGPL-3.0)
paper/       HP40 / HP40-Lite / hpd paper: sources, data, figures, scripts
release/     release artifacts: dist/ (hpd source zip), zenodo/ (upload package)
labs/        experiments: fx2_mixer_lab/, stats/, harvest/
docs/        the Markdown site: index.md, lab notes (docs/lab/), reports (docs/reports/)
```

## Quick start

Verified on Ubuntu 24.04 (WSL2) with g++ 13.3. Both programs are POSIX
builds (they use `mmap`), so on Windows build them under WSL.

```bash
# hpd
cd hpd
make && make test
build/hpd c file            # -> file.hpd
build/hpd d file.hpd        # -> file
build/hpd t file.hpd        # verify CRCs without writing

# hp
cd hp
make                        # build/hp, build/dump_experts, build/bench
bash test/no_float.sh       # the codec must contain no floating point
bash test/roundtrip.sh build/hp FILE...
```

The hp champion flag set is in [`hp/tools/v78_flags.ps1`](hp/tools/v78_flags.ps1).
A bare build is not the champion: see [`docs/index.md`](docs/index.md#hp-the-lab-compressor).

## Documentation

- [`docs/index.md`](docs/index.md): the site home, with status and a map of every document.
- [`docs/lab/`](docs/lab/): the living lab notes: `PLAN.md` (experiment board),
  `RECORD.md` (measurement log), and the research and idea notes.
- [`docs/reports/`](docs/reports/): analysis reports.
- Per-project READMEs: [`hp/README.md`](hp/README.md), [`hpd/README.md`](hpd/README.md),
  [`paper/README.md`](paper/README.md), [`release/README.md`](release/README.md),
  [`labs/README.md`](labs/README.md).

## Licence

`hpd/` is licensed under the GNU Affero General Public License v3.0 ([`hpd/LICENSE`](hpd/LICENSE)).
The licence for `hp/` and `labs/` has not been declared yet.

## History

This repository merges two earlier repositories: `odin-loki/CompressionAlgorithm`
(the hp lab, history unchanged) and `odin-loki/HPv40-Compression-Algorithm`
(HP40 study, hpd, paper, release artifacts). The HPv40 history is kept under
`hpd/`, `paper/` and `release/` with its original commit messages.
