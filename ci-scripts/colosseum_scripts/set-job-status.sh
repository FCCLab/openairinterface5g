#!/bin/bash
set -eu

# sets final exit code and thus jenkins pass or fail
STATUS=$(jq -r '.status' results.json)
echo "job status: ${STATUS}"
[ "${STATUS}" = "successful" ]
