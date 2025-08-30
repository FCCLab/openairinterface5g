#!/bin/bash

export PYTHONPATH=/usr/local/lib/python3.10/site-packages:$PYTHONPATH
python3 -c "import uhd; print(uhd.__version__, uhd.get_version_string())"

python3 "$(dirname "$0")/spectrogram.py" "$@"
