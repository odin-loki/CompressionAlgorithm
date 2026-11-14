# release/

Artifacts that leave the repository: the hpd source package and the Zenodo
upload package.

| path | what |
|---|---|
| [`dist/hpd-1.2.0-src.zip`](dist/hpd-1.2.0-src.zip) | hpd 1.2.0 source package, cited in the paper (§18). Reproducible: `python hpd/tools/make_src_zip.py [OUT_DIR]` |
| [`zenodo/make_package.py`](zenodo/make_package.py) | fills `zenodo/upload/`: paper PDF, source zip, paper materials zip, `SHA256SUMS.txt` |
| [`zenodo/UPLOAD.md`](zenodo/UPLOAD.md) | step-by-step Zenodo upload instructions |
| `zenodo/upload/` | the files as uploaded for the Zenodo record |

The published archives are kept as they were released. Rebuilding them
from the current tree changes their bytes (and their checksums), so only
rebuild for a new version.
